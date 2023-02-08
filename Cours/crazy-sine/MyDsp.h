  #ifndef faust_teensy_h_
#define faust_teensy_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "Audio.h"

#include "Sine.h"
#include "Echo.h"

class MyDsp : public AudioStream    // AudioStream -> classe "racine" pour tout autre classe de la teensy lib
{
  public:
    MyDsp();
    ~MyDsp();
    
    virtual void update(void);
    void setFreq(float freq);
    
  private:
    Sine sine1, sine2;
    Echo echo;
};

#endif
