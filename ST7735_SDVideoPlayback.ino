// ST7735 library example
// Requires SdFat, Arduino_ST7735_STM libraries and stm32duino
// (C)2019 Pawel A. Hernik
// YT video: https://youtu.be/o3AqITHf0mo 

/*
 ST7735 128x160 1.8" LCD pinout (header at the top, from left):
 #1 LED   -> 3.3V
 #2 SCK   -> SCL/D13/PA5
 #3 SDA   -> MOSI/D11/PA7
 #4 A0/DC -> D8/PA1  or any digital
 #5 RESET -> D9/PA0  or any digital
 #6 CS    -> D10/PA2 or any digital
 #7 GND   -> GND
 #8 VCC   -> 3.3V

           SPI2/SPI1
 SD_SCK  - PB13/PA5
 SD_MISO - PB14/PA6
 SD_MOSI - PB15/PA7
 SD_CS   - PB12/PA4
*/

/*
 STM32 SPI1/SPI2 pins:
 
 SPI1 MOSI PA7
 SPI1 MISO PA6
 SPI1 SCK  PA5
 SPI1 CS   PA4

 SPI2 MOSI PB15
 SPI2 MISO PB14
 SPI2 SCK  PB13
 SPI2 CS   PB12
*/

/*
 Comments:
 SD uses faster STM32 SPI1 interface which supports 36 Mbps
 Not all SD cards work at 36MBps
 Slow card at 18Mbps gives 22-23fps
 Fast card at 18Mbps gives 25-26fps
 Fast card at 36Mbps gives 33-34fps
 SdFat library uses DMA for SPI transfer
 Big buffer in RAM is used to speed up SPI/DMA transfer
 SPI1 is shared between LCD and SD card
*/

#include <SPI.h>
#include <Adafruit_GFX.h>

#if (__STM32F1__) // bluepill
#define TFT_CS  PA2
#define TFT_DC  PA1
#define TFT_RST PA0
#include <Arduino_ST7735_STM.h>
#else
#define TFT_CS 10
#define TFT_DC  8
#define TFT_RST 9
//#include <Arduino_ST7735_Fast.h>
#endif

#define SCR_WD 128
#define SCR_HT 160
Arduino_ST7735 lcd = Arduino_ST7735(TFT_DC, TFT_RST, TFT_CS);

#include "SdFat.h"

#define USE_SDIO 0
//const uint8_t SD_CS = PB12;
//SdFat sd(2);
const uint8_t SD_CS = PA4;
SdFat sd(1);
// serial output steam
ArduinoOutStream cout(Serial);
#define sdErrorMsg(msg) sd.errorPrint(F(msg));

SdFile file;

void lcdSPI()
{
  SPI.beginTransaction(SPISettings(36000000, MSBFIRST, SPI_MODE3, DATA_SIZE_16BIT));
}

#define SD_SPEED 36
void sdSPI()
{
  SPI.beginTransaction(SD_SCK_MHZ(SD_SPEED));
}

// ------------------------------------------------
#define BUTTON PB9
int buttonState;
int prevState = HIGH;
long btDebounce    = 30;
long btMultiClick  = 600;
long btLongClick   = 500;
long btLongerClick = 2000;
long btTime = 0, btTime2 = 0;
int clickCnt = 1;

// 0=idle, 1,2,3=click, -1,-2=longclick
int checkButton()
{
  int state = digitalRead(BUTTON);
  if( state == LOW && prevState == HIGH ) { btTime = millis(); prevState = state; return 0; } // button just pressed
  if( state == HIGH && prevState == LOW ) { // button just released
    prevState = state;
    if( millis()-btTime >= btDebounce && millis()-btTime < btLongClick ) { 
      if( millis()-btTime2<btMultiClick ) clickCnt++; else clickCnt=1;
      btTime2 = millis();
      return clickCnt; 
    } 
  }
  if( state == LOW && millis()-btTime >= btLongerClick ) { prevState = state; return -2; }
  if( state == LOW && millis()-btTime >= btLongClick ) { prevState = state; return -1; }
  return 0;
}

int prevButtonState=0;

int handleButton()
{
  prevButtonState = buttonState;
  buttonState = checkButton();
  return buttonState;
}

// --------------------------------------------------------------------------
#define NLINES 30
uint16_t buf[200*NLINES]; 
char txt[30];
int statMode=0, prevStat=0;

// Params:
// name - file name
// x,y - start x,y on the screen
// wd,ht - width, height of the video (raw data has no header with such info)
// nl - num lines read in one operation (nl*wd*2 bytes are loaded)
// skipFr - num frames to skip
void showVideo(char *name, int x, int y, int wd, int ht, int nl, int skipFr)
{
  lcd.setTextColor(YELLOW,BLACK);
  if(!file.open(name, O_CREAT | O_RDONLY)) {
    lcdSPI(); lcd.fillScreen(YELLOW);
    sd.errorHalt(F("File open failed"));
  }
  file.seekSet(0);
  unsigned long sdStartTime,frTime,lcdTime,sdTime=0;
  handleButton();
  while(file.available()) {
    sdTime = 0;
    frTime = millis();
    for(int i=0;i<ht/nl;i++) {
      sdStartTime = millis();
      int rd = file.read(buf,wd*2*nl);
      sdTime += millis()-sdStartTime;
      lcdSPI();
      for(int j=0;j<nl;j++) lcd.drawImage(0,i*nl+j+(statMode>0?0:4),lcd.width(),1,buf+20+j*wd);
    }
    frTime = millis()-frTime;
    lcdTime = frTime-sdTime;
    lcd.setCursor(0,lcd.height()-7);
    if(buttonState>0) {
      if(++statMode>2) statMode=0;
    }
    if(statMode!=prevStat) {
      lcdSPI();
      if(statMode==0) { lcd.fillRect(0,0,lcd.width(),4,BLACK); lcd.fillRect(0,lcd.height()-4,lcd.width(),4,BLACK); }
      else lcd.fillRect(0,lcd.height()-8,lcd.width(),8,BLACK);
    }
    if(statMode==1)
      snprintf(txt,30,"Fr/SD/LCD: %2ld/%2ld/%2ld %2d fps",frTime,sdTime,lcdTime,1000/frTime);
    if(statMode==2)
      snprintf(txt,30,"%2ld fps",1000/frTime);
    if(statMode>0) {
      lcdSPI(); lcd.print(txt);
    }
    prevStat = statMode;
    if(skipFr>0) file.seekCur(wd*ht*2*skipFr);
    if(handleButton()<0) break;
  }
  if(buttonState<0 && prevButtonState==0) nextVideo();
  file.close();
}
// --------------------------------------------------------------------------

void setup(void)
{
  Serial.begin(115200);
  pinMode(BUTTON, INPUT_PULLUP);
  lcd.init();
  lcd.setRotation(3);
  lcd.fillScreen(BLACK);
  lcd.setCursor(0,20); lcd.print("Init");

  //delay(8000);
  if(!sd.cardBegin(SD_CS, SD_SCK_MHZ(SD_SPEED))) {
    lcdSPI(); lcd.fillScreen(RED);
    sdErrorMsg("\nSD Card initialization failed.\n");
  }
  if(!sd.fsBegin()) {
    lcdSPI(); lcd.fillScreen(MAGENTA);
    sdErrorMsg("\nFile System initialization failed.\n");
    return;
  }
  //sd.ls("/",0);
}

// --------------------------------------------------------------------------

int videoNo=0;

void nextVideo()
{
  if(++videoNo>1) videoNo=0;
}

void loop(void)
{
  if(videoNo==0) { showVideo("food22.raw",0,0, 200,120, 30,1); if(buttonState==0) nextVideo(); }
  else           { showVideo("cars120.raw",0,0, 200,120, 30,0); if(buttonState==0) nextVideo(); }
}

// ------------------------------------------------

