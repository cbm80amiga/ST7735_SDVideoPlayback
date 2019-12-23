# ST7735_SDVideoPlayback
Smooth video playback from SD on STM32 and ST7735 1.8" 128x160 using DMA and fast SPI 36Mbps interface

YouTube video:

https://youtu.be/o3AqITHf0mo


## Connections (header at the top):

|LCD pin|LCD pin name|Arduino|
|--|--|--|
 |#01| LED| 3.3V|
 |#02| SCK |PA5/SCK|
 |#03| SCA |PA7/MOSI|
 |#04| A0/DC|PA1 or any digital
 |#05| RESET|PA0 or any digital|
 |#06| CS|PA2 or any digital|
 |#07| GND | GND|
 |#08| VCC | 3.3V|

|SD pin|SD pin name|Arduino|
|--|--|--|
|#01| SD_SCK| PA5|
|#02| SD_MISO |PA6|
|#03| SD_MOSI |PA7|
|#04| SD_CS |PA4|


## Comments:
- Tested with stm32duino and Arduino IDE 1.6.5
- SD uses faster STM32 SPI1 interface which supports 36 Mbps
- Not all SD cards work at 36MBps
- Slow card at 18Mbps gives 22-23fps
- Fast card at 18Mbps gives 25-26fps
- Fast card at 36Mbps gives 33-34fps
- SdFat library uses DMA mode for SPI transfer
- Big buffer in RAM is used to speed up SPI/DMA transfer from SD
- SPI1 is shared between LCD and SD card
- Demo videos used in the video are taken from my older project and are not optimal (200x120 resolution - only 160x120 part is displayed on LCD)
 
