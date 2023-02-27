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
 
AudioPlaySdRaw playRaw1; 

// which input on the audio shield will be used?
//const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;

AudioMixer4 mix1;
AudioMixer4 mix2;

AudioRecordQueue queue1;
AudioOutputI2S audioOutput;

AudioConnection c1(sound0, 0, mix1, 0);
AudioConnection c2(sound1, 0, mix1, 1);
AudioConnection c3(sound2, 0, mix1, 2);
AudioConnection c4(sound3, 0, mix1, 3);
AudioConnection c5(mix1, 0, mix2, 0); 
AudioConnection c6(sound4, 0, mix2, 1);
AudioConnection c7(sound5, 0, mix2, 2);
AudioConnection c8(playRaw1, 0, mix2, 3);

AudioConnection patchCord5(mix2, 0, audioOutput, 0);
AudioConnection patchCord6(mix2, 0, audioOutput, 1);
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

uint8_t preset;
uint8_t ledPreset;
bool rec=false;

// The file where data is recorded
File frec;

void setup() {
  Serial.begin(9600);
  Serial.println("Teensy LoopStation debug");

  AudioMemory(60);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(0.7);

  mix1.gain(0, 0.4);
  mix1.gain(1, 0.4);
  mix1.gain(2, 0.4);
  mix1.gain(3, 0.4);
  mix2.gain(1, 0.4);
  mix2.gain(2, 0.4);

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

  setPreset(7); // 7 set preset 1
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
    trellis.setLED(i);
    if((i+2)%4==0) {
     trellis.writeDisplay();
     delay(45);
    } 
  }

  // then turn them off
  for (uint8_t i=0; i<numKeys; i++) {
    if((i+1)%4==0) continue;
    trellis.clrLED(i);
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
      trellis.setLED(i);    
    }
    trellis.writeDisplay();
    delay(80);
    for (int8_t i=numKeys-1; i>=0; i--) {
      if((i+1)%4==0) continue;
      trellis.clrLED(i);    
    }
    trellis.writeDisplay();
    delay(75);
    }
}

void trellisLedBackAnimation(){
  // light up all the LEDs in order
  for (int8_t i=numKeys-1; i>=0; i--) {
    if((i+1)%4==0) continue;
    trellis.setLED(i);
    if(i%4==0) {
     trellis.writeDisplay();
     delay(45);
    }   
  }

  // then turn them off
  for (int8_t i=numKeys-1; i>=0; i--) {
    if((i+1)%4==0) continue;
    trellis.clrLED(i);
    if(i%4==0) {
     trellis.writeDisplay();
     delay(45);
    } 
  }
}

void dropAnimation(uint8_t i){
  if((i+1)%4 == 0) return;
  if((i+2)%4 != 0) {
    trellis.setLED(i+1); 
    previousLedMillis[i+1]=currentMillis+(ledInterval/6);
  }
  if(i%4 != 0) {
    trellis.setLED(i-1);
    previousLedMillis[i-1]=currentMillis+(ledInterval/6);
  }  
  if(i>3) {
    trellis.setLED(i-4); 
    previousLedMillis[i-4]=currentMillis+(ledInterval/6);
  } 
  if(i<12) {
    trellis.setLED(i+4); 
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
    if(i == LEDREC || i == ledPreset) continue;
    if (trellis.isLED(i) && (currentMillis - previousLedMillis[i] >= ledInterval)){
      trellis.clrLED(i);
      trellis.writeDisplay();
    }
  }
}

void loop() {
  delay(30); // 30ms delay is required, dont remove me!
  
  currentMillis = millis();
  updateLeds();

  if (trellis.readSwitches()) {
    for (uint8_t i=0; i<numKeys; i++) {
      if (trellis.justPressed(i))
        playSketchAndAnimate(i);
    }
  }

  if(rec) continueRecording();
}

void switchRecord() {
  if(rec) stopRecording();
  else startRecording();
}

void setPreset(uint8_t i) {
  if(ledPreset != i) {
    trellis.clrLED(ledPreset);
    ledPreset = i;
    preset = (i-3)/4;
    trellis.setLED(i);
    trellis.writeDisplay();
  }
}

void playSketchAndAnimate(uint8_t i) {
  trellis.setLED(i);
  trellis.writeDisplay();

  dropAnimation(i);
  
  switch(i){
    case 0:
      Serial.print("Memory: ");
      Serial.println(AudioMemoryUsage());
      trellisLedFrontAnimation();
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      switchRecord();
      break;
    case 4:
      break;
    case 5:
      break;
    case 6:
      break;
    case 7:
      setPreset(i);
      trellisLedFrontAnimation();
      break;
    case 8:
      if(preset == 1) sound0.play(Crash1);
      else if(preset == 2) sound0.play(Crash2);
      else if(preset == 3) sound0.play(Shaker3);
      break;
    case 9:
      if(preset == 1) sound1.play(Cowbell1);
      else if(preset == 2) sound1.play(Tom2);
      else if(preset == 3) sound1.play(Tom3);
      break;
    case 10:
      if(preset == 1) sound2.play(Snare1);
      else if(preset == 2) sound2.play(Snare2);
      else if(preset == 3) sound2.play(Snare3);
      break;
    case 11:
      setPreset(i);
      trellisLedBlinkAnimation();
      break;
    case 12:
      if(preset == 1) sound3.play(Kick1);
      else if(preset == 2) sound3.play(Kick2);
      else if(preset == 3) sound3.play(Kick3);
      break;
    case 13:
      if(preset == 1) sound4.play(Clap1);
      else if(preset == 2) sound4.play(Openhat2);
      else if(preset == 3) sound4.play(Openhat3);
      break;
    case 14:
      if(preset == 1) sound5.play(Hihat1);
      else if(preset == 2) sound5.play(Hihat2);
      else if(preset == 3) sound5.play(Hihat3);
      break;
    case 15:
      setPreset(i);
      trellisLedBackAnimation();
      break;
    default:
      Serial.print("Unexpected default in playSketchAndAnimate's switch case"); 
      break;
  }

  previousLedMillis[i]=currentMillis;
}

void startRecording() {
  Serial.println("startRecording");
  if (SD.exists("REC/RECORD.RAW")) {
    // The SD library writes new data to the end of the
    // file, so to start a new recording, the old file
    // must be deleted before new data is written.
    SD.remove("RECORD.RAW");
  }
  frec = SD.open("REC/RECORD.RAW", FILE_WRITE);
  if (frec) {
    queue1.begin();
    rec = true;
  }
}

void continueRecording() {
  if (queue1.available() >= 2) {
    byte buffer[512];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer+256, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    // write all 512 bytes to the SD card
    //elapsedMicros usec = 0;
    frec.write(buffer, 512);
    // Uncomment these lines to see how long SD writes
    // are taking.  A pair of audio blocks arrives every
    // 5802 microseconds, so hopefully most of the writes
    // take well under 5802 us.  Some will take more, as
    // the SD library also must write to the FAT tables
    // and the SD card controller manages media erase and
    // wear leveling.  The queue1 object can buffer
    // approximately 301700 us of audio, to allow time
    // for occasional high SD card latency, as long as
    // the average write time is under 5802 us.
    //Serial.print("SD write, us=");
    //Serial.println(usec);
  }
}

void stopRecording() {
  Serial.println("stopRecording");
  queue1.end();
  if (rec) {
    while (queue1.available() > 0) {
      frec.write((byte*)queue1.readBuffer(), 256);
      queue1.freeBuffer();
    }
    frec.close();
  }
  rec=false;
  startPlaying();
}

void startPlaying() {
  Serial.println("startPlaying");
  playRaw1.play("REC/RECORD.RAW");
}
