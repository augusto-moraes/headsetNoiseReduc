#include <Audio.h>
#include "MyDsp.h"

MyDsp myDsp;
AudioOutputI2S out;
AudioControlSGTL5000 audioShield;
AudioConnection patchCord0(myDsp,0,out,0);
AudioConnection patchCord1(myDsp,0,out,1);

int midiKeys[] = { 72, 12, 20, 24, 67, 12, 20, 24, 64, 12, 20, 24, 69, 12, 20, 12, 71, 12, 20, 12, 70, 12, 69, 12, 20, 12, 67, 16, 76, 16, 79, 16, 81, 12, 20, 12, 77, 12, 79, 12, 20, 12, 76, 12, 20, 12, 72, 12, 74, 12, 71, 12, 20, 24 };
//int midiKeys[] = {72,67,64,69,71,70,69,67,76,79,81,77,79,76,72,74,71};
int midiKeysLength;
int it;

float keyToFreq(int key) 
{
  return pow(2.0,(key-69.0)/12.0)*440.0;
}

void setup() {
  AudioMemory(2);
  audioShield.enable();
  audioShield.volume(0.5);

  midiKeysLength = sizeof(midiKeys)/sizeof(int);
  it = 0;
}

void loop() {
  float freqToPlay = keyToFreq(midiKeys[it%midiKeysLength]);
  Serial.println(freqToPlay);
  myDsp.setFreq(freqToPlay);
  it++;
  delay(100);
}
