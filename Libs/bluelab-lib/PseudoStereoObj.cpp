//
//  PseudoStereoObj.cpp
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#include <FilterRBJNX.h>

#include <USTProcess.h>
#include <USTStereoWidener.h>
#include <BLUtils.h>

#include "PseudoStereoObj.h"


PseudoStereoObj::PseudoStereoObj(BL_FLOAT sampleRate, BL_FLOAT width, int mode)
{
    // Cutoff freq for filters
    // NOTE: stay low, otherwise the right channel
    // will have more volume than the right
    // But do not stay too low, otherwise the sound will have more bass
    // Between 450 and 670 Hz seems good
    
    mCutoffFreq = 3500.0; //100.0;
    //mCutoffFreq = 500.0;
    
    mSampleRate = sampleRate;
    mWidth = width;
    mMode = mode;
    
    mFilter0 = NULL;
    mFilter1 = NULL;
    
    for (int k = 0; k < 2; k++)
    {
        for (int i = 0; i < MAX_MODE + 1; i++)
        {
            mModeFilters[i][k] = NULL;
        }
    }
    
    InitFilters();
    
    mStereoWidener = new USTStereoWidener();
}

PseudoStereoObj::~PseudoStereoObj()
{
    delete mFilter0;
    delete mFilter1;
    
    delete mStereoWidener;
}

void
PseudoStereoObj::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    InitFilters();
}
                              
void
PseudoStereoObj::SetWidth(BL_FLOAT width)
{
    mWidth = width;
}

void
PseudoStereoObj::SetCutoffFreq(BL_FLOAT freq)
{
#define EPS 1e-10
  if (std::fabs(freq - mCutoffFreq) < EPS)
        return;
    
    mCutoffFreq = freq;
    
    InitFilters();
}

void
PseudoStereoObj::SetMode(BL_FLOAT mode)
{
    mMode = MIN_MODE + mode*(MAX_MODE - MIN_MODE);
}

// Not up to date
// => needs stereo sound rotation
void
PseudoStereoObj::ProcessSample(BL_FLOAT sampIn,
                               BL_FLOAT *sampOut0,
                               BL_FLOAT *sampOut1)
{
    // See: http://csoundjournal.com/issue14/PseudoStereo.html
    /*
        a1 AllPass ain
        a2 AllPass a1
        ...
        ...
        aout1 = ain*kwidth + a1
        aout2 = -a2*kwidth + a1
   */
    
    BL_FLOAT a1 = mFilter0->Process(sampIn);
    BL_FLOAT a2 = mFilter0/*1*/->Process(a1);
    
    for (int i = 0; i < mMode; i++)
    {
        /* a1 AllPass a1
		   a2 AllPass a1
         */
        
#if 0
        a1 = mFilter0->Process(a1);
        a2 = mFilter1->Process(a1);
#else
        a1 = mModeFilters[i][0]->Process(a1);
        a2 = mModeFilters[i][1]->Process(a1);
#endif
        
        // NOTE: needs stereo sound rotation here
    }
    
    *sampOut0 = sampIn*mWidth + a1;
    *sampOut1 = -a2*mWidth + a1;
}

void
PseudoStereoObj::ProcessSamples(const WDL_TypedBuf<BL_FLOAT> &sampsIn,
                                WDL_TypedBuf<BL_FLOAT> *sampsOut0,
                                WDL_TypedBuf<BL_FLOAT> *sampsOut1)
{
    WDL_FFT_COMPLEX rotation = USTProcess::ComputeComplexRotation(M_PI/4.0);
    
    sampsOut0->Resize(sampsIn.GetSize());
    sampsOut1->Resize(sampsIn.GetSize());
    
    for (int i = 0; i < sampsIn.GetSize(); i++)
    {
        BL_FLOAT sampIn = sampsIn.Get()[i];
    
        //
        BL_FLOAT a1 = mFilter0->Process(sampIn);
        BL_FLOAT a2 = mFilter1->Process(a1);
    
        //
        for (int j = 0; j < mMode; j++)
        {
#if 0
            a1 = mFilter0->Process(a1);
            a2 = mFilter1->Process(a1);
#else
            a1 = mModeFilters[j][0]->Process(a1);
            a2 = mModeFilters[j][1]->Process(a1);
#endif
        }
        
        // Gerzon
        BL_FLOAT sampOut0 = sampIn*mWidth + a1;
        BL_FLOAT sampOut1 = -a2*mWidth + a1;
        
        // Stereo sound rotation
        USTProcess::RotateSound(&sampOut0, &sampOut1, rotation);
        
        sampsOut0->Get()[i] = sampOut0;
        sampsOut1->Get()[i] = sampOut1;
    }
}

void
PseudoStereoObj::ProcessSamples(vector<WDL_TypedBuf<BL_FLOAT> > *samplesVec)
{
    if (samplesVec->empty())
        return;
    
    if (samplesVec->size() != 2)
        return;
    
    USTProcess::StereoToMono(samplesVec);
    
    WDL_TypedBuf<BL_FLOAT> mono = (*samplesVec)[0];
    ProcessSamples(mono, &(*samplesVec)[0], &(*samplesVec)[1]);
    
    // Adjust the width by default
#define WIDTH_ADJUST 1.0
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    samples.push_back(&(*samplesVec)[0]);
    samples.push_back(&(*samplesVec)[1]);

    // #bl-iplug2
    //USTProcess::StereoWiden(&samples, WIDTH_ADJUST);
    mStereoWidener->StereoWiden(&samples, WIDTH_ADJUST);
}

void
PseudoStereoObj::InitFilters()
{
    if (mFilter0 != NULL)
        delete mFilter0;
    
    if (mFilter1 != NULL)
        delete mFilter1;
    
    BL_FLOAT freq0 = GetCutoffFrequency(0);
    
    mFilter0 = new FilterRBJNX(1/*mNumFilters*/, FILTER_TYPE_ALLPASS,
                              mSampleRate, freq0/*mCutoffFreq*/);
    mFilter1 = new FilterRBJNX(1/*mNumFilters*/, FILTER_TYPE_ALLPASS,
                              mSampleRate, freq0/*mCutoffFreq*/);
    
    
    for (int i = 0; i < MAX_MODE + 1; i++)
    {
        BL_FLOAT freq1 = GetCutoffFrequency(i + 1);
        
        for (int k = 0; k < 2; k++)
        {
           if (mModeFilters[i][k] != NULL)
               delete mModeFilters[i][k];
    
            mModeFilters[i][k] = new FilterRBJNX(1/*mNumFilters*/, FILTER_TYPE_ALLPASS,
                                                mSampleRate, freq1/*mCutoffFreq*/);
        }
    }
}

#if 0 // Random (for debugging)
BL_FLOAT
PseudoStereoObj::GetCutoffFrequency(int filterNum)
{
    // Random
    BL_FLOAT rnd0 = ((BL_FLOAT)rand())/RAND_MAX;
    BL_FLOAT freq = rnd0*mSampleRate*0.5;
    
    return freq;
}
#endif

#if 1 // Cut the frequeny fomain logarithmically
BL_FLOAT
PseudoStereoObj::GetCutoffFrequency(int filterNum)
{
    BL_FLOAT t = ((BL_FLOAT)(filterNum + 1))/(MAX_MODE + 2);
    
    t = BLUtils::ApplyParamShape(t, (BL_FLOAT)(1.0/4.0));
    
    BL_FLOAT freq = t*mSampleRate*0.5;
    
    return freq;
}
#endif
