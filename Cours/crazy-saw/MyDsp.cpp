#include "MyDsp.h"

#define AUDIO_OUTPUTS 1

#define MULT_16 32767

MyDsp::MyDsp() : 
AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
sawtooth(AUDIO_SAMPLE_RATE_EXACT),
echo(AUDIO_SAMPLE_RATE_EXACT,10000)
{
  echo.setDel(10000);
  echo.setFeedback(0.5);
}

MyDsp::~MyDsp(){}

// set sine wave frequency
void MyDsp::setFreq(float freq){
  sawtooth.setFrequency(freq);
}

void MyDsp::update(void) {
  audio_block_t* outBlock[AUDIO_OUTPUTS];
  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    outBlock[channel] = allocate();
    if (outBlock[channel]) {
      for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float currentSample = (sawtooth.tick()*2 - 1)*0.5; // ligne importante (ce qu'il faudra que je fasse) -> echo (fait de l'echo) ; *0.5 (0.5 est le "volume") // je peux faire une pedale echo avec echo tick
        currentSample = max(-1,min(1,currentSample));        // evite wrapping (force l'echantillon entre -1 et 1)
        int16_t val = currentSample*MULT_16;
        outBlock[channel]->data[i] = val;
      }
      transmit(outBlock[channel], channel);
      release(outBlock[channel]);
    }
  }
}
