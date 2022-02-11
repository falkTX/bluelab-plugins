//
//  Oscillator.cpp
//  Synthesis
//
//  Created by Martin on 08.04.14.
//
//

#include <BLTypes.h>

#include "Oscillator.h"

// Niko
#define USE_SINE_LUT 1

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#if USE_SINE_LUT
#include <SinLUT2.h>
SIN_LUT_CREATE(OSCILLATOR_SIN_LUT, 4096);
#endif


void Oscillator::setMode(OscillatorMode mode) {
    mOscillatorMode = mode;
}

void Oscillator::setFrequency(double frequency) {
    mFrequency = frequency;
    updateIncrement();
}

void Oscillator::setSampleRate(double sampleRate) {
    mSampleRate = sampleRate;
    updateIncrement();
}

void Oscillator::updateIncrement() {
    mPhaseIncrement = mFrequency * 2 * mPI / mSampleRate;
}

void Oscillator::generate(double* buffer, int nFrames) {
    const double twoPI = 2 * mPI;
    switch (mOscillatorMode) {
        case OSCILLATOR_MODE_SINE:
            for (int i = 0; i < nFrames; i++) {
                
#if !USE_SINE_LUT
                buffer[i] = sin(mPhase);
#else
                SIN_LUT_GET(OSCILLATOR_SIN_LUT, buffer[i], mPhase);
#endif
                
                mPhase += mPhaseIncrement;
                while (mPhase >= twoPI) {
                    mPhase -= twoPI;
                }
            }
            break;
        case OSCILLATOR_MODE_SAW:
            for (int i = 0; i < nFrames; i++) {
                buffer[i] = 1.0 - (2.0 * mPhase / twoPI);
                mPhase += mPhaseIncrement;
                while (mPhase >= twoPI) {
                    mPhase -= twoPI;
                }
            }
            break;
        case OSCILLATOR_MODE_SQUARE:
            for (int i = 0; i < nFrames; i++) {
                if (mPhase <= mPI) {
                    buffer[i] = 1.0;
                } else {
                    buffer[i] = -1.0;
                }
                mPhase += mPhaseIncrement;
                while (mPhase >= twoPI) {
                    mPhase -= twoPI;
                }
            }
            break;
        case OSCILLATOR_MODE_TRIANGLE:
            for (int i = 0; i < nFrames; i++) {
                double value = -1.0 + (2.0 * mPhase / twoPI);
                buffer[i] = 2.0 * (fabs(value) - 0.5);
                mPhase += mPhaseIncrement;
                while (mPhase >= twoPI) {
                    mPhase -= twoPI;
                }
            }
            break;
    }
}

double Oscillator::nextSample() {
    double value = 0.0;
    if(isMuted) return value;
    
    switch (mOscillatorMode) {
        case OSCILLATOR_MODE_SINE:
        {
#if !USE_SINE_LUT
            value = sin(mPhase);
#else
            SIN_LUT_GET(OSCILLATOR_SIN_LUT, value, mPhase);
#endif
        }
            break;
        case OSCILLATOR_MODE_SAW:
            value = 1.0 - (2.0 * mPhase / twoPI);
            break;
        case OSCILLATOR_MODE_SQUARE:
            if (mPhase <= mPI) {
                value = 1.0;
            } else {
                value = -1.0;
            }
            break;
        case OSCILLATOR_MODE_TRIANGLE:
            value = -1.0 + (2.0 * mPhase / twoPI);
            value = 2.0 * (fabs(value) - 0.5);
            break;
    }
    mPhase += mPhaseIncrement;
    while (mPhase >= twoPI) {
        mPhase -= twoPI;
    }
    return value;
}

Oscillator::Oscillator()
:   mOscillatorMode(OSCILLATOR_MODE_SINE),
    mPI(2*acos(0.0)),
    twoPI(2 * mPI),
    isMuted(true),
    mFrequency(440.0),
    mPhase(0.0),
    mSampleRate(44100.0)
{
#if USE_SINE_LUT
    SIN_LUT_INIT(OSCILLATOR_SIN_LUT);
#endif
    
    updateIncrement();
};

void
Oscillator::setPhase(BL_FLOAT phase)
{
    mPhase = phase;
}
