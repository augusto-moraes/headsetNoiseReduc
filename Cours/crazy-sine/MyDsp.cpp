#include "MyDsp.h"

#define AUDIO_OUTPUTS 1

#define MULT_16 32767

MyDsp::MyDsp() : 
AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
sine1(AUDIO_SAMPLE_RATE_EXACT),
sine2(AUDIO_SAMPLE_RATE_EXACT),
echo(AUDIO_SAMPLE_RATE_EXACT,10000)
{
  echo.setDel(10000);
  echo.setFeedback(0.5);
}

MyDsp::~MyDsp(){}

// set sine wave frequency
void MyDsp::setFreq(float freq){
  sine1.setFrequency(freq);
  sine2.setFrequency(1.5*freq);
}

void MyDsp::update(void) {
  audio_block_t* outBlock[AUDIO_OUTPUTS];
  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    outBlock[channel] = allocate();
    if (outBlock[channel]) {
      for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        //float currentSample = echo.tick(sine.tick())*0.5;
        //float currentSample = echo.tick(sine1.tick()*0.2 + sine2.tick()*0.1);
        float currentSample = sine1.tick()*0.3 + sine2.tick()*0.2;
        currentSample = max(-1,min(1,currentSample));
        int16_t val = currentSample*MULT_16;
        outBlock[channel]->data[i] = val;
      }
      transmit(outBlock[channel], channel);
      release(outBlock[channel]);
    }
  }
}
