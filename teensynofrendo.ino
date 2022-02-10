extern "C" {
  #include "iopins.h"  
  #include "emuapi.h"  
}
#include "keyboard_osd.h"
#include "Wire.h" 

extern "C" {
#include "nes_emu.h"
}

#include "tft_t_dma.h"
TFT_T_DMA tft = TFT_T_DMA(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO, TFT_TOUCH_CS, TFT_TOUCH_INT);

bool vgaMode = false;

static unsigned char  palette8[PALETTE_SIZE];
static unsigned short palette16[PALETTE_SIZE];
static IntervalTimer myTimer;
volatile boolean vbl=true;
static int skip=0;
static elapsedMicros tius;

static void vblCount() { 
  if (vbl) {
    vbl = false;
  } else {
    vbl = true;
  }
}

void emu_SetPaletteEntry(unsigned char r, unsigned char g, unsigned char b, int index)
{
  if (index<PALETTE_SIZE) {
    //Serial.println("%d: %d %d %d\n", index, r,g,b);
    palette8[index]  = RGBVAL8(r,g,b);
    palette16[index] = RGBVAL16(r,g,b);   
  }
}

void emu_DrawVsync(void)
{
  volatile boolean vb=vbl;
  skip += 1;
  skip &= VID_FRAME_SKIP;
  if (!vgaMode) {
    while (vbl==vb) {};
  }
}

void emu_DrawLine(unsigned char * VBuf, int width, int height, int line) 
{
  if (!vgaMode) {
    tft.writeLine(width,1,line, VBuf, palette16);
  }
}  

void emu_DrawLine16(unsigned short * VBuf, int width, int height, int line) 
{
  if (!vgaMode) {
    if (skip==0) {
      tft.writeLine(width,height,line, VBuf);
    }
  }   
} 


void emu_DrawScreen(unsigned char * VBuf, int width, int height, int stride) 
{
  if (!vgaMode) {
    if (skip==0) {
      tft.writeScreen(width,height-TFT_VBUFFER_YCROP,stride, VBuf+(TFT_VBUFFER_YCROP/2)*stride, palette16);
    }
  } 
}

int emu_FrameSkip(void)
{
  return skip;
}

void * emu_LineBuffer(int line)
{
  if (!vgaMode) {
    return (void*)tft.getLineBuffer(line);    
  } 
}

// ****************************************************
// the setup() method runs once, when the sketch starts
// ****************************************************
void setup() {
  
  tft.begin();

  emu_init(); 

  myTimer.begin(vblCount, 16666);  // 60Hz = 1/0.016666

  Wire1.begin();
  
  Wire1.beginTransmission(0x20);
  Wire1.write(0x00);
  Wire1.write(0xFF);
  Wire1.endTransmission();

  Wire1.beginTransmission(0x20);
  Wire1.write(0x06);
  Wire1.write(0xFF);
  Wire1.endTransmission();
  
}

// ****************************************************
// the loop() method runs continuously
// ****************************************************
void loop(void) 
{
  if (menuActive()) {
    uint16_t bClick = emu_DebounceLocalKeys();
    int action = handleMenu(bClick);
    char * filename = menuSelection();   
    if (action == ACTION_RUNTFT) {
      toggleMenu(false);
      vgaMode = false;
      emu_start();        
      emu_Init(filename);
      //digitalWrite(TFT_CS, 1);
      //digitalWrite(SD_CS, 1);       
      tft.fillScreenNoDma( RGBVAL16(0x00,0x00,0x00) );
      tft.startDMA();      
    }    
    else if (action == ACTION_RUNVGA)  {         
    }         
    delay(20);
  }
  else if (callibrationActive()) {
    uint16_t bClick = emu_DebounceLocalKeys();
    handleCallibration(bClick);
  } 
  else {
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)    
#else
    handleVirtualkeyboard();
#endif    
    if ( (!virtualkeyboardIsActive()) || (vgaMode) ) {  
      emu_Step();   
      uint16_t bClick = emu_DebounceLocalKeys();
      emu_Input(bClick);      
    }
  }  
}

#include "AudioPlaySystem.h"

AudioPlaySystem mymixer;
#ifndef HAS_T4_VGA
#include <Audio.h>
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)    
//AudioOutputMQS  mqs;
//AudioConnection patchCord9(mymixer, 0, mqs, 1);
AudioOutputI2S  i2s1;
AudioConnection patchCord8(mymixer, 0, i2s1, 0);
AudioConnection patchCord9(mymixer, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;
#else
AudioOutputAnalog dac1;
AudioConnection   patchCord1(mymixer, dac1);
#endif
#endif

void emu_sndInit() {
  Serial.println("sound init");  
  mymixer.start();
}

void emu_sndPlaySound(int chan, int volume, int freq)
{
  if (chan < 6) {
    mymixer.sound(chan, freq, volume); 
  }
  /*
  Serial.print(chan);
  Serial.print(":" );  
  Serial.print(volume);  
  Serial.print(":" );  
  Serial.println(freq); 
  */ 
}

void emu_sndPlayBuzz(int size, int val) {
  mymixer.buzz(size,val); 
  //Serial.print((val==1)?1:0); 
  //Serial.print(":"); 
  //Serial.println(size); 
}
