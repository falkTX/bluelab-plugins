//
//  StereoWidthProcess3.cpp
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>
#include <BLUtilsMath.h>

#include <PolarViz.h>
#include <SourcePos.h>

#include <DebugGraph.h>

#include <SourcePos.h>

#include "StereoWidthProcess3.h"

// Debug
#include <GraphControl11.h>


#if 0
BUG: wobble with stereoize (due to fft reconstruction, and wobbling pure frequencies)

IDEA: eat map depending on intensity ?
#endif

// BAD: make the identity sound narrow
//
// Fill missing values for source position
// (But good for display)
#define FILL_MISSING_VALUES 0

// Temporal smoothing of the distances
#define TEST_TEMPORAL_SMOOTHING 0
#define TEMPORAL_DIST_SMOOTH_COEFF 0.9 //0.9999 //0.9 //0.9 //0.5 //0.9

// Spatial smoothing of the distances
// (smooth from near frequencies)
#define TEST_SPATIAL_SMOOTHING 0 // 0
#define SPATIAL_SMOOTHING_WINDOW_SIZE 128

// Interesting:
// (especially temporal smoothing,
// spatial smooothing is not so relevant)
// - shows sources better
// - but removes many temporary points
#define SMOOTH_FOR_DISPLAY 0

// Remove just some values
// If threshold is > -100dB, there is a blank half circle
// at the basis
#define THRESHOLD_DB -120.0

// Maximum width change
// NOTE: the number of correctly computed result
// doesn't change when increasing width !
//
// Origin
//#define MAX_WIDTH_CHANGE 4.0
// Increased !
#define MAX_WIDTH_CHANGE 16.0
#define MAX_WIDTH_CHANGE_STEREOIZE 200.0

// Change mic separation distance, or spread all the source ?
#define CHANGE_MIC_DISTANCE 0 //1

#define PHASES_DIFF_HACK 0

// GOOD at 1: avoids wobbles and sound artifacts
//
// See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
#define SIMPLE_ALGO 1 // Avoids wobble

//#define MAX_WIDTH_CHANGE_SIMPLE_ALGO 4.0
#define MAX_WIDTH_CHANGE_SIMPLE_ALGO 8.0

// NOTE about algos:
// - the first algo, based on source pos, is better for spatialization
// (but it makes vibrations on pure tones ("wobble"), for example
// on a piano note
//
// - the second algo ("simple"), does not make viration,
// but the spatialisation is not as good
//

// NOTE: there was DebugDrawer code here (class definition etc.)


// FIX: On Protools, when width parameter was reset to 0,
// by using alt + click, the gain was increased compared to bypass
//
// On Protools, when alt + click to reset parameter,
// Protools manages all, and does not reset the parameter exactly to the default value
// (precision 1e-8).
// Then the coeff is not exactly 1, then the gain coeff is applied.
#define FIX_WIDTH_PRECISION 1
#define WIDTH_PARAM_PRECISION 1e-6


StereoWidthProcess3::StereoWidthProcess3(int bufferSize,
                                         BL_FLOAT overlapping, BL_FLOAT oversampling,
                                         BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mWidthChange = 0.0;
    
    mFakeStereo = false;

    mDisplayMode = SIMPLE;
        
    GenerateRandomCoeffs(bufferSize/2);
    
    // Smoothing
    mSourceRsHisto =
        new SmoothAvgHistogram(bufferSize/2, TEMPORAL_DIST_SMOOTH_COEFF, 0.0);
    mSourceThetasHisto =
        new SmoothAvgHistogram(bufferSize/2, TEMPORAL_DIST_SMOOTH_COEFF, 0.0);
    
    mProcessNum = 0;
    mDisplayRefreshRate = GetDisplayRefreshRate();

    int dummyWinSize = 5;
    mSmoother = new CMA2Smoother(bufferSize, dummyWinSize);
}

StereoWidthProcess3::~StereoWidthProcess3()
{
    // Smoothing
    delete mSourceRsHisto;
    delete mSourceThetasHisto;

    delete mSmoother;
}

void
StereoWidthProcess3::Reset()
{
    Reset(mBufferSize, mOverlapping, mOversampling, mSampleRate);
}

void
StereoWidthProcess3::Reset(int bufferSize, int overlapping, int oversampling,
                          BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mWidthValuesX.Resize(0);
    mWidthValuesY.Resize(0);
    mColorWeights.Resize(0);
    
    // Smoothing
    mSourceRsHisto->Reset();
    mSourceThetasHisto->Reset();
    
    mProcessNum = 0;
    mDisplayRefreshRate = GetDisplayRefreshRate();
}

void
StereoWidthProcess3::SetWidthChange(BL_FLOAT widthChange)
{
    mWidthChange = widthChange;
}

void
StereoWidthProcess3::SetFakeStereo(bool flag)
{
    mFakeStereo = flag;
}

void
StereoWidthProcess3::ProcessInputSamplesPre(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                            const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer)
{
#if SIMPLE_ALGO
    if (ioSamples->size() < 2)
        return;
    
    if (mFakeStereo)
        // Do nothing
        return;
    
    SimpleStereoWiden(ioSamples, mWidthChange);
#endif
}

void
StereoWidthProcess3::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                                     const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
#define EPS 1e-15
    
    if (ioFftSamples->size() < 2)
        return;
    
    //
    // Prepare the data
    //
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2] = { *(*ioFftSamples)[0], *(*ioFftSamples)[1] };
              
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples[0]);
    BLUtils::TakeHalf(&fftSamples[1]);
    
    WDL_TypedBuf<BL_FLOAT> magns[2];
    WDL_TypedBuf<BL_FLOAT> phases[2];
    
    BLUtilsComp::ComplexToMagnPhase(&magns[0], &phases[0], fftSamples[0]);
    BLUtilsComp::ComplexToMagnPhase(&magns[1], &phases[1], fftSamples[1]);
    
    WDL_TypedBuf<BL_FLOAT> sourceRs;
    WDL_TypedBuf<BL_FLOAT> sourceThetas;
    
    //
    // Apply the effect of the width knob
    //
    if (mFakeStereo)
    {
        Stereoize(&phases[0], &phases[1]);
        
        // Be sure the magns are in mono
        WDL_TypedBuf<BL_FLOAT> monoMagns;
        BLUtils::StereoToMono(&monoMagns, magns[0], magns[1]);
        magns[0] = monoMagns;
        magns[1] = monoMagns;
    }
    else
    {
#if !SIMPLE_ALGO
        ApplyWidthChange(&magns[0], &magns[1], &phases[0], &phases[1],
                         &sourceRs, &sourceThetas);
#endif
    }
    
    // Do not refresh the display at each step !
    // (too CPU costly)
    mProcessNum++;
    
    if (mProcessNum % mDisplayRefreshRate == 0)
    {
        WDL_TypedBuf<BL_FLOAT> freqs;
        BLUtilsFft::FftFreqs(&freqs, phases[0].GetSize(), mSampleRate);
    
        //
        // Prepare polar coordinates for display
        //
    
        // For all display methods
        ComputeColorWeightsMagns(&mColorWeights, magns[0], magns[1]);
    
        // Makes the overall color more homogeneous
        // (make details more appearant ?)
        //ComputeColorWeightsFreqs(&mColorWeights, freqs);
    
        // Choose a display method
        if (mDisplayMode == SIMPLE)
        {
            //
            // Simple: (phase diff, magn)
            //
        
            // Diff
            WDL_TypedBuf<BL_FLOAT> diffPhases;
            BLUtils::Diff(&diffPhases, phases[0], phases[1]);
    
            // Mono magns
            WDL_TypedBuf<BL_FLOAT> monoMagns;
            BLUtils::StereoToMono(&monoMagns, magns[0], magns[1]);
    
            // Be sure we won't go outside the circle at loud volume
            BLUtils::ClipMax(&monoMagns, (BL_FLOAT)1.0);
        
#if SMOOTH_FOR_DISPLAY
            TemporalSmoothing(&monoMagns, &diffPhases);
            SpatialSmoothing(&monoMagns, &diffPhases);
#endif
        
            // Display the phase diff
            PolarViz::PolarToCartesian(monoMagns, diffPhases,
                                       &mWidthValuesX, &mWidthValuesY, THRESHOLD_DB);

            // Reverse X, to have correct Left/Right orientation when displaying
            BLUtils::MultValues(&mWidthValuesX, (BL_FLOAT)-1.0);
        
        } else if (((mDisplayMode == SOURCE) || (mDisplayMode == SCANNER)) &&
                   // Don't display modes source or scanner when in fake stereo
                   // (because it displays badly
                   !mFakeStereo)
        {
            WDL_TypedBuf<BL_FLOAT> timeDelays;
            BLUtils::ComputeTimeDelays(&timeDelays, phases[0], phases[1], mSampleRate);
        
            SourcePos::MagnPhasesToSourcePos(&sourceRs, &sourceThetas,
                                             magns[0],  magns[1],
                                             phases[0], phases[1],
                                             freqs,
                                             timeDelays);
        
            // GOOD with float time delays
        
            // Fill missing values (but only for display) !
            BLUtils::FillMissingValues(&sourceRs, true, (BL_FLOAT)UTILS_VALUE_UNDEFINED);
            BLUtils::FillMissingValues(&sourceThetas, true, (BL_FLOAT)UTILS_VALUE_UNDEFINED);

            // Just in case
            BLUtils::ReplaceValue(&sourceThetas, (BL_FLOAT)UTILS_VALUE_UNDEFINED, (BL_FLOAT)0.0);
        
            WDL_TypedBuf<BL_FLOAT> thetasRot = sourceThetas;
            BLUtils::AddValues(&thetasRot, (BL_FLOAT)(-M_PI/2.0));
        
            if (mDisplayMode == SOURCE)
            {
                //
                // Source: (direction, distance)
                //
            
                // R / theta
                BLUtils::ReplaceValue(&sourceRs, (BL_FLOAT)UTILS_VALUE_UNDEFINED, (BL_FLOAT)0.0);
                NormalizeRs(&sourceRs, true);
            
#if SMOOTH_FOR_DISPLAY
                TemporalSmoothing(&sourceRs, &thetasRot);
                SpatialSmoothing(&sourceRs, &thetasRot);
#endif

                PolarViz::PolarToCartesian2(sourceRs, thetasRot,
                                            &mWidthValuesX, &mWidthValuesY);
                
            } else if (mDisplayMode == SCANNER)
            {
                //
                // Scanner: (direction, magn)
                //
            
                // magnitude / azimuth
                WDL_TypedBuf<BL_FLOAT> monoMagns;
                BLUtils::StereoToMono(&monoMagns, magns[0], magns[1]);
            
                WDL_TypedBuf<BL_FLOAT> monoMagnsDB;
                BLUtils::AmpToDBNorm(&monoMagnsDB, monoMagns, (BL_FLOAT)1e-15, (BL_FLOAT)-120.0);
            
                // Discard magnitudes where source R is not defined
                // This avoids an horizontal line in the display, when width = 0
                for (int i = 0; i < sourceRs.GetSize(); i++)
                {
                    BL_FLOAT r = sourceRs.Get()[i];
                    if (r < EPS)
                        monoMagnsDB.Get()[i] = 0.0;
                }
            
#if SMOOTH_FOR_DISPLAY
                TemporalSmoothing(&monoMagnsDB, &thetasRot);
                SpatialSmoothing(&monoMagnsDB, &thetasRot);
#endif
            
                PolarViz::PolarToCartesian2(monoMagnsDB, thetasRot,
                                            &mWidthValuesX, &mWidthValuesY);
            }
        
            // Reverse X, to have correct Left/Right orientation when displaying
            BLUtils::MultValues(&mWidthValuesX, (BL_FLOAT)-1.0);
        }
    
        if (((mDisplayMode == SOURCE) || (mDisplayMode == SCANNER)) &&
            // Don't display modes source or scanner when in fake stereo
            // (because it displays badly
            mFakeStereo)
        {
            // Clear the graph
            // To avoid that the last values keeps displaying.
            // (thas could be confusing for the user)
            BLUtils::FillAllValue(&mWidthValuesX, (BL_FLOAT)0.0);
            BLUtils::FillAllValue(&mWidthValuesY, (BL_FLOAT)0.0);
        
            BLUtils::FillAllValue(&mColorWeights, (BL_FLOAT)0.0);
        }
    }
    
    //
    // Re-synthetise the data with the new diff
    //
    BLUtilsComp::MagnPhaseToComplex(&fftSamples[0], magns[0], phases[0]);
    fftSamples[0].Resize(fftSamples[0].GetSize()*2);
    
    BLUtilsComp::MagnPhaseToComplex(&fftSamples[1], magns[1], phases[1]);
    fftSamples[1].Resize(fftSamples[1].GetSize()*2);
    
#if 0
    // TEST: try to avoid transforming the first bin (0Hz)
    //fftSamples[0].Get()[0] = (*ioFftSamples)[0]->Get()[0];
    //fftSamples[1].Get()[0] = (*ioFftSamples)[1]->Get()[0];
#endif
    
    BLUtilsFft::FillSecondFftHalf(&fftSamples[0]);
    BLUtilsFft::FillSecondFftHalf(&fftSamples[1]);
    
    // Result
    *(*ioFftSamples)[0] = fftSamples[0];
    *(*ioFftSamples)[1] = fftSamples[1];
}

void
StereoWidthProcess3::GetWidthValues(WDL_TypedBuf<BL_FLOAT> *xValues,
                                    WDL_TypedBuf<BL_FLOAT> *yValues,
                                    WDL_TypedBuf<BL_FLOAT> *outColorWeights)
{
    *xValues = mWidthValuesX;
    *yValues = mWidthValuesY;
    
    if (outColorWeights != NULL)
        *outColorWeights = mColorWeights;
}

void
StereoWidthProcess3::SetDisplayMode(enum DisplayMode mode)
{
    mDisplayMode = mode;
}

void
StereoWidthProcess3::ApplyWidthChange(WDL_TypedBuf<BL_FLOAT> *ioMagnsL,
                                      WDL_TypedBuf<BL_FLOAT> *ioMagnsR,
                                      WDL_TypedBuf<BL_FLOAT> *ioPhasesL,
                                      WDL_TypedBuf<BL_FLOAT> *ioPhasesR,
                                      WDL_TypedBuf<BL_FLOAT> *outSourceRs,
                                      WDL_TypedBuf<BL_FLOAT> *outSourceThetas)
{
    WDL_TypedBuf<BL_FLOAT> freqs;
    BLUtilsFft::FftFreqs(&freqs, ioPhasesL->GetSize(), mSampleRate);
    
    BL_FLOAT widthFactor = ComputeFactor(mWidthChange, MAX_WIDTH_CHANGE);
    
    WDL_TypedBuf<BL_FLOAT> timeDelays;
    BLUtils::ComputeTimeDelays(&timeDelays, *ioPhasesL, *ioPhasesR, mSampleRate);
    
    // Compute left and right distances
    outSourceRs->Resize(ioPhasesL->GetSize());
    outSourceThetas->Resize(ioPhasesL->GetSize());
    
    // Get source position (polar coordinates)
    SourcePos::MagnPhasesToSourcePos(outSourceRs, outSourceThetas,
                                     *ioMagnsL, *ioMagnsR,
                                     *ioPhasesL, *ioPhasesL,
                                     freqs,
                                     timeDelays);
    
    // Use phase diff instead of azimuth ?
#if PHASES_DIFF_HACK
    // Diff
    WDL_TypedBuf<BL_FLOAT> diffPhases;
    BLUtils::Diff(&diffPhases, *ioPhasesL, *ioPhasesR);
    
    *outSourceThetas = diffPhases;
#endif
    
    // Fill missing & Smooth ?

    // BAD
#if FILL_MISSING_VALUES
    BLUtils::FillMissingValues(outSourceRs, true, UTILS_VALUE_UNDEFINED);
    BLUtils::FillMissingValues(outSourceThetas, true, UTILS_VALUE_UNDEFINED);
#endif
    
#if TEST_TEMPORAL_SMOOTHING
    TemporalSmoothing(outSourceRs, outSourceThetas);
#endif

#if TEST_SPATIAL_SMOOTHING
    SpatialSmoothing(outSourceRs, outSourceThetas);
#endif
    
    BL_FLOAT micSep = SourcePos::GetDefaultMicSeparation();
    
#if CHANGE_MIC_DISTANCE
    // Modify the width (mics distance)
    BL_FLOAT newMicsDistance = widthFactor*micSep;
#endif

#if !CHANGE_MIC_DISTANCE
    // Keep the mics distance and change the sources angle
    BL_FLOAT newMicsDistance = micSep;
    
    // Change sources angle
    for (int i = 0; i < outSourceThetas->GetSize(); i++)
    {
        BL_FLOAT theta = outSourceThetas->Get()[i];

        theta *= widthFactor;
        
        outSourceThetas->Get()[i] = theta;
    }
#endif

    // Retrieve magn and phase from azimuth and magns
    SourcePos::SourcePosToMagnPhases(*outSourceRs, *outSourceThetas,
                                     newMicsDistance,
                                     widthFactor,
                                     ioMagnsL, ioMagnsR,
                                     ioPhasesL, ioPhasesR,
                                     freqs,
                                     timeDelays);
}

void
StereoWidthProcess3::Stereoize(WDL_TypedBuf<BL_FLOAT> *ioPhasesL,
                               WDL_TypedBuf<BL_FLOAT> *ioPhasesR)

{
    if (ioPhasesL->GetSize() != ioPhasesR->GetSize())
    // We could have empty R
        return;
    
    // Strange method, but that works
    // Uses random numbers
    
    // Good, to be at the maximum when the width increase is at maximum
    // With more, we get strange effect
    // when the circle is full and we pull more the width increase
    BL_FLOAT stereoizeCoeff =  M_PI/MAX_WIDTH_CHANGE_STEREOIZE;
    BL_FLOAT widthChange =
        BLUtils::FactorToDivFactor(mWidthChange, (BL_FLOAT)MAX_WIDTH_CHANGE_STEREOIZE);
    
    for (int i = 0; i < ioPhasesL->GetSize(); i++)
    {
        BL_FLOAT phaseL = ioPhasesL->Get()[i];
        BL_FLOAT phaseR = ioPhasesR->Get()[i];
        
        BL_FLOAT middle = (phaseL + phaseR)/2.0;

        BL_FLOAT rnd = mRandomCoeffs.Get()[i];
        
        // A bit strange...
        BL_FLOAT diff = stereoizeCoeff*rnd;
        diff *= widthChange;
        
        BL_FLOAT newPhaseL = middle - diff/2.0;
        BL_FLOAT newPhaseR = middle + diff/2.0;
        
        ioPhasesL->Get()[i] = newPhaseL;
        ioPhasesR->Get()[i] = newPhaseR;
    }
}

void
StereoWidthProcess3::GenerateRandomCoeffs(int size)
{
    srand(114242111);
    
    mRandomCoeffs.Resize(size);
    for (int i = 0; i < mRandomCoeffs.GetSize(); i++)
    {
        BL_FLOAT rnd0 = 1.0 - 2.0*((BL_FLOAT)rand())/RAND_MAX;
        
        mRandomCoeffs.Get()[i] = rnd0;
    }
}

BL_FLOAT
StereoWidthProcess3::ComputeFactor(BL_FLOAT normVal, BL_FLOAT maxVal)
{
    BL_FLOAT res = 0.0;
    if (normVal < 0.0)
        res = normVal + 1.0;
    else
        res = normVal*(maxVal - 1.0) + 1.0;

    return res;
}

void
StereoWidthProcess3::TemporalSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
                                      WDL_TypedBuf<BL_FLOAT> *sourceThetas)
{
    // Left
    mSourceRsHisto->AddValues(*sourceRs);
    mSourceRsHisto->GetValues(sourceRs);
    
    // Right
    mSourceThetasHisto->AddValues(*sourceThetas);
    mSourceThetasHisto->GetValues(sourceThetas);
}

void
StereoWidthProcess3::SpatialSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
                                     WDL_TypedBuf<BL_FLOAT> *sourceThetas)
{
    int windowSize = sourceRs->GetSize()/SPATIAL_SMOOTHING_WINDOW_SIZE;
    
    WDL_TypedBuf<BL_FLOAT> smoothSourceRs;
    mSmoother->ProcessOne(*sourceRs, &smoothSourceRs, windowSize);
    *sourceRs = smoothSourceRs;
    
    WDL_TypedBuf<BL_FLOAT> smoothSourceThetas;
    mSmoother->ProcessOne(*sourceThetas, &smoothSourceThetas, windowSize);
    *sourceThetas = smoothSourceThetas;
}

void
StereoWidthProcess3::NormalizeRs(WDL_TypedBuf<BL_FLOAT> *sourceRs,
                                bool discardClip)
{
    // 100 m
#define MAX_DIST 100.0
    
    for (int i = 0; i < sourceRs->GetSize(); i++)
    {
        BL_FLOAT r = sourceRs->Get()[i];

        r /= MAX_DIST;
        
        if (r > 1.0)
        {
            if (discardClip)
                // discard too big r, to avoid acumulation
                // in the border of the circle
                r = 0.0;
            else
                r = 1.0;
        }
        
        r = BLUtils::ApplyParamShape(r, (BL_FLOAT)4.0);
        
        sourceRs->Get()[i] = r;
    }
}

void
StereoWidthProcess3::ComputeColorWeightsMagns(WDL_TypedBuf<BL_FLOAT> *outWeights,
                                             const WDL_TypedBuf<BL_FLOAT> &magnsL,
                                             const WDL_TypedBuf<BL_FLOAT> &magnsR)
{
    // Mono
    WDL_TypedBuf<BL_FLOAT> monoMagns;
    BLUtils::StereoToMono(&monoMagns, magnsL, magnsR);
    
    // dB
    WDL_TypedBuf<BL_FLOAT> monoMagnsDB;
    BLUtils::AmpToDBNorm(&monoMagnsDB, monoMagns, (BL_FLOAT)1e-15, (BL_FLOAT)-120.0);
    
    // Coeffs
    BLUtils::MultValues(&monoMagnsDB, (BL_FLOAT)4.0);
    
    *outWeights = monoMagnsDB;
}

void
StereoWidthProcess3::ComputeColorWeightsFreqs(WDL_TypedBuf<BL_FLOAT> *outWeights,
                                             const WDL_TypedBuf<BL_FLOAT> &freqs)
{
    outWeights->Resize(freqs.GetSize());
    
    for (int i = 0; i < outWeights->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/outWeights->GetSize();
    
        // Logical to use log since frequency perception is logarithmic
        t = BLUtils::AmpToDBNorm(t, (BL_FLOAT)1e-15, (BL_FLOAT)-120.0);
        
        BL_FLOAT w = (t + 1.0)/2.0;
        
        //w = BLUtils::ApplyParamShape(w, 4.0);
        w *= 2.0;
        
        outWeights->Get()[i] = w;
    }
}

void
StereoWidthProcess3::SimpleStereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples, BL_FLOAT widthChange)
{
    BL_FLOAT width = ComputeFactor(widthChange, MAX_WIDTH_CHANGE_SIMPLE_ALGO);
    
#if FIX_WIDTH_PRECISION
    width /= WIDTH_PARAM_PRECISION;
    width = bl_round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        BL_FLOAT in_left = (*ioSamples)[0]->Get()[i];
        BL_FLOAT in_right = (*ioSamples)[1]->Get()[i];
        
#if 0 // Initial version (goniometer)
        
        // Calculate scale coefficient
        BL_FLOAT coef_S = width*0.5;
        
        BL_FLOAT m = (in_left + in_right)*0.5;
        BL_FLOAT s = (in_right - in_left )*coef_S;
        
        BL_FLOAT out_left = m - s;
        BL_FLOAT out_right = m + s;
        
#if 0 // First correction of volume (not the best)
        out_left  /= 0.5 + coef_S;
        out_right /= 0.5 + coef_S;
#endif
        
#endif
     
    // GOOD (loose a bit some volume when increasing width)
#if 1 // Volume adjusted version
        // Calc coefs
        BL_FLOAT tmp = 1.0/MAX(1.0 + width, 2.0);
        BL_FLOAT coef_M = 1.0 * tmp;
        BL_FLOAT coef_S = width * tmp;
        
        // Then do this per sample
        BL_FLOAT m = (in_left + in_right)*coef_M;
        BL_FLOAT s = (in_right - in_left )*coef_S;
        
        BL_FLOAT out_left = m - s;
        BL_FLOAT out_right = m + s;
#endif
       
#if 1 // Test Niko (adjust final volume)
        if (width > 1.0)
        {
            // Works well with MAX_WIDTH = 4
            // Makes a small volume increase near 1,
            // just after 1
            BL_FLOAT coeff = 1.3;
            
            out_left *= coeff;
            out_right *= coeff;
        }
#endif
        
        (*ioSamples)[0]->Get()[i] = out_left;
        (*ioSamples)[1]->Get()[i] = out_right;
    }
}

int
StereoWidthProcess3::GetDisplayRefreshRate()
{
    int refreshRate = mOverlapping;
    
    return refreshRate;
}
