# Teensy Loop Station

This repository contains the source code of our Teensy-made loop station & drumpad guitar pedal, built for a school project.
The aim of this project is to help students get started to work on a project around embedded real-time audio signal processing.

## How Does it Works ? 

### The Hardware

* We connected a Teensy 4.0 to a Audio Shield. 
* We then added a Adafruit Trellis button matrix (I2C Protocol) and we added LEDs to it.
* We also connected four potentiometers to the teensy.
* The whole hardware fits inside a 3D printed box.

### The Software

* We use the `Teensy Audio Library` in order to use the Audio Shield, the `Wire Library` to communicate with the trellis board via I2C Protocol,
the `SD` and `SPI` libraries to read our SD card, and finally the `Adafruit_Trellis Library` to control the trellis board.

* We have added the 18 drums samples in three Presets C++ sources. Those are saved in hexadecimal sketch arrays (In fact, we converted them from Wav 16 bits to Sketch using the AudioLibrary code wav2sketch). They are played using the Teensy Audio Library's `AudioPlayMemory`.

* We saved the other (longer) samples in a fast SD card. They can be accessed via the Teensy Audio Library's `AudioPlaySdWav`.

* The `AudioPlayMemory` and `AudioPlaySdWav` sound interfaces are mixed and patched to the same `AudioOutputI2S`.

* Playing the teensy loop station is simple : whenever you touch a button, you have a sound in your output (and a fancy visual animation). The main function that manages this action is `playSketchAndAnimate(int i)` function, which have as input the pressed button. The teensy is always checking if any of the trellis button have been pressed, and if it does, it calls the playSketchAndAnimate funciton.

* The particular thing about our loop station is that it is able to record multiple tracks while you are playing it. In order to achieve this, we've created a 3 dimension array which memorises : the button played and the timing. The 3rd dimension allows simultaneous Writing and Reading : if I already have something recorded in a track A, I can playback it from the track A while recoding in a new track B. Once the recording is over, the track B becomes the new main (playback) track, and the track A becomes the next one to be recorded/replaced. This allows the user to record as many tracks layers as them likes. (checkout record and playRec functions)

* Most of the functions (playAndRec, record and animations) uses millis() timing. Like this, teensy knows if it's time to play the next chord or turning on a LED without having to wait with a delay.

* We have a `clearLoopMemory()` function whichs sets a flag saying that the recMem array is clear. We don't actually need to clean it as the next recording will crush it.

* The teensy loop station works with 3 presets that can be changed with `switchPresets()`

* We developped some fancy animations in order to make our product more attractive.

* The source code is commented, so do not hesitate to check it out.

We choose to keep the whole source code in the .ino file because, at first glance, we thought that this options would behave faster in an embeded system. However, we could have choosen a C++ solution (the use of classes would probably lead us to a more readable code).

We managed to build a fast and optimised real-time machine that smoothly plays and records samples, and which supports a wide range of presets.
