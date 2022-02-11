//
//  StereoVizProcess3.cpp
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#include <BLUtils.h>
#include <PolarViz.h>
#include <SourcePos.h>

#include <DebugGraph.h>

#include <SourcePos.h>

#include <StereoVizVolRender3.h>

#include <StereoWidthProcess3.h>

#include "StereoVizProcess3.h"

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
//
// NOTE: seems to look better for "source" mode
//
#define TEST_SPATIAL_SMOOTHING 1 // 0
#define SPATIAL_SMOOTHING_WINDOW_SIZE 128 // 512

// Remove just some values
// If threshold is > -100dB, there is a blank half circle
// at the basis
#define THRESHOLD_DB -120.0

#define PHASES_DIFF_HACK 0

// Down-scale a little the points distance in source mode
// Otherwise they go a bit outside the screen in volume rendering
#define SOURCE_MODE_SCALE 0.8


// Method that compute resulting sound, depen ding on the selection
//
// NOTE: strangly, the sound resynthesis seems to sound better when it is at 0
//
#define FORCE_COMPUTE_RESULT_SOUND 1 //0 //1

// Set to 0 to erase fully
#define UNSELECTED_FACTOR 0.25
#define SELECTED_FACTOR 8.0

#define Y_LOG_SCALE_FACTOR 3.5


#define SPECTRO_HEIGHT_COEFF 0.7

#define USE_FLAT_POLAR 0 //1
#define USE_FLAT_POLAR_SPECTRO 1 //0


StereoVizProcess3::StereoVizProcess3(int bufferSize,
                                     BL_FLOAT overlapping, BL_FLOAT oversampling,
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
    
    mVolRender = NULL;
    
    mDisplayMode = SIMPLE;
    
    mAngularMode = false;
    
    mStereoWidth = 0.0;
}

StereoVizProcess3::~StereoVizProcess3()
{
    // Smoothing
    delete mSourceRsHisto;
    delete mSourceThetasHisto;
}

void
StereoVizProcess3::Reset()
{
    Reset(mOverlapping, mOversampling, mSampleRate);
}

void
StereoVizProcess3::Reset(int overlapping, int oversampling,
                         BL_FLOAT sampleRate)
{
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
#if 0
    mWidthValuesX.Resize(0);
    mWidthValuesY.Resize(0);
    mColorWeights.Resize(0);
#endif
    
    // Smoothing
    mSourceRsHisto->Reset();
    mSourceThetasHisto->Reset();
    
    mProcessNum = 0;
    mDisplayRefreshRate = GetDisplayRefreshRate();
    
    if (mVolRender != NULL)
        mVolRender->SetDisplayRefreshRate(mDisplayRefreshRate);
}

void
StereoVizProcess3::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
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
    
    long numSlices = mVolRender->GetNumSlices();
    for (int i = 0; i < 2; i++)
    {
        mBufs[i].push_back(fftSamples[i]);
        
        if (mBufs[i].size() > numSlices)
            mBufs[i].pop_front();
    }
    
    // Not optimized
    ApplyStereoWidth(fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns[2];
    WDL_TypedBuf<BL_FLOAT> phases[2];
    
    BLUtils::ComplexToMagnPhase(&magns[0], &phases[0], fftSamples[0]);
    BLUtils::ComplexToMagnPhase(&magns[1], &phases[1], fftSamples[1]);
    
    // Do not refresh the display at each step !
    // (too CPU costly)
    mProcessNum++;
    
    // When computing result sound, must get the result at each step
    // (and then the point flags at each step).
    if ((mProcessNum % mDisplayRefreshRate == 0) || FORCE_COMPUTE_RESULT_SOUND)
    {
        WDL_TypedBuf<BL_FLOAT> widthValuesX;
        WDL_TypedBuf<BL_FLOAT> widthValuesY;
        WDL_TypedBuf<BL_FLOAT> colorWeights;
        
        ComputeResult(magns, phases, &widthValuesX, &widthValuesY, &colorWeights);
        
        if (mVolRender != NULL)
        {
            mVolRender->AddCurveValuesWeight(widthValuesX,
                                             widthValuesY,
                                             colorWeights);
        }
    }
    
    //
    // Re-synthetise the data with the new diff
    //
    BLUtils::MagnPhaseToComplex(&fftSamples[0], magns[0], phases[0]);
    BLUtils::MagnPhaseToComplex(&fftSamples[1], magns[1], phases[1]);
    
    // Get the center of selection in the RayCaster
    GetSelectedData(fftSamples);
    
    fftSamples[0].Resize(fftSamples[0].GetSize()*2);
    fftSamples[1].Resize(fftSamples[1].GetSize()*2);
    
    BLUtils::FillSecondFftHalf(&fftSamples[0]);
    BLUtils::FillSecondFftHalf(&fftSamples[1]);
    
    // Result
    *(*ioFftSamples)[0] = fftSamples[0];
    *(*ioFftSamples)[1] = fftSamples[1];
}

#if 0
void
StereoVizProcess3::GetWidthValues(WDL_TypedBuf<BL_FLOAT> *xValues,
                                  WDL_TypedBuf<BL_FLOAT> *yValues,
                                  WDL_TypedBuf<BL_FLOAT> *outColorWeights)
{
    *xValues = mWidthValuesX;
    *yValues = mWidthValuesY;
    
    if (outColorWeights != NULL)
        *outColorWeights = mColorWeights;
}
#endif

void
StereoVizProcess3::SetDisplayMode(enum DisplayMode mode)
{
    mDisplayMode = mode;
    
    RecomputeResult();
}

void
StereoVizProcess3::SetAngularMode(bool flag)
{
    mAngularMode = flag;
    
    RecomputeResult();
}

void
StereoVizProcess3::SetStereoWidth(BL_FLOAT stereoWidth, bool recomputeAll)
{
    mStereoWidth = stereoWidth;
    
    if (recomputeAll)
        RecomputeResult();
}

void
StereoVizProcess3::SetVolRender(StereoVizVolRender3 *volRender)
{
    mVolRender = volRender;
    
    if (mVolRender != NULL)
    {
        mVolRender->SetDisplayRefreshRate(mDisplayRefreshRate);
    }
}

void
StereoVizProcess3::TemporalSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
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
StereoVizProcess3::SpatialSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
                                    WDL_TypedBuf<BL_FLOAT> *sourceThetas)
{
    int windowSize = sourceRs->GetSize()/SPATIAL_SMOOTHING_WINDOW_SIZE;
    
    WDL_TypedBuf<BL_FLOAT> smoothSourceRs;
    CMA2Smoother::ProcessOne(*sourceRs, &smoothSourceRs, windowSize);
    *sourceRs = smoothSourceRs;
    
    WDL_TypedBuf<BL_FLOAT> smoothSourceThetas;
    CMA2Smoother::ProcessOne(*sourceThetas, &smoothSourceThetas, windowSize);
    *sourceThetas = smoothSourceThetas;
}

void
StereoVizProcess3::NormalizeRs(WDL_TypedBuf<BL_FLOAT> *sourceRs,
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
        
        r = BLUtils::ApplyParamShape(r, 4.0);
        
        sourceRs->Get()[i] = r;
    }
}

void
StereoVizProcess3::ComputeColorWeightsMagns(WDL_TypedBuf<BL_FLOAT> *outWeights,
                                            const WDL_TypedBuf<BL_FLOAT> &magnsL,
                                            const WDL_TypedBuf<BL_FLOAT> &magnsR)
{
    // Mono
    WDL_TypedBuf<BL_FLOAT> monoMagns;
    BLUtils::StereoToMono(&monoMagns, magnsL, magnsR);
    
    // dB
    WDL_TypedBuf<BL_FLOAT> monoMagnsDB;
    BLUtils::AmpToDBNorm(&monoMagnsDB, monoMagns, 1e-15, -120.0);
    
#if 0 // NOTE: was not good
      // Saturated the colormap a lot
    // Coeffs
    BLUtils::MultValues(&monoMagnsDB, 4.0);
#endif
    
    *outWeights = monoMagnsDB;
}

void
StereoVizProcess3::ComputeColorWeightsFreqs(WDL_TypedBuf<BL_FLOAT> *outWeights,
                                             const WDL_TypedBuf<BL_FLOAT> &freqs)
{
    outWeights->Resize(freqs.GetSize());
    
    for (int i = 0; i < outWeights->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/outWeights->GetSize();
    
        // Logical to use log since frequency preception is logarithmic
        t = BLUtils::AmpToDBNorm(t, 1e-15, -120.0);
        
        BL_FLOAT w = (t + 1.0)/2.0;
        
        //w = BLUtils::ApplyParamShape(w, 4.0);
        w *= 2.0;
        
        outWeights->Get()[i] = w;
    }
}

int
StereoVizProcess3::GetDisplayRefreshRate()
{
    // Force return 1, to be able to catch all the buffers
    // to re-play a selection
    return 1;
    
    //int refreshRate = mOverlapping;
    //return refreshRate;
}


#if 0
void
StereoVizProcess3::DiscardUnselected(WDL_TypedBuf<BL_FLOAT> *magns0,
                                     WDL_TypedBuf<BL_FLOAT> *magns1)
{
   if (mVolRender != NULL)
   {
       vector<bool> pointFlags;
       mVolRender->GetPointsSelection(&pointFlags);
       
       if (pointFlags.size() != magns0->GetSize())
           return;
       
       for (int i = 0; i < magns0->GetSize(); i++)
       {
           bool flag = pointFlags[i];
           
           if (!flag)
           {
               magns0->Get()[i] *= UNSELECTED_FACTOR;
               magns1->Get()[i] *= UNSELECTED_FACTOR;
           }
       }
   }
}

// Balance is in [-1.0, 1.0]
void
StereoVizProcess3::DiscardUnselectedStereo(WDL_TypedBuf<BL_FLOAT> *magns0,
                                           WDL_TypedBuf<BL_FLOAT> *magns1,
                                           const WDL_TypedBuf<BL_FLOAT> &balances)
{
    if (mVolRender != NULL)
    {
        vector<bool> pointFlags;
        mVolRender->GetPointsSelection(&pointFlags);
        
        if (pointFlags.size() != magns0->GetSize())
            return;
        
        for (int i = 0; i < magns0->GetSize(); i++)
        {
            bool flag = pointFlags[i];
            if (flag)
            {
                BL_FLOAT balance = balances.Get()[i];
                
                // Normalize
                balance = (balance + 1.0)/2.0;
                
                magns0->Get()[i] *= SELECTED_FACTOR;
                magns0->Get()[i] *= 1.0 - balance;
                
                magns1->Get()[i] *= SELECTED_FACTOR;
                magns1->Get()[i] *= balance;
            }
        }
    }
}
#endif

void
StereoVizProcess3::ComputeSpectroY(WDL_TypedBuf<BL_FLOAT> *yValues, int numValues)
{
    yValues->Resize(numValues);
    
    for (int i = 0; i < yValues->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/yValues->GetSize();
        
        t = BLUtils::LogScaleNormInv(t, 1.0, Y_LOG_SCALE_FACTOR);
        
        yValues->Get()[i] = t*SPECTRO_HEIGHT_COEFF;
    }
}

void
StereoVizProcess3::GetSelectedData(WDL_TypedBuf<WDL_FFT_COMPLEX> ioFftSamples[2])
{
    // If not selection, play all
    if (!mVolRender->SelectionEnabled())
        return;
    
    vector<bool> selection;
    vector<BL_FLOAT> xCoords;
    
    long sliceNum;
    bool found = mVolRender->GetCenterSliceSelection(&selection, &xCoords, &sliceNum);
    if (!found)
        // Empty selection
    {
        // Make silence
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < ioFftSamples[i].GetSize(); j++)
            {
                // Mute the fft sample
                WDL_FFT_COMPLEX &val = ioFftSamples[i].Get()[j];
            
                val.re = 0.0;
                val.im = 0.0;
            
                ioFftSamples[i].Get()[j] = val; // Useless
            }
        }
        
        return;
    }
    
    if (selection.size() != ioFftSamples[0].GetSize())
        // Just in case
        return;
    
    if (sliceNum >= mBufs[0].size())
        // Just in case
        return;
    
    for (int i = 0; i < 2; i++)
    {
        const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf = mBufs[i][sliceNum];
        ioFftSamples[i] = buf;
        
        if (buf.GetSize() != selection.size())
            // Just in case
            break;
        
        for (int j = 0; j < selection.size(); j++)
        {
            bool selected = selection[j];
            
            // If selected, keep the sound
            if (selected)
                continue;
            
#if 0 // HACK
      // => makes many metallic sounds
            BL_FLOAT x = xCoords[j];
            
            // Left/right
            //
            // => doesn't make a neat left/right separation
            // but makes a light left/right separation
            // (because left and right voxels doesn't match strictly
            // with left/right channels)
            //
            if ((i == 0) && (x >= 0.0))
                continue;
            
            if ((i == 1) && (x <= 0.0))
                continue;
#endif
            
            // Mute the fft sample
            
            WDL_FFT_COMPLEX &val = ioFftSamples[i].Get()[j];
            
            val.re = 0.0;
            val.im = 0.0;
            
            ioFftSamples[i].Get()[j] = val; // Useless
        }
    }
}

void
StereoVizProcess3::ComputeResult(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                 const WDL_TypedBuf<BL_FLOAT> phases[2],
                                 WDL_TypedBuf<BL_FLOAT> *widthValuesX,
                                 WDL_TypedBuf<BL_FLOAT> *widthValuesY,
                                 WDL_TypedBuf<BL_FLOAT> *colorWeights)
{
    WDL_TypedBuf<BL_FLOAT> freqs;
    BLUtils::FftFreqs(&freqs, phases[0].GetSize(), mSampleRate);
    
    //
    // Prepare polar coordinates for display
    //
    
    // For all display methods
    ComputeColorWeightsMagns(colorWeights, magns[0], magns[1]);
    
    // Makes the overall color more homogeneous
    // (make details more appearant ?)
    //ComputeColorWeightsFreqs(colorWeights, freqs);
    
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
        BLUtils::ClipMax(&monoMagns, 1.0);
        
#if TEST_TEMPORAL_SMOOTHING
        TemporalSmoothing(&monoMagns, &diffPhases);
#endif
        
#if TEST_SPATIAL_SMOOTHING
        SpatialSmoothing(&monoMagns, &diffPhases);
#endif
        
        // Display the phase diff
        PolarViz::PolarToCartesian(monoMagns, diffPhases,
                                   widthValuesX, widthValuesY, THRESHOLD_DB);
        
        // Reverse X, to have correct Left/Right orientation when displaying
        BLUtils::MultValues(widthValuesX, -1.0);
        
    } else if ((mDisplayMode == SOURCE) || (mDisplayMode == SCANNER))
    {
        WDL_TypedBuf<BL_FLOAT> sourceRs;
        WDL_TypedBuf<BL_FLOAT> sourceThetas;
        
        WDL_TypedBuf<BL_FLOAT> timeDelays;
        BLUtils::ComputeTimeDelays(&timeDelays, phases[0], phases[1], mSampleRate);
        
        SourcePos::MagnPhasesToSourcePos(&sourceRs, &sourceThetas,
                                         magns[0],  magns[1],
                                         phases[0], phases[1],
                                         freqs,
                                         timeDelays);
        
        // GOOD with float time delays
        
        // Fill missing values (but only for display) !
        BLUtils::FillMissingValues(&sourceRs, true, UTILS_VALUE_UNDEFINED);
        BLUtils::FillMissingValues(&sourceThetas, true, UTILS_VALUE_UNDEFINED);
        
        // Just in case
        BLUtils::ReplaceValue(&sourceThetas, UTILS_VALUE_UNDEFINED, 0.0);
        
        WDL_TypedBuf<BL_FLOAT> thetasRot = sourceThetas;
        BLUtils::AddValues(&thetasRot, -M_PI/2.0);
        
        if (mDisplayMode == SOURCE)
        {
            //
            // Source: (direction, distance)
            //
            
            // R / theta
            BLUtils::ReplaceValue(&sourceRs, UTILS_VALUE_UNDEFINED, 0.0);
            NormalizeRs(&sourceRs, true);
            
#if TEST_TEMPORAL_SMOOTHING
            TemporalSmoothing(&sourceRs, &thetasRot);
#endif
            
#if TEST_SPATIAL_SMOOTHING
            SpatialSmoothing(&sourceRs, &thetasRot);
#endif
            
            for (int i = 0; i < sourceRs.GetSize(); i++)
            {
                BL_FLOAT r = sourceRs.Get()[i];
                r *= SOURCE_MODE_SCALE;
                
                sourceRs.Get()[i] = r;
            }
            
            PolarViz::PolarToCartesian2(sourceRs, thetasRot,
                                        widthValuesX, widthValuesY);
            
        } else if (mDisplayMode == SCANNER)
        {
            //
            // Scanner: (direction, magn)
            //
            
            // magnitude / azimuth
            WDL_TypedBuf<BL_FLOAT> monoMagns;
            BLUtils::StereoToMono(&monoMagns, magns[0], magns[1]);
            
            WDL_TypedBuf<BL_FLOAT> monoMagnsDB;
            BLUtils::AmpToDBNorm(&monoMagnsDB, monoMagns, 1e-15, -120.0);
            
            // Discard magnitudes where source R is not defined
            // This avoids an horizontal line in the display, when width = 0
            for (int i = 0; i < sourceRs.GetSize(); i++)
            {
                BL_FLOAT r = sourceRs.Get()[i];
                if (r < EPS)
                    monoMagnsDB.Get()[i] = 0.0;
            }
            
#if TEST_TEMPORAL_SMOOTHING
            TemporalSmoothing(&monoMagnsDB, &thetasRot);
#endif
            
#if TEST_SPATIAL_SMOOTHING
            SpatialSmoothing(&monoMagnsDB, &thetasRot);
#endif
            
            PolarViz::PolarToCartesian2(monoMagnsDB, thetasRot,
                                        widthValuesX, widthValuesY);
        }
        
        // Reverse X, to have correct Left/Right orientation when displaying
        BLUtils::MultValues(widthValuesX, -1.0);
    }
    else if ((mDisplayMode == SPECTROGRAM) || (mDisplayMode == SPECTROGRAM2))
    {
        WDL_TypedBuf<BL_FLOAT> sourceRs;
        WDL_TypedBuf<BL_FLOAT> sourceThetas;
        
        // NOTE: this is a copy of above
        WDL_TypedBuf<BL_FLOAT> timeDelays;
        BLUtils::ComputeTimeDelays(&timeDelays, phases[0], phases[1], mSampleRate);
        
        SourcePos::MagnPhasesToSourcePos(&sourceRs, &sourceThetas,
                                         magns[0],  magns[1],
                                         phases[0], phases[1],
                                         freqs,
                                         timeDelays);
        
        // Simple difference of magns
        if (mDisplayMode == SPECTROGRAM2)
        {
#define MAGNS_DIFF_COEFF 1000.0
            WDL_TypedBuf<BL_FLOAT> diffMagns;
            BLUtils::ComputeDiff(&diffMagns, magns[0], magns[1]);
            BLUtils::MultValues(&diffMagns, -MAGNS_DIFF_COEFF*M_PI/2.0);
            BLUtils::AddValues(&diffMagns, M_PI/2.0);
            BLUtils::ClipMin(&diffMagns, 0.0);
            BLUtils::ClipMax(&diffMagns, M_PI);
            sourceThetas = diffMagns;
        }
        
        // Special case for mono signals
        bool sameMagns = BLUtils::IsEqual(magns[0], magns[1]);
        bool samePhases = BLUtils::IsEqual(phases[0], phases[1]);
        if (sameMagns && samePhases)
            // Mono signal
        {
            // Set the angle in the middle
            BLUtils::FillAllValue(&sourceThetas, M_PI/2.0);
        }
        
        
        // GOOD with float time delays
        
        // Fill missing values (but only for display) !
        
        //BLUtils::FillMissingValues(&sourceRs, true, UTILS_VALUE_UNDEFINED);
        BLUtils::FillMissingValues(&sourceThetas, true, UTILS_VALUE_UNDEFINED);
        
        // Just in case
        BLUtils::ReplaceValue(&sourceThetas, UTILS_VALUE_UNDEFINED, 0.0);
        
        WDL_TypedBuf<BL_FLOAT> thetasRot = sourceThetas;
        BLUtils::AddValues(&thetasRot, -M_PI/2.0);
        
        *widthValuesX = thetasRot;
        BL_FLOAT coeff = 1.0/M_PI;
        BLUtils::MultValues(widthValuesX, coeff);
        
        ComputeSpectroY(widthValuesY, widthValuesX->GetSize());
        
//#if !USE_FLAT_POLAR_SPECTRO
        BLUtils::PolarToCartesianFlat(widthValuesX, widthValuesY);
//#endif
        
#if 0 // TEST for more coherent colormap application
        // (results are a bit different)
        WDL_TypedBuf<BL_FLOAT> monoMagns;
        BLUtils::StereoToMono(&monoMagns, magns[0], magns[1]);
        mColorWeights = monoMagns;
#endif
        
        // TEST: try spatial smoothing
        // => consumes more resources
        //#if TEST_SPATIAL_SMOOTHING
        //            SpatialSmoothing(&mWidthValuesX, &mWidthValuesY);
        //#endif
        
    }
    
    if (mDisplayMode != SPECTROGRAM)
    {
        if (mVolRender != NULL)
        {
#if USE_FLAT_POLAR
            BLUtils::CartesianToPolarFlat(&mWidthValuesX, &mWidthValuesY);
#endif
        }
    }
    
    if (!mAngularMode)
    {
        BLUtils::CartesianToPolarFlat(widthValuesX, widthValuesY);
     
        // HACK
        // Reduce (in SIMPLE mode, this goes too much outside on the sides)
        if (mDisplayMode == SIMPLE)
            BLUtils::MultValues(widthValuesX, 0.5);
    }
}

void
StereoVizProcess3::RecomputeResult()
{
    if (mVolRender == NULL)
        return;
    
    mVolRender->Clear();
    
    for (int i = 0; i < mBufs[0].size(); i++)
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2];
        
        fftSamples[0] = mBufs[0][i];
        fftSamples[1] = mBufs[1][i];
    
        // Apply the stereo width 
        ApplyStereoWidth(fftSamples);
        
        WDL_TypedBuf<BL_FLOAT> magns[2];
        WDL_TypedBuf<BL_FLOAT> phases[2];
    
        BLUtils::ComplexToMagnPhase(&magns[0], &phases[0], fftSamples[0]);
        BLUtils::ComplexToMagnPhase(&magns[1], &phases[1], fftSamples[1]);
    
    
        WDL_TypedBuf<BL_FLOAT> widthValuesX;
        WDL_TypedBuf<BL_FLOAT> widthValuesY;
        WDL_TypedBuf<BL_FLOAT> colorWeights;
    
        ComputeResult(magns, phases, &widthValuesX, &widthValuesY, &colorWeights);
    
        if (mVolRender != NULL)
        {
            mVolRender->AddCurveValuesWeight(widthValuesX,
                                             widthValuesY,
                                             colorWeights);
        }
    }
}

// Not optimized
// Convert back to sample, then re-convert to magns + phases
void
StereoVizProcess3::ApplyStereoWidth(WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2])
{
    // Prepara fft data
    fftSamples[0].Resize(fftSamples[0].GetSize()*2);
    fftSamples[1].Resize(fftSamples[1].GetSize()*2);
    
    BLUtils::FillSecondFftHalf(&fftSamples[0]);
    BLUtils::FillSecondFftHalf(&fftSamples[1]);

    // Convert to samples
    WDL_TypedBuf<BL_FLOAT> samples[2];
    FftProcessObj16::FftToSamples(fftSamples[0], &samples[0]);
    FftProcessObj16::FftToSamples(fftSamples[1], &samples[1]);
    
    // Prepare stereo widen
    vector<WDL_TypedBuf<BL_FLOAT> *> stereoInputs;
    stereoInputs.resize(2);
    stereoInputs[0] = &samples[0];
    stereoInputs[1] = &samples[1];
    
    // Stereo widen
    StereoWidthProcess3::SimpleStereoWiden(&stereoInputs, mStereoWidth);
    
    // Convert back to fft
    FftProcessObj16::SamplesToFft(samples[0], &fftSamples[0]);
    FftProcessObj16::SamplesToFft(samples[1], &fftSamples[1]);
    
    // Re-pack output fft data (take half of the complexes)
    BLUtils::TakeHalf(&fftSamples[0]);
    BLUtils::TakeHalf(&fftSamples[1]);
}
