//
//  SMVProcess4.cpp
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <PolarViz.h>
#include <SourcePos.h>

#include <DebugGraph.h>

#include <SourcePos.h>

#include <SMVVolRender3.h>
#include <StereoWidthProcess3.h>

// Computers
#include <SMVProcessXComputerScope.h>
#include <SMVProcessXComputerScopeFlat.h>
#include <SMVProcessXComputerDiff.h>

//#include <SMVProcessXComputerDiffFlat.h>
#include <SMVProcessXComputerLissajous.h>

#include <SMVProcessXComputerLissajousEXP.h>

#include <SMVProcessXComputerFreq.h>
#include <SMVProcessXComputerChroma.h>

#include <SMVProcessYComputerMagn.h>
#include <SMVProcessYComputerFreq.h>
#include <SMVProcessYComputerChroma.h>

#include <SMVProcessYComputerPhasesFreq.h>
#include <SMVProcessYComputerPhasesTime.h>

#include <SMVProcessColComputerMagn.h>
#include <SMVProcessColComputerFreq.h>
#include <SMVProcessColComputerChroma.h>

#include <SMVProcessColComputerPhasesFreq.h>
#include <SMVProcessColComputerPhasesTime.h>

#include <SMVProcessColComputerPan.h>
#include <SMVProcessColComputerMidSide.h>

#include <PhasesUnwrapper.h>

#include <StereoWidenProcess.h>

#include <Axis3D.h>
#include <Axis3DFactory2.h>
#include <TimeAxis3D.h>

#include <RayCaster2.h>

#include <CMA2Smoother.h>

#include "SMVProcess4.h"

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

// Play selection not only in clip mod + Cmd key pressed
// NOTE: also defined in RayCaster
#define FIX_PLAY_SELECTION 1

// Play only thresholded data, even if there is no selection
#define FIX_PLAY_THRESHOLDED 1

// TEST
#define COMPUTE_PHASES_GRADIENT_FREQ 0 //1
#define COMPUTE_PHASES_GRADIENT_TIME 0 //1

// FIX: Changing the stereo width had no effect on sound
#define FIX_STEREO_WIDTH_NO_SOUND_EFFECT 1

// FIX: apply correctly stereo width in play mode
#define FIX_STEREO_WIDTH_PLAY_MODE 1

// FIX: the samples buffer was not affected by stero width
#define FIX_SAMPLES_STEREO_WIDTH 1

// FIX: since fixes of stereo width sound,
// the sound didn't match the time selection
// (move the time selection => sound was not played from the correct time)
#define FIX_SELECTION_PLAY 1

// 0.2 seconds
//#define TIME_AXIS_SPACING_SECONDS 0.2
#define TIME_AXIS_NUM_LABELS 6

#define USE_SIMD 1

// Apply stereo width on samples in the main class (better sound)
#define STEREO_WIDEN_SAMPLES 1

SMVProcess4::SMVProcess4(int bufferSize,
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
    
    // Modes
    mModeX = MODE_X_SCOPE;
    mModeY = MODE_Y_MAGN;
    mModeColor = MODE_COLOR_MAGN;
    
    mAngularMode = false;
    
    mStereoWidth = 0.0;
    
    //
    mAxisFactory = new Axis3DFactory2();
    mAxisFactory->Reset(bufferSize, sampleRate);
    
    // Computers
    //
    
    // X
    mXComputers[MODE_X_SCOPE] = new SMVProcessXComputerScope(mAxisFactory);
    mXComputers[MODE_X_SCOPE_FLAT] = new SMVProcessXComputerScopeFlat(mAxisFactory);
    //mXComputers[MODE_X_DIFF] = new SMVProcessXComputerDiff();
 
    //mXComputers[MODE_X_DIFF_FLAT] = new SMVProcessXComputerDiffFlat();
    mXComputers[MODE_X_LISSAJOUS] = new SMVProcessXComputerLissajous(mAxisFactory);
    
    mXComputers[MODE_X_EXP] = new SMVProcessXComputerLissajousEXP(mAxisFactory, sampleRate);
    
    mXComputers[MODE_X_FREQ] = new SMVProcessXComputerFreq(mAxisFactory);
    mXComputers[MODE_X_CHROMA] = new SMVProcessXComputerChroma(mAxisFactory, sampleRate);
    
    // Y
    mYComputers[MODE_Y_MAGN] = new SMVProcessYComputerMagn(mAxisFactory);
    mYComputers[MODE_Y_FREQ] = new SMVProcessYComputerFreq(mAxisFactory);
    mYComputers[MODE_Y_CHROMA] = new SMVProcessYComputerChroma(mAxisFactory, sampleRate);
    
    mYComputers[MODE_Y_PHASES_FREQ] = new SMVProcessYComputerPhasesFreq(mAxisFactory);
    mYComputers[MODE_Y_PHASES_TIME] = new SMVProcessYComputerPhasesTime(mAxisFactory);
    
    // Color
    mColComputers[MODE_COLOR_MAGN] = new SMVProcessColComputerMagn();
    mColComputers[MODE_COLOR_FREQ] = new SMVProcessColComputerFreq();
    mColComputers[MODE_COLOR_CHROMA] = new SMVProcessColComputerChroma(sampleRate);
    
    mColComputers[MODE_COL_PHASES_FREQ] = new SMVProcessColComputerPhasesFreq();
    mColComputers[MODE_COL_PHASES_TIME] = new SMVProcessColComputerPhasesTime();
    
    mColComputers[MODE_COL_PAN] = new SMVProcessColComputerPan();
    mColComputers[MODE_COL_MS] = new SMVProcessColComputerMidSide();
    
    //
    mPhasesUnwrapper = new PhasesUnwrapper(0);
    
    // For play button
    mPlayMode = RECORD;
    
    // Play
    //
    mCurrentPlayLine = 0;
    
    mIsPlaying = false;
    mHostIsPlaying = false;
    
    mTimeAxis = NULL;

    int dummyWindowSize = 5;
    mSmoother = new CMA2Smoother(bufferSize, dummyWindowSize);
}

SMVProcess4::~SMVProcess4()
{
    // Smoothing
    delete mSourceRsHisto;
    delete mSourceThetasHisto;
    
    // Computers
    for (int i = 0; i < MODE_X_NUM_MODES; i++)
    {
        delete mXComputers[i];
    }
    
    for (int i = 0; i < MODE_Y_NUM_MODES; i++)
    {
        delete mYComputers[i];
    }
    
    for (int i = 0; i < MODE_COLOR_NUM_MODES; i++)
    {
        delete mColComputers[i];
    }
    
    if (mTimeAxis != NULL)
        delete mTimeAxis;
    
    delete mAxisFactory;

    delete mSmoother;
}

void
SMVProcess4::Reset()
{
    Reset(mOverlapping, mOversampling, mSampleRate);
}

void
SMVProcess4::Reset(int overlapping, int oversampling,
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
    
    // Computers
    for (int i = 0; i < MODE_X_NUM_MODES; i++)
    {
        mXComputers[i]->Reset(sampleRate);
    }
    
    for (int i = 0; i < MODE_Y_NUM_MODES; i++)
    {
        mYComputers[i]->Reset(sampleRate);;
    }
    
    for (int i = 0; i < MODE_COLOR_NUM_MODES; i++)
    {
        mColComputers[i]->Reset(sampleRate);;
    }

    mPhasesUnwrapper->Reset();
    
    // Play
    mCurrentPlayLine = 0;
    
    mAxisFactory->Reset(mBufferSize, sampleRate);
    CreateAxes();
}

void
SMVProcess4::SetPlayMode(PlayMode mode)
{
    mPlayMode = mode;
}

void
SMVProcess4::ProcessInputSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                 const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer)
{
    if (ioSamples->size() < 2)
        return;
    
    mCurrentSamples[0] = *(*ioSamples)[0];
    mCurrentSamples[1] = *(*ioSamples)[1];
}

void
SMVProcess4::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                             const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
#define EPS 1e-15
    
    if (ioFftSamples->size() < 2)
        return;
    
    if (mPlayMode == BYPASS)
    {
        // Hide play bar
        mVolRender->SetPlayBarPos(-1.0);
        
        return;
    }
    
    //
    // Prepare the data
    //
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2] = { *(*ioFftSamples)[0], *(*ioFftSamples)[1] };
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples[0]);
    BLUtils::TakeHalf(&fftSamples[1]);
    
    if (mVolRender != NULL)
    {
        long numSlices = mVolRender->GetNumSlices();
        mPhasesUnwrapper->SetHistorySize(numSlices);
    }
    
    // Play
    if (mPlayMode == PLAY)
    {
        long numSlices = mVolRender->GetNumSlices();
        
        if (mIsPlaying)
        {
            BL_FLOAT t = ((BL_FLOAT)mCurrentPlayLine)/numSlices;
            mVolRender->SetPlayBarPos(t);
        }
        
        bool canPlay = true;
        if (mVolRender->IsVolumeSelectionEnabled())
        {
            int startLine;
            int endLine;
            // Don't play if selection is totally out of bounds
            canPlay = mVolRender->GetSelectionBoundsLines(&startLine, &endLine);
            
            // When moving selection quickly while playing,
            // the play bar must move quickly
            // to not fal aoutside selection
            if (mCurrentPlayLine < startLine)
                mCurrentPlayLine = startLine;
            
            // End condition
            if (mCurrentPlayLine > endLine)
                mCurrentPlayLine = startLine;
        }
        else
            // No selection
        {
            if (mCurrentPlayLine > numSlices)
                mCurrentPlayLine = 0;
        }
        
        
        // Get the current data
        if (mCurrentPlayLine < mBufs[0].size()) // Test just in case
        {
            //fftSamples[0] = mBufs[0][mCurrentPlayLine];
            //fftSamples[1] = mBufs[1][mCurrentPlayLine];
            
            GetSelectedData(fftSamples, mCurrentPlayLine);
        }
        
        if (!canPlay)
        {
            // Hide play bar
            mVolRender->SetPlayBarPos(-1.0);
        }
        
        if (!mIsPlaying || !canPlay)
        {
            BLUtils::FillAllZero(&fftSamples[0]);
            BLUtils::FillAllZero(&fftSamples[1]);
            
             //mVolRender->SetPlayBarPos(-1.0);
        }
        
        if (mIsPlaying)
        {
            // Increment
            mCurrentPlayLine++;
        }
    }


    // Record
    if (mPlayMode == RECORD)
    {
        // Hide play bar
        // (for the next time we will enter PLAY mode)
        mVolRender->SetPlayBarPos(-1.0);
        
        long numSlices = mVolRender->GetNumSlices();
        for (int i = 0; i < 2; i++)
        {
            mBufs[i].push_back(fftSamples[i]);
        
            if (mBufs[i].size() > numSlices)
                mBufs[i].pop_front();
        }
    
        // Must keep samples too, because we need them for example for Lissajous
        // We need to keep samples history because when we recompute the whole data,
        // for example when changing the color mode, we need all the data
        // (and the the samples too)
        for (int i = 0; i < 2; i++)
        {
            mSamplesBufs[i].push_back(mCurrentSamples[i]);
            
            if (mSamplesBufs[i].size() > numSlices)
                mSamplesBufs[i].pop_front();
        }
        
#if !STEREO_WIDEN_SAMPLES
        
#if FIX_SAMPLES_STEREO_WIDTH
        ApplyStereoWidth(mCurrentSamples);
#endif
        
        // Apply stereo width
        ApplyStereoWidth(fftSamples);
#endif
        
        WDL_TypedBuf<BL_FLOAT> magns[2];
        WDL_TypedBuf<BL_FLOAT> phases[2];
        BLUtilsComp::ComplexToMagnPhase(&magns[0], &phases[0], fftSamples[0]);
        BLUtilsComp::ComplexToMagnPhase(&magns[1], &phases[1], fftSamples[1]);
    
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
        
            ComputeResult(mCurrentSamples, magns, phases,
                          &widthValuesX, &widthValuesY, &colorWeights);
        
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
        BLUtilsComp::MagnPhaseToComplex(&fftSamples[0], magns[0], phases[0]);
        BLUtilsComp::MagnPhaseToComplex(&fftSamples[1], magns[1], phases[1]);
        
        // Get the center of selection in the RayCaster
        long currentSliceNum = mVolRender->GetCurrentSlice();
#if !FIX_STEREO_WIDTH_NO_SOUND_EFFECT
        GetSelectedData(fftSamples, currentSliceNum);
#else
        
#if !FIX_SELECTION_PLAY
        // Keep the fftSamples, don't get them from history
        // So we keep the sound of the previous stereo widening
        // (keep the widened sound)
        GetSelectedData(fftSamples, currentSliceNum, false);
#else
        // We must extract the slice, so when we move the selection
        // over the time, the sound changes
        GetSelectedData(fftSamples, currentSliceNum, true);
        
#if !STEREO_WIDEN_SAMPLES
        // And the apply stereo width on the extracted slice
        ApplyStereoWidth(fftSamples);
#endif
        
#endif
        
#endif
        
        // Hide play bar
        mVolRender->SetPlayBarPos(-1.0);
    }
        
    // Common part
    //
    
    fftSamples[0].Resize(fftSamples[0].GetSize()*2);
    fftSamples[1].Resize(fftSamples[1].GetSize()*2);
    
    BLUtilsFft::FillSecondFftHalf(&fftSamples[0]);
    BLUtilsFft::FillSecondFftHalf(&fftSamples[1]);
    
    // Result
    *(*ioFftSamples)[0] = fftSamples[0];
    *(*ioFftSamples)[1] = fftSamples[1];
}

void
SMVProcess4::SetModeX(enum ModeX mode)
{
    mModeX = mode;
    
    // Axis
    Axis3D *xAxis = mXComputers[mModeX]->CreateAxis();
    SetupAxis(xAxis);
    mVolRender->SetAxis(0, xAxis);
    
    bool xIsPolarNotScalable = IsXPolarNotScalable();
    if (xIsPolarNotScalable)
    {
        // If we ignore Y, create a dummy axis on Y
        Axis3D *xAxis = mAxisFactory->CreateEmptyAxis(Axis3DFactory2::ORIENTATION_Y);
        SetupAxis(xAxis);
        mVolRender->SetAxis(1, xAxis);
    }
    else
    {
        // Recompute Y
        Axis3D *yAxis = mYComputers[mModeY]->CreateAxis();
        SetupAxis(yAxis);
        mVolRender->SetAxis(1, yAxis);
    }
    
    RecomputeResult();
}

void
SMVProcess4::SetModeY(enum ModeY mode)
{
    mModeY = mode;
    
    bool xIsPolarNotScalable = IsXPolarNotScalable();
    
    // Axis
    if (!xIsPolarNotScalable)
    {
        Axis3D *yAxis = mYComputers[mModeY]->CreateAxis();
        SetupAxis(yAxis);
        mVolRender->SetAxis(1, yAxis);
    }
    
    RecomputeResult();
}

void
SMVProcess4::SetModeColor(enum ModeColor mode)
{
    mModeColor = mode;
    
    RecomputeResult();
}

void
SMVProcess4::SetAngularMode(bool flag)
{
    mAngularMode = flag;
    
    RecomputeResult();
}

void
SMVProcess4::SetStereoWidth(BL_FLOAT stereoWidth, bool hostIsPlaying)
{
    bool recomputeAll = !hostIsPlaying;
    
#if FIX_STEREO_WIDTH_PLAY_MODE
    recomputeAll = true;
    
    if ((mPlayMode == RECORD) && hostIsPlaying)
        recomputeAll = false;
#endif
    
    mStereoWidth = stereoWidth;
    
    if (recomputeAll)
        RecomputeResult();
}

void
SMVProcess4::SetVolRender(SMVVolRender3 *volRender)
{
    mVolRender = volRender;
    
    if (mVolRender != NULL)
    {
        mVolRender->SetDisplayRefreshRate(mDisplayRefreshRate);
        
        long numSlices = mVolRender->GetNumSlices();
        mPhasesUnwrapper->SetHistorySize(numSlices);
    }
    
    CreateAxes();
}

void
SMVProcess4::SetIsPlaying(bool flag)
{
    mIsPlaying = flag;
}

void
SMVProcess4::SetHostIsPlaying(bool flag)
{
    mHostIsPlaying = flag;
}

void
SMVProcess4::ResetTimeAxis()
{
    InitTimeAxis();
}

void
SMVProcess4::UpdateTimeAxis(BL_FLOAT currentTime)
{
    mTimeAxis->Update(currentTime);
    
    mPrevTime = currentTime;
}

void
SMVProcess4::TemporalSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
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
SMVProcess4::SpatialSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
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
SMVProcess4::NormalizeRs(WDL_TypedBuf<BL_FLOAT> *sourceRs,
                                bool discardClip)
{
    // 100 m
#define MAX_DIST 100.0
    
    for (int i = 0; i < sourceRs->GetSize(); i++)
    {
        BL_FLOAT r = sourceRs->Get()[i];

        //r /= MAX_DIST;
        r *= (1.0/MAX_DIST);
        
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
SMVProcess4::ComputeColorWeightsMagns(WDL_TypedBuf<BL_FLOAT> *outWeights,
                                            const WDL_TypedBuf<BL_FLOAT> &magnsL,
                                            const WDL_TypedBuf<BL_FLOAT> &magnsR)
{
    // Mono
    WDL_TypedBuf<BL_FLOAT> monoMagns;
    BLUtils::StereoToMono(&monoMagns, magnsL, magnsR);
    
    // dB
    WDL_TypedBuf<BL_FLOAT> monoMagnsDB;
    BLUtils::AmpToDBNorm(&monoMagnsDB, monoMagns, (BL_FLOAT)1e-15, (BL_FLOAT)-120.0);
    
#if 0 // NOTE: was not good
      // Saturated the colormap a lot
    // Coeffs
    BLUtils::MultValues(&monoMagnsDB, (BL_FLOAT)4.0);
#endif
    
    *outWeights = monoMagnsDB;
}

void
SMVProcess4::ComputeColorWeightsFreqs(WDL_TypedBuf<BL_FLOAT> *outWeights,
                                             const WDL_TypedBuf<BL_FLOAT> &freqs)
{
    outWeights->Resize(freqs.GetSize());
    
    for (int i = 0; i < outWeights->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/outWeights->GetSize();
    
        // Logical to use log since frequency preception is logarithmic
        t = BLUtils::AmpToDBNorm(t, (BL_FLOAT)1e-15, (BL_FLOAT)-120.0);
        
        //BL_FLOAT w = (t + 1.0)/2.0;
        BL_FLOAT w = (t + 1.0)*0.5;
        
        //w = BLUtils::ApplyParamShape(w, 4.0);
        w *= 2.0;
        
        outWeights->Get()[i] = w;
    }
}

int
SMVProcess4::GetDisplayRefreshRate()
{
    // Force return 1, to be able to catch all the buffers
    // to re-play a selection
    return 1;
    
    //int refreshRate = mOverlapping;
    //return refreshRate;
}


#if 0
void
SMVProcess4::DiscardUnselected(WDL_TypedBuf<BL_FLOAT> *magns0,
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
SMVProcess4::DiscardUnselectedStereo(WDL_TypedBuf<BL_FLOAT> *magns0,
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
                //balance = (balance + 1.0)/2.0;
                balance = (balance + 1.0)*0.5;
                
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
SMVProcess4::ComputeSpectroY(WDL_TypedBuf<BL_FLOAT> *yValues, int numValues)
{
    yValues->Resize(numValues);
    
    BL_FLOAT sizeInv = 1.0;
    if (yValues->GetSize() > 0.0)
        sizeInv = 1.0/yValues->GetSize();
    
    for (int i = 0; i < yValues->GetSize(); i++)
    {
        //BL_FLOAT t = ((BL_FLOAT)i)/yValues->GetSize();
        BL_FLOAT t = ((BL_FLOAT)i)*sizeInv;
        
        t = BLUtils::LogScaleNormInv(t, (BL_FLOAT)1.0, (BL_FLOAT)Y_LOG_SCALE_FACTOR);
        
        yValues->Get()[i] = t*SPECTRO_HEIGHT_COEFF;
    }
}

void
SMVProcess4::GetSelectedData(WDL_TypedBuf<WDL_FFT_COMPLEX> ioFftSamples[2],
                             long sliceNum, bool extractSliceHistory)
{
    // If not selection, play all
#if !FIX_PLAY_SELECTION
    // Selection 2D
    if (!mVolRender->IsSelectionEnabled())
        return;
#else
    // To be able to play only thresholded data
    // even when there is no selection
#if !FIX_PLAY_THRESHOLDED
    // Selection 3D
    if (!mVolRender->IsVolumeSelectionEnabled())
        return;
#endif
#endif
    
    vector<bool> selection;
    vector<BL_FLOAT> xCoords;
    bool found = mVolRender->GetSliceSelection(&selection, &xCoords, sliceNum);
    if (!found)
        // Empty selection
    {
        // Make silence
        for (int i = 0; i < 2; i++)
        {
#if !USE_SIMD
            for (int j = 0; j < ioFftSamples[i].GetSize(); j++)
            {
                // Mute the fft sample
                WDL_FFT_COMPLEX &val = ioFftSamples[i].Get()[j];
            
                val.re = 0.0;
                val.im = 0.0;
            
                ioFftSamples[i].Get()[j] = val; // Useless
            }
#else
            BLUtils::FillAllZero(&ioFftSamples[i]);
#endif
        }
        
        return;
    }
    
    if (selection.size() != ioFftSamples[0].GetSize())
        // Just in case
        return;
    
    if (sliceNum >= mBufs[0].size())
        // Just in case
        return;
    
    if (extractSliceHistory)
    {
        ioFftSamples[0] = mBufs[0][sliceNum];
        ioFftSamples[1] = mBufs[1][sliceNum];
        
#if !STEREO_WIDEN_SAMPLES
        
#if FIX_STEREO_WIDTH_PLAY_MODE
        ApplyStereoWidth(ioFftSamples);
#endif
        
#endif
    }
    
    for (int i = 0; i < 2; i++)
    {
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
SMVProcess4::ComputeResult(const WDL_TypedBuf<BL_FLOAT> samples[2],
                           const WDL_TypedBuf<BL_FLOAT> magns[2],
                           const WDL_TypedBuf<BL_FLOAT> phases[2],
                           WDL_TypedBuf<BL_FLOAT> *widthValuesX,
                           WDL_TypedBuf<BL_FLOAT> *widthValuesY,
                           WDL_TypedBuf<BL_FLOAT> *colorWeights)
{
    WDL_TypedBuf<BL_FLOAT> magns0[2] = { magns[0], magns[1] };
    WDL_TypedBuf<BL_FLOAT> phases0[2] = { phases[0], phases[1] };
    
    // Phases unwrap
    WDL_TypedBuf<BL_FLOAT> phasesUnwrapFreq = phases0[0];
    mPhasesUnwrapper->UnwrapPhasesFreq(&phasesUnwrapFreq);

    // Take the phase[1], just in case we need both phases in a computer
    WDL_TypedBuf<BL_FLOAT> phasesUnwrapTime = phases0[0/*1*/];
    mPhasesUnwrapper->UnwrapPhasesTime(&phasesUnwrapTime);
    
#if !COMPUTE_PHASES_GRADIENT_FREQ
    mPhasesUnwrapper->NormalizePhasesFreq(&phasesUnwrapFreq);
#else
    mPhasesUnwrapper->ComputePhasesGradientFreqs(&phasesUnwrapFreq);
    mPhasesUnwrapper->NormalizePhasesGradientFreqs(&phasesUnwrapFreq);
#endif

#if !COMPUTE_PHASES_GRADIENT_TIME
    mPhasesUnwrapper->NormalizePhasesTime(&phasesUnwrapTime);
#else
    mPhasesUnwrapper->ComputePhasesGradientTime(&phasesUnwrapTime);
    mPhasesUnwrapper->NormalizePhasesGradientTime(&phasesUnwrapTime);
#endif
    
    WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2] = { phasesUnwrapFreq, phasesUnwrapTime };
    
    WDL_TypedBuf<BL_FLOAT> resultX;
    WDL_TypedBuf<BL_FLOAT> resultYPolar;
    bool isPolar = false;
    bool isScalable = false;
    BL_FLOAT polarCenter[2] = { 0.0, 0.0 };
    mXComputers[mModeX]->ComputeX(samples, magns0, phases0,
                                  &resultX, &resultYPolar,
                                  &isPolar, polarCenter, &isScalable);
    if (!isPolar)
        *widthValuesX = resultX;
    
    WDL_TypedBuf<BL_FLOAT> resultY;
    mYComputers[mModeY]->ComputeY(magns0, phases0, phasesUnwrap, &resultY/*widthValuesY*/);
    if (!isPolar)
        *widthValuesY = resultY;
    
    mColComputers[mModeColor]->ComputeCol(magns0, phases0, phasesUnwrap, colorWeights);
    
    if (isPolar)
    {
        if (!isScalable)
        {
            *widthValuesX = resultX;
            *widthValuesY = resultYPolar;
        }
        else
        // It is scalable !
        {
            // Multiply direction by Y (polar view, Y is distance)
            //
            
            widthValuesX->Resize(resultX.GetSize());
            widthValuesY->Resize(resultY.GetSize());
        
            for (int i = 0; i < resultX.GetSize(); i++)
            {
                BL_FLOAT x = resultX.Get()[i];
                BL_FLOAT ypolar = resultYPolar.Get()[i];
            
                BL_FLOAT dir[2] = { x - polarCenter[0], ypolar - polarCenter[1] };
                BL_FLOAT norm = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1]);
                if (norm > 0.0)
                {
                    dir[0] /= norm;
                    dir[1] /= norm;
                }
            
                BL_FLOAT y = resultY.Get()[i];
                widthValuesX->Get()[i] = polarCenter[0] + dir[0]*y;
                widthValuesY->Get()[i] = polarCenter[1] + dir[1]*y;
            }
        }
    }
}

void
SMVProcess4::RecomputeResult()
{
    if (mVolRender == NULL)
        return;
    
    mVolRender->Clear();
    
    for (int i = 0; i < mBufs[0].size(); i++)
    {
        WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2];
        fftSamples[0] = mBufs[0][i];
        fftSamples[1] = mBufs[1][i];
    
#if !STEREO_WIDEN_SAMPLES
        // Apply the stereo width
        ApplyStereoWidth(fftSamples);
#endif
        
        WDL_TypedBuf<BL_FLOAT> samples[2];
        samples[0] = mSamplesBufs[0][i];
        samples[1] = mSamplesBufs[1][i];
        
#if !STEREO_WIDEN_SAMPLES
        
#if FIX_SAMPLES_STEREO_WIDTH
        ApplyStereoWidth(samples);
#endif
    
#endif
        
        WDL_TypedBuf<BL_FLOAT> magns[2];
        WDL_TypedBuf<BL_FLOAT> phases[2];
    
        BLUtilsComp::ComplexToMagnPhase(&magns[0], &phases[0], fftSamples[0]);
        BLUtilsComp::ComplexToMagnPhase(&magns[1], &phases[1], fftSamples[1]);
    
    
        WDL_TypedBuf<BL_FLOAT> widthValuesX;
        WDL_TypedBuf<BL_FLOAT> widthValuesY;
        WDL_TypedBuf<BL_FLOAT> colorWeights;
    
        ComputeResult(samples, magns, phases,
                      &widthValuesX, &widthValuesY, &colorWeights);
    
        if (mVolRender != NULL)
        {
            mVolRender->AddCurveValuesWeight(widthValuesX,
                                             widthValuesY,
                                             colorWeights);
        }
    }
}

#if 0
// Not optimized
// Convert back to sample, then re-convert to magns + phases
void
SMVProcess4::ApplyStereoWidth(WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2])
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
#endif

#if 1
// Latest version (optimized)
// Apply directly on complex
void
SMVProcess4::ApplyStereoWidth(WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2])
{
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > samples;
    samples.resize(2);
    samples[0] = &fftSamples[0];
    samples[1] = &fftSamples[1];
    
    StereoWidenProcess::StereoWiden(&samples, mStereoWidth);
}
#endif

// For samples (not fft)
void
SMVProcess4::ApplyStereoWidth(WDL_TypedBuf<BL_FLOAT> samples[2])
{
    vector<WDL_TypedBuf<BL_FLOAT> * > samples0;
    samples0.resize(2);
    samples0[0] = &samples[0];
    samples0[1] = &samples[1];
    
    StereoWidenProcess::StereoWiden(&samples0, mStereoWidth);
}

void
SMVProcess4::SetupAxis(Axis3D *axis)
{
    axis->SetDoOverlay(true);
    
    RayCaster2 *rayCaster = mVolRender->GetRayCaster();
    axis->SetPointProjector(rayCaster);
}

void
SMVProcess4::CreateAxes()
{
    // Axis
    Axis3D *xAxis = mXComputers[mModeX]->CreateAxis();
    SetupAxis(xAxis);
    mVolRender->SetAxis(0, xAxis);
    
    bool xIsPolarNotScalable = IsXPolarNotScalable();
    if (!xIsPolarNotScalable)
    {
        Axis3D *yAxis = mYComputers[mModeY]->CreateAxis();
        SetupAxis(yAxis);
        mVolRender->SetAxis(1, yAxis);
    }
    
    Axis3D *zAxis = mAxisFactory->CreateEmptyAxis(Axis3DFactory2::ORIENTATION_Z);
    SetupAxis(zAxis);
    mVolRender->SetAxis(2, zAxis);
    
    //
    if (mTimeAxis != NULL)
        delete mTimeAxis;
    mTimeAxis = new TimeAxis3D(zAxis);
    mPrevTime = 0.0;
    
    InitTimeAxis();
}

bool
SMVProcess4::IsXPolarNotScalable()
{
    // Empty values
    const WDL_TypedBuf<BL_FLOAT> samples[2];
    const WDL_TypedBuf<BL_FLOAT> magns[2];
    const WDL_TypedBuf<BL_FLOAT> phases[2];
    
    // Compute dummy result on empty values,
    // in order to retrieve the polar and scalable flags
    WDL_TypedBuf<BL_FLOAT> resultX;
    WDL_TypedBuf<BL_FLOAT> resultYPolar;
    bool isPolar = false;
    bool isScalable = false;
    BL_FLOAT polarCenter[2] = { 0.0, 0.0 };
    mXComputers[mModeX]->ComputeX(samples, magns, phases,
                                  &resultX, &resultYPolar,
                                  &isPolar, polarCenter, &isScalable);
    
    if (isPolar && !isScalable)
        return true;
    
    return false;
}

void
SMVProcess4::InitTimeAxis()
{
    long numSlices = mVolRender->GetNumSlices();
    BL_FLOAT duration = TimeAxis3D::ComputeTimeDuration(numSlices, mBufferSize,
                                                      mOverlapping, mSampleRate);
    //mTimeAxis->Init(mBufferSize, duration, TIME_AXIS_SPACING_SECONDS);
    BL_FLOAT spacing = duration/TIME_AXIS_NUM_LABELS;
    mTimeAxis->Init(mBufferSize, duration, spacing);
    
    // Set the time value from previous
    mTimeAxis->Update(mPrevTime);
}

#endif // IGRAPHICS_NANOVG
