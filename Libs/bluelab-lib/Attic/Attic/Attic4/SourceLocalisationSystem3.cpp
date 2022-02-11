//
//  SourceLocalisationSystem3.cpp
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#include <lice.h>

#include <DelayLinePhaseShift2.h>

#include <BLUtils.h>
#include <BLDebug.h>

#include <PPMFile.h>

#include <FftProcessObj16.h>

#include "SourceLocalisationSystem3.h"

// Process only before 6KHz
#define CUTOFF_FREQ 6000.0
//#define CUTOFF_FREQ 12000.0 // Raindrops
//#define CUTOFF_FREQ 22050.0

// Low pass filter
#define LOW_PASS_FREQ 100.0 //700.0 //100.0
#define LOW_PASS_FREQ 10.0 /// Motor

// ORIG (makes several detections with a single sine sweep source)
#define INTER_MIC_DISTANCE 0.2 // 20cm

// TEST (good with a single sine sweep source)
//#define INTER_MIC_DISTANCE 0.02 //0.2 // 20cm

#define SOUND_SPEED 340.0

//#define TIME_INTEGRATE_SMOOTH_FACTOR 0.98
#define TIME_INTEGRATE_SMOOTH_FACTOR 0.9 // Not used...
//#define TIME_INTEGRATE_SMOOTH_FACTOR 0.5
//#define TIME_INTEGRATE_SMOOTH_FACTOR 0.02 //0.5 //0.98

//

// Original algo
// "Localization of multiple sound sources with two microphonesaé
// Chen Liu
#define ALGO_LIU 0 //1 //0 //1 //0

// Far better:
// - avoids a constant fake source at the center
// - more accurate
//
// Find minima
#define ALGO_NIKO 1 //0 //1 //0

// "Azimuthal sound localization using coincidence of timing across frequency on a robotic platform"
// Laurent Calmes
// WARNING: some hard coded values!
// NOTE: with test project with 2 sine, Calmes works far better to extract the 2 sources !
#define ALGO_CALMES 0 //0 //1
#define DEBUG_CALMES 0 //1

//

// Stencil
#define USE_STENCIL 1 //0 // TEST2 1 //0 //1
//#define USE_STENCIL_METHOD3 1
#define DEBUG_STENCIL_MASK 0 //1 // => test passes !

// Binarize before stencil
#define USE_STENCIL_MINIMA 0 //1 // NEW

// Test validated !
#define DEBUG_DELAY_LINE 0 //1

// Display localization curve over the view
#define DEBUG_DISPLAY_LOCALIZATION_CURVE 1

#define DEBUG_COLOR_FACTOR 255.0 // Saturates a lot when displaying debug localization points
//#define DEBUG_COLOR_FACTOR 1.0 // Do not saturate => display well localization curve

// For coincident microphones
#define ALGO_AMP_DIFF 1 //0 //1


SourceLocalisationSystem3::SourceLocalisationSystem3(Mode mode,
                                                     int bufferSize,
                                                     BL_FLOAT sampleRate,
                                                     int numBands)
{
    mMode = mode;
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    mNumBands = numBands;
    
    mIntermicDistanceCoeff = 1.0;
    
    mTimeSmooth = 0.9;
    
    Init();
}

SourceLocalisationSystem3::~SourceLocalisationSystem3()
{
    DeleteDelayLines();
}

void
SourceLocalisationSystem3::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    Init();
}

void
SourceLocalisationSystem3::AddSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2])
{
#if DEBUG_DELAY_LINE
    DelayLinePhaseShift2::DBG_Test(mBufferSize, mSampleRate, samples[0]);
#endif
    
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> samples0[2] = { samples[0], samples[1] };
    samples0[0].Resize(numBins);
    samples0[1].Resize(numBins);
    
    // Low pass
    LowPassFilter(&samples0[0]);
    LowPassFilter(&samples0[1]);
    
    // Compute coincidence
//#if !ALGO_AMP_DIFF
    if (mMode == PHASE_DIFF)
        ComputeCoincidences(samples0, &mCoincidence);
//#else
    if (mMode == AMP_DIFF)
        ComputeAmpCoincidences(samples0, &mCoincidence);
//#endif
    
    if (mMode == PHASE_DIFF)
    {
#if ALGO_LIU
        // NOTE: with FindMinima() + Threshold(), there is always
        // a detected source at the middle (azimuth 0)
    
        //FindMinima(&mCoincidence);
        FindMinima4(&mCoincidence); // Absolute minima
        //FindMinima5(&mCoincidence); // Ponderate with value
    
        TimeIntegrate(&mCoincidence);
    
        //Threshold(&mCoincidence); // Should be done !
    
#if !USE_STENCIL
        FreqIntegrate(mCoincidence, &mCurrentLocalization);
#else
        FreqIntegrateStencil(mCoincidence, &mCurrentLocalization);
#endif
    
        // TEST
        //BLUtils::MultValues(&mCurrentLocalization, 1.0/16.0);
        //BLDebug::DumpData("loc.txt", mCurrentLocalization);
#endif
   
        //
    
#if ALGO_NIKO
        //DBG_DumpCoincidence("coincidence0", mCoincidence, 10000.0);
    
        TimeIntegrate(&mCoincidence);
    
        //DBG_DumpCoincidence("coincidence1", mCoincidence, 10000.0);
    
#if !USE_STENCIL
        FreqIntegrate(mCoincidence, &mCurrentLocalization);
    
        // FindMinima3(&mCurrentLocalization); // ORIG
    
        // TEST
        BLUtils::MultValues(&mCurrentLocalization, 5.0);
        InvertValues(&mCurrentLocalization); //
#else
    
        //DBG_DumpCoincidence("coincidence0", mCoincidence, 10000.0);
    
#if USE_STENCIL_MINIMA
        FindMinima4(&mCoincidence);
#endif
    
        //DBG_DumpCoincidence("coincidence0", mCoincidence, 10000.0);
    
        //BuildMinMaxMap(&mCoincidence);
    
        //DBG_DumpCoincidence("coincidence1", mCoincidence, 1.0); //10000.0);
    
        //TimeIntegrate(&mCoincidence); // TEST
    
        //DBG_DumpCoincidence("coincidence2", mCoincidence, 10000.0); // TEST
    
        // Custom simple method
        FreqIntegrateStencil(mCoincidence, &mCurrentLocalization);
        // Correlation
        //FreqIntegrateStencil2(mCoincidence, &mCurrentLocalization);
        // Two steps method
        //FreqIntegrateStencil3(mCoincidence, &mCurrentLocalization);
    
        //FindMinima3(&mCurrentLocalization); // ORIG

        // For BuildMinMaxMap
        //BLUtils::MultValues(&mCurrentLocalization, 1.0/34000.0);
    
#if !USE_STENCIL_MINIMA
        // TEST
        BLUtils::MultValues(&mCurrentLocalization, (BL_FLOAT)5.0);
    
        InvertValues(&mCurrentLocalization); //
#else
        BLUtils::MultValues(&mCurrentLocalization, (BL_FLOAT)0.01);
#endif
    
#endif
    
#endif
    
#if ALGO_CALMES
        TimeIntegrate(&mCoincidence);

#if DEBUG_CALMES
        //DBG_DumpCoincidence("coincidence0", mCoincidence, 10000.0);
#endif
    
        //FreqIntegrateStencilCalmes(mCoincidence, &mCurrentLocalization);
        FreqIntegrateStencilCalmes2(mCoincidence, &mCurrentLocalization);
    
#if DEBUG_CALMES
        //DBG_DumpCoincidence("debug", mCoincidence, 10000.0);
#endif

#endif
    
        //DBG_DumpCoincidence("coincidence", mCoincidence, 10000.0);
    
        //BLDebug::DumpData("loc.txt", mCurrentLocalization);
    
#if DEBUG_DISPLAY_LOCALIZATION_CURVE
        //DBG_DisplayLocalizationcurve();
#endif
    }
    
//#if ALGO_AMP_DIFF
    if (mMode == AMP_DIFF)
    {
        TimeIntegrate(&mCoincidence);
        FreqIntegrate(mCoincidence, &mCurrentLocalization);
    
        BLUtils::MultValues(&mCurrentLocalization, (BL_FLOAT)5.0);
    }
//#endif
}

void
SourceLocalisationSystem3::GetLocalization(WDL_TypedBuf<BL_FLOAT> *localization)
{
    *localization = mCurrentLocalization;
}

int
SourceLocalisationSystem3::GetNumBands()
{
    return mNumBands;
}

void
SourceLocalisationSystem3::SetTimeSmooth(BL_FLOAT smoothCoeff)
{
    mTimeSmooth = smoothCoeff;
}

void
SourceLocalisationSystem3::SetIntermicDistanceCoeff(BL_FLOAT intermicCoeff)
{
    mIntermicDistanceCoeff = intermicCoeff;
    
    Init();
}

void
SourceLocalisationSystem3::DBG_GetCoincidence(int *width, int *height, WDL_TypedBuf<BL_FLOAT> *data)
{
    vector<WDL_TypedBuf<BL_FLOAT> > coincidence = mCoincidence;

#if DEBUG_DISPLAY_LOCALIZATION_CURVE
    DBG_DisplayLocalizationcurve(&coincidence, mCurrentLocalization);
#endif
    
    *width = 0;
    *height = 0;
    
    if (coincidence.empty())
        return;
    
    *width = coincidence.size();
    *height = coincidence[0].GetSize();
    
    data->Resize((*width)*(*height));
    
    for (int i = 0; i < *width; i++)
    {
        for (int j = 0; j < *height; j++)
        {
            BL_FLOAT val = coincidence[i].Get()[j];
            
            val *= DEBUG_COLOR_FACTOR;
            
            data->Get()[i + j*(*width)] = val;
        }
    }
}

void
SourceLocalisationSystem3::Init()
{ 
    DeleteDelayLines();
    
    mDelayLines[0].clear();
    mDelayLines[1].clear();
    
    BL_FLOAT maxDelay = 0.5*(INTER_MIC_DISTANCE*mIntermicDistanceCoeff)/SOUND_SPEED;
    
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    //
    for (int k = 0; k < 2; k++)
    {
        for (int i = 0; i < numBins; i++)
        {
            vector<DelayLinePhaseShift2 *> bandLines;
            for (int j = 0; j < mNumBands; j++)
            {
                //BL_FLOAT delay = maxDelay*sin((((BL_FLOAT)(j - 1))/(mNumBands - 1))*M_PI - M_PI/2.0);
	      BL_FLOAT delay = maxDelay*std::sin((((BL_FLOAT)j)/(mNumBands - 1))*M_PI - M_PI/2.0);
                
                if (k == 1)
                {
                    delay = -delay;
                }
                
                DelayLinePhaseShift2 *line = new DelayLinePhaseShift2(mSampleRate, mBufferSize,
                                                                      i, maxDelay*2.0, delay);
        
                bandLines.push_back(line);
            }
        
            mDelayLines[k].push_back(bandLines);
        }
    }
    
#if USE_STENCIL
    GenerateStencilMasks();
#endif
}

void
SourceLocalisationSystem3::ComputeCoincidences(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                                              vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    if (samples[0].GetSize() != numBins)
        return;
    if (samples[1].GetSize() != numBins)
        return;
    
    if (mDelayedSamples[0].size() != numBins)
    {
        // Allocate delay result
        for (int k = 0; k < 2; k++)
        {
            vector<WDL_FFT_COMPLEX> samps;
            samps.resize(mNumBands);
            
            for (int i = 0; i < numBins; i++)
            {
                mDelayedSamples[k].push_back(samps);
            }
        }
    }
    
    // Apply delay
    for (int k = 0; k < 2; k++)
    {
        WDL_FFT_COMPLEX *samplesBuf = samples[k].Get();
        WDL_FFT_COMPLEX del;
        
        // Bins
        for (int i = 0; i < mDelayLines[k].size(); i++)
        {
            WDL_FFT_COMPLEX samp = samplesBuf[i];

            // Bands
            for (int j = 0; j < mDelayLines[k][i].size(); j++)
            {
                // A line in frequency
                DelayLinePhaseShift2 *line = mDelayLines[k][i][j];
                
                //WDL_FFT_COMPLEX del = line->ProcessSample(samples[k].Get()[i]);
                COMP_MULT(samp, line->mPhaseShiftComplex, del);
                
                mDelayedSamples[k][i][j] = del;
            }
        }
    }
    
    // Substract
    //
    if (coincidence->size() != mNumBands)
        coincidence->resize(mNumBands);
    
    if (mResultDiff.GetSize() != mDelayedSamples[0].size())
        mResultDiff.Resize(mDelayedSamples[0].size());
    
    for (int j = 0; j < mNumBands; j++)
    {
        ComputeDiff(&mResultDiff, mDelayedSamples[0], mDelayedSamples[1], j);
        
        //
        if ((*coincidence)[j].GetSize() != mResultDiff.GetSize())
        {
            (*coincidence)[j].Resize(mResultDiff.GetSize());
        }
        
        BLUtils::ComplexToMagn(&(*coincidence)[j], mResultDiff);
    }
}

void
SourceLocalisationSystem3::ComputeAmpCoincidences(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                                                  vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
#define FIX_EPS_MAGNS 1
    
    // Init / allocate
    if (coincidence->size() != mNumBands)
        coincidence->resize(mNumBands);
    
    for (int i = 0; i < coincidence->size(); i++)
    {
        if ((*coincidence)[i].GetSize() != samples[0].GetSize())
        {
            (*coincidence)[i].Resize(samples[0].GetSize());
        }
        
        BLUtils::FillAllZero(&(*coincidence)[i]);
    }
    
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    if (samples[0].GetSize() != numBins)
        return;
    if (samples[1].GetSize() != numBins)
        return;
    
    // Compute
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        const WDL_FFT_COMPLEX &sampLeft = samples[0].Get()[i];
        const WDL_FFT_COMPLEX &sampRight = samples[1].Get()[i];
        
        BL_FLOAT l = COMP_MAGN(sampLeft);
        BL_FLOAT r = COMP_MAGN(sampRight);
        
#if FIX_EPS_MAGNS
#define EPS 1e-10
        
        if ((std::fabs(l) < EPS) && (std::fabs(r) < EPS))
            continue;
#endif
        
        BL_FLOAT angle = std::atan2(r, l);
        
        BL_FLOAT panNorm = angle/M_PI;
        panNorm = (panNorm + 0.25);
        panNorm = 1.0 - panNorm;
        
        // With 2, display goes outside of view with extreme pans
#define SCALE_FACTOR 1.8 //2.0
        panNorm = (panNorm - 0.5)*SCALE_FACTOR + 0.5;
        
        int bandIdx = panNorm*(mNumBands - 1);
        
        (*coincidence)[bandIdx].Get()[i] += (l + r)*0.5; // += 1.0
    }
}

void
SourceLocalisationSystem3::ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *resultDiff,
                                       const vector<vector<WDL_FFT_COMPLEX> > &buf0,
                                       const vector<vector<WDL_FFT_COMPLEX> > &buf1,
                                       int bandIndex)
{
    if (buf0.empty())
        return;
    
    if (buf0.size() != buf1.size())
        return;
    
    if (resultDiff->GetSize() != buf0.size())
        resultDiff->Resize(buf0.size());
    
    for (int i = 0; i < buf0.size(); i++)
    {
        WDL_FFT_COMPLEX val0 = buf0[i][bandIndex];
        WDL_FFT_COMPLEX val1 = buf1[i][bandIndex];
            
        WDL_FFT_COMPLEX d;
        
#if 1 // ORIG
        d.re = val0.re - val1.re;
        d.im = val0.im - val1.im;
#endif
        
#if 0 // TEST
        BL_FLOAT magn0 = COMP_MAGN(val0);
        BL_FLOAT magn1 = COMP_MAGN(val1);
        
        BL_FLOAT diff = std::fabs(magn0 - magn1);
        
        d.re = diff;
        d.im = 0.0;
#endif
        
        resultDiff->Get()[i] = d;
    }
}

void
SourceLocalisationSystem3::FindMinima(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
#define INF 1e15;
    
    // Cutoff
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    for (int i = 0; i < numBins; i++)
    {
        int minIndex = 0;
        BL_FLOAT minVal = INF;
        
        for (int j = 0; j < mNumBands; j++)
        {
            BL_FLOAT co = (*coincidence)[j].Get()[i];
            
            if (co < minVal)
            {
                minVal = co;
                minIndex = j;
            }
        }
        
        for (int j = 0; j < mNumBands; j++)
        {
            BL_FLOAT co = (*coincidence)[j].Get()[i];
            if (co <= minVal)
                (*coincidence)[j].Get()[i] = 1.0;
            else
                (*coincidence)[j].Get()[i] = 0.0;
        }
    }
}

// Find only 1 minimum
void
SourceLocalisationSystem3::FindMinima2(WDL_TypedBuf<BL_FLOAT> *coincidence)
{
#define INF 1e15;
    
    BL_FLOAT minVal = INF;
    for (int i = 0; i < coincidence->GetSize(); i++)
    {
        BL_FLOAT co = coincidence->Get()[i];
            
        if (co < minVal)
        {
            minVal = co;
        }
    }
        
    for (int i = 0; i < coincidence->GetSize(); i++)
    {
        BL_FLOAT co = coincidence->Get()[i];
        if (co <= minVal)
            coincidence->Get()[i] = 1.0;
        else
            coincidence->Get()[i] = 0.0;
        
    }
}

// Find several minima
void
SourceLocalisationSystem3::FindMinima3(WDL_TypedBuf<BL_FLOAT> *coincidence)
{
#define THRS 1e10
    
    WDL_TypedBuf<BL_FLOAT> minima;
    BLUtils::FindMinima(*coincidence, &minima, (BL_FLOAT)(THRS*2.0));
    
    for (int i = 0; i < coincidence->GetSize(); i++)
    {
        BL_FLOAT mini = minima.Get()[i];
        
        if (mini < THRS)
            coincidence->Get()[i] = 1.0;
        else
            coincidence->Get()[i] = 0.0;
        
    }
}

void
SourceLocalisationSystem3::NormalizeMinima(WDL_TypedBuf<BL_FLOAT> *coincidence)
{
#define THRS 1e10
    
    WDL_TypedBuf<BL_FLOAT> minima;
    BLUtils::FindMinima(*coincidence, &minima, (BL_FLOAT)(THRS*2.0));
    
    // Find extrema
    BL_FLOAT minVal = INF;
    BL_FLOAT maxVal = -INF;
    for (int i = 0; i < coincidence->GetSize(); i++)
    {
        BL_FLOAT val = coincidence->Get()[i];
        
        if (val < minVal)
            minVal = val;
        if (val > maxVal)
            maxVal = val;
    }
    
    // Normalize (and invert)
    for (int i = 0; i < coincidence->GetSize(); i++)
    {
        BL_FLOAT val = coincidence->Get()[i];
        
        val = 1.0 - (val - minVal)/(maxVal - minVal);
        
        coincidence->Get()[i] = val;
    }
}

void
SourceLocalisationSystem3::FindMinima4(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
    if (coincidence->empty())
        return;
    if ((*coincidence)[0].GetSize() == 0)
        return;
    
    // Bins
    for (int j = 0; j < (*coincidence)[0].GetSize(); j++)
    {
        WDL_TypedBuf<BL_FLOAT> result;
        result.Resize(coincidence->size());
        BLUtils::FillAllValue(&result, (BL_FLOAT)0.0);
        
        BL_FLOAT prevValue = (*coincidence)[0].Get()[j];
        
        // Bands
        for (int i = 0; i < coincidence->size(); i++)
        {
            BL_FLOAT currentValue = (*coincidence)[i].Get()[j];
            
            BL_FLOAT nextValue = 0.0;
            if (i < coincidence->size() - 1)
                nextValue = (*coincidence)[i + 1].Get()[j];
            
            if ((currentValue < prevValue) && (currentValue < nextValue))
            {
                result.Get()[i] = 1.0;
            }
            
            prevValue = currentValue;
        }

        // Copy result
        for (int i = 0; i < coincidence->size(); i++)
        {
            (*coincidence)[i].Get()[j] = result.Get()[i];
        }
    }
}

void
SourceLocalisationSystem3::FindMinima5(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
    if (coincidence->empty())
        return;
    if ((*coincidence)[0].GetSize() == 0)
        return;
    
    // Bins
    for (int j = 0; j < (*coincidence)[0].GetSize(); j++)
    {
        WDL_TypedBuf<BL_FLOAT> result;
        result.Resize(coincidence->size());
        BLUtils::FillAllValue(&result, (BL_FLOAT)0.0);
        
        BL_FLOAT prevValue = (*coincidence)[0].Get()[j];
        
        // Compute band amplitude
        BL_FLOAT bandAmplitude = 0.0;
        for (int i = 0; i < coincidence->size(); i++)
        {
            BL_FLOAT val = (*coincidence)[i].Get()[j];
            bandAmplitude += val;
        }
        //if (!coincidence->empty())
        //    bandAmplitude /= coincidence->size();
        
        // Bands
        for (int i = 0; i < coincidence->size(); i++)
        {
            BL_FLOAT currentValue = (*coincidence)[i].Get()[j];
            
            BL_FLOAT nextValue = 0.0;
            if (i < coincidence->size() - 1)
                nextValue = (*coincidence)[i + 1].Get()[j];
            
            if ((currentValue < prevValue) && (currentValue < nextValue))
            {
                result.Get()[i] = bandAmplitude; //1.0;
            }
            
            prevValue = currentValue;
        }
        
        // Copy result
        for (int i = 0; i < coincidence->size(); i++)
        {
            (*coincidence)[i].Get()[j] = result.Get()[i];
        }
    }
}

void
SourceLocalisationSystem3::TimeIntegrate(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence)
{    
    if (mPrevCoincidence.size() != ioCoincidence->size())
        mPrevCoincidence = *ioCoincidence;
    
    BLUtils::Smooth(ioCoincidence, &mPrevCoincidence, mTimeSmooth/*TIME_INTEGRATE_SMOOTH_FACTOR*/);
}

void
SourceLocalisationSystem3::Threshold(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence)
{
    // Threshold is set greater than or equal to zero.
    // A greater value of threshold can remove phantom coincidences
#define THRESHOLD 1.0 //0.0
    
    for (int i = 0; i < ioCoincidence->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &coincidence = (*ioCoincidence)[i];
        
        for (int j = 0; j < coincidence.GetSize(); j++)
        {
            BL_FLOAT val = coincidence.Get()[j];
            if (val < THRESHOLD)
                val = 0.0;
            
            coincidence.Get()[j] = val;
        }
    }
}

void
SourceLocalisationSystem3::FreqIntegrate(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                        WDL_TypedBuf<BL_FLOAT> *localization)
{
    localization->Resize(coincidence.size());
    
    // Bands
    for (int i = 0; i < coincidence.size(); i++)
    {
        const WDL_TypedBuf<BL_FLOAT> &freqs = coincidence[i];
        
        BL_FLOAT sumFreqs = BLUtils::ComputeSum(freqs);
        
        localization->Get()[i] = sumFreqs;
    }
}

void
SourceLocalisationSystem3::FreqIntegrateStencil(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                                WDL_TypedBuf<BL_FLOAT> *localization)
{
#define PENALITY 0.0
//#define PENALITY -0.1 // Better
//#define PENALITY -0.2 // Far better
#define EPS 1e-15
    
    localization->Resize(coincidence.size());
    
    // Bands
    for (int i = 0; i < coincidence.size(); i++)
    {
        const vector<WDL_TypedBuf<BL_FLOAT> > &mask = mStencilMasks[i];
        
#if 1 //0 // DEBUG
        if (i == coincidence.size()/2)
        {
            //DBG_DumpCoincidence("coincidence", coincidence, 10000.0);
            //DBG_DumpCoincidence("mask", mask, 1.0);
        }
#endif
        
        // Num points used in the summation for each azimuth
        BL_FLOAT sumWeights = 0.0;
        BL_FLOAT sum = 0.0;
        
        // Bands
        for (int j = 0; j < coincidence.size(); j++)
        {
            // Bins
            for (int k = 0; k < coincidence[j].GetSize(); k++)
            {
                BL_FLOAT maskVal = mask[j].Get()[k];
                
                if (maskVal > 0.0)
                {
                    BL_FLOAT val = coincidence[j].Get()[k];
                    val *= maskVal;
                    
                    sum += val;
                    
                    if (val < EPS)
                        sum += PENALITY;
                    
                    //numPoints++;
                    sumWeights += maskVal;
                }
            }
        }
        
        if (sumWeights > 0.0)
            sum /= sumWeights;
        
        //
        if (sum < 0.0)
            sum = 0.0;
        
        // Re-normalized, to be consistent with the version
        // of FreqIntegrate() without stencil
        sum *= coincidence.size();
        
        // Hack
        sum *= 6.0;
        
        localization->Get()[i] = sum;
    }
}

void
SourceLocalisationSystem3::FreqIntegrateStencilCalmes(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                                      WDL_TypedBuf<BL_FLOAT> *localization)
{
#define INF 1e15
    
    localization->Resize(coincidence.size());
 
    BLUtils::FillAllZero(localization);
    
    if (coincidence.empty())
        return;
    
#if DEBUG_CALMES
    vector<WDL_TypedBuf<BL_FLOAT> > coincidenceCopy = coincidence;
    for (int i = 0; i < coincidenceCopy.size(); i++)
    {
        BLUtils::FillAllZero(&coincidenceCopy[i]);
    }
#endif
    
    // Bins
    for (int i = 0; i < coincidence[0].GetSize(); i++)
    {
        // Find the best azimuth index for a given frequency
        int bestBandIdx = 0;
        BL_FLOAT bestBandVal = INF;
        
        // Bands
        for (int j = 0; j < coincidence.size(); j++)
        {
            const vector<WDL_TypedBuf<BL_FLOAT> > &mask = mStencilMasks[j];
            
            // Compute the value
            BL_FLOAT sum = 0.0;
            BL_FLOAT sumWeights = 0.0;
            
#if 0 //DEBUG_CALMES
            WDL_TypedBuf<BL_FLOAT> maskLine;
            maskLine.Resize(coincidence.size());
            for (int k = 0; k < maskLine.GetSize(); k++)
            {
                maskLine.Get()[k] = mask[k].Get()[i];
            }
            
            BLDebug::DumpData("mask.txt", maskLine);
#endif
            
            // Bands
            for (int k = 0; k < coincidence.size(); k++)
            {
                BL_FLOAT maskVal = mask[k].Get()[i];
                if (maskVal > 0.0)
                {
                    BL_FLOAT val = coincidence[k].Get()[i];
                    val *= maskVal;
                    
                    sum += val;
                    
                    sumWeights += maskVal;
                }
            }
            
            if (sumWeights > 0.0)
                sum /= sumWeights;
            
            // Update the minimum
            if (sum < bestBandVal)
            {
                bestBandVal = sum;
                bestBandIdx = j;
            }
            
#if 0 //DEBUG_CALMES
            // Strange, only blurs a little the raw coincidence
            coincidenceCopy[j].Get()[i] = sum;
#endif
        }

#if DEBUG_CALMES
        coincidenceCopy[bestBandIdx].Get()[i] = 1.0;
#endif
        
        //localization->Get()[bestBandIdx] += 1.0/30.0;
        localization->Get()[bestBandIdx] += bestBandVal*62.0;
    }
    
#if DEBUG_CALMES
    ((vector<WDL_TypedBuf<BL_FLOAT> > &)coincidence) = coincidenceCopy;
#endif

}

void
SourceLocalisationSystem3::FreqIntegrateStencilCalmes2(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                                       WDL_TypedBuf<BL_FLOAT> *localization)
{    
    localization->Resize(coincidence.size());
    BLUtils::FillAllZero(localization);
    
    if (coincidence.empty())
        return;
    
    // Compute amplitudes
    WDL_TypedBuf<BL_FLOAT> amplitudes;
    amplitudes.Resize(coincidence[0].GetSize());
    BLUtils::FillAllZero(&amplitudes);
    
    // Bins
    for (int i = 0; i < coincidence[0].GetSize(); i++)
    {
        // Bands
        for (int j = 0; j < coincidence.size(); j++)
        {
            BL_FLOAT val = coincidence[j].Get()[i];
            amplitudes.Get()[i] += val;
        }
    }
    BL_FLOAT sumAmplitudes = BLUtils::ComputeSum(amplitudes);
    
    // Compute coeffs
    WDL_TypedBuf<BL_FLOAT> ampCoeffs = amplitudes;
    if (sumAmplitudes > 0.0)
    {
        BLUtils::MultValues(&ampCoeffs, (BL_FLOAT)(1.0/sumAmplitudes));
    }
    
    //
    //
    
    // Bins
    for (int i = 0; i < coincidence[0].GetSize(); i++)
    {
        // Keep score for each mask
        WDL_TypedBuf<BL_FLOAT> maskScores;
        maskScores.Resize(coincidence.size());
        
        // Compute masks scores
        
        // Bands
        for (int j = 0; j < coincidence.size(); j++)
        {
            const vector<WDL_TypedBuf<BL_FLOAT> > &mask = mStencilMasks[j];
            
            // Compute the value
            BL_FLOAT sum = 0.0;
            BL_FLOAT sumWeights = 0.0;
            
            // Bands
            for (int k = 0; k < coincidence.size(); k++)
            {
                BL_FLOAT maskVal = mask[k].Get()[i];
                if (maskVal > 0.0)
                {
                    BL_FLOAT val = coincidence[k].Get()[i];
                    val *= maskVal;
                    
                    sum += val;
                    
                    sumWeights += maskVal;
                }
            }
            
            if (sumWeights > 0.0)
                sum /= sumWeights;
            
            maskScores.Get()[j] = sum;
        }
        
        // Sum scores
        BL_FLOAT sumMaskScores = BLUtils::ComputeSum(maskScores);
        
        if (sumMaskScores <= 0.0)
            continue;
        
        // Compute the weight of each localisation band
        //
        
        // Bands
        for (int j = 0; j < maskScores.GetSize(); j++)
        {
            BL_FLOAT score = maskScores.Get()[j];
            score /= sumMaskScores;
            
            score = 1.0 - score;
            
#if 1
            // Ponderate with amps
            score *= ampCoeffs.Get()[i]; // NEW
#endif
            
            localization->Get()[j] += score;
        }
    }
    
    // Adjust the floor of the curve (the floor is around 272...)
    BL_FLOAT locFloor = BLUtils::ComputeMin(*localization);
    BLUtils::AddValues(localization, -locFloor);
}

// Very very costly. Unusable like that.
void
SourceLocalisationSystem3::FreqIntegrateStencil2(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                                 WDL_TypedBuf<BL_FLOAT> *localization)
{
    localization->Resize(coincidence.size());
    
    // Bands
    for (int i = 0; i < coincidence.size(); i++)
    {
        const vector<WDL_TypedBuf<BL_FLOAT> > &mask = mStencilMasks[i];
                
        vector<WDL_TypedBuf<BL_FLOAT> > corr;
        BLUtils::CrossCorrelation2D(coincidence, mask, &corr);
        
        BL_FLOAT corrMaxVal = BLUtils::FindMaxValue(corr);
        
        localization->Get()[i] = corrMaxVal;
        
#if 1 //0 // DEBUG
        if (i == coincidence.size()/2)
        {
            //DBG_DumpCoincidence("coincidence", coincidence, 10000.0);
            //DBG_DumpCoincidence("mask", mask, 1.0);
            //DBG_DumpCoincidence("correlation", corr, 10000.0);
        }
#endif
    }
}

// Use two steps method described in https://pdfs.semanticscholar.org/665b/802b0236201adff4df707a26080cf808873e.pdf
// "A SIMPLE BINARY IMAGE SIMILARITY MATCHING METHOD BASED ON EXACT PIXEL MATCHING" - Mikiyas Teshome.
void
SourceLocalisationSystem3::FreqIntegrateStencil3(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                                 WDL_TypedBuf<BL_FLOAT> *localization)
{
    localization->Resize(coincidence.size());
    
    // Bands
    for (int i = 0; i < coincidence.size(); i++)
    {
        const vector<WDL_TypedBuf<BL_FLOAT> > &mask = mStencilMasks[i];
        
        BL_FLOAT matchScore = BLUtils::BinaryImageMatch(coincidence, mask);
        
        localization->Get()[i] = matchScore;
        
#if 1 //0 // DEBUG
        if (i == coincidence.size()/2)
        {
            //DBG_DumpCoincidence("coincidence", coincidence, 10000.0);
            //DBG_DumpCoincidence("mask", mask, 1.0);
        }
#endif
    }
}

void
SourceLocalisationSystem3::InvertValues(WDL_TypedBuf<BL_FLOAT> *localization)
{
    BL_FLOAT maxVal = BLUtils::ComputeMax(*localization);
    
    for (int i = 0; i < localization->GetSize(); i++)
    {
        BL_FLOAT val = localization->Get()[i];
        
        val = maxVal - val;
        if (val < 0.0)
            val = 0.0;
        
        localization->Get()[i] = val;
    }
}
    
void
SourceLocalisationSystem3::DeleteDelayLines()
{
    for (int k = 0; k < 2; k++)
    {
        for (int i = 0; i < mDelayLines[k].size(); i++)
        {
            vector<DelayLinePhaseShift2 *> &bandLines = mDelayLines[k][i];
            
            for (int j = 0; j < bandLines.size(); j++)
            {
                DelayLinePhaseShift2 *line = bandLines[j];
                delete line;
            }
        }
    }
}

void
SourceLocalisationSystem3::GenerateMask(vector<WDL_TypedBuf<BL_FLOAT> > *mask,
                                        int numBins, int numBands, int bandIndex)
{
    // Allocate
    mask->resize(numBands);
    WDL_TypedBuf<BL_FLOAT> zeros;
    BLUtils::ResizeFillZeros(&zeros, numBins);
    for (int i = 0; i < numBands; i++)
    {
        (*mask)[i] = zeros;
    }
    
    //
    BL_FLOAT ITDmax = 0.5*(INTER_MIC_DISTANCE*mIntermicDistanceCoeff)/SOUND_SPEED;
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    // STENCIL_HACK: this way, we have exactly the good number of lines
    // compared to the coincidence mask
    ITDmax *= 2.0;
    
    //
    BL_FLOAT azimuth = (((BL_FLOAT)bandIndex)/(mask->size() - 1))*M_PI - M_PI/2.0;
    
    // Bins
    for (int j = 1/*0*/; j < numBins; j++)
    {
        BL_FLOAT fm = j*hzPerBin;
            
        BL_FLOAT gammaMinf = -ITDmax*fm*(1.0 + std::sin(azimuth));
        BL_FLOAT gammaMaxf = ITDmax*fm*(1.0 - std::sin(azimuth));
            
        int gammaMin = std::ceil(gammaMinf);
        int gammaMax = std::floor(gammaMaxf);
            
        for (int k = gammaMin; k <= gammaMax; k++)
        {
	  BL_FLOAT x0 = std::sin(azimuth) + ((BL_FLOAT)k)/(ITDmax*fm);
                
	  if (std::fabs(x0) > 1.0)
                continue;
                
	  BL_FLOAT xf = std::asin(x0);
                
            //
            xf = (xf/M_PI) + 0.5;
                
            //
            xf = xf*(mask->size() - 1);
                
            int x = bl_round(xf);
                    
            (*mask)[x].Get()[j] = 1.0;
        }
    }
    
    if (bandIndex == numBands/2)
    {
        //DBG_DumpCoincidence("mask0", *mask, 1.0);
    }
    
    //
//#if !USE_STENCIL_METHOD3
    MaskDilation(mask);
    
    //MaskMinMax(mask); // => makes larger bells
//#endif
    
    if (bandIndex == numBands/2)
    {
        //DBG_DumpCoincidence("mask1", *mask, 1.0);
    }
}

void
SourceLocalisationSystem3::MaskDilation(vector<WDL_TypedBuf<BL_FLOAT> > *mask)
{
#define MASK_DILATION_VALUE 0.5 //0.0 //1.0
#define EPS 1e-15
    
    if (mask->empty())
        return;
    if ((*mask)[0].GetSize() == 0)
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> > maskCopy = *mask;
    
    // Bands
    for (int i = 0; i < mask->size(); i++)
    {
        // Bins
        for (int j = 0; j < (*mask)[i].GetSize(); j++)
        {
            BL_FLOAT val = maskCopy[i].Get()[j];
            
            if (std::fabs(val) < EPS)
            {
                BL_FLOAT prevVal = 0.0;
                if (i - 1 >= 0)
                {
                    prevVal = maskCopy[i - 1].Get()[j];
                }
                
                BL_FLOAT nextVal = 0.0;
                if (i + 1 <= maskCopy.size() - 1)
                {
                    nextVal = maskCopy[i + 1].Get()[j];
                }
                
                if ((prevVal > 0.0) || (nextVal > 0.0))
                    (*mask)[i].Get()[j] = MASK_DILATION_VALUE;
            }
        }
    }
}

void
SourceLocalisationSystem3::MaskMinMax(vector<WDL_TypedBuf<BL_FLOAT> > *mask)
{
    //const BL_FLOAT undefineValue = -1e15;
    const BL_FLOAT undefineValue = -1.0;
    
    if (mask->empty())
        return;

    // Init
    vector<WDL_TypedBuf<BL_FLOAT> > minMaxMap = *mask;
    for (int i = 0; i < minMaxMap.size(); i++)
    {
        BLUtils::FillAllValue(&minMaxMap[i], undefineValue);
    }
    
    // Iterate over lines
    for (int i = 0; i < (*mask)[0].GetSize(); i++)
    {
        // Generate the source line
        WDL_TypedBuf<BL_FLOAT> line;
        line.Resize(mask->size());
        for (int j = 0; j < mask->size(); j++)
        {
            line.Get()[j] = (*mask)[j].Get()[i];
        }
        
        WDL_TypedBuf<BL_FLOAT> newLine;
        newLine.Resize(line.GetSize());
        BLUtils::FillAllValue(&newLine, undefineValue);
        
        // Put the minima
        for (int j = 0; j < line.GetSize(); j++)
        {
            BL_FLOAT val = line.Get()[j];
            if (val > 0.0)
                newLine.Get()[j] = 1.0;
        }
        
        // Put the maxima
        //
        
        // Extremities
        if (newLine.Get()[0] < 0.0)
            newLine.Get()[0] = 0.0;
        
        if (newLine.Get()[newLine.GetSize() - 1] < 0.0)
            newLine.Get()[newLine.GetSize() - 1] = 0.0;
        
        // Iterate and put the max values in the middle of min values
        int currentIndex = 0;
        while(currentIndex < newLine.GetSize())
        {
            int startIndex = currentIndex;
            for (int k = startIndex; k < newLine.GetSize(); k++)
            {
                if (newLine.Get()[k] > 0.0)
                {
                    startIndex = k;
                    break;
                }
            }
            
            int endIndex = startIndex + 1;
            for (int k = endIndex; k < newLine.GetSize(); k++)
            {
                if (newLine.Get()[k] > 0.0)
                {
                    endIndex = k;
                    break;
                }
            }
            
            // Middle
            if (endIndex - startIndex > 1)
            {
                int maxIndex = (startIndex + endIndex)/2.0;
                newLine.Get()[maxIndex] = 0.0;
            }
            
            currentIndex = endIndex; // + 1;
        }
        
        // Gradients
        bool extendBounds = true;
        BLUtils::FillMissingValues3(&newLine, extendBounds, undefineValue);
        
#if 0 // Square! => Makes sharper mask, but not sharper bell
        BLUtils::ApplyPow(&newLine, 2.0);
#endif
        
        // Copy the result line
        for (int j = 0; j < newLine.GetSize(); j++)
        {
            minMaxMap[j].Get()[i] = newLine.Get()[j];
        }
    }
    
    // Finish
    *mask = minMaxMap;
}

void
SourceLocalisationSystem3::GenerateStencilMasks()
{
    int numBins = (CUTOFF_FREQ/mSampleRate)*mBufferSize;
    
    mStencilMasks.clear();
    mStencilMasks.resize(mNumBands);
    
    for (int i = 0; i < mNumBands; i++)
    {
        GenerateMask(&mStencilMasks[i], numBins, mNumBands, i);
        
#if DEBUG_STENCIL_MASK
        if (i == mNumBands/2) // Middle
        //if (i == mNumBands/4) // Left
        {
            WDL_TypedBuf<BL_FLOAT> dumpMask;
            dumpMask.Resize(mStencilMasks[i].size()*mStencilMasks[i][0].GetSize());
            
            // Bands
            for (int j = 0; j < mStencilMasks[i].size(); j++)
            {
                // Bins
                for (int k = 0; k < mStencilMasks[i][0].GetSize(); k++)
                {
                    dumpMask.Get()[j + (mStencilMasks[i][0].GetSize() - k - 1)*mStencilMasks[i].size()] =
                                mStencilMasks[i][j].Get()[k];
                }
            }
            
            int width = mStencilMasks[i].size();
            int height = mStencilMasks[i][0].GetSize();
            BL_FLOAT colorCoeff = 1.0; //10000.0;
            
            LICE_IBitmap *bmp = new LICE_MemBitmap(width, height, 1);
            LICE_pixel *pixels = bmp->getBits();
            for (int i = 0; i < width*height; i++)
            {
                BL_FLOAT val = dumpMask.Get()[i];
                int pix = val*255*colorCoeff;
                if (pix > 255)
                    pix = 255;
                
                pixels[i] = LICE_RGBA(pix, pix, pix, 255);
            }
            
            char fileNamePNG[256];
            sprintf(fileNamePNG, "mask-%d.png", i);
            char fullFilename[MAX_PATH];
            sprintf(fullFilename, "/Users/applematuer/Documents/BlueLabAudio-Debug/%s", fileNamePNG);
            
            // #bl-iplug2
#if 0
            LICE_WritePNG(fullFilename, bmp, true);
#endif
            delete bmp;
        }
#endif
    }
}

void
SourceLocalisationSystem3::BuildMinMaxMap(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence)
{
    BLUtils::BuildMinMaxMapHoriz(coincidence);
}

void
SourceLocalisationSystem3::LowPassFilter(WDL_TypedBuf<WDL_FFT_COMPLEX> *samples)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    int startBin = LOW_PASS_FREQ/hzPerBin;
    
    if (startBin == 0)
        return;
    
    for (int i = 0; i < startBin; i++)
    {
        WDL_FFT_COMPLEX &comp = samples->Get()[i];
        comp.re = 0.0;
        comp.im = 0.0;
    }
}

void
SourceLocalisationSystem3::DBG_DumpCoincidence(const char *fileName,
                                              const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                              BL_FLOAT colorCoeff)
{
    int width = coincidence.size();
    if (width == 0)
        return;
    
    int height = coincidence[0].GetSize();
    
#define SUMMATION_BAND 0 //1
#define SUMMATION_BAND_SIZE 20
    
#if SUMMATION_BAND
    WDL_TypedBuf<BL_FLOAT> summationBand;
    BLUtils::ResizeFillZeros(&summationBand, width);
    
    height = height + SUMMATION_BAND_SIZE;
#endif
    
    BL_FLOAT *image = new BL_FLOAT[width*height];
    for (int j = 0; j < height; j++)
    {
#if SUMMATION_BAND
        if (j >= coincidence[0].GetSize())
            continue;
#endif
        for (int i = 0; i < width; i++)
        {
            BL_FLOAT val = coincidence[i].Get()[j];
        
            image[i + (height - j - 1)*width] = val;
            
#if SUMMATION_BAND
            summationBand.Get()[i] += val;
#endif
        }
    }
    
#if SUMMATION_BAND
    BLUtils::MultValues(&summationBand, 0.005);
    
    for (int j = height - SUMMATION_BAND_SIZE; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            BL_FLOAT val = summationBand.Get()[i];
            
            image[i + (height - j - 1)*width] = val;
        }
    }
#endif
    
    // png
    // #bl-iplug2
    //LICE_IBitmap *bmp = new LICE_MemBitmap(width, height, 1);
    LICE_IBitmap *bmp = NULL;
    
    LICE_pixel *pixels = bmp->getBits();
    for (int i = 0; i < width*height; i++)
    {
        BL_FLOAT val = image[i];
        int pix = val*255*colorCoeff;
        if (pix > 255)
            pix = 255;
        
        pixels[i] = LICE_RGBA(pix, pix, pix, 255);
    }
    
    char fileNamePNG[256];
    sprintf(fileNamePNG, "%s.png", fileName);
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Users/applematuer/Documents/BlueLabAudio-Debug/%s", fileNamePNG);
    
    // #bl-iplug2
#if 0
    LICE_WritePNG(fullFilename, bmp, true);
#endif
    
    delete bmp;
}

void
SourceLocalisationSystem3::DBG_DumpCoincidenceLine(const char *fileName,
                                                  int index,
                                                  const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence)
{
    int height = coincidence.size();
    if (height == 0)
        return;
    
    WDL_TypedBuf<BL_FLOAT> data;
    data.Resize(height);
    for (int i = 0; i < height; i++)
    {
        data.Get()[i] = coincidence[i].Get()[index];
    }
    
    BLDebug::DumpData(fileName, data);
}

void
SourceLocalisationSystem3::DBG_DumpFftSamples(const char *fileName,
                                              const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples)
{
    WDL_TypedBuf<BL_FLOAT> samples;
    FftProcessObj16::HalfFftToSamples(fftSamples, &samples);

    BLDebug::DumpData(fileName, samples);
}

void
SourceLocalisationSystem3::DBG_DumpFftSamples(const char *fileName,
                                              const vector<WDL_FFT_COMPLEX> &fftSamples)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> samples;
    samples.Resize(fftSamples.size());
    
    for (int i = 0; i < fftSamples.size(); i++)
    {
        WDL_FFT_COMPLEX samp = fftSamples[i];
    
        samples.Get()[i] = samp;
    }
    
    DBG_DumpFftSamples(fileName, samples);
}

void
SourceLocalisationSystem3::DBG_DumpCoincidenceSum(const char *fileName,
                                                  vector<WDL_TypedBuf<BL_FLOAT> > &coincidence)
{
    WDL_TypedBuf<BL_FLOAT> sum;
    sum.Resize(coincidence.size());
    
    // Bands
    for (int i = 0; i < coincidence.size(); i++)
    {
        BL_FLOAT s = BLUtils::ComputeSum(coincidence[i]);
                                     
        sum.Get()[i] = s;
    }
    
    BLDebug::DumpData(fileName, sum);
}

void
SourceLocalisationSystem3::DBG_DumpCoincidenceSum2(const char *fileName,
                                                   vector<WDL_TypedBuf<BL_FLOAT> > &coincidence)
{
    if (coincidence.empty())
        return;

    // Invert
    vector<WDL_TypedBuf<BL_FLOAT> > coInv;
    coInv.resize(coincidence[0].GetSize());
    for (int i = 0; i < coInv.size(); i++)
        coInv[i].Resize(coincidence.size());
    
    // Bands
    for (int i = 0; i < coincidence.size(); i++)
    {
        // Bins
        for (int j = 0; j < coincidence[i].GetSize(); j++)
        {
            BL_FLOAT val = coincidence[i].Get()[j];
            
            coInv[j].Get()[i] = val;
        }
    }
    
    // Find minima
    for (int i = 0; i < coInv.size(); i++)
    {
        FindMinima3(&coInv[i]);
    }
    
    // Sum
    WDL_TypedBuf<BL_FLOAT> sum;
    sum.Resize(coincidence.size());
    BLUtils::FillAllZero(&sum);
    
    // Bins
    for (int i = 0; i < coInv.size(); i++)
    {
        // Bands
        for (int j = 0; j < coInv[i].GetSize(); j++)
        {
            BL_FLOAT val = coInv[i].Get()[j];
        
            sum.Get()[j] += val;
        }
    }
    
    BLDebug::DumpData(fileName, sum);
}

void
SourceLocalisationSystem3::DBG_DisplayLocalizationcurve(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence,
                                                        const  WDL_TypedBuf<BL_FLOAT> &localization)
{
#define COEFF 0.25
    
    if (localization.GetSize() == 0)
        return;
    
    WDL_TypedBuf<BL_FLOAT> localizationNorm = localization;
    BLUtils::Normalize(&localizationNorm);
    
    // Bands
    for (int i = 0; i < coincidence->size(); i++)
    {
        // Bins
        BL_FLOAT loca = localizationNorm.Get()[i];
        int y = loca*COEFF*(*coincidence)[i].GetSize();
        
        BL_FLOAT val = (*coincidence)[i].Get()[y];
        
        // Invert overlay
        val = 1.0 - val;
        
        (*coincidence)[i].Get()[y] = val;
        
#if 0 // Test...
        // Fake invert overlay
        (*coincidence)[i].Get()[y] = 1.0;
      
        if (y + 1 < (*coincidence)[i].GetSize() - 1)
        {
            (*coincidence)[i].Get()[y + 1] = 0.0;
        }
#endif
    }
}
