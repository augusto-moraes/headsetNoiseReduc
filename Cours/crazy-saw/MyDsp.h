#ifndef faust_teensy_h_
#define faust_teensy_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "Audio.h"

#include "Phasor.h"
#include "Echo.h"

class MyDsp : public AudioStream
{
  public:
    MyDsp();
    ~MyDsp();
    
    virtual void update(void);    // virtual member -> overwrites a member in iheritance (in this case, from AudioStream) -> that means everytime i call update of audiostream for MyDsp, it will call this update here
    void setFreq(float freq);
    
  private:
    Phasor sawtooth;
    Echo echo;
};

#endif
