# HOPE Song List

The songs the device knows about, in order. The **title** is what appears on the screen. The **file** is what must be on the SD card, inside a folder called `MP3`.

To change the list, edit the titles below and send it back (or ask Claude to "update the song list from songs.md"). The code that has to match is `song_list[]` in `Core/Src/UI/ui_renderer.c`.

| # | SD card file | Title shown on screen | Notes |
|---|---|---|---|
| 1 | `MP3/0001.mp3` | Twinkle Twinkle | |
| 2 | `MP3/0002.mp3` | Amazing Grace | |
| 3 | `MP3/0003.mp3` | You are my sunshine | |
| 4 | `MP3/0004.mp3` | Piano | |
| 5 | `MP3/0005.mp3` | Rain And Piano | |
| 6 | `MP3/0006.mp3` | Lullaby 6 | ⚠️ placeholder (the title was blank), needs the real name |
| 7 | `MP3/0007.mp3` | Brahms Lullaby | |
| 8 | `MP3/0008.mp3` | Rock-a-bye Baby | |
| 9 | `MP3/0009.mp3` | Hush Little Baby | |
| 10 | `MP3/0010.mp3` | Frere Jacques | |
| 11 | `MP3/0011.mp3` | Row Your Boat | |
| 12 | `MP3/0012.mp3` | Baa Baa Black Sheep | |
| 13 | `MP3/0013.mp3` | Itsy Bitsy Spider | |
| 14 | `MP3/0014.mp3` | Wheels on the Bus | |
| 15 | `MP3/0015.mp3` | Mary Had a Lamb | |
| 16 | `MP3/0016.mp3` | Silent Night | |
| 17 | `MP3/0017.mp3` | All the Pretty Little Horses | |
| 18 | `MP3/0018.mp3` | Golden Slumbers | |
| 19 | `MP3/0019.mp3` | Schubert Lullaby | |
| 20 | `MP3/0020.mp3` | Sleep Baby Sleep | |
| 21 | `MP3/0021.mp3` | Go to Sleep Little Baby | |
| 22 | `MP3/0022.mp3` | Are You Sleeping | |
| 23 | `MP3/0023.mp3` | Somewhere Over the Rainbow | |
| 24 | `MP3/0024.mp3` | Beautiful Dreamer | |
| 25 | `MP3/0025.mp3` | Summertime | |
| — | `MP3/0026.mp3` | *(not in the list)* | **Alarm sound.** Played when an alarm goes off (`ALARM_TONE` in `ui_task.c`) |

## Things to know when picking titles
- **Length:** the song list shows about **19 characters** per row; longer titles get cut off there. The player screen shows 17 characters and scrolls longer titles, so they're fine there.
- **Characters:** stick to plain English letters, numbers and basic punctuation. The screen font has no accented letters (é, ü…) or emoji.
- **Number of songs:** currently fixed at 25. If you want more or fewer, say so and the code (`MAX_SONGS` and the song list) will be updated to match.
- **File names:** the 4-digit number is what matters. `0001.mp3` is the safest; most modules also accept extra text after it, e.g. `0001_twinkle.mp3`. Format the card as FAT32.
