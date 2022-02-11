//
//  StereoWidthProcess.cpp
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#include <Utils.h>
#include <PolarViz.h>
#include <Debug.h>

#include "StereoWidthProcess.h"

// Remove just some values
// If threshold is > -100dB, there is a blank half circle
// at the basis
#define THRESHOLD_DB -120.0

// Test value
//#define STEREOIZE_COEFF 0.1

// Good, to be at the maximum when the width increase is at maximum
// With more, we get strange effect
// when the circle is full and we pull more the width increase
#define STEREOIZE_COEFF 0.015

StereoWidthProcess::StereoWidthProcess(int bufferSize)
{
    mWidthChange = 0.0;
}

StereoWidthProcess::~StereoWidthProcess() {}

void
StereoWidthProcess::Reset()
{
    Reset(4, 1, 44100.0);
}

void
StereoWidthProcess::Reset(int overlapping, int oversampling,
                          double sampleRate)
{
    mWidthValuesX.Resize(0);
    mWidthValuesY.Resize(0);
}

void
StereoWidthProcess::SetWidthChange(double widthChange)
{
    mWidthChange = widthChange;
}

//void
//StereoWidthProcess::SetMode(enum Mode mode)
//{
//    mMode = mode;
//}

void
StereoWidthProcess::ProcessInputSamples(vector<WDL_TypedBuf<double> * > *ioSamples)
{
    if (ioSamples->empty())
        return;
    
    if (ioSamples->size() == 1)
        mCurrentSamples = *(*ioSamples)[0];
    else if (ioSamples->size() > 1)
    {
        Utils::StereoToMono(&mCurrentSamples, *(*ioSamples)[0], *(*ioSamples)[1]);
    }
}

void
StereoWidthProcess::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples)
{
    // TODO: manage mono to stereo
    if (ioFftSamples->size() < 2)
        return;
    
    //
    // Prepare the data
    //
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2] = { *(*ioFftSamples)[0], *(*ioFftSamples)[1] };
              
    // Take half of the complexes
    Utils::TakeHalf(&fftSamples[0]);
    Utils::TakeHalf(&fftSamples[1]);
    
    WDL_TypedBuf<double> magns[2];
    WDL_TypedBuf<double> phases[2];
    
    Utils::ComplexToMagnPhase(&magns[0], &phases[0], fftSamples[0]);
    Utils::ComplexToMagnPhase(&magns[1], &phases[1], fftSamples[1]);
    
#if 1
    Stereoize(&phases[0], &phases[1]);
#endif
    
    //
    // Apply the effect of the width knob
    //
    ApplyWidthChange(&phases[0], &phases[1]);
    
    
    //
    // Prepare polar coordinates
    //
    
    // Diff
    WDL_TypedBuf<double> diffPhases;
    Utils::Diff(&diffPhases, phases[0], phases[1]);
    
    // Mono magns
    WDL_TypedBuf<double> monoMagns;
    Utils::StereoToMono(&monoMagns, magns[0], magns[1]);
    
    // Get the values
    //if (mMode == POLAR_LEVEL)
    
    // TEST
    //WDL_TypedBuf<double> middles;
    //Utils::ComputeAvg(&middles, phases[0], phases[1]);
    //Utils::SubstractValues(&phases[0], middles);
    
#if 1 // ORIG
    PolarViz::PolarToCartesian(monoMagns, diffPhases,
                               &mWidthValuesX, &mWidthValuesY, THRESHOLD_DB);
#endif
    
#if 0
    // TEST (polar samples ?)
    PolarViz::PolarToCartesian(magns[0], phases[0],
                               &mWidthValuesX, &mWidthValuesY, THRESHOLD_DB);
#endif
    
    //else if (mMode == POLAR_SAMPLES)
        //PolarViz::PolarSamplesToCartesian(monoMagns, diffPhases, phases[0],
        //                                  &mWidthValuesX, &mWidthValuesY);
        //PolarViz::PolarSamplesToCartesian2(monoMagns, diffPhases, mCurrentSamples,
        //                                   &mWidthValuesX, &mWidthValuesY);
    
    //
    // Re-synthetise the data with the new diff
    //
    Utils::MagnPhaseToComplex(&fftSamples[0], magns[0], phases[0]);
    fftSamples[0].Resize(fftSamples[0].GetSize()*2);
    
    Utils::MagnPhaseToComplex(&fftSamples[1], magns[1], phases[1]);
    fftSamples[1].Resize(fftSamples[1].GetSize()*2);
    
    Utils::FillSecondFftHalf(&fftSamples[0]);
    Utils::FillSecondFftHalf(&fftSamples[1]);
    
    // Result
    *(*ioFftSamples)[0] = fftSamples[0];
    *(*ioFftSamples)[1] = fftSamples[1];
}

void
StereoWidthProcess::GetWidthValues(WDL_TypedBuf<double> *xValues,
                                   WDL_TypedBuf<double> *yValues)
{
    *xValues = mWidthValuesX;
    *yValues = mWidthValuesY;
}

void
StereoWidthProcess::ApplyWidthChange(WDL_TypedBuf<double> *ioPhasesL,
                                     WDL_TypedBuf<double> *ioPhasesR)
{
    for (int i = 0; i < ioPhasesL->GetSize(); i++)
    {
        double phaseL = ioPhasesL->Get()[i];
        double phaseR = ioPhasesR->Get()[i];
        
        double middle = (phaseL + phaseR)/2.0;
        double diff = phaseR - phaseL;
        
        double prevDiff = diff;
        diff *= mWidthChange;
        
        if (diff*prevDiff < 0.0)
            // Sign change
            diff = 0.0;
        
        double newPhaseL = middle - diff/2.0;
        double newPhaseR = middle + diff/2.0;
        
        ioPhasesL->Get()[i] = newPhaseL;
        ioPhasesR->Get()[i] = newPhaseR;
    }
}

void
StereoWidthProcess::Stereoize(WDL_TypedBuf<double> *ioPhasesL,
                              WDL_TypedBuf<double> *ioPhasesR)

{
    srand(114242111);
    
    for (int i = 0; i < ioPhasesL->GetSize(); i++)
    {
        double phaseL = ioPhasesL->Get()[i];
        double phaseR = ioPhasesR->Get()[i];
        
        double middle = (phaseL + phaseR)/2.0;

        double rnd = 1.0 - 2.0*((double)rand())/RAND_MAX;
        double diff = STEREOIZE_COEFF*rnd;
        
        double newPhaseL = middle - diff/2.0;
        double newPhaseR = middle + diff/2.0;
        
        ioPhasesL->Get()[i] = newPhaseL;
        ioPhasesR->Get()[i] = newPhaseR;
    }
}