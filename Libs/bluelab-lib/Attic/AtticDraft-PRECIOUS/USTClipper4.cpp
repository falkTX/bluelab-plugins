//
//  USTClipper4.cpp
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#include <USTClipperDisplay4.h>
#include <Utils.h>
#include <Debug.h>

#include <OversampProcessObj.h>
#include <IIRFilterLow12dB.h>
#include <NRBJFilter.h>

#include <OversampProcessObj2.h>

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


// USTClipper3: Problem, low pass filter used when down sampling
// => cuts a bit the high frequencies in the result

class ClipperOverObj : public OversampProcessObj
{
public:
    ClipperOverObj(int oversampling, double sampleRate);
    
    virtual ~ClipperOverObj();
    
    void Reset(double sampleRate);
    
    void ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer);
    
    void SetClipValue(double clipValue);
    
protected:
    double mClipValue;
    
    WDL_TypedBuf<double> mCopyBuffer;
    
#if NYQUIST_FILTER
#if !USE_RBJ_FILTER
    IIRFilterLow12dB *mFilter;
#else
    NRBJFilter *mFilter;
#endif
#endif
};

ClipperOverObj::ClipperOverObj(int oversampling, double sampleRate)
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
    double QFactor = 0.707;
    double gain = -60.0; //-100.0;
    mFilter = new NRBJFilter(8, //32, //4, //1,
                             FILTER_TYPE_HISHELF,
                             sampleRate*oversampling,
                             sampleRate*SAMPLE_RATE_COEFF,
                             QFactor, gain);
#endif
    
#endif
#endif
}

ClipperOverObj::~ClipperOverObj()
{
#if NYQUIST_FILTER
    delete mFilter;
#endif
}

void
ClipperOverObj::Reset(double sampleRate)
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
ClipperOverObj::ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer)
{
    if (mCopyBuffer.GetSize() != ioBuffer->GetSize())
        mCopyBuffer.Resize(ioBuffer->GetSize());
    
    for (int i = 0 ; i < ioBuffer->GetSize(); i++)
    {
        double input = ioBuffer->Get()[i];
        double output = 0.0;
        
        //USTClipper4::ComputeClipping(input, &output, mClipValue);
        USTClipper4::ComputeClippingKnee(input, &output,
                                         mClipValue, KNEE_RATIO);
        
        ioBuffer->Get()[i] = output;
    }
    
#if NYQUIST_FILTER // Filter for Nyquist
    WDL_TypedBuf<double> samples = *ioBuffer;
    
    mFilter->Process(ioBuffer, samples);
#endif
}

void
ClipperOverObj::SetClipValue(double clipValue)
{
    mClipValue = clipValue;
}
#endif

#if USE_OVEROBJ2
class ClipperOverObj : public OversampProcessObj2
{
public:
    ClipperOverObj(int oversampling, double sampleRate);
    
    virtual ~ClipperOverObj();
    
    void Reset(double sampleRate);
    
    void ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer);
    
    void SetClipValue(double clipValue);
    
protected:
    double mClipValue;
    
    WDL_TypedBuf<double> mCopyBuffer;
};

ClipperOverObj::ClipperOverObj(int oversampling, double sampleRate)
: OversampProcessObj2(oversampling, sampleRate)
{
    mClipValue = 2.0;
}

ClipperOverObj::~ClipperOverObj() {}

void
ClipperOverObj::Reset(double sampleRate)
{
    OversampProcessObj2::Reset(sampleRate);
}

void
ClipperOverObj::ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer)
{
    if (mCopyBuffer.GetSize() != ioBuffer->GetSize())
        mCopyBuffer.Resize(ioBuffer->GetSize());
    
    for (int i = 0 ; i < ioBuffer->GetSize(); i++)
    {
        double input = ioBuffer->Get()[i];
        double output = 0.0;
        
        USTClipper4::ComputeClipping(input, &output, mClipValue);
        
        ioBuffer->Get()[i] = output;
    }
}

void
ClipperOverObj::SetClipValue(double clipValue)
{
    mClipValue = clipValue;
}
#endif


USTClipper4::USTClipper4(GraphControl10 *graph, double sampleRate)
{
    mIsEnabled = true;
    mSampleRate = sampleRate;
    
    mClipperDisplay = new USTClipperDisplay4(graph, sampleRate);
    
    for (int i = 0; i < 2; i++)
        mClipObjs[i] = new ClipperOverObj(OVERSAMPLING, sampleRate);
}

USTClipper4::~USTClipper4()
{
    delete mClipperDisplay;
    
    for (int i = 0; i < 2; i++)
        delete mClipObjs[i];
}

void
USTClipper4::Reset(double sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < 2; i++) 
        mClipObjs[i]->Reset(sampleRate);
    
    mClipperDisplay->Reset(sampleRate);
}

void
USTClipper4::SetEnabled(bool flag)
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
USTClipper4::SetClipValue(double clipValue)
{
    mClipValue = clipValue;
    
    mClipperDisplay->SetClipValue(clipValue);
    
    for (int i = 0; i < 2; i++)
        mClipObjs[i]->SetClipValue(clipValue);
    
    // DEBUG
    DBG_TestClippingKnee(clipValue);
}

void
USTClipper4::SetZoom(double zoom)
{
    mClipperDisplay->SetZoom(zoom);
}


void
USTClipper4::Process(vector<WDL_TypedBuf<double> > *ioSamples)
{
    if (ioSamples->size() != 2)
        return;
    
    if (!mIsEnabled)
        return;
    
    // Display
    // Mono in
    WDL_TypedBuf<double> monoIn;
    Utils::StereoToMono(&monoIn, (*ioSamples)[0], (*ioSamples)[1]);
    mClipperDisplay->AddSamples(monoIn);
    
    // Process
    for (int i = 0; i < 2; i++)
    {
        if (i >= ioSamples->size())
            continue;
        
        //mClipObjs[i]->ProcessSamplesBuffer(&(*ioSamples)[i]);
        
        WDL_TypedBuf<double> input = (*ioSamples)[i];
        WDL_TypedBuf<double> output = (*ioSamples)[i];
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
    
    WDL_TypedBuf<double> monoOut;
    Utils::StereoToMono(&monoOut, (*ioSamples)[0], (*ioSamples)[1]);
    mClipperDisplay->AddClippedSamples(monoOut);
}

void
USTClipper4::ComputeClipping(double inSample, double *outSample, double clipValue)
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
USTClipper4::ComputeClippingKnee(double inSample, double *outSample,
                                 double clipValue, double kneeRatio)
{
    *outSample = 0.0;
    
    // Compute clip value depending on the input value
    //
    // Use knee
    //
    // See: https://dsp.stackexchange.com/questions/28548/differences-between-soft-knee-and-hard-knee-in-dynamic-range-compression-drc
    
    // 100% compressor slope => limiter
    double slope = 1.0;
    
    // Knee width
    double w = kneeRatio*2.0*clipValue;
    
    // Under threshold and knee => do nothing
    double absSamp = fabs(inSample);
    if (false) //(absSamp - clipValue < -w/2.0)
    {
        *outSample = inSample;
    }
    
    // In knee
    //if (fabs(absSamp - clipValue) <= w/2.0)
    {
        // Lagrange: https://en.wikipedia.org/wiki/Lagrange_polynomial
    
        double p0[2] = { 0.0, 0.0 };
        double p1[2] = { clipValue - w/2, clipValue - w/2 };
        //double p2[2] = { clipValue + w/2, clipValue };
        double p2[2];
        double amp = (w/2 + clipValue);
        double gain = (clipValue/amp)*slope + (1.0 - slope)*1.0;
        p2[0] = clipValue + w/2;
        p2[1] = gain*amp;
        
        double p3[2];
        p3[0] = 1.0;
        p3[1] = clipValue*slope + (1.0 - slope);
        
        double x = absSamp;
    
        double y = Utils::LagrangeInterp4(x, p0, p1, p2, p3);
        
        *outSample = y;
        
#if 1
        // DEBUG
        WDL_TypedBuf<double> dbgVecX;
        dbgVecX.Resize(4);
#define COEFF 1000.0
        dbgVecX.Get()[0] = p0[0]*COEFF;
        dbgVecX.Get()[1] = p1[0]*COEFF;
        dbgVecX.Get()[2] = p2[0]*COEFF;
        dbgVecX.Get()[3] = p3[0]*COEFF;
        
        WDL_TypedBuf<double> dbgVecY;
        dbgVecY.Resize(4);
        dbgVecY.Get()[0] = p0[1];
        dbgVecY.Get()[1] = p1[1];
        dbgVecY.Get()[2] = p2[1];
        dbgVecY.Get()[3] = p3[1];
        
        Debug::DumpData("x.txt", dbgVecX);
        Debug::DumpData("y.txt", dbgVecY);
#endif
    }
    
    // After knee
    if (false) //(absSamp - clipValue > w/2.0)
    {
        if (absSamp > EPS)
        {
            double gain = (clipValue/absSamp)*slope + (1.0 - slope);
            *outSample = inSample*gain;
        }
    }
    
    if (inSample < 0.0)
        *outSample = - *outSample;
}

void
USTClipper4::DBG_TestClippingKnee(double clipValue)
{
    WDL_TypedBuf<double> buf;
    buf.Resize(1024);
    
    for (int i = 0; i < buf.GetSize(); i++)
    {
        double t = ((double)i)/(buf.GetSize() - 1);
        double t2;
        ComputeClippingKnee(t, &t2, clipValue, KNEE_RATIO);
        //USTClipper4::ComputeClipping(t, &t2, clipValue);
        
        buf.Get()[i] = t2;
    }
    
    Debug::DumpData("clip.txt", buf);
}
