#include <Audio.h>
#include "MyDsp.h"

MyDsp myDsp;
AudioOutputI2S out;
AudioControlSGTL5000 audioShield;
AudioConnection patchCord0(myDsp,0,out,0);
AudioConnection patchCord1(myDsp,0,out,1);

int midiKeys[] = {72,67,64,69,71,70,69,67,76,79,81,77,79,76,72,74,71};
int midiKeysLength;
int count;

float keyToFreq(int key) 
{
  return pow(2.0,(key-69.0)/12.0)*440.0;
}

void waitButton() 
{
  while(!digitalRead(0)) {}
  
  float freqToPlay = keyToFreq(midiKeys[count]);
  Serial.println(freqToPlay);
  myDsp.setFreq(freqToPlay);
  count = (count+1)%midiKeysLength;
  
  while(digitalRead(0)) {}
}

void setup() {
  AudioMemory(2);
  audioShield.enable();
  audioShield.volume(0.5);

  pinMode(0, INPUT);

  midiKeysLength = sizeof(midiKeys)/sizeof(int);
  count = 0;
}

void loop() {
  myDsp.setFreq(0);
  delay(100);
  waitButton();
}
