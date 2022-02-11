//
//  USTClipper4.cpp
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#include <USTClipperDisplay4.h>
#include <BLUtils.h>
#include <BLDebug.h>

#define OVERSAMP_OBJ_CLASS OversampProcessObj3
//#define OVERSAMP_OBJ OversampProcessObj4

//#include <OversampProcessObj.h>
//#include <OversampProcessObj2.h>
#include <OversampProcessObj3.h>
#include <OversampProcessObj4.h>

#include <FilterIIRLow12dB.h>
#include <FilterRBJNX.h>

#include <DelayObj4.h>

#include "USTClipper4.h"

//#define CLIPPER_EPS 1e-3
//#define CLIPPER_EPS 1e-5 // -100dB
#define CLIPPER_EPS 1.99 // 6dB

// See: https://www.siraudiotools.com/StandardCLIP_manual.php
//#define OVERSAMPLING 8 //16 //8 //4
#define OVERSAMPLING 4

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

#define KNEE_RATIO 0.25

#define EPS 1e-15

// Delay the input samples, so on the display,
// the input samples and the processed samples will be aligned
#define DISPLAY_LATENCY_COMPENSATION 1

// USTClipper3: Problem, low pass filter used when down sampling
// => cuts a bit the high frequencies in the result

class ClipperOverObj4 : public OVERSAMP_OBJ_CLASS
{
public:
    ClipperOverObj4(int oversampling, BL_FLOAT sampleRate);
    
    virtual ~ClipperOverObj4();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void SetClipValue(BL_FLOAT clipValue);
    
protected:
    BL_FLOAT mClipValue;
    
    WDL_TypedBuf<BL_FLOAT> mCopyBuffer;
    
#if NYQUIST_FILTER
#if !USE_RBJ_FILTER
    FilterIIRLow12dB *mFilter;
#else
    FilterRBJNX *mFilter;
#endif
#endif
};

ClipperOverObj4::ClipperOverObj4(int oversampling, BL_FLOAT sampleRate)
: OVERSAMP_OBJ_CLASS(oversampling, sampleRate, OVEROBJ_FILTER_NYQUIST)
{
    mClipValue = 2.0;
    
#if NYQUIST_FILTER
#if !USE_RBJ_FILTER
    mFilter = new FilterIIRLow12dB();
    mFilter->Init(sampleRate*SAMPLE_RATE_COEFF, sampleRate*oversampling);
#else
    
#if 1
    mFilter = new FilterRBJNX(1, //8, //32, //4,
                             FILTER_TYPE_LOWPASS,
                             sampleRate*oversampling,
                             sampleRate*SAMPLE_RATE_COEFF);
#endif
#if 0
    BL_FLOAT QFactor = 0.707;
    BL_FLOAT gain = -60.0; //-100.0;
    mFilter = new FilterRBJNX(8, //32, //4, //1,
                             FILTER_TYPE_HISHELF,
                             sampleRate*oversampling,
                             sampleRate*SAMPLE_RATE_COEFF,
                             QFactor, gain);
#endif
    
#endif
#endif
}

ClipperOverObj4::~ClipperOverObj4()
{
#if NYQUIST_FILTER
    delete mFilter;
#endif
}

/*void
ClipperOverObj4::Reset(BL_FLOAT sampleRate)
{
    OVERSAMP_OBJ_CLASS::Reset(sampleRate);
    
#if NYQUIST_FILTER
#if !USE_RBJ_FILTER
    mFilter->Init(sampleRate*SAMPLE_RATE_COEFF, sampleRate*mOversampling);
#else
    mFilter->SetSampleRate(sampleRate*mOversampling);
    mFilter->SetCutoffFreq(sampleRate*SAMPLE_RATE_COEFF);
#endif
#endif
}*/

void
ClipperOverObj4::Reset(BL_FLOAT sampleRate, int blockSize)
{
    OVERSAMP_OBJ_CLASS::Reset(sampleRate, blockSize);
    
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
ClipperOverObj4::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mCopyBuffer.GetSize() != ioBuffer->GetSize())
        mCopyBuffer.Resize(ioBuffer->GetSize());
    
    for (int i = 0 ; i < ioBuffer->GetSize(); i++)
    {
        BL_FLOAT input = ioBuffer->Get()[i];
        BL_FLOAT output = 0.0;
        
        //USTClipper4::ComputeClipping(input, &output, mClipValue);
        USTClipper4::ComputeClippingKnee(input, &output,
                                         mClipValue, KNEE_RATIO);
        
        ioBuffer->Get()[i] = output;
    }
    
#if NYQUIST_FILTER // Filter for Nyquist
    WDL_TypedBuf<BL_FLOAT> samples = *ioBuffer;
    
    mFilter->Process(ioBuffer, samples);
#endif
}

void
ClipperOverObj4::SetClipValue(BL_FLOAT clipValue)
{
    mClipValue = clipValue;
}
#endif

#if USE_OVEROBJ2
class ClipperOverObj4 : public OversampProcessObj2
{
public:
    ClipperOverObj4(int oversampling, BL_FLOAT sampleRate);
    
    virtual ~ClipperOverObj4();
    
    void Reset(BL_FLOAT sampleRate);
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void SetClipValue(BL_FLOAT clipValue);
    
protected:
    BL_FLOAT mClipValue;
    
    WDL_TypedBuf<BL_FLOAT> mCopyBuffer;
};

ClipperOverObj4::ClipperOverObj4(int oversampling, BL_FLOAT sampleRate)
: OversampProcessObj2(oversampling, sampleRate)
{
    mClipValue = 2.0;
}

ClipperOverObj4::~ClipperOverObj4() {}

void
ClipperOverObj4::Reset(BL_FLOAT sampleRate)
{
    OversampProcessObj2::Reset(sampleRate);
}

void
ClipperOverObj4::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mCopyBuffer.GetSize() != ioBuffer->GetSize())
        mCopyBuffer.Resize(ioBuffer->GetSize());
    
    for (int i = 0 ; i < ioBuffer->GetSize(); i++)
    {
        BL_FLOAT input = ioBuffer->Get()[i];
        BL_FLOAT output = 0.0;
        
        USTClipper4::ComputeClipping(input, &output, mClipValue);
        
        ioBuffer->Get()[i] = output;
    }
}

void
ClipperOverObj4::SetClipValue(BL_FLOAT clipValue)
{
    mClipValue = clipValue;
}
#endif


USTClipper4::USTClipper4(GraphControl11 *graph, BL_FLOAT sampleRate)
{
    mIsEnabled = true;
    mSampleRate = sampleRate;
    
    mClipperDisplay = new USTClipperDisplay4(graph, sampleRate);
    
    for (int i = 0; i < 2; i++)
        mClipObjs[i] = new ClipperOverObj4(OVERSAMPLING, sampleRate);
    
    mCurrentBlockSize = 1024;
    
    mInputDelay = NULL;
    
#if DISPLAY_LATENCY_COMPENSATION
    int latency = GetLatency();
    mInputDelay = new DelayObj4(latency);
#endif
}

USTClipper4::~USTClipper4()
{
    delete mClipperDisplay;
    
    for (int i = 0; i < 2; i++)
        delete mClipObjs[i];
}

/*void
USTClipper4::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < 2; i++) 
        mClipObjs[i]->Reset(sampleRate);
    
    mClipperDisplay->Reset(sampleRate);
}*/

void
USTClipper4::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mCurrentBlockSize = blockSize;
    
    mSampleRate = sampleRate;
    
    for (int i = 0; i < 2; i++)
        mClipObjs[i]->Reset(sampleRate, blockSize);
    
    mClipperDisplay->Reset(sampleRate);
    
#if DISPLAY_LATENCY_COMPENSATION
    int latency = GetLatency();
    mInputDelay->Reset();
    mInputDelay->SetDelay(latency);
#endif
}

void
USTClipper4::SetEnabled(bool flag)
{
    mIsEnabled = flag;
    
    // When disabled, manage to clear the waveform
    if (!flag)
    {
        Reset(mSampleRate, mCurrentBlockSize);
        
        mClipperDisplay->SetDirty();
    }
}

void
USTClipper4::SetClipValue(BL_FLOAT clipValue)
{
    mClipValue = clipValue;
    
    mClipperDisplay->SetClipValue(clipValue);
    
    for (int i = 0; i < 2; i++)
        mClipObjs[i]->SetClipValue(clipValue);
    
    // DEBUG
    //DBG_TestClippingKnee(clipValue);
}

void
USTClipper4::SetZoom(BL_FLOAT zoom)
{
    mClipperDisplay->SetZoom(zoom);
}

int
USTClipper4::GetLatency()
{
    if (mIsEnabled && (mClipObjs[0] != NULL))
    {
        int latency = mClipObjs[0]->GetLatency();
        
        return latency;
    }
    
    return 0;
}

void
USTClipper4::Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples)
{
    if (ioSamples->size() != 2)
        return;
    
    if (!mIsEnabled)
        return;
    
    // Display
    // Mono in
    WDL_TypedBuf<BL_FLOAT> monoIn;
    BLUtils::StereoToMono(&monoIn, (*ioSamples)[0], (*ioSamples)[1]);
    
#if DISPLAY_LATENCY_COMPENSATION
    mInputDelay->ProcessSamples(&monoIn);
#endif
    
    mClipperDisplay->AddSamples(monoIn);
    
    // Process
    for (int i = 0; i < 2; i++)
    {
        if (i >= ioSamples->size())
            continue;
        
        //mClipObjs[i]->ProcessSamplesBuffer(&(*ioSamples)[i]);
        
        //WDL_TypedBuf<BL_FLOAT> input = (*ioSamples)[i];
        //WDL_TypedBuf<BL_FLOAT> output = (*ioSamples)[i];
        //mClipObjs[i]->Process(input.Get(), output.Get(), input.GetSize());
        
        WDL_TypedBuf<BL_FLOAT> buf = (*ioSamples)[i];
        mClipObjs[i]->Process(&buf);
        
        // Take result into accounts only if clipper value is > 0%
        // (so possible to ignore the clipper process)
        //
        // But we still will feed the oversampler anyway !
        if (mClipValue </*>*/ CLIPPER_EPS)
        {
            //(*ioSamples)[i] = output;
            (*ioSamples)[i] = buf;
        }
    }
    
    WDL_TypedBuf<BL_FLOAT> monoOut;
    BLUtils::StereoToMono(&monoOut, (*ioSamples)[0], (*ioSamples)[1]);
    mClipperDisplay->AddClippedSamples(monoOut);
}

void
USTClipper4::ComputeClipping(BL_FLOAT inSample, BL_FLOAT *outSample, BL_FLOAT clipValue)
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

void
USTClipper4::ComputeClippingKnee(BL_FLOAT inSample, BL_FLOAT *outSample,
                                 BL_FLOAT clipValue, BL_FLOAT kneeRatio)
{
    *outSample = 0.0;
    
    // Compute clip value depending on the input value
    //
    // Use knee
    //
    // See: https://dsp.stackexchange.com/questions/28548/differences-between-soft-knee-and-hard-knee-in-dynamic-range-compression-drc
    
    // 100% compressor slope => limiter
    // NOTE: knee not working with slope != 1.0
    BL_FLOAT slope = 1.0; //0.75;
    
    // Knee width
    BL_FLOAT w = kneeRatio*2.0*clipValue;
    
    // Under threshold and knee => do nothing
    BL_FLOAT absSamp = std::fabs(inSample);
    if (absSamp - clipValue < -w/2.0)
    {
        *outSample = inSample;
    }
    
    // In knee
    if (std::fabs(absSamp - clipValue) <= w/2.0)
    {
        // See: https://christianfloisand.wordpress.com/2014/07/16/dynamics-processing-compressorlimiter-part-3/
        BL_FLOAT t = (absSamp - clipValue + w/2.0)/w;
        
        BL_FLOAT slope0 = 1.0;
        BL_FLOAT slope1 = 1.0 - slope;
        BL_FLOAT s = (1.0 - t)*slope0 + t*slope1;
        
        // Some cooking (but it works!)
        // NOTE: works only for slope = 1.0
        s = s*s;
        
        BL_FLOAT gain0 = (clipValue - w/2.0)/absSamp;
        BL_FLOAT gain1 = clipValue/absSamp;
        
        //gain1 += (1.0 - slope)*0.27; // => ok for 0.75% (0.0675)
        //gain1 += (1.0 - slope)*0.4;  // => ok for 0.5%  (0.2)
        //gain1 += (1.0 - slope)*0.8;  // => ok for 0.25% (0.6)
        
        BL_FLOAT gain = s*gain0 + (1.0 - s)*gain1;
        
        *outSample = inSample*gain;
    }
    
    // After knee
    if (absSamp - clipValue > w/2.0)
    //if (absSamp - clipValue > 0.0)
    {
        if (absSamp > EPS)
        {
            BL_FLOAT gain0 = clipValue/absSamp;
            BL_FLOAT gain1 = 1.0;
            BL_FLOAT gain = slope*gain0 + (1.0 - slope)*gain1;
            
            *outSample = inSample*gain;
        }
    }
}

void
USTClipper4::DBG_TestClippingKnee(BL_FLOAT clipValue)
{
    WDL_TypedBuf<BL_FLOAT> buf;
    buf.Resize(1024);
    
    for (int i = 0; i < buf.GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/(buf.GetSize() - 1);
        BL_FLOAT t2;
        ComputeClippingKnee(t, &t2, clipValue, KNEE_RATIO);
        //USTClipper4::ComputeClipping(t, &t2, clipValue);
        
        buf.Get()[i] = t2;
    }
    
    BLDebug::DumpData("clip.txt", buf);
}
