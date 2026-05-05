#include "DFPLAYER_driver.h"
#include <stdbool.h>
#include "main.h"
#include <stdint.h>

#define VERSION 0xFF
#define START_BYTE 0x7E
#define COMM_LENGTH 0x06
#define PLAY 0x03
#define PAUSE 0x0E
#define STOP 0x16
#define PREV 0x02
#define NEXT 0x01
#define SET_VOL 0x06
#define NONE 0x00
#define RESET 0x0C
#define END_BYTE 0xEF
#define FEEDBACK_BYTE 0x01
#define REPEAT_PLAY 0x11
#define RESUME 0x0D
#define SELECT_DEVICE 0x09
#define DEVICE_SD     0x02
#define PLAYBACK_FINISHED 0x3D
#define DF_RX_BUF_SIZE 64

uint8_t df_rx_buf[DF_RX_BUF_SIZE];
volatile uint16_t df_old_pos = 0;
volatile uint8_t uart_tx_ready = 1;
volatile uint8_t uart_rx_ready = 0;
static uint8_t pData[10];
uint8_t playback_status = 0; // 1 <= playback finished
Queue q;
extern UART_HandleTypeDef huart1;
uint8_t rx_buffer[10];

void df_try_start_tx();
bool is_empty();
bool is_full();
void source_select();

//check for mSD card error and pass to Service----------------
//add error checking for df player

void initQueue() {
	q.front = 0;
	q.rear = 0;
	q.count = 0;
}

bool enqueue(df_cmd_t cmd) {

	if (is_full()) {
		return false;
	}

	__disable_irq();
	q.tx_buf[q.rear] = cmd;
	q.rear = (q.rear + 1) % MAX_SIZE;
	q.count++;
	__enable_irq();
	df_try_start_tx();
	return true;

}

// of enqueue fails return false----------------------------------------------------
void df_player_init() {
	// send 0x0C reset
	initQueue();
	df_cmd_t cmd = { .cmd = RESET, .param_high = NONE, .param_low = NONE };
	set_volume(5);
	source_select();
	enqueue(cmd);
}

void play(uint16_t track) { //if track != 0000 you play a new track
	uint8_t low_byte = (track & 0x00FF);
	uint8_t high_byte = ((track & 0XFF00) >> 8);
	df_cmd_t cmd =
			{ .cmd = PLAY, .param_high = high_byte, .param_low = low_byte };
	enqueue(cmd);
}

void source_select() {
	uint8_t source = 2;
	uint8_t low_byte = (DEVICE_SD & 0x00FF);
	uint8_t high_byte = ((DEVICE_SD & 0XFF00) >> 8);
	df_cmd_t cmd = { .cmd = SELECT_DEVICE, .param_high = high_byte, .param_low =
			low_byte };
	enqueue(cmd);
}

void pause() {
	df_cmd_t cmd = { .cmd = PAUSE, .param_high = NONE, .param_low = NONE };
	enqueue(cmd);
}

void stop() {
	df_cmd_t cmd = { .cmd = STOP, .param_high = NONE, .param_low = NONE };
	enqueue(cmd);
}

void resume() {
	df_cmd_t cmd = { .cmd = RESUME, .param_high = NONE, .param_low = NONE };
	enqueue(cmd);
}

//-----------------------------------------------------------------------------------use for 1 hour playback if needed
void repeat(uint16_t track) { //Repeat tracks for longer play
	uint8_t low_byte = (track & 0x00FF);
	uint8_t high_byte = ((track & 0XFF00) >> 8);
	df_cmd_t cmd = { .cmd = REPEAT_PLAY, .param_high = high_byte, .param_low =
			low_byte };
	enqueue(cmd);
}

//--------------------------------------------------------------------------------------------

void set_volume(uint8_t vol) {
	if (vol > 30)
		vol = 30; //df player only gets to 30
	if (vol < 0)
		vol = 0;
	uint8_t low_byte = (vol & 0x00FF);
	uint8_t high_byte = ((vol & 0XFF00) >> 8);
	df_cmd_t cmd = { .cmd = SET_VOL, .param_high = high_byte, .param_low =
			low_byte };
	enqueue(cmd);

}

void change_track(uint8_t track) {

	uint8_t command = 0;
	if (track == TRACK_NEXT)
		command = NEXT;
	else
		command = PREV;
	df_cmd_t cmd = { .cmd = command, .param_high = NONE, .param_low =
	NONE };
	enqueue(cmd);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) { //runs in ISR
	if (huart->Instance == USART1) {
		uart_tx_ready = 1; // The conveyer belt is empty!
		df_try_start_tx();
	}
}

bool is_empty() {
	return q.count == 0;
}

bool is_full() {
	return q.count == MAX_SIZE;
}

bool dequeue(df_cmd_t *out) {

	if (is_empty()) {
		return 0;
	}
	*out = q.tx_buf[q.front];
	q.front = (q.front + 1) % MAX_SIZE;
	q.count--;

	return 1;
}

uint8_t* df_build_packet(uint8_t cmd, uint8_t param_high, uint8_t param_low) {

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

void uart_tx(uint8_t *tx_buffer) {
	HAL_UART_Transmit_DMA(&huart1, tx_buffer, 10);
}

void df_try_start_tx() {

	if (uart_tx_ready && !is_empty()) {
		uart_tx_ready = 0;
		df_cmd_t cmd;
		if (dequeue(&cmd)) {
			uart_tx(df_build_packet(cmd.cmd, cmd.param_high, cmd.param_low));
		}

	}

}


