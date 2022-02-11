//
//  SMVProcessXComputerLissajousEXP.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <BLUtils.h>

#include <Axis3DFactory2.h>

#include "SMVProcessXComputerLissajousEXP.h"

#define SQR2_INV 0.70710678118655

// Lissajous
//#define LISSAJOUS_CLIP_DISTANCE 1.0
// Set to a bit less than 1
// (otherwise some sound will be clipped when playing)
#define LISSAJOUS_CLIP_DISTANCE 0.95

// Totally psychedelic effects...
#define TOTAL_PSYCHE_EXPE 0

// Scale the sound instead of the result coordinates
// => so we stay in bounds (and we won't loose samples that would be too far)
#define PRE_SCALE 1

SMVProcessXComputerLissajousEXP::SMVProcessXComputerLissajousEXP(Axis3DFactory2 *axisFactory,
                                                                 BL_FLOAT sampleRate)
{
    mAxisFactory = axisFactory;
    
    mSampleRate = sampleRate;
}

SMVProcessXComputerLissajousEXP::~SMVProcessXComputerLissajousEXP() {}

void
SMVProcessXComputerLissajousEXP::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

void
SMVProcessXComputerLissajousEXP::ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                          const WDL_TypedBuf<BL_FLOAT> magns[2],
                                          const WDL_TypedBuf<BL_FLOAT> phases[2],
                                          WDL_TypedBuf<BL_FLOAT> *resultX,
                                          WDL_TypedBuf<BL_FLOAT> *resultY,
                                          bool *isPolar, BL_FLOAT polarCenter[2],
                                          bool *isScalable)
{
    // Init
    if (isPolar != NULL)
        *isPolar = true;
    
    if (polarCenter != NULL)
    {
        polarCenter[0] = 0.0;
        polarCenter[1] = 0.5; //0.0; //0.5;
    }
    
#if TOTAL_PSYCHE_EXPE
    if (isScalable != NULL)
        *isScalable = true;
#else
    if (isScalable != NULL)
        *isScalable = false;
#endif
    
    resultX->Resize(magns[0].GetSize());
    if (resultY != NULL)
        resultY->Resize(magns[0].GetSize());
    
    // Compute
    //
    // Standard should be applied on samples, here it is applied on fft samples
    
    WDL_TypedBuf<BL_FLOAT> lissajousSamples[2];
    //ComputeLissajous(samples, lissajousSamples, true); // Here just for debug

    // Lissajous on Fft!
    ComputeLissajousFft(magns, phases, lissajousSamples, true);
    
    // Warning: samples size is twice the magns size
    // So take only half of the samples
    // (otherwise we would have no display after)
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT x = lissajousSamples[0].Get()[i];
        BL_FLOAT y = lissajousSamples[1].Get()[i];
        
        resultX->Get()[i] = x;
        
        if (resultY != NULL)
            resultY->Get()[i] = y;
    }
    
    // We need less fat in x
    BLUtils::MultValues(resultX, (BL_FLOAT)0.5);
    
    // We need positive y
    BLUtils::AddValues(resultY, (BL_FLOAT)1.0);
    BLUtils::MultValues(resultY, (BL_FLOAT)0.5);
    
#if !TOTAL_PSYCHE_EXPE && !PRE_SCALE
    BLUtils::MultValues(resultX, (BL_FLOAT)2.0);
    
    BLUtils::MultValues(resultY, (BL_FLOAT)2.0);
    BLUtils::AddValues(resultY, (BL_FLOAT)-0.5);
    
#if 1 // NEW: scale better !
    BLUtils::MultValues(resultX, (BL_FLOAT)16.0);
    
    BLUtils::AddValues(resultY, (BL_FLOAT)-0.5);
    BLUtils::MultValues(resultY, (BL_FLOAT)16.0);
    BLUtils::AddValues(resultY, (BL_FLOAT)0.5);
#endif
    
#endif
    
    //BLDebug::DumpData("expx.txt", *resultX);
    //BLDebug::DumpData("expy.txt", *resultY);
    
#if 0 //1 // Was just a TEST...
    // Normalize by magns
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT x = resultX->Get()[i];
        BL_FLOAT y = resultY->Get()[i];
     
        BL_FLOAT x0 = x - polarCenter[0];
        BL_FLOAT y0 = y - polarCenter[1];
     
        BL_FLOAT magn = magns[0].Get()[i];
        if (magn > 0.0)
        {
            x0 /= magn;
            y0 /= magn;
        }
        
        x = x0 + polarCenter[0];
        y = y0 + polarCenter[1];
        
        resultX->Get()[i] = x;
        
        if (resultY != NULL)
            resultY->Get()[i] = y;
    }
#endif
}

Axis3D *
SMVProcessXComputerLissajousEXP::CreateAxis()
{
    Axis3D *axis = mAxisFactory->CreateLeftRightAxis(Axis3DFactory2::ORIENTATION_X);
    
    return axis;
}

// From USTProcess
void
SMVProcessXComputerLissajousEXP::ComputeLissajous(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                                  WDL_TypedBuf<BL_FLOAT> lissajousSamples[2],
                                                  bool fitInSquare)
{
    lissajousSamples[0].Resize(samples[0].GetSize());
    lissajousSamples[1].Resize(samples[0].GetSize());
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        BL_FLOAT l = samples[0].Get()[i];
        BL_FLOAT r = samples[1].Get()[i];
        
        if (fitInSquare)
        {
            if (l < -LISSAJOUS_CLIP_DISTANCE)
                l = -LISSAJOUS_CLIP_DISTANCE;
            if (l > LISSAJOUS_CLIP_DISTANCE)
                l = LISSAJOUS_CLIP_DISTANCE;
            
            if (r < -LISSAJOUS_CLIP_DISTANCE)
                r = -LISSAJOUS_CLIP_DISTANCE;
            if (r > LISSAJOUS_CLIP_DISTANCE)
                r = LISSAJOUS_CLIP_DISTANCE;
        }
        
        // Rotate
        BL_FLOAT angle = std::atan2(r, l);
        BL_FLOAT dist = std::sqrt(l*l + r*r);
        
        if (fitInSquare)
        {
            BL_FLOAT coeff = SQR2_INV;
            dist *= coeff;
        }
        
        angle = -angle;
        angle -= M_PI/4.0;
        
        BL_FLOAT x = dist*cos(angle);
        BL_FLOAT y = dist*sin(angle);
        
#if 0 // Keep [-1, 1] bounds
        // Center
        x = (x + 1.0)*0.5;
        y = (y + 1.0)*0.5;
#endif
        
        lissajousSamples[0].Get()[i] = x;
        lissajousSamples[1].Get()[i] = y;
    }
}

void
SMVProcessXComputerLissajousEXP::ComputeLissajousFft(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                                     const WDL_TypedBuf<BL_FLOAT> phases[2],
                                                     WDL_TypedBuf<BL_FLOAT> lissajousSamples[2],
                                                     bool fitInSquare)
{
    lissajousSamples[0].Resize(magns[0].GetSize());
    lissajousSamples[1].Resize(magns[0].GetSize());
    
    // NOTE: after expes with Scilab, this method is closely related to ifft
    // If we plot (ifft(x), ifft(y)), we have something close to the simple version
    // of Lissajous with samples.
    
#if 1 //0 // Compute samples only at time 0
    // NOTE: Fails for a pure sine wave: return only one peak
    int tBinNum = 0;
    
    //int offset = magns[0].GetSize()/2;
    //tBinNum += offset;
    
    WDL_TypedBuf<BL_FLOAT> leftSamples;
    BLUtils::ComputeSamplesByFrequency(&leftSamples, mSampleRate, magns[0], phases[0], tBinNum);
    
    WDL_TypedBuf<BL_FLOAT> rightSamples;
    BLUtils::ComputeSamplesByFrequency(&rightSamples, mSampleRate, magns[1], phases[1], tBinNum);
    
    // GOOD: seems better with this coeff !
#if !PRE_SCALE
#define TIME0_COEFF 4.0
#else
#define TIME0_COEFF 128.0 //64.0
#endif
    BLUtils::MultValues(&leftSamples, (BL_FLOAT)TIME0_COEFF);
    BLUtils::MultValues(&rightSamples, (BL_FLOAT)TIME0_COEFF);
#endif

    // Simply recompute the samples, to check that the algorithm is working
    // => GOOD ! (Keep it for debug !)
#if 0 //1
    WDL_TypedBuf<BL_FLOAT> leftSamples;
    leftSamples.Resize(magns[0].GetSize());
    BLUtils::FillAllZero(&leftSamples);
    
    WDL_TypedBuf<BL_FLOAT> rightSamples;
    rightSamples.Resize(magns[0].GetSize());
    BLUtils::FillAllZero(&rightSamples);
    
    // Add an offset, so the previously windowed signal is centered
    // (maximum in the center)
    int offset = magns[0].GetSize()/2;
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> leftSamplesFreq;
        BLUtils::ComputeSamplesByFrequency(&leftSamplesFreq, mSampleRate,
                                         magns[0], phases[0],
                                         i + offset);
        
        WDL_TypedBuf<BL_FLOAT> rightSamplesFreq;
        BLUtils::ComputeSamplesByFrequency(&rightSamplesFreq, mSampleRate,
                                         magns[1], phases[1],
                                         i + offset);
        
        // Find the maxima
        for (int j = 0; j < leftSamplesFreq.GetSize(); j++)
        {
            BL_FLOAT lf = leftSamplesFreq.Get()[j];
            BL_FLOAT rf = rightSamplesFreq.Get()[j];
            
            leftSamples.Get()[i] += lf;
            rightSamples.Get()[i] += rf;
        }
    }
    
    // Must multiply by 2 !
    // (Is it because we only take half of the fft ?)
    BLUtils::MultValues(&leftSamples, 2.0);
    BLUtils::MultValues(&rightSamples, 2.0);
#endif
    
   
    // Compute lissajous on new computed samples
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT l = leftSamples.Get()[i];
        BL_FLOAT r = rightSamples.Get()[i];
        
        if (fitInSquare)
        {
            if (l < -LISSAJOUS_CLIP_DISTANCE)
                l = -LISSAJOUS_CLIP_DISTANCE;
            if (l > LISSAJOUS_CLIP_DISTANCE)
                l = LISSAJOUS_CLIP_DISTANCE;
            
            if (r < -LISSAJOUS_CLIP_DISTANCE)
                r = -LISSAJOUS_CLIP_DISTANCE;
            if (r > LISSAJOUS_CLIP_DISTANCE)
                r = LISSAJOUS_CLIP_DISTANCE;
        }
        
        // Rotate
        
        // For samples
        BL_FLOAT angle = std::atan2(r, l);
        BL_FLOAT dist = std::sqrt(l*l + r*r);
        
        if (fitInSquare)
        {
            BL_FLOAT coeff = SQR2_INV;
            dist *= coeff;
        }
        
        angle = -angle;
        angle -= M_PI/4.0;
        
        BL_FLOAT x = dist*cos(angle);
        BL_FLOAT y = dist*sin(angle);
        
#if 0 // Keep [-1, 1] bounds
        // Center
        x = (x + 1.0)*0.5;
        y = (y + 1.0)*0.5;
#endif
        
        lissajousSamples[0].Get()[i] = x;
        lissajousSamples[1].Get()[i] = y;
    }
}
