//
//  PhaseCorrectObj.cpp
//  BL-PitchShift
//
//  Created by Pan on 03/04/18.
//
//

#include "BLUtils.h"
#include "PhaseCorrectObj.h"


PhaseCorrectObj::PhaseCorrectObj(int bufferSize, int oversampling, BL_FLOAT sampleRate)
{
    Reset(bufferSize, oversampling, sampleRate);
}

PhaseCorrectObj::~PhaseCorrectObj() {}

void
PhaseCorrectObj::Reset(int bufferSize, int oversampling, BL_FLOAT sampleRate)
{
    mFirstTime = true;
    
    mBufferSize = bufferSize;
    mOversampling = oversampling;
    mSampleRate = sampleRate;
}

void
PhaseCorrectObj::Process(const WDL_TypedBuf<BL_FLOAT> &freqs,
                         WDL_TypedBuf<BL_FLOAT> *ioPhases)
{
    if (mFirstTime)
    {
        mPrevPhases = *ioPhases;
        
        // TEST
        //BLUtils::FillAllZero(&mPrevPhases);
        
        mFirstTime = false;
        
        return;
    }
    
    WDL_TypedBuf<BL_FLOAT> newPhases;
    newPhases.Resize(freqs.GetSize());
    
    BL_FLOAT t = ((BL_FLOAT)mBufferSize)/mSampleRate;
    //BL_FLOAT oneSampleT = 1.0/mSampleRate;
    
    // TODO: this could be precomputed
    // (sometimes)
    for (int i = 0; i < newPhases.GetSize(); i++)
    {
        BL_FLOAT freq = freqs.Get()[i];
        
        // How many periods since the next buffer start
        BL_FLOAT numPeriods = freq*t/mOversampling;
        
        // Which size is the last, uncomplete period ?
        BL_FLOAT rem = numPeriods - ((int)numPeriods);
        
        // So at which phase will we start next ?
        BL_FLOAT newPhase = rem*2.0*M_PI;
        
        //BL_FLOAT oneSampleFreq = oneSampleT*freq*2.0*M_PI;
        //newPhase += 2.0*oneSampleFreq;
        
        newPhase = MapToPi(newPhase);
        
        newPhases.Get()[i] = newPhase;
    }
    
    for (int i = 0; i < ioPhases->GetSize(); i++)
    {
        //BL_FLOAT phase = 0.0; //ioPhases->Get()[i];
        
        // Where did we stop at last pass ?
        BL_FLOAT prevPhase = mPrevPhases.Get()[i];
        BL_FLOAT currentPhase = ioPhases->Get()[i];
        mPrevPhases.Get()[i] = currentPhase;
        
        BL_FLOAT deltaPhase = currentPhase - prevPhase;
        
        //BL_FLOAT delta = phase - prevPhase;
        
        // BL_FLOAT newPhase = newPhases.Get()[i] + prevPhase + currentPhase; //- delta;
        
        BL_FLOAT newPhase = newPhases.Get()[i];
        
        newPhase += deltaPhase;
        
        newPhase = MapToPi(newPhase);
        
        ioPhases->Get()[i] = prevPhase;
        
        //mPrevPhases.Get()[i] = newPhase;
    }
   
#if 0
    static int count = 0;
    if (count++ == 10)
    {
        BLDebug::DumpData("freqs.txt", freqs);
        BLDebug::DumpData("new-phases.txt", newPhases);
        BLDebug::DumpData("prev-phases.txt", mPrevPhases);
        
        exit(0);
    }
#endif
}

BL_FLOAT
PhaseCorrectObj::MapToPi(BL_FLOAT val)
{
    /* Map delta phase into +/- Pi interval */
    val =  fmod(val, 2.0*M_PI);
    if (val <= -M_PI)
        val += 2.0*M_PI;
    if (val > M_PI)
        val -= 2.0*M_PI;
    
    return val;
}
