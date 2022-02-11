//
//  Oscillator.h
//  Synthesis
//
//  Created by Martin on 08.04.14.
//
//

#ifndef __Synthesis__Oscillator__
#define __Synthesis__Oscillator__

#include <math.h>

enum OscillatorMode {
    OSCILLATOR_MODE_SINE,
    OSCILLATOR_MODE_SAW,
    OSCILLATOR_MODE_SQUARE,
    OSCILLATOR_MODE_TRIANGLE
};

class Oscillator {
private:
    OscillatorMode mOscillatorMode;
    const double mPI;
    double mFrequency;
    double mPhase;
    double mSampleRate;
    double mPhaseIncrement;
    void updateIncrement();
    const double twoPI;
    bool isMuted;
public:
    void setMode(OscillatorMode mode);
    void setFrequency(double frequency);
    void setSampleRate(double sampleRate);
    void generate(double* buffer, int nFrames);
    inline void setMuted(bool muted) { isMuted = muted; }
    double nextSample();
    
    Oscillator();
    
    // Niko
    void setPhase(BL_FLOAT phase);
};

#endif /* defined(__Synthesis__Oscillator__) */
