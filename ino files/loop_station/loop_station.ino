#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "Adafruit_Trellis.h"

#include "preset1.h"
#include "preset2.h"
#include "preset3.h"

#define LEDREC 3
#define RECBLINK 600

AudioPlayMemory    sound0;
AudioPlayMemory    sound1;  // six memory players, so we can play
AudioPlayMemory    sound2;  // all six sounds simultaneously
AudioPlayMemory    sound3;
AudioPlayMemory    sound4;
AudioPlayMemory    sound5;

AudioPlaySdWav playWav;

AudioMixer4 mix1;
AudioMixer4 finalMix;

AudioRecordQueue queue1;
AudioOutputI2S audioOutput;

AudioConnection c1(sound0, 0, mix1, 0);
AudioConnection c2(sound1, 0, mix1, 1);
AudioConnection c3(sound2, 0, mix1, 2);
AudioConnection c4(sound3, 0, mix1, 3);
AudioConnection c5(mix1, 0, finalMix, 0); 
AudioConnection c6(sound4, 0, finalMix, 1);
AudioConnection c7(sound5, 0, finalMix, 2);
AudioConnection c8(playWav, 0, finalMix, 3);

AudioConnection patchCord0(finalMix, 0, audioOutput, 0);
AudioConnection patchCord1(finalMix, 0, audioOutput, 1);
AudioControlSGTL5000 sgtl5000_1;

Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0);

#define numKeys 16

// [Trellis] INT wire to pin #A2 (SCL to SCL and SDA to SDA is default)
#define INTPIN A2

// [Teensy Audio Shield]
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

const int ledInterval = 200;

unsigned long currentMillis = 0;
unsigned long previousLedMillis[numKeys];

unsigned long startReplayTime;

uint8_t preset=1;
bool rec=false;

// (i,j) = (button, timming)
// le premier element du tableu indique si il y a ou pas un enregistrement
// l'enregistrement finit avec une touche fictive -1
#define TOUCH 0
#define TIME 1
#define loopSize 100

long int recMem[2][2][loopSize];
uint8_t swap=0;
uint8_t antiSwap=1;
uint8_t recIndex=1;
uint8_t replayIndex=0;

void setup() {
  Serial.begin(9600);
  Serial.println("Teensy LoopStation debug");

  AudioMemory(60);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.7);

  mix1.gain(0, 0.4);
  mix1.gain(1, 0.4);
  mix1.gain(2, 0.4);
  mix1.gain(3, 0.4);
  finalMix.gain(0, 0.4);
  finalMix.gain(1, 0.4);
  finalMix.gain(2, 0.4);
  finalMix.gain(3, 0.4);

  // INT pin requires a pullup
  pinMode(INTPIN, INPUT);
  digitalWrite(INTPIN, HIGH);
  trellis.begin(0x70);

  // SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    trellisLedErrorAnimation();
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

  // rec params
  recMem[swap][TOUCH][0]=-1;
  trellisLedFrontAnimation();
}

void trellisLedErrorAnimation() {
  for (uint8_t i=0; i<numKeys; i++) {
    if(i%5==0) {
      trellis.setLED(i);
      trellis.writeDisplay();
      delay(30); 
    }
  }

  for (uint8_t i=0; i<numKeys; i++) {
    if(i%3==0) {
      trellis.setLED(i);
      trellis.writeDisplay();
      delay(30); 
    }
  }
}

void trellisLedFrontAnimation(){
  // light up all the LEDs in order
  for (uint8_t i=0; i<numKeys; i++) {
    if((i+1)%4==0) continue;
    trellis.clrLED(i);
    if((i+2)%4==0) {
     trellis.writeDisplay();
     delay(45);
    } 
  }

  // then turn them off
  for (uint8_t i=0; i<numKeys; i++) {
    if((i+1)%4==0) continue;
    trellis.setLED(i);
    if((i+2)%4==0) {
     trellis.writeDisplay();
     delay(45);
    } 
  }
}

void trellisLedBlinkAnimation(){
  for(int8_t k=0;k<4;k++) {
    for (int8_t i=numKeys-1; i>=0; i--) {
      if((i+1)%4==0) continue;
      trellis.clrLED(i);    
    }
    trellis.writeDisplay();
    delay(80);
    for (int8_t i=numKeys-1; i>=0; i--) {
      if((i+1)%4==0) continue;
      trellis.setLED(i);    
    }
    trellis.writeDisplay();
    delay(75);
    }
}

void trellisLedBackAnimation(){
  // light up all the LEDs in order
  for (int8_t i=numKeys-1; i>=0; i--) {
    if((i+1)%4==0) continue;
    trellis.clrLED(i);
    if(i%4==0) {
     trellis.writeDisplay();
     delay(45);
    }   
  }

  // then turn them off
  for (int8_t i=numKeys-1; i>=0; i--) {
    if((i+1)%4==0) continue;
    trellis.setLED(i);
    if(i%4==0) {
     trellis.writeDisplay();
     delay(45);
    } 
  }
}

void dropAnimation(uint8_t i){
  if((i+1)%4 == 0) return;
  if((i+2)%4 != 0) {
    trellis.clrLED(i+1); 
    previousLedMillis[i+1]=currentMillis+(ledInterval/6);
  }
  if(i%4 != 0) {
    trellis.clrLED(i-1);
    previousLedMillis[i-1]=currentMillis+(ledInterval/6);
  }  
  if(i>3) {
    trellis.clrLED(i-4); 
    previousLedMillis[i-4]=currentMillis+(ledInterval/6);
  } 
  if(i<12) {
    trellis.clrLED(i+4); 
    previousLedMillis[i+4]=currentMillis+(ledInterval/6);
  }
  trellis.writeDisplay(); 
}

void recAnimation() {
  if(currentMillis - previousLedMillis[LEDREC] >= RECBLINK) {
    if (trellis.isLED(LEDREC)) {
      trellis.clrLED(LEDREC);
      previousLedMillis[LEDREC]=currentMillis;
    } else {
      trellis.setLED(LEDREC);
      previousLedMillis[LEDREC]=currentMillis;
    }
    trellis.writeDisplay();
  }
}

void updateLeds(){
  if(rec) recAnimation();
  else if(trellis.isLED(LEDREC)) {
    trellis.clrLED(LEDREC);
    trellis.writeDisplay();
  }
  
  for (uint8_t i=0; i<numKeys; i++) {
    if((i+1)%4==0) continue;
    if (!trellis.isLED(i) && (currentMillis - previousLedMillis[i] >= ledInterval)){
      trellis.setLED(i);
      trellis.writeDisplay();
    }
  }
}

void loop() {
  delay(30); // 30ms delay is required, dont remove me!
  
  currentMillis = millis();
  updateLeds();
  
  if(anythingRecorded()) playRec();

  if (trellis.readSwitches()) {
    for (uint8_t i=0; i<numKeys; i++) {
      if (trellis.justPressed(i))
        playSketchAndAnimate(i);
    }
  }
}

void switchRecord() {
  if(rec) stopRecording();
  else {
    recMem[swap][TIME][0]=currentMillis;
    rec=true;
  }
}

void switchPreset() {
  preset=((preset+1)%3)+1;
  
  switch(preset){
    case 1:
      trellis.setLED(7);
      trellis.clrLED(11);
      trellis.clrLED(15);
      trellis.writeDisplay();
      trellisLedFrontAnimation();
      break;
    case 2:
      trellis.clrLED(7);
      trellis.setLED(11);
      trellis.setLED(15);
      trellis.writeDisplay();
      trellisLedBlinkAnimation();
      break;
    case 3:
      trellis.clrLED(7);
      trellis.clrLED(11);
      trellis.setLED(15);
      trellis.writeDisplay();
      trellisLedBackAnimation();
      break;
    default:
      Serial.println("Unexpected behaviour");
      break;
  }
}

void clearLoopMemory() {
  recMem[0][TOUCH][0] = -1; 
  recMem[1][TOUCH][0] = -1;
}

void playSketchAndAnimate(uint8_t i) {
  trellis.clrLED(i);
  trellis.writeDisplay();
  
  if(rec) record(i);
  
  switch(i){
    case 0:
      if(preset == 1) playWav.play("LIB/PRESET2/SYNTH2.WAV");
      else if(preset == 2) playWav.play("LIB/PRESET1/SALSA.WAV");
      else playWav.play("LIB/PRESET3/GUN1.WAV");
      dropAnimation(i);
      break;
    case 1:
      if(preset == 1) playWav.play("LIB/PRESET2/RYTHM.WAV");
      else if(preset == 2) playWav.play("LIB/PRESET1/WAO.WAV");
      else playWav.play("LIB/PRESET3/MELO.WAV");
      break;
    case 2:
      if(preset == 1) playWav.play("LIB/PRESET2/SYNTH1.WAV");
      else if(preset == 2) playWav.play("LIB/PRESET1/FUNK.WAV");
      else playWav.play("LIB/PRESET3/GUN2.WAV");
      dropAnimation(i);
      break;
    case 3:
      switchRecord();
      break;
    case 4:
      if(preset == 1) playWav.play("LIB/PRESET3/PIANO1.WAV");
      else if(preset == 2) playWav.play("LIB/PRESET1/BASS.WAV");
      else playWav.play("LIB/PRESET3/PIANO1.WAV");
      break;
    case 5:
      if(preset == 1) playWav.play("LIB/PRESET2/SYNTH3.WAV");
      else if(preset == 2) playWav.play("LIB/PRESET1/WAO.WAV");
      else playWav.play("LIB/PRESET3/FUNK.WAV");
      break;
    case 6:
      if(preset == 1) playWav.play("LIB/PRESET3/PIANO2.WAV");
      else if(preset == 2) playWav.play("LIB/PRESET1/BASS.WAV");
      else playWav.play("LIB/PRESET3/PIANO2.WAV");
      break;
    case 7:
      clearLoopMemory();
      trellisLedFrontAnimation();
      trellisLedFrontAnimation();
      trellisLedBlinkAnimation();
      break;
    case 8:
      if(preset == 1) sound0.play(Crash1);
      else if(preset == 2) sound0.play(Crash2);
      else sound0.play(Shaker3);
      break;
    case 9:
      if(preset == 1) sound1.play(Cowbell1);
      else if(preset == 2) sound1.play(Tom2);
      else sound1.play(Tom3);
      break;
    case 10:
      if(preset == 1) sound2.play(Snare1);
      else if(preset == 2) sound2.play(Snare2);
      else sound2.play(Snare3);
      break;
    case 11:
      //setPreset(i);
      //Serial.println(i);
      //trellisLedBlinkAnimation();
      break;
    case 12:
      //dropAnimation(i);
      if(preset == 1) sound3.play(Kick1);
      else if(preset == 2) sound3.play(Kick2);
      else sound3.play(Kick3);
      break;
    case 13:
      if(preset == 1) sound4.play(Clap1);
      else if(preset == 2) sound4.play(Openhat2);
      else sound4.play(Openhat3);
      break;
    case 14:
      //dropAnimation(i);
      if(preset == 1) sound5.play(Hihat1);
      else if(preset == 2) sound5.play(Hihat2);
      else sound5.play(Hihat3);
      break;
    case 15:
      switchPreset();
      //trellisLedBackAnimation();
      break;
    default:
      Serial.println("Unexpected default in playSketchAndAnimate's switch case"); 
      break;
  }

  previousLedMillis[i]=currentMillis;
}

bool anythingRecorded() { return recMem[0][TOUCH][0] == -2 || recMem[1][TOUCH][0] == -2; }

void swapSwap() {
  antiSwap=swap;
  if(swap == 1) swap=0;
  else swap=1;
}

void stopRecording() {
  if(anythingRecorded()) {
    uint8_t k=0;
    while(recMem[antiSwap][TOUCH][k] != LEDREC && k<loopSize) {
      if(k>=recIndex) {
        recMem[swap][TOUCH][k] = recMem[antiSwap][TOUCH][k];
        recMem[swap][TIME][k] = recMem[antiSwap][TIME][k];
      }
      k++;
    }
  }
  recIndex = 1;
  recMem[swap][TOUCH][0]=-2;
  swapSwap();
  recMem[swap][TOUCH][0]=-1;
  replayIndex=0;
  rec=false;
}

void record(uint8_t i) {
  if(recIndex >= 0) {
    recMem[swap][TOUCH][recIndex]=i;
    recMem[swap][TIME][recIndex]=currentMillis;
  } else Serial.println("Unexpected record with recIndex = 0"); 
  recIndex++;

  if(recIndex >= loopSize)
    stopRecording();
}

void playRec() {
  if(replayIndex == 0) {
    startReplayTime = currentMillis;
    replayIndex++;
  }

  if((currentMillis - startReplayTime) >= (recMem[antiSwap][TIME][replayIndex]-recMem[antiSwap][TIME][0])) {
    if(recMem[antiSwap][TOUCH][replayIndex] != LEDREC) playSketchAndAnimate(recMem[antiSwap][TOUCH][replayIndex]);
    replayIndex= recMem[antiSwap][TOUCH][replayIndex] == LEDREC ? 0 : (replayIndex+1)%loopSize;
  }
}
