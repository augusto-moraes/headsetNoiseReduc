#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "Adafruit_Trellis.h"

#include "Preset1.h"
#include "Preset2.h"
#include "Preset3.h"

#define LEDREC 3 // pad key to start/stop recording
#define RECBLINK 600 // blinking rate of the recording LED

AudioPlayMemory sound0;
AudioPlayMemory sound1;  // six memory players, so we can play
AudioPlayMemory sound2;  // all six sounds simultaneously
AudioPlayMemory sound3;
AudioPlayMemory sound4;
AudioPlayMemory sound5;

AudioPlaySdWav playWav;

AudioMixer4 mix1;
AudioMixer4 finalMix;

AudioConnection c1(sound0, 0, mix1, 0);
AudioConnection c2(sound1, 0, mix1, 1);
AudioConnection c3(sound2, 0, mix1, 2);
AudioConnection c4(sound3, 0, mix1, 3);
AudioConnection c5(mix1, 0, finalMix, 0); 
AudioConnection c6(sound4, 0, finalMix, 1);
AudioConnection c7(sound5, 0, finalMix, 2);
AudioConnection c8(playWav, 0, finalMix, 3);

AudioOutputI2S audioOutput;

AudioConnection patchCord0(finalMix, 0, audioOutput, 0);
AudioConnection patchCord1(finalMix, 0, audioOutput, 1);
AudioControlSGTL5000 sgtl5000_1;

Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0);

#define numKeys 16 // number of keys in the trellis drumpad

#define INTPIN A2 // [Trellis] INT wire to pin #A2 (SCL to SCL and SDA to SDA is default)

// [Teensy Audio Shield]
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

const int ledInterval = 200; // blinking time when I touch a button

unsigned long currentMillis = 0;
unsigned long previousLedMillis[numKeys]; // memorises when I touched a button for the last time (used to waiting to turning the LED on again)
unsigned long startReplayTime; // marks the begining of my replay track

uint8_t preset=1; // current preset 
bool rec=false; // recording state flag

// (i,j) = (button, timming)
// le premier element du tableu indique si il y a ou pas un enregistrement
// l'enregistrement finit avec une touche fictive 3 (LEDREC)
#define TOUCH 0 // stands for the touched button coordinates
#define TIME 1 // stands for when the button was touched
#define loopSize 100 // maximum size of a track (nb of buttons)

long int recMem[2][2][loopSize]; // recMem is the recording memory. It memorises the played notes and the timing in the record() function
uint8_t swap=0; // swap (and antiSwap) allows to record while replaying a saved track. Once the recording is done, the swap track becomes the new main track
uint8_t antiSwap=1;
uint8_t recIndex=1; // recording index (recIndex++ each time I add a new key)
uint8_t replayIndex=0; // replay index (same, but for replaying)

void setup() {
  Serial.begin(9600);
  Serial.println("Teensy LoopStation debug");

  AudioMemory(20);
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

  pinMode(INTPIN, INPUT);
  digitalWrite(INTPIN, HIGH);
  trellis.begin(0x70);

  // SD card slot check
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      loadingAnimation();
    }
  }

  // rec params
  recMem[swap][TOUCH][0]=-1;
  trellisLedFrontAnimation();
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

// ######################################## //
// ########## Main Functions ############## //
// ######################################## //

// checks LED blinking timing (based on Arduino Several Things at the Same Time) 
void updateLeds() {
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

// play track in the record array (recMem) in loop (finishes at the end of the array OR when the record button is pressed)
void playRec() {
  if(replayIndex == 0) {
    startReplayTime = currentMillis;
    replayIndex++;
  }

  // "swap" is used so we can read and write in recMem at the same time
  if((currentMillis - startReplayTime) >= (recMem[antiSwap][TIME][replayIndex]-recMem[antiSwap][TIME][0])) {
    if(recMem[antiSwap][TOUCH][replayIndex] != LEDREC) playSketchAndAnimate(recMem[antiSwap][TOUCH][replayIndex]);
    replayIndex= recMem[antiSwap][TOUCH][replayIndex] == LEDREC ? 0 : (replayIndex+1)%loopSize;
  }
}

// manages what each button does (samples played, changes preset, record, etc)
// and adds some delay (blinking) to the button that has been pressed
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
      // malfunctioning button 
      break;
    case 12:
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
      if(preset == 1) sound5.play(Hihat1);
      else if(preset == 2) sound5.play(Hihat2);
      else sound5.play(Hihat3);
      break;
    case 15:
      switchPreset();
      break;
    default:
      Serial.println("Unexpected default in playSketchAndAnimate's switch case"); 
      break;
  }

  previousLedMillis[i]=currentMillis;
}

// changes samples profile (1, 2 or 3) and animates it
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

// ##################################
// ###### playing and recording #####
// ##################################

// first element of any of the tracks in recMem equals -2 means there is a valid record to be played
bool anythingRecorded() { return recMem[0][TOUCH][0] == -2 || recMem[1][TOUCH][0] == -2; }

// if there is anything recorded in one track, it records in the other one (thus we can replay and record at the same time)
void record(uint8_t i) {
  if (recIndex >= 0 && i!=7) {
    recMem[swap][TOUCH][recIndex]=i;
    recMem[swap][TIME][recIndex]=currentMillis;
  } else Serial.println("Unexpected record with recIndex = 0"); 
  recIndex++;

  if(recIndex >= loopSize || i==7)
    stopRecording();
}

void switchRecord() {
  if(rec) stopRecording();
  else {
    recMem[swap][TIME][0]=currentMillis;
    rec=true;
  }
}

// once the rec is over, we swapSwap() in order to replay the new track (and the old one is now avaliable to recording as antiSwap)
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

// clear the loop = set the flag (first elem of recMem) to -1. if it's not equals -2, it means there is "nothing recorded"
void clearLoopMemory() {
  recMem[0][TOUCH][0] = -1; 
  recMem[1][TOUCH][0] = -1;
}

// could have used swap = !swap?
void swapSwap() {
  antiSwap=swap;
  if(swap == 1) swap=0;
  else swap=1;
}

// #######################
// ###### animations #####
// #######################

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

void trellisLedFrontAnimation(){
  for (uint8_t i=0; i<numKeys; i++) {
    if((i+1)%4==0) continue;
    trellis.clrLED(i);
    if((i+2)%4==0) {
     trellis.writeDisplay();
     delay(45);
    } 
  }

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
  for (int8_t i=numKeys-1; i>=0; i--) {
    if((i+1)%4==0) continue;
    trellis.clrLED(i);
    if(i%4==0) {
     trellis.writeDisplay();
     delay(45);
    }   
  }

  for (int8_t i=numKeys-1; i>=0; i--) {
    if((i+1)%4==0) continue;
    trellis.setLED(i);
    if(i%4==0) {
     trellis.writeDisplay();
     delay(45);
    } 
  }
}

void loadingAnimation() {
  for (uint8_t i=0; i<numKeys; i++) 
    trellis.clrLED(i);
  delay(100);

  for(int k=numKeys;k>=0;k--) {
    for (uint8_t i=0; i<k; i++) {
      for (uint8_t j=0; j<k; j++) {
        if(i==j) trellis.setLED(j);
        else trellis.clrLED(j);
        trellis.writeDisplay();
      }
      delay(350);
    }
  }

  for(int8_t k=0;k<4;k++) {
    for (int8_t i=numKeys-1; i>=0; i--) trellis.clrLED(i);    
    trellis.writeDisplay();
    delay(200);
    for (int8_t i=numKeys-1; i>=0; i--) trellis.setLED(i);    
    trellis.writeDisplay();
    delay(200);
  }
  delay(100);
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
