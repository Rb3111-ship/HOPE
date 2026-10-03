/*
 * DFPLAYER_driver.c
 *
 * Created on: 4 May 2026
 * Author: whp27
 */

#include "FreeRTOS.h"
#include "task.h"
#include "DFPLAYER_driver.h"
#include <stdbool.h>
#include "main.h"
#include <stdint.h>

/* Serial Packet Protocol Structure Constants */
#define VERSION 0xFF
#define START_BYTE 0x7E
#define COMM_LENGTH 0x06
#define PLAY_MP3_FOLDER 0x12 // play /MP3/NNNN.mp3 by file NAME (0x03 would use FAT copy order)
#define PAUSE 0x0E
#define STOP 0x16
#define PREV 0x02
#define NEXT 0x01
#define SET_VOL 0x06
#define NONE 0x00
#define RESET 0x0C
#define END_BYTE 0xEF
#define FEEDBACK_BYTE 0x00   // no ACK replies needed; "track finished" (0x3D) is sent regardless
#define RESUME 0x0D
#define SELECT_DEVICE 0x09
#define DEVICE_SD     0x02
#define EVT_FINISHED_SD 0x3D // module -> MCU: track on the SD card finished playing
#define FRAME_LEN 10

/* Buffer Allocations and Hardware Flow Variables */
static volatile uint8_t uart_tx_ready = 1; // Tracks if the physical UART bus is free to transmit
static uint8_t pData[FRAME_LEN];    // Packet buffer passed directly to DMA (only rebuilt after TX complete)
static Queue q;                     // Software ring buffer instance managing packet serialization
extern UART_HandleTypeDef huart1;

/* Receive side: bytes arrive one at a time by interrupt and are assembled into frames */
static uint8_t rx_byte;
static uint8_t rx_frame[FRAME_LEN];
static uint8_t rx_index = 0;
static df_finished_cb_t finished_cb = NULL;

static void df_try_start_tx(void);
static bool is_empty(void);
static bool is_full(void);

/*
 * The TX queue is touched from the music task (enqueue) and from the UART TX
 * complete interrupt (dequeue), so every access goes through this lock. It
 * picks the ISR-safe critical section variant when called from an interrupt.
 */
static inline UBaseType_t df_lock(void) {
	if (__get_IPSR() != 0U) {
		return taskENTER_CRITICAL_FROM_ISR();
	}
	taskENTER_CRITICAL();
	return 0;
}

static inline void df_unlock(UBaseType_t saved) {
	if (__get_IPSR() != 0U) {
		taskEXIT_CRITICAL_FROM_ISR(saved);
	} else {
		taskEXIT_CRITICAL();
	}
}

/**
 * @brief Zeroes out the internal management cursors of the software queue.
 */
static void initQueue(void) {
	q.front = 0;
	q.rear = 0;
	q.count = 0;
}

/**
 * @brief Places a compiled command structure into the software transmission queue.
 * @param cmd The target command layout structure to buffer.
 * @return True if buffered successfully, False if the buffer is full.
 */
static bool enqueue(df_cmd_t cmd) {

	UBaseType_t s = df_lock();
	if (is_full()) {
		df_unlock(s);
		return false;
	}
	q.tx_buf[q.rear] = cmd;
	q.rear = (q.rear + 1) % MAX_SIZE;
	q.count++;
	df_unlock(s);

	// Kickoff physical transmission if the hardware bus is currently idle
	df_try_start_tx();
	return true;
}

/**
 * @brief Arms single-byte interrupt reception for messages coming from the module.
 */
static void df_rx_start(void) {
	HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

void df_set_finished_callback(df_finished_cb_t cb) {
	finished_cb = cb;
}

void df_player_init(void) {
	// send 0x0C reset
	initQueue();
	rx_index = 0;
	df_rx_start();
	df_cmd_t cmd = { .cmd = RESET, .param_high = NONE, .param_low = NONE };
	enqueue(cmd);
}

/**
 * @brief Plays /MP3/NNNN.mp3 on the SD card, where NNNN is the track number (1-3000).
 */
void df_play(uint16_t track) {
	uint8_t low_byte = (track & 0x00FF);
	uint8_t high_byte = ((track & 0XFF00) >> 8);
	df_cmd_t cmd = { .cmd = PLAY_MP3_FOLDER, .param_high = high_byte,
			.param_low = low_byte };
	enqueue(cmd);
}

void df_source_select(void) {
	uint8_t low_byte = (DEVICE_SD & 0x00FF);
	uint8_t high_byte = ((DEVICE_SD & 0XFF00) >> 8);
	df_cmd_t cmd = { .cmd = SELECT_DEVICE, .param_high = high_byte, .param_low =
			low_byte };
	enqueue(cmd);
}

void df_pause(void) {
	df_cmd_t cmd = { .cmd = PAUSE, .param_high = NONE, .param_low = NONE };
	enqueue(cmd);
}

void df_stop(void) {
	df_cmd_t cmd = { .cmd = STOP, .param_high = NONE, .param_low = NONE };
	enqueue(cmd);
}

void df_resume(void) {
	df_cmd_t cmd = { .cmd = RESUME, .param_high = NONE, .param_low = NONE };
	enqueue(cmd);
}

void df_set_volume(uint8_t vol) {
	if (vol > 30)
		vol = 30; //df player only gets to 30

	uint8_t low_byte = (vol & 0x00FF);
	uint8_t high_byte = ((vol & 0XFF00) >> 8);
	df_cmd_t cmd = { .cmd = SET_VOL, .param_high = high_byte, .param_low =
			low_byte };
	enqueue(cmd);
}

void df_change_track(uint8_t track) {

	uint8_t command = 0;
	if (track == TRACK_NEXT)
		command = NEXT;
	else
		command = PREV;
	df_cmd_t cmd = { .cmd = command, .param_high = NONE, .param_low =
	NONE };
	enqueue(cmd);
}

/**
 * @brief UART Transmission Complete Callback triggered automatically via HAL upon DMA completion.
 * @details Re-arms the ready flag inside interrupt context and pushes the next packet.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) { //runs in ISR
	if (huart->Instance == USART1) {
		uart_tx_ready = 1; // The conveyer belt is empty!
		df_try_start_tx();
	}
}

/**
 * @brief Checks a complete 10-byte frame from the module and reports "track finished".
 */
static void df_handle_frame(void) {
	if (rx_frame[1] != VERSION || rx_frame[2] != COMM_LENGTH
			|| rx_frame[9] != END_BYTE) {
		return;
	}
	uint16_t sum = 0;
	for (int i = 1; i <= 6; i++) {
		sum += rx_frame[i];
	}
	uint16_t checksum = (uint16_t) ((rx_frame[7] << 8) | rx_frame[8]);
	if ((uint16_t) (sum + checksum) != 0) {
		return; // corrupted frame
	}
	if (rx_frame[3] == EVT_FINISHED_SD && finished_cb != NULL) {
		finished_cb((uint16_t) ((rx_frame[5] << 8) | rx_frame[6]));
	}
}

/**
 * @brief UART Receive Complete Callback: one byte from the module (runs in ISR).
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		if (rx_index == 0 && rx_byte != START_BYTE) {
			// wait for the start of a frame
		} else {
			rx_frame[rx_index++] = rx_byte;
			if (rx_index >= FRAME_LEN) {
				df_handle_frame();
				rx_index = 0;
			}
		}
		df_rx_start();
	}
}

/**
 * @brief UART error (noise, overrun, framing): HAL stops reception, so restart it.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		rx_index = 0;
		if (huart->gState == HAL_UART_STATE_READY) {
			uart_tx_ready = 1; // a TX DMA error also ends up here
			df_try_start_tx();
		}
		df_rx_start();
	}
}

static bool is_empty(void) {
	return q.count == 0;
}

static bool is_full(void) {
	return q.count == MAX_SIZE;
}

/**
 * @brief Extracts the oldest pending data frame from the software buffer.
 * @note Caller must hold df_lock().
 * @param out Pointer destination to write the structure.
 * @return 1 if successfully extracted, 0 if empty.
 */
static bool dequeue(df_cmd_t *out) {

	if (is_empty()) {
		return 0;
	}
	*out = q.tx_buf[q.front];
	q.front = (q.front + 1) % MAX_SIZE;
	q.count--;

	return 1;
}

/**
 * @brief Packs data parameters into the 10-byte serial layout required by the DFPlayer.
 * @details Handles necessary two's-complement checksum algebra calculations dynamically.
 * @return Static pointer referencing the populated transmission array buffer.
 */
static uint8_t* df_build_packet(uint8_t cmd, uint8_t param_high, uint8_t param_low) {

	uint16_t checksum = 0;
	uint8_t checksum_data[6] = { VERSION, COMM_LENGTH, cmd, FEEDBACK_BYTE,
			param_high, param_low };

	for (uint8_t i = 0; i < 6; i++) {
		checksum += checksum_data[i];
	}
	checksum = 0 - checksum;
	uint8_t low_byte = (checksum & 0x00FF);
	uint8_t high_byte = ((checksum & 0XFF00) >> 8);

	pData[0] = START_BYTE;
	pData[1] = VERSION;
	pData[2] = COMM_LENGTH;
	pData[3] = cmd;
	pData[4] = FEEDBACK_BYTE;
	pData[5] = param_high;
	pData[6] = param_low;
	pData[7] = high_byte;
	pData[8] = low_byte;
	pData[9] = END_BYTE;

	return pData;
}

static void uart_tx(uint8_t *tx_buffer) {
	if (HAL_UART_Transmit_DMA(&huart1, tx_buffer, FRAME_LEN) != HAL_OK) {
		uart_tx_ready = 1; // didn't start; the command is dropped rather than wedging the queue
	}
}

/**
 * @brief Starts sending the next queued command if the UART is idle.
 * @details Called from the music task (after enqueue) and from the TX complete ISR.
 */
static void df_try_start_tx(void) {
	df_cmd_t cmd;
	bool start = false;

	UBaseType_t s = df_lock();
	if (uart_tx_ready && dequeue(&cmd)) {
		uart_tx_ready = 0; // claimed: nobody else can start a transfer now
		start = true;
	}
	df_unlock(s);

	if (start) {
		uart_tx(df_build_packet(cmd.cmd, cmd.param_high, cmd.param_low));
	}
}
