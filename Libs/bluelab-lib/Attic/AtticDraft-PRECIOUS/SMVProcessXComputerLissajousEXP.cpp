//
//  SMVProcessXComputerLissajousEXP.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <Utils.h>

#include <Debug.h>

#include "SMVProcessXComputerLissajousEXP.h"

#if 0
NOTE: ComputePolarSamples() should be applied on samples, here it is applied on fft samples
#endif

#define SQR2_INV 0.70710678118655

// Lissajous
#define LISSAJOUS_CLIP_DISTANCE 1.0

SMVProcessXComputerLissajousEXP::SMVProcessXComputerLissajousEXP(double sampleRate)
{
    mSampleRate = sampleRate;
}

SMVProcessXComputerLissajousEXP::~SMVProcessXComputerLissajousEXP() {}

void
SMVProcessXComputerLissajousEXP::Reset(double sampleRate)
{
    mSampleRate = sampleRate;
}

void
SMVProcessXComputerLissajousEXP::ComputeX(const WDL_TypedBuf<double> samples[2],
                                       const WDL_TypedBuf<double> magns[2],
                                       const WDL_TypedBuf<double> phases[2],
                                       WDL_TypedBuf<double> *resultX,
                                       WDL_TypedBuf<double> *resultY,
                                       bool *isPolar, double polarCenter[2],
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
    
    if (isScalable != NULL)
        *isScalable = true; //false; //true;
    
    resultX->Resize(magns[0].GetSize());
    if (resultY != NULL)
        resultY->Resize(magns[0].GetSize());
    
    // Debug
    //Debug::DumpData("samples0.txt", samples[0]);
    //Debug::DumpData("samples1.txt", samples[1]);
    
    // Compute
    WDL_TypedBuf<double> lissajousSamples[2];
    //ComputeLissajous(samples, lissajousSamples, true);
    ComputeLissajousFft(magns, phases, lissajousSamples, true);
    
    // Warning: samples size is twice the magns size
    // So take only half of the samples
    // (otherwise we would have no display after)
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        double x = lissajousSamples[0].Get()[i];
        double y = lissajousSamples[1].Get()[i];
        
        resultX->Get()[i] = x;
        
        if (resultY != NULL)
            resultY->Get()[i] = y;
    }
    
    // We need less fat in x
    Utils::MultValues(resultX, 0.5);
    
    // We need positive y
    Utils::AddValues(resultY, 1.0);
    Utils::MultValues(resultY, 0.5);
    
#if 0 //1 // TEST
    // Normalize by magns
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        double x = resultX->Get()[i];
        double y = resultY->Get()[i];
     
        double x0 = x - polarCenter[0];
        double y0 = y - polarCenter[1];
     
        double magn = magns[0].Get()[i];
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
    
    // TESTS: try to make match samples ids to fft ids
    // Here, Lissajous is computed from samples (the only possible way)
    // In SMVProcess4, we multiply by amplitude or frequency
    // but this is the fft order.
    //
    // Mismatch between samples order and fft order makes impossible to
    // have the expected view when multiplying by amplitudes in SMVProcess4
#if 0 //1
    // Sort from samples order to fft order
    WDL_TypedBuf<int> fftIds;
    Utils::SamplesIdsToFftIds(phases[0], &fftIds);

    WDL_TypedBuf<double> copyResultX = *resultX;
    WDL_TypedBuf<double> copyResultY = *resultY;
    for (int i = 0; i < resultX->GetSize(); i++)
    {
        int id = 0;
        if (i < fftIds.GetSize() - 1)
        {
            id = fftIds.Get()[i];
        }
        
        resultX->Get()[id] = copyResultX.Get()[i];
        resultY->Get()[id] = copyResultY.Get()[i];
    }
#endif
    
#if 0
    // Sort from samples order to fft order
    WDL_TypedBuf<int> sampleIds;
    Utils::FftIdsToSamplesIdsSym(phases[0], &sampleIds);
    
    WDL_TypedBuf<double> copyResultX = *resultX;
    WDL_TypedBuf<double> copyResultY = *resultY;
    for (int i = 0; i < resultX->GetSize(); i++)
    {
        int id = 0;
        if (i < sampleIds.GetSize() - 1)
        {
            id = sampleIds.Get()[i];
        }
        
        resultX->Get()[i] = copyResultX.Get()[id];
        resultY->Get()[i] = copyResultY.Get()[id];
    }
#endif
}

// From USTProcess
void
SMVProcessXComputerLissajousEXP::ComputeLissajous(const WDL_TypedBuf<double> samples[2],
                                               WDL_TypedBuf<double> lissajousSamples[2],
                                               bool fitInSquare)
{
    lissajousSamples[0].Resize(samples[0].GetSize());
    lissajousSamples[1].Resize(samples[0].GetSize());
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        double l = samples[0].Get()[i];
        double r = samples[1].Get()[i];
        
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
        double angle = atan2(r, l);
        double dist = sqrt(l*l + r*r);
        
        if (fitInSquare)
        {
            double coeff = SQR2_INV;
            dist *= coeff;
        }
        
        angle = -angle;
        angle -= M_PI/4.0;
        
        double x = dist*cos(angle);
        double y = dist*sin(angle);
        
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
SMVProcessXComputerLissajousEXP::ComputeLissajousFft(const WDL_TypedBuf<double> magns[2],
                                                  const WDL_TypedBuf<double> phases[2],
                                                  WDL_TypedBuf<double> lissajousSamples[2],
                                                  bool fitInSquare)
{
    lissajousSamples[0].Resize(magns[0].GetSize());
    lissajousSamples[1].Resize(magns[0].GetSize());
    
#if 1 //0 // Compute at time 0
    // NOTE: Fails for a pure sine wave: return only one peak
    int tBinNum = 0;
    
    WDL_TypedBuf<double> leftSamples;
    Utils::ComputeSamplesByFrequency(&leftSamples, mSampleRate, magns[0], phases[0], tBinNum);
    
    WDL_TypedBuf<double> rightSamples;
    Utils::ComputeSamplesByFrequency(&rightSamples, mSampleRate, magns[1], phases[1], tBinNum);
    
    // TEST
#define TIME0_COEFF 4.0
    Utils::MultValues(&leftSamples, TIME0_COEFF);
    Utils::MultValues(&rightSamples, TIME0_COEFF);
#endif
    
#if 0 // Compute the maximum
    // NOTE: Fails for a pure sine wave: return only one peak
    WDL_TypedBuf<double> leftSamples;
    leftSamples.Resize(magns[0].GetSize());
    Utils::FillAllZero(&leftSamples);
    
    WDL_TypedBuf<double> rightSamples;
    rightSamples.Resize(magns[0].GetSize());
    Utils::FillAllZero(&rightSamples);
    
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        WDL_TypedBuf<double> leftSamples0;
        Utils::ComputeSamplesByFrequency(&leftSamples0, mSampleRate, magns[0], phases[0], i);
        
        WDL_TypedBuf<double> rightSamples0;
        Utils::ComputeSamplesByFrequency(&rightSamples0, mSampleRate, magns[1], phases[1], i);
        
        // Find the maxima
        for (int j = 0; j < leftSamples.GetSize(); j++)
        {
            double l = leftSamples.Get()[j];
            double r = rightSamples.Get()[j];
            
            double l0 = leftSamples0.Get()[j];
            double r0 = rightSamples0.Get()[j];
            
            if (fabs(l0 - r0) > fabs(l - r))
            {
                leftSamples.Get()[j] = l0;
                rightSamples.Get()[j] = r0;
            }
        }
    }
#endif

#if 0 //1 // Simply recompute the samples, to check
    // => GOOD !
    WDL_TypedBuf<double> leftSamples;
    leftSamples.Resize(magns[0].GetSize());
    Utils::FillAllZero(&leftSamples);
    
    WDL_TypedBuf<double> rightSamples;
    rightSamples.Resize(magns[0].GetSize());
    Utils::FillAllZero(&rightSamples);
    
    // Add an offset, so the previously windowed signal is centered
    // (maximum in the center)
    int offset = magns[0].GetSize()/2;
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        WDL_TypedBuf<double> leftSamplesFreq;
        Utils::ComputeSamplesByFrequency(&leftSamplesFreq, mSampleRate,
                                         magns[0], phases[0],
                                         i + offset);
        
        WDL_TypedBuf<double> rightSamplesFreq;
        Utils::ComputeSamplesByFrequency(&rightSamplesFreq, mSampleRate,
                                         magns[1], phases[1],
                                         i + offset);
        
        // Find the maxima
        for (int j = 0; j < leftSamplesFreq.GetSize(); j++)
        {
            double lf = leftSamplesFreq.Get()[j];
            double rf = rightSamplesFreq.Get()[j];
            
            leftSamples.Get()[i] += lf;
            rightSamples.Get()[i] += rf;
        }
    }
    
    // Must multiply by 2 !
    // (Is it because we only take half of the fft ?)
    Utils::MultValues(&leftSamples, 2.0);
    Utils::MultValues(&rightSamples, 2.0);
#endif
    
    //Debug::DumpData("left.txt", leftSamples);
    //Debug::DumpData("right.txt", rightSamples);
    
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        //double l = magns[0].Get()[i];
        //double r = magns[1].Get()[i];
        
        double l = leftSamples.Get()[i];
        double r = rightSamples.Get()[i];
        
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
        double angle = atan2(r, l);
        double dist = sqrt(l*l + r*r);
        
        if (fitInSquare)
        {
            double coeff = SQR2_INV;
            dist *= coeff;
        }
        
        angle = -angle;
        angle -= M_PI/4.0;
        
        double x = dist*cos(angle);
        double y = dist*sin(angle);
        
#if 0 // Keep [-1, 1] bounds
        // Center
        x = (x + 1.0)*0.5;
        y = (y + 1.0)*0.5;
#endif
        
        lissajousSamples[0].Get()[i] = x;
        lissajousSamples[1].Get()[i] = y;
    }
}
