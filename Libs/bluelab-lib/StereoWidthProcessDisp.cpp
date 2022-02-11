//
//  StereoWidthProcessDisp.cpp
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

#include "StereoWidthProcessDisp.h"

// Temporal smoothing of the distances
#define TEMPORAL_DIST_SMOOTH_COEFF 0.9 //0.9999 //0.9 //0.9 //0.5 //0.9

// Spatial smoothing of the distances
// (smooth from near frequencies)
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

#define FIX_MONO 1

StereoWidthProcessDisp::StereoWidthProcessDisp(int bufferSize,
                                               BL_FLOAT overlapping,
                                               BL_FLOAT oversampling,
                                               BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    // Smoothing
    mSourceRsHisto = new SmoothAvgHistogram(bufferSize/2, TEMPORAL_DIST_SMOOTH_COEFF, 0.0);
    mSourceThetasHisto = new SmoothAvgHistogram(bufferSize/2, TEMPORAL_DIST_SMOOTH_COEFF, 0.0);
    
    mProcessNum = 0;
    mDisplayRefreshRate = GetDisplayRefreshRate();
    
    // NEW
    //mDisplayMode = SOURCE;
    mDisplayMode = SCANNER;

    int dummyWinSize = 5;
    mSmoother = new CMA2Smoother(bufferSize, dummyWinSize);
}

StereoWidthProcessDisp::~StereoWidthProcessDisp()
{
    // Smoothing
    delete mSourceRsHisto;
    delete mSourceThetasHisto;

    delete mSmoother;
}

void
StereoWidthProcessDisp::Reset()
{
    Reset(mBufferSize, mOverlapping, mOversampling, mSampleRate);
}

void
StereoWidthProcessDisp::Reset(int bufferSize, int overlapping, int oversampling,
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
StereoWidthProcessDisp::SetDisplayMode(enum DisplayMode mode)
{
    mDisplayMode = mode;
}

void
StereoWidthProcessDisp::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
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
        
        } else if ((mDisplayMode == SOURCE) || (mDisplayMode == SCANNER))
        {
            WDL_TypedBuf<BL_FLOAT> timeDelays;
            BLUtils::ComputeTimeDelays(&timeDelays, phases[0], phases[1], mSampleRate);

            SourcePos::MagnPhasesToSourcePos(&sourceRs, &sourceThetas,
                                             magns[0],  magns[1],
                                             phases[0], phases[1],
                                             freqs,
                                             timeDelays);
        
#if FIX_MONO
            for (int i = 0; i < magns[0].GetSize(); i++)
            {
                BL_FLOAT l = magns[0].Get()[i];
                BL_FLOAT r = magns[1].Get()[i];
                
                BL_FLOAT pl = phases[0].Get()[i];
                BL_FLOAT pr = phases[1].Get()[i];
                
                if (std::fabs(l - r) > EPS)
                    continue;
                
                if (std::fabs(pl - pr) > EPS)
                    continue;
                
                // Here, we are in mono
                
                // Compute some sort of radius (so we will have a line in the sterewodth display)
                sourceRs.Get()[i] = (l + r)*0.5;
                
                // Set a theta, so just after the rotation below, it will be 0
                sourceThetas.Get()[i] = M_PI/2.0;
            }
#endif
            
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
                
                // Test flat
                //mWidthValuesX = sourceRs;
                //mWidthValuesY = thetasRot;
                //BLUtils::PolarToCartesianFlat(&mWidthValuesX, &mWidthValuesY);
            }
        
            // Reverse X, to have correct Left/Right orientation when displaying
            BLUtils::MultValues(&mWidthValuesX, (BL_FLOAT)-1.0);
        }
    }
}

void
StereoWidthProcessDisp::GetWidthValues(WDL_TypedBuf<BL_FLOAT> *xValues,
                                       WDL_TypedBuf<BL_FLOAT> *yValues,
                                       WDL_TypedBuf<BL_FLOAT> *outColorWeights)
{
    *xValues = mWidthValuesX;
    *yValues = mWidthValuesY;
    
    if (outColorWeights != NULL)
        *outColorWeights = mColorWeights;
}

void
StereoWidthProcessDisp::TemporalSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
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
StereoWidthProcessDisp::SpatialSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
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
StereoWidthProcessDisp::NormalizeRs(WDL_TypedBuf<BL_FLOAT> *sourceRs,
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
StereoWidthProcessDisp::ComputeColorWeightsMagns(WDL_TypedBuf<BL_FLOAT> *outWeights,
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
StereoWidthProcessDisp::ComputeColorWeightsFreqs(WDL_TypedBuf<BL_FLOAT> *outWeights,
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

int
StereoWidthProcessDisp::GetDisplayRefreshRate()
{
    int refreshRate = mOverlapping;
    
    return refreshRate;
}
