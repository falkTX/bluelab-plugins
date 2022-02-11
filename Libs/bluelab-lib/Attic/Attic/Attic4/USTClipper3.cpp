//
//  USTClipper3.cpp
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#include <USTClipperDisplay4.h>
#include <BLUtils.h>

#include <OversampProcessObj.h>
#include <FilterIIRLow12dB.h>
#include <FilterRBJNX.h>

#include <OversampProcessObj2.h>

#include "USTClipper3.h"

//#define CLIPPER_EPS 1e-3
//#define CLIPPER_EPS 1e-5 // -100dB
#define CLIPPER_EPS 1.99 // 6dB

// See: https://www.siraudiotools.com/StandardCLIP_manual.php
#define OVERSAMPLING 8 //16 //8 //4

// Use OversampProcessObj or OversampProcessObj2 ?
//
// NOTE: OversampProcessObj2 is costly when downsampling (kernel size...)
//
// NOTE: Set to 1 at the origin, but disabled because OversampProcessObj2
// consumes too much CPU resources
// Instead, use OversampProcessObj with new integrated Nyquist filter
// (seems to give the same results, with less CPU consumption)
#define USE_OVEROBJ2 0

#if !USE_OVEROBJ2

// See: https://nickwritesablog.com/introduction-to-oversampling-for-alias-reduction
//
// and: https://www.sciencedirect.com/topics/engineering/antialiasing-filter

// NOTE: set to 1 at the origin, but disabled because we now have OVEROBJ_FILTER_NYQUIST
#define NYQUIST_FILTER 0 // 1

// Better: avoid global gain decrease
#define USE_RBJ_FILTER 1

// NEW
// Set Nyquist filter in the parent generic class
#define OVEROBJ_FILTER_NYQUIST 1

// Do not use 0.5 (e.g filtering at 22050Hz for sample rate 44100Hz)
// Because due to the filter slope, we will remove freqs between 20000Hz and 22000Hz
//#define SAMPLE_RATE_COEFF 0.5

// It is safe to kee some frequencies just above sampleRate/2
// (see docs on internet)
//#define SAMPLE_RATE_COEFF 0.75

#define SAMPLE_RATE_COEFF 0.5 //0.75 //4.0 //0.5


// USTClipper3: Problem, low pass filter used when down sampling
// => cuts a bit the high frequencies in the result

class ClipperOverObj3 : public OversampProcessObj
{
public:
    ClipperOverObj3(int oversampling, BL_FLOAT sampleRate);
    
    virtual ~ClipperOverObj3();
    
    void Reset(BL_FLOAT sampleRate);
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void SetClipValue(BL_FLOAT clipValue);
    
protected:
    BL_FLOAT mClipValue;
    
    WDL_TypedBuf<BL_FLOAT> mCopyBuffer;
    
#if NYQUIST_FILTER
#if !USE_RBJ_FILTER
    IIRFilterLow12dB *mFilter;
#else
    NRBJFilter *mFilter;
#endif
#endif
};

ClipperOverObj3::ClipperOverObj3(int oversampling, BL_FLOAT sampleRate)
: OversampProcessObj(oversampling, sampleRate, OVEROBJ_FILTER_NYQUIST)
{
    mClipValue = 2.0;
    
#if NYQUIST_FILTER
#if !USE_RBJ_FILTER
    mFilter = new IIRFilterLow12dB();
    mFilter->Init(sampleRate*SAMPLE_RATE_COEFF, sampleRate*oversampling);
#else
    
#if 1
    mFilter = new NRBJFilter(1, //8, //32, //4,
                             FILTER_TYPE_LOWPASS,
                             sampleRate*oversampling,
                             sampleRate*SAMPLE_RATE_COEFF);
#endif
#if 0
    BL_FLOAT QFactor = 0.707;
    BL_FLOAT gain = -60.0; //-100.0;
    mFilter = new NRBJFilter(8, //32, //4, //1,
                             FILTER_TYPE_HISHELF,
                             sampleRate*oversampling,
                             sampleRate*SAMPLE_RATE_COEFF,
                             QFactor, gain);
#endif
    
#endif
#endif
}

ClipperOverObj3::~ClipperOverObj3()
{
#if NYQUIST_FILTER
    delete mFilter;
#endif
}

void
ClipperOverObj3::Reset(BL_FLOAT sampleRate)
{
    OversampProcessObj::Reset(sampleRate);
    
#if NYQUIST_FILTER
#if !USE_RBJ_FILTER
    mFilter->Init(sampleRate*SAMPLE_RATE_COEFF, sampleRate*mOversampling);
#else
    mFilter->SetSampleRate(sampleRate*mOversampling);
    mFilter->SetCutoffFreq(sampleRate*SAMPLE_RATE_COEFF);
#endif
#endif
}

void
ClipperOverObj3::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mCopyBuffer.GetSize() != ioBuffer->GetSize())
        mCopyBuffer.Resize(ioBuffer->GetSize());
    
    for (int i = 0 ; i < ioBuffer->GetSize(); i++)
    {
        BL_FLOAT input = ioBuffer->Get()[i];
        BL_FLOAT output = 0.0;
        
        USTClipper3::ComputeClipping(input, &output, mClipValue);
        
        ioBuffer->Get()[i] = output;
    }
    
#if NYQUIST_FILTER // Filter for Nyquist
    WDL_TypedBuf<BL_FLOAT> samples = *ioBuffer;
    
    mFilter->Process(ioBuffer, samples);
#endif
}

void
ClipperOverObj3::SetClipValue(BL_FLOAT clipValue)
{
    mClipValue = clipValue;
}
#endif

#if USE_OVEROBJ2
class ClipperOverObj3 : public OversampProcessObj2
{
public:
    ClipperOverObj3(int oversampling, BL_FLOAT sampleRate);
    
    virtual ~ClipperOverObj3();
    
    void Reset(BL_FLOAT sampleRate);
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void SetClipValue(BL_FLOAT clipValue);
    
protected:
    BL_FLOAT mClipValue;
    
    WDL_TypedBuf<BL_FLOAT> mCopyBuffer;
};

ClipperOverObj3::ClipperOverObj3(int oversampling, BL_FLOAT sampleRate)
: OversampProcessObj2(oversampling, sampleRate)
{
    mClipValue = 2.0;
}

ClipperOverObj3::~ClipperOverObj3() {}

void
ClipperOverObj3::Reset(BL_FLOAT sampleRate)
{
    OversampProcessObj2::Reset(sampleRate);
}

void
ClipperOverObj3::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mCopyBuffer.GetSize() != ioBuffer->GetSize())
        mCopyBuffer.Resize(ioBuffer->GetSize());
    
    for (int i = 0 ; i < ioBuffer->GetSize(); i++)
    {
        BL_FLOAT input = ioBuffer->Get()[i];
        BL_FLOAT output = 0.0;
        
        USTClipper3::ComputeClipping(input, &output, mClipValue);
        
        ioBuffer->Get()[i] = output;
    }
}

void
ClipperOverObj3::SetClipValue(BL_FLOAT clipValue)
{
    mClipValue = clipValue;
}
#endif


USTClipper3::USTClipper3(GraphControl11 *graph, BL_FLOAT sampleRate)
{
    mIsEnabled = true;
    mSampleRate = sampleRate;
    
    mClipperDisplay = new USTClipperDisplay4(graph, sampleRate);
    
    for (int i = 0; i < 2; i++)
        mClipObjs[i] = new ClipperOverObj3(OVERSAMPLING, sampleRate);
}

USTClipper3::~USTClipper3()
{
    delete mClipperDisplay;
    
    for (int i = 0; i < 2; i++)
        delete mClipObjs[i];
}

void
USTClipper3::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < 2; i++) 
        mClipObjs[i]->Reset(sampleRate);
    
    mClipperDisplay->Reset(sampleRate);
}

void
USTClipper3::SetEnabled(bool flag)
{
    mIsEnabled = flag;
    
    // When disabled, manage to clear the waveform
    if (!flag)
    {
        Reset(mSampleRate);
        
        mClipperDisplay->SetDirty();
    }
}

void
USTClipper3::SetClipValue(BL_FLOAT clipValue)
{
    mClipValue = clipValue;
    
    mClipperDisplay->SetClipValue(clipValue);
    
    for (int i = 0; i < 2; i++)
        mClipObjs[i]->SetClipValue(clipValue);
}

void
USTClipper3::SetZoom(BL_FLOAT zoom)
{
    mClipperDisplay->SetZoom(zoom);
}


void
USTClipper3::Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples)
{
    if (ioSamples->size() != 2)
        return;
    
    if (!mIsEnabled)
        return;
    
    // Display
    // Mono in
    WDL_TypedBuf<BL_FLOAT> monoIn;
    BLUtils::StereoToMono(&monoIn, (*ioSamples)[0], (*ioSamples)[1]);
    mClipperDisplay->AddSamples(monoIn);
    
    // Process
    for (int i = 0; i < 2; i++)
    {
        if (i >= ioSamples->size())
            continue;
        
        //mClipObjs[i]->ProcessSamplesBuffer(&(*ioSamples)[i]);
        
        WDL_TypedBuf<BL_FLOAT> input = (*ioSamples)[i];
        WDL_TypedBuf<BL_FLOAT> output = (*ioSamples)[i];
        mClipObjs[i]->Process(input.Get(), output.Get(), input.GetSize());
        
        // Take result into accounts only if clipper value is > 0%
        // (so possible to ignore the clipper process)
        //
        // But we still will feed the oversampler anyway !
        if (mClipValue </*>*/ CLIPPER_EPS)
        {
            (*ioSamples)[i] = output;
        }
    }
    
    WDL_TypedBuf<BL_FLOAT> monoOut;
    BLUtils::StereoToMono(&monoOut, (*ioSamples)[0], (*ioSamples)[1]);
    mClipperDisplay->AddClippedSamples(monoOut);
}

void
USTClipper3::ComputeClipping(BL_FLOAT inSample, BL_FLOAT *outSample, BL_FLOAT clipValue)
{
#if 0 // Old code, for clipper in %
    clipValue = 2.0 - clipValue;
#endif
    
    *outSample = inSample;
    
    if (inSample > clipValue)
        *outSample = clipValue;
    
    if (inSample < -clipValue)
        *outSample = -clipValue;
}
