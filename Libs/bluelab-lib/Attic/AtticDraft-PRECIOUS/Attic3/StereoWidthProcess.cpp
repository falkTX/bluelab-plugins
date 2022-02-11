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
#include <DebugGraph.h>

#include "StereoWidthProcess.h"

#if TEST_TEMPORAL_SMOOTHING
#define TEMPORAL_DIST_SMOOTH_COEFF 0.9
#endif

#if 0
TODO (GLOBAL): check that distances are well computed

PROBLEM: with with change coeff = 1 => identity transform doesn't work'
CHECK: correct spread when transform with width = 1

TODO: make inverse process work (distance => magn phases) !

#endif

#define USE_DEBUG_GRAPH 1

//#define DEBUG_MAX_DIST 100.0
#define DEBUG_MAX_DIST 10.0

// NOTE: warning test negative diff !!!!!!!!
//

// Remove just some values
// If threshold is > -100dB, there is a blank half circle
// at the basis
#define THRESHOLD_DB -120.0

//
// Scales
//

//#define MAX_WIDTH_CHANGE 15.0
#define MAX_WIDTH_CHANGE 4.0

// ...
#define MAX_WIDTH_CHANGE_STEREOIZE 200.0

// Why defining it ?
//#define SOURCE_DISTANCE 1.0 //0.001
//#define SOURCE_DISTANCE 0.001

// Let's say 1cm
#define MICROPHONES_SEPARATION 0.01

// TEST
#define TEST_THRESHOLD_AMPS 1

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
    
    GenerateRandomCoeffs(bufferSize/2);
    
#if TEST_TEMPORAL_SMOOTHING
    mDistLHisto = new SmoothAvgHistogram(bufferSize/2, TEMPORAL_DIST_SMOOTH_COEFF, 0.0);
    mDistRHisto = new SmoothAvgHistogram(bufferSize/2, TEMPORAL_DIST_SMOOTH_COEFF, 0.0);
#endif
}

StereoWidthProcess::~StereoWidthProcess()
{
#if TEST_TEMPORAL_SMOOTHING
    delete mDistLHisto;
    delete mDistRHisto;
#endif
}

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
    
#if TEST_TEMPORAL_SMOOTHING
    mDistLHisto->Reset();
    mDistRHisto->Reset();
#endif
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
    
    // ignore width change => well spread dots
#if 1 // origin: 1
    //
    // Apply the effect of the width knob
    //
    if (mFakeStereo)
        Stereoize(&phases[0], &phases[1]);
    else
        ApplyWidthChange(&magns[0], &magns[1], &phases[0], &phases[1]);
#endif
    
    //
    // Prepare polar coordinates
    //
    
    // Diff
    WDL_TypedBuf<double> diffPhases;
    Utils::Diff(&diffPhases, phases[0], phases[1]);
    
    // Mono magns
    WDL_TypedBuf<double> monoMagns;
    Utils::StereoToMono(&monoMagns, magns[0], magns[1]);
    
#if 1 // original
    // Display the diff
    // Polar to cartesian
    PolarViz::PolarToCartesian(monoMagns, diffPhases,
                               &mWidthValuesX, &mWidthValuesY, THRESHOLD_DB);
#endif
   
#if 0 // Display the magns and phases, not diff phases
    // Polar to cartesian
    PolarViz::PolarToCartesian(magns[0], phases[0],
                               &mWidthValuesX, &mWidthValuesY, THRESHOLD_DB);
    // TODO: display the second channel too
#endif
    
    // CURRENT TEST
#if 0 // Test with phase average ponderated with magnitude
    double avgDiff = ComputeAvgPhaseDiff(monoMagns, diffPhases);
    
    WDL_TypedBuf<double> avgPhaseDiff;
    avgPhaseDiff.Resize(monoMagns.GetSize());
    Utils::FillAllValue(&avgPhaseDiff, avgDiff);
    
#define DEBUG_COEFF 1000.0
    Utils::MultValues(&avgPhaseDiff, DEBUG_COEFF);
    
    PolarViz::PolarToCartesian(monoMagns, avgPhaseDiff,
                               &mWidthValuesX, &mWidthValuesY, THRESHOLD_DB);
    
#endif
    
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
    
    //double widthChange = Utils::FactorToDivFactor(mWidthChange, MAX_WIDTH_CHANGE);
    double widthFactor = ComputeFactor(mWidthChange, MAX_WIDTH_CHANGE);
    
#if USE_DEBUG_GRAPH
    WDL_TypedBuf<double> originMagnsL = *ioMagnsL;
#endif
    
    Debug::DumpData("startL.txt", *ioMagnsL);
    Debug::DumpData("startR.txt", *ioMagnsR);
    
    // Compute left and right distances
    WDL_TypedBuf<double> distsL;
    distsL.Resize(ioPhasesL->GetSize());
    
    WDL_TypedBuf<double> distsR;
    distsR.Resize(ioPhasesL->GetSize());
    
    for (int i = 0; i < ioPhasesL->GetSize(); i++)
    {
        double magnL = ioMagnsL->Get()[i];
        double magnR = ioMagnsR->Get()[i];
        
        double phaseL = ioPhasesL->Get()[i];
        double phaseR = ioPhasesR->Get()[i];
        
        double freq = freqs.Get()[i];
        
        double distL;
        double distR;
        MagnPhasesToDistances(&distL, &distR,
                              magnL, magnR, phaseL, phaseR, freq);
        
        distsL.Get()[i] = distL;
        distsR.Get()[i] = distR;
        
#if TEST_THRESHOLD_AMPS
#define THRS 0.0001
        if (magnL < THRS)
        // Too low amplitude
        {
            // Don't change
            distsL.Get()[i] = 0.0;
            distsR.Get()[i] = 0.0;
        }
#endif
    }
    
    //
    // Here, we have all the distances
    //
    // Smooth !
    //

#if TEST_TEMPORAL_SMOOTHING
    // Left
    mDistLHisto->AddValues(distsL);
    mDistLHisto->GetValues(&distsL);
    
    // Right
    mDistRHisto->AddValues(distsR);
    mDistRHisto->GetValues(&distsR);
#endif

#if TEST_SPATIAL_SMOOTHING
    // Test 128
    int windowSize = distsL.GetSize()/128;
    
    WDL_TypedBuf<double> smoothDistsL;
    CMA2Smoother::ProcessOne(distsL, &smoothDistsL, windowSize);
    distsL = smoothDistsL;
    
    WDL_TypedBuf<double> smoothDistsR;
    CMA2Smoother::ProcessOne(distsR, &smoothDistsR, windowSize);
    distsR = smoothDistsR;
#endif
    
    // Compute the new magns and phases (depending on the width factor)
    for (int i = 0; i < ioPhasesL->GetSize(); i++)
    {
        // Retrieve the left and right distance
        double distL = distsL.Get()[i];
        double distR = distsR.Get()[i];
        
        // Modify the width
        distL *= widthFactor;
        distR *= widthFactor;
        
        double freq = freqs.Get()[i];
        
        // Init
        double newMagnL = ioMagnsL->Get()[i];
        double newMagnR = ioMagnsR->Get()[i];
        
        double newPhaseL = ioPhasesL->Get()[i];
        double newPhaseR = ioPhasesR->Get()[i];
        
        // Modify
        DistancesToMagnPhases(distL, distR, &newMagnL, &newMagnR,
                              &newPhaseL, &newPhaseR, freq);
        
        // Result
        ioPhasesL->Get()[i] = newPhaseL;
        ioPhasesR->Get()[i] = newPhaseR;
        
        ioMagnsL->Get()[i] = newMagnL;
        ioMagnsR->Get()[i] = newMagnR;
    }
    
    Debug::DumpData("endL.txt", *ioMagnsL);
    Debug::DumpData("endR.txt", *ioMagnsR);
    
#if USE_DEBUG_GRAPH
#if 1 //dists
    DebugGraph::SetCurveValues(distsL,
                               0, // curve num
                               -DEBUG_MAX_DIST, DEBUG_MAX_DIST, // min and max y
                               3.0,
                               255, 0, 0);
    
    DebugGraph::SetCurveValues(distsR,
                               1, // curve num
                               -DEBUG_MAX_DIST, DEBUG_MAX_DIST, // min and max y
                               2.0,
                               0, 255, 0);
#endif
    
    DebugGraph::SetCurveValues(originMagnsL,
                               3, // curve num
                               0.0, 0.01, // min and max y
                               1.0,
                               0, 0, 255);
#endif
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
        double rnd0 = 1.0 - 2.0*((double)rand())/RAND_MAX;
        
        mRandomCoeffs.Get()[i] = rnd0;
    }
}

void
StereoWidthProcess::MagnPhasesToDistances(double *distL, double *distR,
                                          double magnL, double magnR,
                                          double phaseL, double phaseR,
                                          double freq)
{
#define EPS 1e-15
    
    // See: https://dsp.stackexchange.com/questions/30736/how-to-calculate-an-azimuth-of-a-sound-source-in-stereo-signal
    // Thx :)
    //
    
    if (distL != NULL)
        *distL = 0.0;
    
    if (distR != NULL)
        *distR = 0.0;
    
    const double micsDistance = MICROPHONES_SEPARATION;
    
    // 340 m/s
    const double c = 340.0;
    
    // Compute the delay from the phase difference
    double phaseDiff = phaseR - phaseL;
    
#if 0
    // Avoid negative phase diff
    // (will make problems below)
    phaseDiff = Utils::MapToPi(phaseDiff);
    phaseDiff += 2.0*M_PI;
#endif
    
    //Debug::AppendValue("phaseDiff0.txt", phaseDiff);
    
    if (freq < EPS)
        // Just in case
        return;
        
    // Time difference
    double T = (1.0/freq)*(phaseDiff/(2.0*M_PI));
    
    // Compute magn ration from the magnitude
    if (magnR < EPS)
    // No sound !
        return;
        
    // Magns ratio
    double G = magnL/magnR;
    
    //Debug::AppendValue("startG.txt", G);
    
    // Distance to the right
    double denomB = (1.0 - G);
    if (fabs(denomB) < EPS)
        return;
    
    double b = G*T*c/denomB;
    
    //Debug::AppendValue("startB.txt", b);
    
    // Distance ot the left
    if (fabs(G) < EPS)
        return;
    double a = ((1.0 - G)/G)*b;
    
    // Distance L
    double distL2 = (a + b)*(a + b) - micsDistance*micsDistance;
    
#if 0 // origin
    if (distL2 < 0.0)
        return;
    
    if (distL != NULL)
        *distL = sqrt(distL2);
#endif
#if 1 // test
    if (distL != NULL)
    {
        if (distL2 < 0.0)
            *distL = -sqrt(-distL2);
        else
            *distL = sqrt(distL2);
    }
#endif
    
    // Distance R
    double distR2 = b*b - micsDistance*micsDistance;
    
#if 0 // origin
    if (distR2 < 0.0)
        return;
    
    if (distR != NULL)
        *distR = sqrt(distR2);
#endif
#if 1
    if (distR != NULL)
    {
        if (distR2 < 0.0)
            *distR = -sqrt(-distR2);
        else
            *distR = sqrt(distR2);
    }
#endif
}

void
StereoWidthProcess::DistancesToMagnPhases(double distL, double distR,
                                          double *ioMagnL, double *ioMagnR,
                                          double *ioPhaseL, double *ioPhaseR,
                                          double freq)
{
#define EPS 1e-15
    
    // Reverse process of the previous method
    
    const double micsDistance = MICROPHONES_SEPARATION;
    
    // 340 m/s
    const double c = 340.0;
    
    double distL2 = distL*distL;
    double distR2 = distR*distR;
    
    double b2 = distR2 + micsDistance*micsDistance;
    double b = sqrt(b2);
    
    //Debug::AppendValue("endB.txt", b);
    
    double ab2 = distL2 + micsDistance*micsDistance; ///
    if (ab2 < 0.0)
        // Just in case
        return;
    double ab = sqrt(ab2);
    double a = ab - b;
    
    // Get G
    if (fabs(a + b) < EPS)
        return;
    
    double G = b/(a + b);
    
    //Debug::AppendValue("endG.txt", G);
    
    double denomT = G*c;
    if (fabs(denomT) < EPS)
        return;
    
    // Get T
    double T = (b*(1.0 - G))/denomT;
    
    // Get phase diff
    double phaseDiff = T*freq*2.0*M_PI;
    
#if 0
    // Avoid negative phase diff
    // (will make problems below)
    phaseDiff = Utils::MapToPi(phaseDiff);
    phaseDiff += 2.0*M_PI;
#endif
    
    // Test:
    if (fabs(G) < EPS)
        return;

#if 0 // Method 1: take the left, and adjust for the right
    const double origMagnL = *ioMagnL;
    //const double origMagnR = *ioMagnR;
    
    const double origPhaseL = *ioPhaseL;
    //const double origPhaseR = *ioPhaseR;
    
    // Compute the left and right coefficient
    
    if (distL + distR < EPS)
        // Same point
    {
        // Same magn (we take the left for example)
        *ioMagnR = *ioMagnL;
        *ioPhaseR = *ioPhaseL;
        
        return;
    }
    
    // Method "1"
    *ioMagnR = origMagnL/G;
    *ioPhaseR = origPhaseL + phaseDiff;
    
#if 0 //test 1
    double t = distL/(distL + distR);

    *ioMagnL = t*origMagnL + (1.0 - t)*origMagnR;
    *ioMagnR = (1.0 - t)*origMagnL + t*origMagnR;
    
    *ioPhaseL = t*origPhaseL + (1.0 - t)*origPhaseR;
    *ioPhaseR = (1.0 - t)*origPhaseL + t*origPhaseR;
#endif
    
#if 0 //test 2
    double t = distL/(distL + distR);
    
    double tL = distR/(distL + distR);
    double tR = distL/(distL + distR);
    
    if (t < 0.5)
        tL = 1.0;
    
    if (t > 0.5)
        tR = 1.0;
    
    // TEST (not good...)
    double newMagnR = origMagnL/G;
    double newPhaseR = origPhaseL + phaseDiff;
    
    *ioMagnL = tL*origMagnL + (1.0 - tL)*newMagnR;
    *ioMagnR = (1.0 - tR)*origMagnL + tR*newMagnR;
    
    *ioPhaseL = tL*origPhaseL + (1.0 - tL)*newPhaseR;
    *ioPhaseR = (1.0 - tR)*origPhaseL + tR*newPhaseR;
#endif

#if 0 //test 3
    double middleMagn = (origMagnL + origMagnR)/2.0;
    double middlePhase = (origPhaseL + origPhaseR)/2.0;
    
    double newMagnL = middleMagn - 0.5*middleMagn;
    double newPhaseL = middlePhase - 0.5*middlePhase;
    
    double newMagnR = newMagnL/G;
    double newPhaseR = newPhaseL + phaseDiff;
    
    *ioMagnL = newMagnL;
    *ioMagnR = newMagnR;
    
    *ioPhaseL = newPhaseL;
    *ioPhaseR = newPhaseR;
#endif
    
#endif
    
#if 1 // Method 2: take the middle, and adjust for left and right
    double middleMagn = (*ioMagnL + *ioMagnR)/2.0;
    //double diffMagn = (*ioMagnR - *ioMagnL);
    
    double middlePhase = (*ioPhaseL + *ioPhaseR)/2.0;
    
    // Compute the left and right coefficient
    
    if (distL + distR < EPS)
        // Same point
    {
        // Same magn (we take the left for example)
        *ioMagnR = *ioMagnL;
        *ioPhaseR = *ioPhaseL;
        
        return;
    }
    
    *ioMagnR = 2.0*middleMagn/(G + 1.0);
    *ioMagnL = *ioMagnR*G;
    
    *ioPhaseL = middlePhase - phaseDiff/2.0;
    *ioPhaseR = middlePhase + phaseDiff/2.0;
#endif
}

double
StereoWidthProcess::ComputeFactor(double normVal, double maxVal)
{
    double res = 0.0;
    if (normVal < 0.0)
        res = normVal + 1.0;
    else
        res = normVal*(maxVal - 1.0) + 1.0;

    return res;
}

double
StereoWidthProcess::ComputeAvgPhaseDiff(const WDL_TypedBuf<double> &monoMagns,
                                        const WDL_TypedBuf<double> &diffPhases)
{
    if (diffPhases.GetSize() == 0)
        return 0.0;
    
    double avg = 0.0;
    
    for (int i = 0; i < diffPhases.GetSize(); i++)
    {
        double magn = monoMagns.Get()[i];
        double diff = diffPhases.Get()[i];
        
        double val = magn*diff;
        
        avg += val;
    }
    
    avg /= diffPhases.GetSize();
    
    return avg;
}
