#include <stdio.h>
#include <math.h>

#include <OversampProcessObj5.h>

#define WDL_SIMPLEPITCHSHIFT_IMPLEMENT
#if USE_WDL_PITCHSHIFTER2
#include "simple_pitchshift2.h"
#else
#include "simple_pitchshift.h"
#endif

#include <BLUtilsPlug.h>

#include "PitchShifter.h"

#define USE_OVERSAMP_OBJ 1

#if USE_OVERSAMP_OBJ
class PitchShiftOversampObj : public OversampProcessObj5
{
public:
    PitchShiftOversampObj(PitchShifter *shifter, int oversampling);
    virtual ~PitchShiftOversampObj();
    
    void ProcessSamplesBuffer(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                                      vector<WDL_TypedBuf<BL_FLOAT> > *out);

protected:
    PitchShifter *mShifter;
};

PitchShiftOversampObj::PitchShiftOversampObj(PitchShifter *shifter,
                                             int oversampling)
: OversampProcessObj5(oversampling, 44100.0, true)
{
    mShifter = shifter;
}

PitchShiftOversampObj::~PitchShiftOversampObj() {}
    
void
PitchShiftOversampObj::ProcessSamplesBuffer(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                                            vector<WDL_TypedBuf<BL_FLOAT> > *out)
{
    mShifter->ProcessSamples(in, out);
}

#endif


//
PitchShifter::PitchShifter()
{
    mNumChannels = 1;

    mSampleRate = 44100.0;
    mOversampling = 1;
    
    mWDLPitchShifter = new WDL_PITCHSHIFTER();
    mWDLPitchShifter->set_srate(mSampleRate*mOversampling);
    mWDLPitchShifter->set_nch(1);
    
    mWDLPitchShifter->set_shift(1.0);
        
    //  When set to 0.5 for example, the sound plays 2 times slower
    // (without pitching)
    //mWDLPitchShifter->set_tempo(0.5);
    
    // Quality is oversampling (??)
    // When quality is 0, frequencies are oscillating a bit,
    // and transients are very well defined
    // When set to 8, frequencies are very constant, but transients are duplicate
    // (make a "slap back delay" effect)

    // Default
    //mWDLPitchShifter->SetQualityParameter(0);

    // Set to 4: make freqs constant, and do not enlarge transents too much
    mWDLPitchShifter->SetQualityParameter(4);
    
#if USE_OVERSAMP_OBJ
    mOversampObj = new PitchShiftOversampObj(this, 1);
#endif
}

PitchShifter::~PitchShifter()
{
    delete mWDLPitchShifter;

#if USE_OVERSAMP_OBJ
    delete mOversampObj;
#endif
}

void
PitchShifter::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mSampleRate = sampleRate;
    mWDLPitchShifter->set_srate(mSampleRate*mOversampling);

#if USE_OVERSAMP_OBJ
    mOversampObj->Reset(sampleRate, blockSize);
#endif
}

void
PitchShifter::ProcessSamples(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                             vector<WDL_TypedBuf<BL_FLOAT> > *out)
{
    if (in.empty())
        return;

    int nFrames = in[0].GetSize();
    
    WDL_SIMPLEPITCHSHIFT_SAMPLETYPE *buf = mWDLPitchShifter->GetBuffer(nFrames);
    BLUtilsPlug::ChannelsToInterleaved(in, buf);
               
    mWDLPitchShifter->BufferDone(nFrames);

    mCurrentOutput.Resize(nFrames*mNumChannels);
    WDL_SIMPLEPITCHSHIFT_SAMPLETYPE *outBuf = mCurrentOutput.Get();
    int numOut = mWDLPitchShifter->GetSamples(nFrames, outBuf);
    
    BLUtilsPlug::InterleavedToChannels(outBuf, out);
}

void
PitchShifter::Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                      vector<WDL_TypedBuf<BL_FLOAT> > *out)
{
#if USE_OVERSAMP_OBJ
    mOversampObj->Process(in, out);
#else
    Process(in, &out);
#endif
}

void
PitchShifter::SetNumChannels(int nchans)
{
    mNumChannels = nchans;
    mWDLPitchShifter->set_nch(nchans);
}
    
void
PitchShifter::SetFactor(BL_FLOAT factor)
{
    mWDLPitchShifter->set_shift(factor);
}

#if USE_OVERSAMP_OBJ
void
PitchShifter::SetQuality(int quality)
{
    int oversampling = 1;
    switch(quality)
    {
        case 0:
            oversampling = 1;
            break;
            
        case 1:
            oversampling = 4;
            break;
            
        case 2:
            oversampling = 8;
            break;
            
        case 3:
            oversampling = 32;
            break;
            
        default:
            break;
    }

    // TODO: manage better without re-creating the object (if required...)
    delete mOversampObj;
    mOversampObj = new PitchShiftOversampObj(this, oversampling);

    mOversampling = oversampling;
    mWDLPitchShifter->set_srate(mSampleRate*mOversampling);
}
#else
void
PitchShifter::SetQuality(int quality)
{
    int qual = 0;
    switch(quality)
    {
        case 0:
            qual = 0;
            break;
            
        case 1:
            qual = 1;
            break;
            
        case 2:
            qual = 4;
            break;
            
        case 3:
            qual = 8;
            break;
            
        default:
            break;
    }
    
    mWDLPitchShifter->SetQualityParameter(qual);
}
#endif
