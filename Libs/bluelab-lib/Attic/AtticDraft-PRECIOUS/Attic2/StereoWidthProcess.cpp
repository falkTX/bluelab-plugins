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

//
// Scales
//

#define MAX_WIDTH_CHANGE 15.0

// ...
#define MAX_WIDTH_CHANGE_STEREOIZE 200.0

// Why defining it ?
#define SOURCE_DISTANCE 0.001


StereoWidthProcess::StereoWidthProcess(int bufferSize,
                                       double overlapping, double oversampling,
                                       double sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mWidthChange = 0.0;
    
    mFakeStereo = false;
    
    GenerateRandomCoeffs(bufferSize);
}

StereoWidthProcess::~StereoWidthProcess() {}

void
StereoWidthProcess::Reset()
{
    Reset(mOverlapping, mOversampling, mSampleRate);
}

void
StereoWidthProcess::Reset(int overlapping, int oversampling,
                          double sampleRate)
{
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mWidthValuesX.Resize(0);
    mWidthValuesY.Resize(0);
}

void
StereoWidthProcess::SetWidthChange(double widthChange)
{
    mWidthChange = widthChange;
}

void
StereoWidthProcess::SetFakeStereo(bool flag)
{
    mFakeStereo = flag;
}

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
    
    //
    // Apply the effect of the width knob
    //
    if (mFakeStereo)
        Stereoize(&phases[0], &phases[1]);
    else
        ApplyWidthChange(&magns[0], &magns[1], &phases[0], &phases[1]);
    
    //
    // Prepare polar coordinates
    //
    
    // Diff
    WDL_TypedBuf<double> diffPhases;
    Utils::Diff(&diffPhases, phases[0], phases[1]);
    
    // Mono magns
    WDL_TypedBuf<double> monoMagns;
    Utils::StereoToMono(&monoMagns, magns[0], magns[1]);
    
    // Polar to cartesian
    PolarViz::PolarToCartesian(monoMagns, diffPhases,
                               &mWidthValuesX, &mWidthValuesY, THRESHOLD_DB);

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
StereoWidthProcess::ApplyWidthChange(WDL_TypedBuf<double> *ioMagnsL,
                                     WDL_TypedBuf<double> *ioMagnsR,
                                     WDL_TypedBuf<double> *ioPhasesL,
                                     WDL_TypedBuf<double> *ioPhasesR)
{
    WDL_TypedBuf<double> freqs;
    Utils::FftFreqs(&freqs, ioPhasesL->GetSize(), mSampleRate);
    
    double widthChange = Utils::FactorToDivFactor(mWidthChange, MAX_WIDTH_CHANGE);
        
    for (int i = 0; i < ioPhasesL->GetSize(); i++)
    {
        const double originMagnL = ioMagnsL->Get()[i];
        const double originMagnR = ioMagnsR->Get()[i];
        
        const double originPhaseL = ioPhasesL->Get()[i];
        const double originPhaseR = ioPhasesR->Get()[i];
    
        // Get the left and right distance
        
        double freq = freqs.Get()[i];
        
        double distL;
        double distR;
        MagnPhasesToDistances(&distL, &distR, originMagnL, originMagnR,
                              originPhaseL, originPhaseR, freq);
        
        // Modify the width
        distL *= widthChange;
        distR *= widthChange;
        
        double newMagnL = originMagnL;
        double newMagnR = originMagnR;
        
        double newPhaseL = originPhaseL;
        double newPhaseR = originPhaseR;
        
        // TEST
        //distL = 0.0;
        //distR = 0.0;
        
        DistancesToMagnPhases(distL, distR, &newMagnL, &newMagnR,
                              &newPhaseL, &newPhaseR, freq);
        
        // Result
        ioPhasesL->Get()[i] = newPhaseL;
        ioPhasesR->Get()[i] = newPhaseR;
        
        ioMagnsL->Get()[i] = newMagnL;
        ioMagnsR->Get()[i] = newMagnR;
    }
}

void
StereoWidthProcess::Stereoize(WDL_TypedBuf<double> *ioPhasesL,
                              WDL_TypedBuf<double> *ioPhasesR)

{
    // Good, to be at the maximum when the width increase is at maximum
    // With more, we get strange effect
    // when the circle is full and we pull more the width increase
    double stereoizeCoeff =  M_PI/MAX_WIDTH_CHANGE_STEREOIZE;
    double widthChange =
        Utils::FactorToDivFactor(mWidthChange, MAX_WIDTH_CHANGE_STEREOIZE);
    
    for (int i = 0; i < ioPhasesL->GetSize(); i++)
    {
        double phaseL = ioPhasesL->Get()[i];
        double phaseR = ioPhasesR->Get()[i];
        
        double middle = (phaseL + phaseR)/2.0;

        double rnd = mRandomCoeffs.Get()[i];
        
        // A bist strange...
        double diff = stereoizeCoeff*rnd;
        diff *= widthChange;
        
        double newPhaseL = middle - diff/2.0;
        double newPhaseR = middle + diff/2.0;
        
        ioPhasesL->Get()[i] = newPhaseL;
        ioPhasesR->Get()[i] = newPhaseR;
    }
}

void
StereoWidthProcess::GenerateRandomCoeffs(int size)
{
    srand(114242111);
    
    mRandomCoeffs.Resize(size);
    for (int i = 0; i < mRandomCoeffs.GetSize(); i++)
    {
        double rnd = 1.0 - 2.0*((double)rand())/RAND_MAX;
        
        mRandomCoeffs.Get()[i] = rnd;
    }
}

void
StereoWidthProcess::MagnPhasesToDistances(double *distL, double *distR,
                                          double magnL, double magnR,
                                          double phaseL, double phaseR,
                                          double freq)
{
    // See: https://dsp.stackexchange.com/questions/30736/how-to-calculate-an-azimuth-of-a-sound-source-in-stereo-signal
    // Thx :)
    //
    
#define EPS 1e-15

    // TEST
//#define MAGN_MAX_RATIO 1e3
    
    *distL = 0.0;
    *distR = 0.0;
    
    // Shorter distance from the listener to the source
    // (roughtly spoken...)
    const double sourceDistance = SOURCE_DISTANCE;
    
    // 340 m/s
    const double c = 340.0;
    
    // Compute the delay from the phase difference
    double phaseDiff = phaseR - phaseL;
        
    // Avoid negative phase diff
    // (will make problems below)
    phaseDiff = Utils::MapToPi(phaseDiff);
    phaseDiff += 2.0*M_PI;
    
    Debug::AppendValue("phaseDiff0.txt", phaseDiff);
    
    if (freq == 0.0)
        // Just in case
        return;
    
    //double onePeriod = 1.0/freq;
        
    // Time difference
    double T = (1.0/freq)*(phaseDiff/(2.0*M_PI));
    
    //Debug::AppendValue("T0.txt", T);
    
    // Compute magn ration from the magnitude
    if (magnR == 0.0)
    // No sound ?
        return;
        
    // Magns ratio
    double G = magnL/magnR;
    
    //if ((fabs(G - 1.0) < 1.0/MAGN_MAX_RATIO) || (G > MAGN_MAX_RATIO))
    //    // Almost the same level => zero distance !
    //    return;
    
    // WARNING here !!!!!!!!!
    
    // In the original algorithm, L is farther than R,
    // so magnL is smaller than magnR
    //if (G > 1.0)
    //    G = 1.0/G;
    
    // G: ok !!!!
    
    // Second try ("baby method")
        
    // Distance to the right
    double b = G*T*c/(1.0 - G);
    
    // b: ok !!!!
    
    // Distance ot the left
    double a = ((1.0 - G)/G)*b;
    
    // a: ok !!!!
    
    // Distance L
    double distL2 = (a + b)*(a + b) - sourceDistance*sourceDistance;
    if (distL2 < 0.0)
        return;
    *distL = sqrt(distL2);
        
    // Distance R
    double distR2 = b*b - sourceDistance*sourceDistance;
    if (distR2 < 0.0)
        return;
    *distR = sqrt(distR2);
}

void
StereoWidthProcess::DistancesToMagnPhases(double distL, double distR,
                                          double *newMagnL, double *newMagnR,
                                          double *newPhaseL, double *newPhaseR,
                                          double freq)
{
    // Reverse process of the previous method
#define EPS 1e-15
    
    // TEST
//#define MAGN_MAX_RATIO 1e3
    
    // Init
    //*newMagnL = 0.0;
    //*newMagnR = 0.0;
    
    //*newPhaseL = 0.0;
    //*newPhaseR = 0.0;
    
    // Shorter distance from the listener to the source
    // (roughtly spoken...)
    const double sourceDistance = SOURCE_DISTANCE;
    
    // 340 m/s
    const double c = 340.0;
    
    double distL2 = distL*distL;
    double distR2 = distR*distR;
    
    //if (distR2 < EPS)
    //    // Source too close, do not separate
    //    return;
    
    double b2 = distR2 + sourceDistance*sourceDistance;
    double b = sqrt(b2);
    
    double ab2 = distL2 + sourceDistance*sourceDistance;
    if (ab2 < 0.0)
        // Just in case
        return;
    double ab = sqrt(ab2);
    double a = ab - b;
    
    // Get G
    if (fabs(a + b) < EPS)
        return;
    
    double G = b/(a + b);
    
    //if ((fabs(G - 1.0) < 1.0/MAGN_MAX_RATIO) || (G > MAGN_MAX_RATIO)) // Unused ?
    //    // Almost the same level => zero distance !
    //    return;
    
    double denomT = G*c;
    if (fabs(denomT) < EPS) // Unused ?
        return;
    
    // Get T
    double T = (b*(1.0 - G))/denomT;
    
    //Debug::AppendValue("T1.txt", T);
    
    // Get phase diff
    //double onePeriod = 1.0/freq;
    //double phaseDiff = T*2.0*M_PI/onePeriod;
    double phaseDiff = T*freq*2.0*M_PI;
    
    // Avoid negative phase diff
    // (will make problems below)
    phaseDiff = Utils::MapToPi(phaseDiff);
    phaseDiff += 2.0*M_PI;
    
    Debug::AppendValue("phaseDiff1.txt", phaseDiff);
    
    // Test: take the left, and adjust the right
    if (fabs(G) < EPS)
        return;
    
    // Result
    *newMagnR = *newMagnL/G;
    
    *newPhaseR = *newPhaseL + phaseDiff;
}
