//
//  StereoWidthProcess.cpp
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#include <Utils.h>
#include <PolarViz.h>
#include <Utils.h>
#include <Debug.h>
#include <DebugGraph.h>

#include "StereoWidthProcess.h"

#if 0
CHECK: correct spread when transform with width = 1

TODO: make inverse process work (distance => magn phases) ! => check identity transform

TEST: intensity of points == magns (db ?)
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

// Let's say 1cm (Zoom H1)
#define MICROPHONES_SEPARATION 0.01

// TEST
#define TEST_THRESHOLD_AMPS 0

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
    mSourceRsHisto = new SmoothAvgHistogram(bufferSize/2, TEMPORAL_DIST_SMOOTH_COEFF, 0.0);
    mSourceThetasHisto = new SmoothAvgHistogram(bufferSize/2, TEMPORAL_DIST_SMOOTH_COEFF, 0.0);
#endif
}

StereoWidthProcess::~StereoWidthProcess()
{
#if TEST_TEMPORAL_SMOOTHING
    delete mSourceRsHisto;
    delete mSourceThetasHisto;
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
    mSourceRsHisto->Reset();
    mSourceThetasHisto->Reset();
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
    
    // Compute left and right distances
    WDL_TypedBuf<double> sourceRs;
    sourceRs.Resize(ioPhasesL->GetSize());
    
    WDL_TypedBuf<double> sourceThetas;
    sourceThetas.Resize(ioPhasesL->GetSize());
    
    for (int i = 0; i < ioPhasesL->GetSize(); i++)
    {
        double magnL = ioMagnsL->Get()[i];
        double magnR = ioMagnsR->Get()[i];
        
        double phaseL = ioPhasesL->Get()[i];
        double phaseR = ioPhasesR->Get()[i];
        
        double freq = freqs.Get()[i];
        
        double sourceR;
        double sourceTheta;
        bool computed = MagnPhasesToSourcePos(&sourceR, &sourceTheta,
                                              magnL, magnR, phaseL, phaseR, freq);
        
        if (computed)
        {
            sourceRs.Get()[i] = sourceR;
            sourceThetas.Get()[i] = sourceTheta;
        }
        else
        {
            // Set a negative value to the radius, to mark it as not computed
            sourceRs.Get()[i] = UTILS_VALUE_UNDEFINED;
            sourceThetas.Get()[i] = UTILS_VALUE_UNDEFINED;
        }
        
#if TEST_THRESHOLD_AMPS
#define THRS 0.0001
        if (magnL < THRS)
        // Too low amplitude
        {
            // Don't change
            sourceRs.Get()[i] = 0.0;
            sourceThetas.Get()[i] = 0.0;
        }
#endif
    }
    
    //
    // Here, we have all the distances
    //
    // Fill missing values, then
    //
    // Smooth !
    //

#if FILL_MISSING_VALUES
    Utils::FillMissingValues(&sourceRs, true, UTILS_VALUE_UNDEFINED);
    Utils::FillMissingValues(&sourceThetas, true, UTILS_VALUE_UNDEFINED);
#endif
    
#if TEST_TEMPORAL_SMOOTHING
    // Left
    mSourceRsHisto->AddValues(sourceRs);
    mSourceRsHisto->GetValues(&sourceRs);
    
    // Right
    mSourceThetasHisto->AddValues(sourceThetas);
    mSourceThetasHisto->GetValues(&sourceThetas);
#endif

#if TEST_SPATIAL_SMOOTHING
    int windowSize = sourceRs.GetSize()/SPATIAL_SMOOTHING_WINDOW_SIZE;
    
    WDL_TypedBuf<double> smoothSourceRs;
    CMA2Smoother::ProcessOne(sourceRs, &smoothSourceRs, windowSize);
    sourceRs = smoothSourceRs;
    
    WDL_TypedBuf<double> smoothSourceThetas;
    CMA2Smoother::ProcessOne(sourceThetas, &smoothSourceThetas, windowSize);
    sourceThetas = smoothSourceThetas;
#endif
    
    Debug::DumpData("src-r.txt", sourceRs);
    Debug::DumpData("src-theta.txt", sourceThetas);
    
    //Debug::DumpData("magns0.txt", *ioMagnsL);
    //Debug::DumpData("phases0.txt", *ioPhasesL);
    
    // Compute the new magns and phases (depending on the width factor)
    for (int i = 0; i < ioPhasesL->GetSize(); i++)
    {
        // Retrieve the left and right distance
        double sourceR = sourceRs.Get()[i];
        double sourceTheta = sourceThetas.Get()[i];
        
        // Modify the width (mics distance)
        double newMicsDistance = widthFactor*MICROPHONES_SEPARATION;
        
        double freq = freqs.Get()[i];
        
        // Init
        double newMagnL = ioMagnsL->Get()[i];
        double newMagnR = ioMagnsR->Get()[i];
        
        double newPhaseL = ioPhasesL->Get()[i];
        double newPhaseR = ioPhasesR->Get()[i];
        
        // Modify
        SourcePosToMagnPhases(sourceR, sourceTheta,
                              newMicsDistance,
                              &newMagnL, &newMagnR,
                              &newPhaseL, &newPhaseR, freq);
        
        // Result
        ioPhasesL->Get()[i] = newPhaseL;
        ioPhasesR->Get()[i] = newPhaseR;
        
        ioMagnsL->Get()[i] = newMagnL;
        ioMagnsR->Get()[i] = newMagnR;
    }
    
    //Debug::DumpData("magns1.txt", *ioMagnsL);
    //Debug::DumpData("phases1.txt", *ioPhasesL);
    
#if USE_DEBUG_GRAPH
    
#if DEBUG_FREQS_DISPLAY
    DebugGraph::SetCurveValues(originMagnsL,
                               2, // curve num
                               0.0, 0.01, // min and max y
                               1.0,
                               0, 0, 255);
    
#if 1 //r
    DebugGraph::SetCurveValues(sourceRs,
                               0, // curve num
                               -DEBUG_MAX_DIST, DEBUG_MAX_DIST, // min and max y
                               1.0,
                               255, 0, 0);
    
#endif
    
#if 0 // theta
    DebugGraph::SetCurveValues(sourceThetas,
                               1, // curve num
                               -2.0*M_PI, 2.0*M_PI, // min and max y
                               1.0,
                               0, 255, 0);
#endif
#endif
    
#if DEBUG_SPAT_DISPLAY
    WDL_TypedBuf<double> thetasRot = sourceThetas;
    Utils::AddValues(&thetasRot, -M_PI/2.0);
    
    WDL_TypedBuf<double> xValues;
    WDL_TypedBuf<double> yValues;

#if 1 // "flat"
    PolarViz::PolarToCartesian2(sourceRs, thetasRot,
                                &xValues, &yValues);
#endif

#if 0 // "round" (dB scale)
    PolarViz::PolarToCartesian(sourceRs, thetasRot,
                               &xValues, &yValues, -1e16);
#endif

    
    DebugGraph::SetPointValues(xValues, yValues,
                               4,
                               -1.0, 1.0,
                               -1.0, 1.0,
                               4.0,
                               0, 255, 255,
                               false, 0.0);
    
#endif
    
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

bool
StereoWidthProcess::MagnPhasesToSourcePos(double *sourceR, double *sourceTheta,
                                          double magnL, double magnR,
                                          double phaseL, double phaseR,
                                          double freq)
{
#define EPS 1e-15
    
    // See: https://dsp.stackexchange.com/questions/30736/how-to-calculate-an-azimuth-of-a-sound-source-in-stereo-signal
    // Thx :)
    //
    
    //if (sourceR != NULL)
    //    *sourceR = 0.0;
    
    //if (sourceTheta != NULL)
    //    *sourceTheta = 0.0;
    
    if (freq < EPS)
        // Just in case
    {
        // Null frequency
        // Can legitimately set the result to 0
        // and return true
        
        *sourceR = 0.0;
        *sourceTheta = 0.0;
        
        return true;
    }
    
    const double micsDistance = MICROPHONES_SEPARATION;
    
    // Sound speed: 340 m/s
    const double c = 340.0;
    
    // Wavelength
    //double lambda = c/freq;
    
    // Test if we must invert left and right for the later computations
    // In fact, the algorithm is designed to have G < 1.0,
    // i.e magnL < magnR
    //
    bool invertFlag = false;
    if (magnL > magnR)
    {
        invertFlag = true;
        
        double tmpMagn = magnL;
        magnL = magnR;
        magnR = tmpMagn;
        
        double tmpPhase = phaseL;
        phaseL = phaseR;
        phaseR = tmpPhase;
    }
    
    // Compute the delay from the phase difference
    double phaseDiff = phaseR - phaseL;
    
#if 0
    // Avoid negative phase diff
    // (will make problems below)
    phaseDiff = Utils::MapToPi(phaseDiff);
    phaseDiff += 2.0*M_PI;
#endif
    
    // Compute magn ration from the magnitude
    if (magnR < EPS)
        // No sound !
    {
        // Can legitimately set the result to 0
        // and return true
        
        *sourceR = 0.0;
        *sourceTheta = 0.0;
        
        return true;
    }
    
    // Magns ratio
    double G = magnL/magnR;
    
    if (fabs(G) < EPS)
        // Similar magns
        // Can't compute...
    {
        return false;
    }
    
    //
    // Iterate until finding the solution
    //
    // We must do that because phase diff is "cyclic",
    // it doesn't really define the delay T as is.
    //
//#define MAX_ITER 1000
    
//    for (int i = 0; i < MAX_ITER; i++)
   // {
        // Time difference
        double T = (1.0/freq)*(phaseDiff/(2.0*M_PI));
        
        // Add x periods
        //T += i/freq;
    
        // Distance to the right
        double denomB = (1.0 - G);
        if (fabs(denomB) < EPS)
            return false;
    
        double b = G*T*c/denomB;
    
    
#if 1 // worked well, without iterating
        if (b < micsDistance/2.0)
        // The source is very close
        {
            // Do not compute !
            // The source is too close, and we'll get strange results
            *sourceR = 0.0;
            *sourceTheta = 0.0;
        
            return true;
        }
#endif
        
#if 0
        if (b < micsDistance/2.0)
            // The source is very close for this iteration
            continue;
#endif
        
        //double a = ((1.0 - G)/G)*b;
        double a = b/G - b;
    
        // Simpler to use the formula using a and b
        // (simpler for inversion later)
        
        // r
        double r2 = 4.0*b*b + 4.0*a*b + 2.0*a*a - micsDistance*micsDistance;
        if (r2 < 0.0)
            //continue;
            return false;
    
        double r = 0.5*sqrt(r2);
    
        // Theta
        double cosTheta = r/micsDistance + micsDistance/(4.0*r) - b*b/(micsDistance*r);
    
        // See: https://fr.wikipedia.org/wiki/Loi_des_cosinus
        //double cosTheta = (r*r + micsDistance*micsDistance/4.0 - b*b)/(r*micsDistance);
    
#if 1
        if ((cosTheta < -1.0) || (cosTheta > 1.0))
        // Would make incorrect results... give up this iteration
            return false;
#endif
    
    //continue;
        
        double theta = acos(cosTheta);
    
        *sourceR = r;
        *sourceTheta = theta;
    
        //if (invertFlag) ??
        
        return true;
    //}
    
    //return false;
}

bool
StereoWidthProcess::SourcePosToMagnPhases(double sourceR, double sourceTheta,
                                          double micsDistance,
                                          double *ioMagnL, double *ioMagnR,
                                          double *ioPhaseL, double *ioPhaseR,
                                          double freq)
{
#define EPS 1e-15
    
    double middleMagn = (*ioMagnL + *ioMagnR)/2.0;
    double middlePhase = (*ioPhaseL + *ioPhaseR)/2.0;
    
    // Initial condition: set the signal to mono for the given frequency
    *ioMagnL = middleMagn;
    *ioMagnR = middleMagn;
        
    *ioPhaseL = middlePhase;
    *ioPhaseR = middlePhase;
        
    if (micsDistance < EPS)
        // the two mics are at the same position
        // => the signal should be mono !
        return false;
    
    if (sourceR < 0.0)
        // The source position was not previously computed (so not usable)
        // Keep mono signal for the given frequency
        return false;
    
    // Reverse process of the previous method
    
    // 340 m/s
    const double c = 340.0;
    
    // b
    double b2 = (sourceR/micsDistance + micsDistance/(4.0*sourceR) -
                 cos(sourceTheta))*micsDistance*sourceR;
    if (b2 < 0.0)
        return false;
    double b = sqrt(b2);
    
    // a
    
    // See: https://www.algebra.com/algebra/homework/word/geometry/The-length-of-a-median-of-a-triangle.lesson
    // (length of a median of a triangle)
    double aPlusB2 = 4.0*sourceR*sourceR - 2.0*b*b + micsDistance*micsDistance;
    if (aPlusB2 < 0.0)
        return false;
    double aPlusB = 0.5*sqrt(aPlusB2);
    
    double a = aPlusB - b;
    
    if (fabs(a + b) < EPS)
        return false;
    
    // G
    double G = b/(a + b);
    
    double denomT = G*c;
    if (fabs(denomT) < EPS)
        return false;
    
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
    
    // Test
    if (fabs(G) < EPS)
        return false;

#if 0 // Method 1: take the left, and adjust for the right
    const double origMagnL = *ioMagnL;
    const double origPhaseL = *ioPhaseL;
    
    // Method "1"
    *ioMagnR = origMagnL/G;
    *ioPhaseR = origPhaseL + phaseDiff;
#endif
    
#if 1 // Method 2: take the middle, and adjust for left and right
    //double diffMagn = (*ioMagnR - *ioMagnL);
    
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

// unused ?
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
