/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  StereoWidthProcess2.cpp
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>
#include <BLUtilsMath.h>
#include <BLUtilsPhases.h>

#include <PolarViz.h>
#include <DebugGraph.h>

#include "StereoWidthProcess2.h"

// Debug
#include "GraphControl11.h"
#include <KemarHrtf.h>
#include <Hrtf.h>

//#include <FftConvolver7.h>
#include <FftConvolver6.h>

#if 0
PROBLEM: noise/vibration when width at maximum (on car or bees noise)
(this is not due to overlapping)

NOTE: take care of the microphone separation

IDEA: make a test by changing theta instead of d
IDEA: eat map depending on intensity ?
IDEA: group and smooth sources by location
#endif


#define USE_DEBUG_GRAPH 0

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

// Let's say 1cm (Zoom H1)
//#define MICROPHONES_SEPARATION 0.01 // orig
//#define MICROPHONES_SEPARATION 0.1 // prev
//#define MICROPHONES_SEPARATION 1.0
//#define MICROPHONES_SEPARATION 4.0
// GOOD with float time delays
#define MICROPHONES_SEPARATION 10.0
// make identity work good / but the display is too narrow
//#define MICROPHONES_SEPARATION 50.0

// GOOD !
// The original algorithm works for half the cases
// So with the following, we invert the data at the beginning
// to manage the second case.
#define INVERT_ALGO 1

// GOOD !
// Sounds very good with float time delays
#define USE_TIME_DELAY 1 //0 //1

// Change mic separation distance, or spread all the source ?
#define CHANGE_MIC_DISTANCE 0 //1

// New
#define USE_HRTF 0

#define PHASES_DIFF_HACK 0

// See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
#define SIMPLE_ALGO 1

#define MAX_WIDTH_CHANGE_SIMPLE_ALGO 4.0

#define STEREOWIDTH_BUFFER_SIZE 2048

// Draw geometrical shapes, to clarify the running of the algorithm
#if DEBUG_DRAWER_DISPLAY
class DebugDrawer : public GraphCustomDrawer
{
public:
    DebugDrawer()
    {
        mD = 0.0;
        mA = 0.0;
        mB = 0.0;
        mR = 0.0;
        
        mFailCode = 0;
    }
    
    virtual ~DebugDrawer() {}
    
    virtual void PostDraw(NVGcontext *, int width, int height);
    
    void SetD(BL_FLOAT d) { mD = d; }
    void SetA(BL_FLOAT a) { mA = a; }
    void SetB(BL_FLOAT b) { mB = b; }
    void SetR(BL_FLOAT R) { mR = R; }
    void SetTheta(BL_FLOAT theta) { mTheta = theta; }
    
    void SetFail(int code) { mFailCode = code; }
    
protected:
    BL_FLOAT mD;
    BL_FLOAT mA;
    BL_FLOAT mB;
    BL_FLOAT mR;
    BL_FLOAT mTheta;
    
    int mFailCode;
};

void
DebugDrawer::PostDraw(NVGcontext *vg, int width, int height)
{
    const BL_FLOAT scale = 100.0; //10.0; //100; //1000.0;
    
    BL_FLOAT strokeWidth = 2.0;
    
    int color[4] = { 128, 128, 128, 255 };
    int fillColor[4] = { 0, 128, 255, 255 };
    
    nvgStrokeWidth(vg, strokeWidth);
    
    SWAP_COLOR(color);
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    SWAP_COLOR(fillColor);
    nvgFillColor(vg, nvgRGBA(fillColor[0], fillColor[1], fillColor[2], fillColor[3]));
    
    // NOTE: no need for GRAPH_CONTROL_FLIP_Y...
    
    // Draw the center
    nvgSave(vg);
    nvgBeginPath(vg);
    nvgCircle(vg, width/2.0, height/2.0, strokeWidth);
    nvgClosePath(vg);
    nvgStroke(vg);
    nvgRestore(vg);
    
    // D
    nvgSave(vg);
    nvgBeginPath(vg);
    nvgMoveTo(vg, width/2.0, height/2.0);
    nvgLineTo(vg, width/2.0 - scale*mD/2.0, height/2.0);
    nvgStroke(vg);
    nvgRestore(vg);
    
    nvgSave(vg);
    nvgBeginPath(vg);
    nvgMoveTo(vg, width/2.0, height/2.0);
    nvgLineTo(vg, width/2.0 + scale*mD/2.0, height/2.0);
    nvgStroke(vg);
    nvgRestore(vg);
    
    // b
    int bcolor[4] = { 232, 183, 34, 255 };
    SWAP_COLOR(bcolor);
    
    nvgSave(vg);
    
    nvgStrokeColor(vg, nvgRGBA(bcolor[0], bcolor[1], bcolor[2], bcolor[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, width/2.0 + scale*mD/2.0, height/2.0, mB*scale);
    nvgClosePath(vg);
    nvgStroke(vg);
    nvgRestore(vg);
    
    // a + b
    int abcolor[4] = { 255, 241, 216, 255 };
    SWAP_COLOR(abcolor);
    
    nvgSave(vg);
    
    nvgStrokeColor(vg, nvgRGBA(abcolor[0], abcolor[1], abcolor[2], abcolor[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, width/2.0 - scale*mD/2.0, height/2.0, (mA + mB)*scale);
    nvgClosePath(vg);
    nvgStroke(vg);
    nvgRestore(vg);
    
    nvgRestore(vg);
    
    // R
    int rcolor[4] = { 100, 100, 100, 255 };
    SWAP_COLOR(rcolor);
    
    nvgSave(vg);
    
    nvgStrokeColor(vg, nvgRGBA(rcolor[0], rcolor[1], rcolor[2], rcolor[3]));
    
    nvgBeginPath(vg);
    nvgCircle(vg, width/2.0, height/2.0, mR*scale);
    nvgClosePath(vg);
    nvgStroke(vg);
    nvgRestore(vg);
    
    // theta
    int thetacolor[4] = { 0, 255, 0, 255 };
    SWAP_COLOR(thetacolor);
    
    nvgSave(vg);
    
    nvgStrokeWidth(vg, 4.0);
    
    nvgStrokeColor(vg, nvgRGBA(thetacolor[0], thetacolor[1], thetacolor[2], thetacolor[3]));
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, width/2.0, height/2.0);
    
    BL_FLOAT thetax = std::cos(mTheta)*scale*mR;
    BL_FLOAT thetay = std::sin(mTheta)*scale*mR;
    nvgLineTo(vg, width/2.0 + thetax, height/2.0 + thetay);
    nvgStroke(vg);
    nvgRestore(vg);
    
    // b dir
    nvgSave(vg);
    
    nvgStrokeWidth(vg, 5.0);
    nvgStrokeColor(vg, nvgRGBA(bcolor[0], bcolor[1], bcolor[2], bcolor[3]));
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, width/2.0 + scale*mD/2.0, height/2.0);
    
    // Take care, to check b correct length
    BL_FLOAT bDirX = thetax - scale*mD/2.0;
    BL_FLOAT bDirY = thetay;
    BL_FLOAT bDirNorm = std::sqrt(bDirX*bDirX + bDirY*bDirY);
    if (bDirNorm > 0.0)
    {
        bDirX /= bDirNorm;
        bDirY /= bDirNorm;
    }
    
    BL_FLOAT bX = bDirX*scale*mB;
    BL_FLOAT bY = bDirY*scale*mB;
    
    nvgLineTo(vg, width/2.0 + scale*mD/2.0 + bX, height/2.0 + bY);
    nvgStroke(vg);
    nvgRestore(vg);
    
    // a + b dir
    nvgSave(vg);
    
    nvgStrokeWidth(vg, 2.0);
    nvgStrokeColor(vg, nvgRGBA(abcolor[0], abcolor[1], abcolor[2], abcolor[3]));
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, width/2.0 - scale*mD/2.0, height/2.0);
    
    // Take care, to check b correct length
    BL_FLOAT abDirX = thetax + scale*mD/2.0;
    BL_FLOAT abDirY = thetay;
    BL_FLOAT abDirNorm = std::sqrt(abDirX*abDirX + abDirY*abDirY);
    if (abDirNorm > 0.0)
    {
        abDirX /= abDirNorm;
        abDirY /= abDirNorm;
    }
    
    BL_FLOAT abX = abDirX*scale*(mA + mB);
    BL_FLOAT abY = abDirY*scale*(mA + mB);
    
    nvgLineTo(vg, width/2.0 - scale*mD/2.0 + abX, height/2.0 + abY);
    nvgStroke(vg);
    nvgRestore(vg);
    
    // Fail code
    if (mFailCode == 1)
    {
        int fillFailColor1[4] = { 255, 0, 0, 255 };
        
        SWAP_COLOR(fillFailColor1);
        
        nvgFillColor(vg, nvgRGBA(fillFailColor1[0], fillFailColor1[1],
                                 fillFailColor1[2], fillFailColor1[3]));
        
        BL_FLOAT y20 = 20.0;
#if GRAPH_CONTROL_FLIP_Y
        y20 = height - y20;
#endif
        // Draw the center
        nvgSave(vg);
        nvgBeginPath(vg);
        nvgCircle(vg, 20.0, y20, 10.0);
        nvgClosePath(vg);
        nvgFill(vg);
        nvgRestore(vg);
    }
}

#endif // DebugDrawer

//

StereoWidthProcess2::StereoWidthProcess2(IGraphics *pGraphics,
                                         int bufferSize,
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
    mHrtf = NULL;
    for (int i = 0; i < NUM_HRTF_SLICES; i++)
    {
        mConvolvers[i][0] = NULL;
        mConvolvers[i][1] = NULL;
    }
    
    GenerateRandomCoeffs(bufferSize/2);
    
    // Smoothing
    mSourceRsHisto = new SmoothAvgHistogram(bufferSize/2, TEMPORAL_DIST_SMOOTH_COEFF, 0.0);
    mSourceThetasHisto = new SmoothAvgHistogram(bufferSize/2, TEMPORAL_DIST_SMOOTH_COEFF, 0.0);
    
#if DEBUG_DRAWER_DISPLAY
    mDebugDrawer = new DebugDrawer();
    GraphControl11 *graph = DebugGraph::GetGraph();
    graph->AddCustomDrawer(mDebugDrawer);
#endif
    
#if USE_HRTF
    InitHRTF(pGraphics);
#endif

    int dummyWinSize = 5;
    mSmoother = new CMA2Smoother(bufferSize, dummyWinSize);
}

StereoWidthProcess2::~StereoWidthProcess2()
{
    // Smoothing
    delete mSourceRsHisto;
    delete mSourceThetasHisto;
    
#if USE_HRTF
    for (int i = 0; i < NUM_HRTF_SLICES; i++)
    {
        delete mConvolvers[i][0];
        delete mConvolvers[i][1];
    }
    
    if (mHrtf != NULL)
        delete mHrtf;
#endif

    delete mSmoother;
}

void
StereoWidthProcess2::Reset()
{
    Reset(mOverlapping, mOversampling, mSampleRate);
}

void
StereoWidthProcess2::Reset(int overlapping, int oversampling,
                          BL_FLOAT sampleRate)
{
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mWidthValuesX.Resize(0);
    mWidthValuesY.Resize(0);
    mColorWeights.Resize(0);
    
    // Smoothing
    mSourceRsHisto->Reset();
    mSourceThetasHisto->Reset();
}

void
StereoWidthProcess2::SetWidthChange(BL_FLOAT widthChange)
{
    mWidthChange = widthChange;
}

void
StereoWidthProcess2::SetFakeStereo(bool flag)
{
    mFakeStereo = flag;
}

void
StereoWidthProcess2::ProcessInputSamplesPre(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples)
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
StereoWidthProcess2::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples)
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
        Stereoize(&phases[0], &phases[1]);
    else
    {
#if !SIMPLE_ALGO
        ApplyWidthChange(&magns[0], &magns[1], &phases[0], &phases[1],
                         &sourceRs, &sourceThetas);
#endif
    }
    
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
        // Unused ?
        WDL_TypedBuf<BL_FLOAT> timeDelays;
        ComputeTimeDelays(&timeDelays, phases[0], phases[1]);
        
#if 1 // DISABLED SOURCE DISPLAY (0 => debug)
        MagnPhasesToSourcePos(&sourceRs, &sourceThetas,
                              magns[0],  magns[1],
                              phases[0], phases[1],
                              freqs,
                              timeDelays);
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
        
    //
    // Re-synthetise the data with the new diff
    //
    BLUtilsComp::MagnPhaseToComplex(&fftSamples[0], magns[0], phases[0]);
    fftSamples[0].Resize(fftSamples[0].GetSize()*2);
    
    BLUtilsComp::MagnPhaseToComplex(&fftSamples[1], magns[1], phases[1]);
    fftSamples[1].Resize(fftSamples[1].GetSize()*2);
    
    BLUtilsFft::FillSecondFftHalf(&fftSamples[0]);
    BLUtilsFft::FillSecondFftHalf(&fftSamples[1]);
    
    // Result
    *(*ioFftSamples)[0] = fftSamples[0];
    *(*ioFftSamples)[1] = fftSamples[1];
}

void
StereoWidthProcess2::GetWidthValues(WDL_TypedBuf<BL_FLOAT> *xValues,
                                   WDL_TypedBuf<BL_FLOAT> *yValues,
                                   WDL_TypedBuf<BL_FLOAT> *outColorWeights)
{
    *xValues = mWidthValuesX;
    *yValues = mWidthValuesY;
    
    if (outColorWeights != NULL)
        *outColorWeights = mColorWeights;
}

void
StereoWidthProcess2::SetDisplayMode(enum DisplayMode mode)
{
    mDisplayMode = mode;
}

void
StereoWidthProcess2::ApplyWidthChange(WDL_TypedBuf<BL_FLOAT> *ioMagnsL,
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
    ComputeTimeDelays(&timeDelays, *ioPhasesL, *ioPhasesR);
    
#if USE_DEBUG_GRAPH
    WDL_TypedBuf<BL_FLOAT> originMagnsL = *ioMagnsL;
#endif
    
    // Compute left and right distances
    outSourceRs->Resize(ioPhasesL->GetSize());
    outSourceThetas->Resize(ioPhasesL->GetSize());
    
    // Get source position (polar coordinates)
    MagnPhasesToSourcePos(outSourceRs, outSourceThetas,
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
    
#if CHANGE_MIC_DISTANCE
    // Modify the width (mics distance)
    BL_FLOAT newMicsDistance = widthFactor*MICROPHONES_SEPARATION;
#endif

#if !CHANGE_MIC_DISTANCE
    // Keep the mics distance and change the sources angle
    BL_FLOAT newMicsDistance = MICROPHONES_SEPARATION;
    
    // Change sources angle
    for (int i = 0; i < outSourceThetas->GetSize(); i++)
    {
        BL_FLOAT theta = outSourceThetas->Get()[i];

        theta *= widthFactor;
        
        outSourceThetas->Get()[i] = theta;
    }
#endif

#if !USE_HRTF
    // Retrieve magn and phase from azimuth and magns
    SourcePosToMagnPhases(*outSourceRs, *outSourceThetas,
                          newMicsDistance,
                          widthFactor,
                          ioMagnsL, ioMagnsR,
                          ioPhasesL, ioPhasesR,
                          freqs,
                          timeDelays);
#else
    SourcePosToMagnPhasesHRTF(*outSourceRs, *outSourceThetas,
                              newMicsDistance,
                              widthFactor,
                              ioMagnsL, ioMagnsR,
                              ioPhasesL, ioPhasesR,
                              freqs,
                              timeDelays);
#endif
    
    
#if USE_DEBUG_GRAPH
    
#if DEBUG_FREQS_DISPLAY
#define MAGNS_COEFF 0.1 //0.01
    DebugGraph::SetCurveValues(originMagnsL,
                               2, // curve num
                               0.0, MAGNS_COEFF, // min and max y
                               1.0,
                               0, 0, 255);
    
#if 1 //r
#define DEBUG_GRAPH_MAX_DIST 100.0 //1000.0
    BLUtils::ReplaceValue(&sourceRs, UTILS_VALUE_UNDEFINED, 0.0);
    DebugGraph::SetCurveValues(sourceRs,
                               0, // curve num
                               -DEBUG_GRAPH_MAX_DIST, DEBUG_GRAPH_MAX_DIST, // min and max y
                               2.0,
                               255, 0, 0);
    
#endif
    
#if 1 // theta
#define THETA_COEFF 0.5
    BLUtils::ReplaceValue(&sourceThetas, UTILS_VALUE_UNDEFINED, 0.0);
    DebugGraph::SetCurveValues(sourceThetas,
                               1, // curve num
                               -2.0*M_PI*THETA_COEFF, 2.0*M_PI*THETA_COEFF, // min and max y
                               1.0,
                               0, 255, 0);
#endif
#endif
#endif
}

void
StereoWidthProcess2::Stereoize(WDL_TypedBuf<BL_FLOAT> *ioPhasesL,
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
StereoWidthProcess2::GenerateRandomCoeffs(int size)
{
    srand(114242111);
    
    mRandomCoeffs.Resize(size);
    for (int i = 0; i < mRandomCoeffs.GetSize(); i++)
    {
        BL_FLOAT rnd0 = 1.0 - 2.0*((BL_FLOAT)rand())/RAND_MAX;
        
        mRandomCoeffs.Get()[i] = rnd0;
    }
}

void
StereoWidthProcess2::MagnPhasesToSourcePos(WDL_TypedBuf<BL_FLOAT> *outSourceRs,
                                          WDL_TypedBuf<BL_FLOAT> *outSourceThetas,
                                          const WDL_TypedBuf<BL_FLOAT> &magnsL,
                                          const WDL_TypedBuf<BL_FLOAT> &magnsR,
                                          const WDL_TypedBuf<BL_FLOAT> &phasesL,
                                          const WDL_TypedBuf<BL_FLOAT> &phasesR,
                                          const WDL_TypedBuf<BL_FLOAT> &freqs,
                                          const WDL_TypedBuf<BL_FLOAT> &timeDelays)
{
    if (magnsL.GetSize() != magnsR.GetSize())
        // R can be empty in mono
        return;
    
    if (phasesL.GetSize() != phasesR.GetSize())
        // Just in case
        return;
    
    outSourceRs->Resize(phasesL.GetSize());
    outSourceThetas->Resize(phasesL.GetSize());
    
    for (int i = 0; i < phasesL.GetSize(); i++)
    {
        BL_FLOAT magnL = magnsL.Get()[i];
        BL_FLOAT magnR = magnsR.Get()[i];
        
        BL_FLOAT phaseL = phasesL.Get()[i];
        BL_FLOAT phaseR = phasesR.Get()[i];
        
        BL_FLOAT freq = freqs.Get()[i];
        
        BL_FLOAT timeDelay = timeDelays.Get()[i];
        
        BL_FLOAT sourceR;
        BL_FLOAT sourceTheta;
        bool computed = MagnPhasesToSourcePos(&sourceR, &sourceTheta,
                                              magnL, magnR, phaseL, phaseR, freq,
                                              timeDelay);
        
        if (computed)
        {
            outSourceRs->Get()[i] = sourceR;
            outSourceThetas->Get()[i] = sourceTheta;
        }
        else
        {
            // Set a negative value to the radius, to mark it as not computed
            outSourceRs->Get()[i] = UTILS_VALUE_UNDEFINED;
            outSourceThetas->Get()[i] = UTILS_VALUE_UNDEFINED;
        }
    }
}

bool
StereoWidthProcess2::MagnPhasesToSourcePos(BL_FLOAT *sourceR, BL_FLOAT *sourceTheta,
                                          BL_FLOAT magnL, BL_FLOAT magnR,
                                          BL_FLOAT phaseL, BL_FLOAT phaseR,
                                          BL_FLOAT freq, BL_FLOAT timeDelay)
{
//#define EPS 1e-15
    
    // Avoid numerical imprecision when magns are very small and similar
    // (was noticed by abnormal L/R phase inversions)
    //#define EPS 1e-8
    
    // See: https://dsp.stackexchange.com/questions/30736/how-to-calculate-an-azimuth-of-a-sound-source-in-stereo-signal
    // Thx :)
    //
        
#if DEBUG_DRAWER_DISPLAY
  if (std::fabs(freq - DEBUG_DRAWER_DISPLAY_FREQ) < BL_EPS8)
    {
        mDebugDrawer->SetD(0.0);
        mDebugDrawer->SetA(0.0);
        mDebugDrawer->SetB(0.0);
        mDebugDrawer->SetR(0.0);
        mDebugDrawer->SetTheta(0.0);
        
        mDebugDrawer->SetFail(0);
        
        GraphControl11 *graph = DebugGraph::GetGraph();
        //graph->SetDirty(true);
        mGraph->SetDataChanged();
    }
#endif
    
    // Init
    *sourceR = 0.0;
    *sourceTheta = 0.0;
    
    if (freq < BL_EPS8)
        // Just in case
    {
        // Null frequency
        // Can legitimately set the result to 0
        // and return true
        
        return true;
    }
    
    const BL_FLOAT d = MICROPHONES_SEPARATION;
    
#if DEBUG_DRAWER_DISPLAY
    if (std::fabs(freq - DEBUG_DRAWER_DISPLAY_FREQ) < BL_EPS8)
    {
        mDebugDrawer->SetD(d);
        
        GraphControl11 *graph = DebugGraph::GetGraph();
        //graph->SetDirty(true);
        mGraph->SetDataChanged();
    }
#endif
    
    // Sound speed: 340 m/s
    const BL_FLOAT c = 340.0;
    
#if INVERT_ALGO
    // Test if we must invert left and right for the later computations
    // In fact, the algorithm is designed to have G < 1.0,
    // i.e magnL < magnR
    //
    bool invertFlag = false;
    
    if (magnL > magnR)
    {
        invertFlag = true;
        
        BL_FLOAT tmpMagn = magnL;
        magnL = magnR;
        magnR = tmpMagn;
        
        BL_FLOAT tmpPhase = phaseL;
        phaseL = phaseR;
        phaseR = tmpPhase;
        
        // timeDelay ?;
        
        // TEST
        timeDelay = -timeDelay;
    }
#endif
        
    // Test at least if we have sound !
    if ((magnR < BL_EPS8) || (magnL < BL_EPS8))
        // No sound !
    {
        // Can legitimately set the result to 0
        // and return true
        
        //BLDebug::AppendValue("G0.txt", 0.0);
        
        return true;
    }
    
    // Compute the delay from the phase difference
    BL_FLOAT phaseDiff = phaseR - phaseL;
    
    // Avoid negative phase diff (would make problems below)
    phaseDiff = BLUtilsPhases::MapToPi(phaseDiff);
    phaseDiff += 2.0*M_PI;
    
    // Magns ratio
    BL_FLOAT G = magnL/magnR;
    //BLDebug::AppendValue("G0.txt", G);
    
    if (std::fabs(G - 1.0) < BL_EPS8)
        // Similar magns: can't compute...
    {    
        // We will be in mono
        
        return true;
    }
    
    // Time difference
    // (Unused)
    BL_FLOAT T = (1.0/freq)*(phaseDiff/(2.0*M_PI));
    
    // Distance to the right
    BL_FLOAT denomB = (1.0 - G);
    if (std::fabs(denomB) < BL_EPS8)
        // Similar magns (previously already tested)
    {
        // We will be in mono
        
        return true;
    }
    
#if !USE_TIME_DELAY
    BL_FLOAT b = G*T*c/denomB;
#else
    BL_FLOAT b = G*timeDelay*c/denomB;
#endif
    
#if DEBUG_DRAWER_DISPLAY
    if (std::fabs(freq - DEBUG_DRAWER_DISPLAY_FREQ) < BL_EPS8)
    {
        mDebugDrawer->SetB(b);
        
        GraphControl11 *graph = DebugGraph::GetGraph();
        //graph->SetDirty(true);
        mGraph->SetDataChanged();
    }
#endif
    
#if !USE_TIME_DELAY
    BL_FLOAT a = T*c;
#else
    
    BL_FLOAT a = timeDelay*c;
    
#endif
    
#if DEBUG_DRAWER_DISPLAY
    if (std::fabs(freq - DEBUG_DRAWER_DISPLAY_FREQ) < BL_EPS8)
    {
        mDebugDrawer->SetA(a);
        
        GraphControl11 *graph = DebugGraph::GetGraph();
        //graph->SetDirty(true);
        mGraph->SetDataChanged();
    }
#endif
    
    // r
    
    // Use the formula using a and b from he forum
    // (simpler for inversion later)
    
    // Formula from the forum (GOOD !)
    BL_FLOAT r2 = 4.0*b*b + 4.0*a*b + 2.0*a*a - d*d;
    if (r2 < 0.0)
        // No solution: stop
        return false;
    
    BL_FLOAT r = 0.5*std::sqrt(r2);
    
#if DEBUG_DRAWER_DISPLAY
    if (std::fabs(freq - DEBUG_DRAWER_DISPLAY_FREQ) < BL_EPS8)
    {
        mDebugDrawer->SetR(r);
        mDebugDrawer->SetTheta(0.0);
        
        GraphControl11 *graph = DebugGraph::GetGraph();
        //graph->SetDirty(true);
        mGraph->SetDataChanged();
    }
#endif
    
#if 1    // Method from the forum
    // => problem here because the two mics are too close,
    // and acos is > 1.0 so we can't get the angle
    
    // Theta
    BL_FLOAT cosTheta = r/d + d/(4.0*r) - b*b/(d*r);
    
    if ((cosTheta < -1.0) || (cosTheta > 1.0))
    // No solution: stop
    {
#if DEBUG_DRAWER_DISPLAY
      if (std::fabs(freq - DEBUG_DRAWER_DISPLAY_FREQ) < BL_EPS8)
        {
            mDebugDrawer->SetFail(1);
            
            GraphControl11 *graph = DebugGraph::GetGraph();
            //graph->SetDirty(true);
            mGraph->SetDataChanged();
        }
#endif
        
        return false;
    }
    
    BL_FLOAT theta = std::acos(cosTheta);
#endif
    
#if 0 // Method from paper toni.heittola@tut.f
    BL_FLOAT theta = 2.0*M_PI*std::atan(G)/M_PI - M_PI/2.0;
#endif
    
#if DEBUG_DRAWER_DISPLAY
    if (std::fabs(freq - DEBUG_DRAWER_DISPLAY_FREQ) < BL_EPS8)
    {
        mDebugDrawer->SetTheta(theta);
        
        GraphControl11 *graph = DebugGraph::GetGraph();
        //graph->SetDirty(true);
        mGraph->SetDataChanged();
    }
#endif
    
#if INVERT_ALGO
    if (invertFlag)
        theta = M_PI - theta;
#endif
        
    *sourceR = r;
    *sourceTheta = theta;
    
    return true;
}

void
StereoWidthProcess2::SourcePosToMagnPhases(const WDL_TypedBuf<BL_FLOAT> &sourceRs,
                                          const WDL_TypedBuf<BL_FLOAT> &sourceThetas,
                                          BL_FLOAT micsDistance,
                                          BL_FLOAT widthFactor,
                                          WDL_TypedBuf<BL_FLOAT> *ioMagnsL,
                                          WDL_TypedBuf<BL_FLOAT> *ioMagnsR,
                                          WDL_TypedBuf<BL_FLOAT> *ioPhasesL,
                                          WDL_TypedBuf<BL_FLOAT> *ioPhasesR,
                                          const WDL_TypedBuf<BL_FLOAT> &freqs,
                                          const WDL_TypedBuf<BL_FLOAT> &timeDelays)
{
    if (ioMagnsL->GetSize() != ioMagnsR->GetSize())
        // R can be empty in mono
        return;
    
    if (ioPhasesL->GetSize() != ioPhasesR->GetSize())
        // Just in case
        return;
    
    // Compute the new magns and phases (depending on the width factor)
    for (int i = 0; i < ioPhasesL->GetSize(); i++)
    {
        // Retrieve the left and right distance
        BL_FLOAT sourceR = sourceRs.Get()[i];
        BL_FLOAT sourceTheta = sourceThetas.Get()[i];
        
        BL_FLOAT freq = freqs.Get()[i];
        
        // Init
        BL_FLOAT newMagnL = ioMagnsL->Get()[i];
        BL_FLOAT newMagnR = ioMagnsR->Get()[i];
        
        BL_FLOAT newPhaseL = ioPhasesL->Get()[i];
        BL_FLOAT newPhaseR = ioPhasesR->Get()[i];
        
        // Unused ?
        BL_FLOAT timeDelay = timeDelays.Get()[i];
        
        bool computed = SourcePosToMagnPhases2(sourceR, sourceTheta,
                                               micsDistance,
                                               &newMagnL, &newMagnR,
                                               &newPhaseL, &newPhaseR, freq,
                                               timeDelay);
        
        // The result can be undfined for "noisy" parts of the signal
        // (difficult to locate a background noise...)
        if (!computed)
        {
            BL_FLOAT phaseFactor = widthFactor;
                        
            // For these parts, simply narrow or enlarge the phase
            ModifyPhase(&newPhaseL, &newPhaseR, phaseFactor);
        }
        
        // Result
        ioPhasesL->Get()[i] = newPhaseL;
        ioPhasesR->Get()[i] = newPhaseR;
        
        ioMagnsL->Get()[i] = newMagnL;
        ioMagnsR->Get()[i] = newMagnR;
    }
}

void
StereoWidthProcess2::SourcePosToMagnPhasesHRTF(const WDL_TypedBuf<BL_FLOAT> &sourceRs,
                                               const WDL_TypedBuf<BL_FLOAT> &sourceThetas,
                                               BL_FLOAT micsDistance,
                                               BL_FLOAT widthFactor,
                                               WDL_TypedBuf<BL_FLOAT> *ioMagnsL,
                                               WDL_TypedBuf<BL_FLOAT> *ioMagnsR,
                                               WDL_TypedBuf<BL_FLOAT> *ioPhasesL,
                                               WDL_TypedBuf<BL_FLOAT> *ioPhasesR,
                                               const WDL_TypedBuf<BL_FLOAT> &freqs,
                                               const WDL_TypedBuf<BL_FLOAT> &timeDelays)
{
    if (ioMagnsL->GetSize() != ioMagnsR->GetSize())
        // R can be empty in mono
        return;
    
    if (ioPhasesL->GetSize() != ioPhasesR->GetSize())
        // Just in case
        return;
    
#if 0
    WDL_TypedBuf<BL_FLOAT> monoMagns;
    BLUtils::StereoToMono(&monoMagns, *ioMagnsL, *ioMagnsR);
#else
    WDL_TypedBuf<BL_FLOAT> monoMagns = *ioMagnsL;
#endif
    
    WDL_TypedBuf<BL_FLOAT> monoPhases = *ioPhasesL;
    
    // The slices, to be passed to the HRTF
    WDL_TypedBuf<BL_FLOAT> sliceMagns[NUM_HRTF_SLICES];
    WDL_TypedBuf<BL_FLOAT> slicePhases[NUM_HRTF_SLICES];
    
    // Init
    for (int i = 0; i < NUM_HRTF_SLICES; i++)
    {
        BLUtils::ResizeFillZeros(&sliceMagns[i], sourceThetas.GetSize());
        BLUtils::ResizeFillZeros(&slicePhases[i], sourceThetas.GetSize());
    }
    
    // Fill the slices
    for (int i = 0; i < sourceThetas.GetSize(); i++)
    {
        BL_FLOAT theta = sourceThetas.Get()[i];
        int sliceNum = ((theta + M_PI/2.0)/M_PI)*NUM_HRTF_SLICES;
        
        if ((sliceNum < 0) || (sliceNum >= NUM_HRTF_SLICES))
            sliceNum = NUM_HRTF_SLICES/2;
        
        sliceMagns[sliceNum].Get()[i] = monoMagns.Get()[i];
        slicePhases[sliceNum].Get()[i] = monoPhases.Get()[i];
    }
    
    WDL_TypedBuf<BL_FLOAT> resultMagns[NUM_HRTF_SLICES][2];
    WDL_TypedBuf<BL_FLOAT> resultPhases[NUM_HRTF_SLICES][2];
    
    // Pass the slices to the HRTF
    for (int i = 0; i < NUM_HRTF_SLICES; i++)
        for (int j = 0; j < 2; j++)
        {
            mConvolvers[i][j]->Process(sliceMagns[i], slicePhases[i],
                                       &resultMagns[i][j], &resultPhases[i][j]);
        }
    
    // Sum the result
    //
    
    // Reset
    BLUtils::FillAllZero(ioMagnsL);
    BLUtils::FillAllZero(ioMagnsR);
    
    BLUtils::FillAllZero(ioPhasesL);
    BLUtils::FillAllZero(ioPhasesR);
    
    // Sum
    for (int i = 0; i < NUM_HRTF_SLICES; i++)
    {
        BLUtilsComp::ComplexSum(ioMagnsL, ioPhasesL,
                                resultMagns[i][0], resultPhases[i][0]);
        BLUtilsComp::ComplexSum(ioMagnsR, ioPhasesR,
                                resultMagns[i][1], resultPhases[i][1]);
    }
}

bool
StereoWidthProcess2::SourcePosToMagnPhases(BL_FLOAT sourceR, BL_FLOAT sourceTheta,
                                          BL_FLOAT d,
                                          BL_FLOAT *ioMagnL, BL_FLOAT *ioMagnR,
                                          BL_FLOAT *ioPhaseL, BL_FLOAT *ioPhaseR,
                                          BL_FLOAT freq,
                                          BL_FLOAT timeDelay /* unused ? */)
{
    //#define EPS 1e-15
    
#if INVERT_ALGO
    bool invertFlag = (*ioMagnL > *ioMagnR);
    if (invertFlag)
        sourceTheta = M_PI - sourceTheta;
#endif
    
    if (sourceR < BL_EPS)
    // The source position was not previously computed (so not usable)
    // Do not modify the signal !
        return false;
    
    if (d < BL_EPS)
        // The two mics are at the same position
        // => the signal should be mono !
    {        
        BL_FLOAT middleMagn = (*ioMagnL + *ioMagnR)/2.0;
        *ioMagnL = middleMagn;
        *ioMagnR = middleMagn;
        
        // Take the left channel for the phase
        *ioPhaseR = *ioPhaseL;
        
        return true;
    }
    
    // Reverse process of the previous method
    
    // 340 m/s
    const BL_FLOAT c = 340.0;
    
    // b
    BL_FLOAT b2 = (sourceR/d + d/(4.0*sourceR) - std::cos(sourceTheta))*d*sourceR;
    
    if (b2 < 0.0)
        // No solution
        return false;
    
    BL_FLOAT b = std::sqrt(b2);
    
    // a
    
#if 0 // Median theorem
    
    // See: https://www.algebra.com/algebra/homework/word/geometry/The-length-of-a-median-of-a-triangle.lesson
    // (length of a median of a triangle)
    BL_FLOAT sq = 2.0*sourceR*sourceR - b*b + 0.5*micsDistance*micsDistance;
    
    if (sq < 0.0)
    // No solution
        return false;
    
    BL_FLOAT a = std::sqrt(sq) - b;
#endif

#if 0 // Al-Kashi theorem
    BL_FLOAT sq = sourceR*sourceR + 0.25*micsDistance*micsDistance -
                2.0*sourceR*micsDistance*cos(M_PI - sourceTheta);
    
    if (sq < 0.0)
    // No solution
        return false;
    
    BL_FLOAT a = std::sqrt(sq) - b;
#endif
   
#if 1 // Compute A from the source coordinates
    BL_FLOAT sX = sourceR*cos(sourceTheta);
    BL_FLOAT sY = -sourceR*sin(sourceTheta);
    
    BL_FLOAT pX = -0.5*d;
    BL_FLOAT pY = 0.0;
    
    BL_FLOAT aPlusB2 = (sX - pX)*(sX - pX) + (sY - pY)*(sY - pY);
    BL_FLOAT a = std::sqrt(aPlusB2) - b;
#endif

    // GOOD !
    // In all cases, take the abs of a
    a = std::fabs(a);
    
    if (std::fabs(a + b) < BL_EPS)
    // Incorrect
        return false;
    
    // G
    BL_FLOAT G = b/(a + b);
        
    BL_FLOAT denomT = G*c;
    if (std::fabs(denomT) < BL_EPS)
    // One magn is very low
        return false;
    
    // T
    BL_FLOAT T = (b*(1.0 - G))/denomT;
        
    // Phase diff
    BL_FLOAT phaseDiff = T*freq*2.0*M_PI;
    //BL_FLOAT phaseDiff = timeDelay*freq*2.0*M_PI;

#if INVERT_ALGO
    if (invertFlag)
    {
        if (G > 0.0)
            G = 1.0/G;
        
        phaseDiff = 2.0*M_PI - phaseDiff;
    }
#endif
    
    // GOOD ... (old)
#if 0 // Method 1: take the left, and adjust for the right
    const BL_FLOAT origMagnL = *ioMagnL;
    const BL_FLOAT origPhaseL = *ioPhaseL;
    
    // Method "1"
    *ioMagnR = origMagnL/G;
    *ioPhaseR = origPhaseL + phaseDiff;
    
    *ioPhaseR = BLUtils::MapToPi(*ioPhaseR);
#endif
    
    // GOOD: for identity
#if 1 // Method 2: take the middle, and adjust for left and right
    
    // For magns, take the middle and compute the extremities
    BL_FLOAT middleMagn = (*ioMagnL + *ioMagnR)/2.0;
    
    *ioMagnR = 2.0*middleMagn/(G + 1.0);
    *ioMagnL = *ioMagnR*G;
    
    // For phases, take the left and compute the right
    *ioPhaseR = *ioPhaseL + phaseDiff;
    *ioPhaseR = BLUtilsPhases::MapToPi(*ioPhaseR);
#endif
    
    return true;
}

bool
StereoWidthProcess2::SourcePosToMagnPhases2(BL_FLOAT sourceR, BL_FLOAT sourceTheta,
                                           BL_FLOAT d,
                                           BL_FLOAT *ioMagnL, BL_FLOAT *ioMagnR,
                                           BL_FLOAT *ioPhaseL, BL_FLOAT *ioPhaseR,
                                           BL_FLOAT freq,
                                           BL_FLOAT timeDelay /* unused ! */)
{
//#define EPS 1e-15
    
    // Avoid numerical imprecision when magns are very small and similar
    // (was noticed by abnormal L/R phase inversions)
    //#define EPS 1e-8
    
    if (sourceR < BL_EPS8)
        // The source position was not previously computed (so not usable)
        // Do not modify the signal !
    {
        return false;
    }
    
#if INVERT_ALGO
    bool invertFlag = (*ioMagnL > *ioMagnR);
    if (invertFlag)
        sourceTheta = M_PI - sourceTheta;
#endif
    
    if (d < BL_EPS8)
        // The two mics are at the same position
        // => the signal should be mono !
    {
        BL_FLOAT middleMagn = (*ioMagnL + *ioMagnR)/2.0;
        *ioMagnL = middleMagn;
        *ioMagnR = middleMagn;
        
        // Take the left channel for the phase
        *ioPhaseR = *ioPhaseL;
        
        return true;
    }
    
    // Reverse process of the previous method
    
    // 340 m/s
    const BL_FLOAT c = 340.0;
    
    // b
    BL_FLOAT b2 = (sourceR/d + d/(4.0*sourceR) - std::cos(sourceTheta))*d*sourceR;
    if (b2 < 0.0)
    {
        // No solution
        return false;
    }
    
    BL_FLOAT b = std::sqrt(b2);
    
    // Second order equation resolution
    BL_FLOAT A0 = 0.5;
    BL_FLOAT B0 = b;
    BL_FLOAT C0 = -(sourceR*sourceR - b*b + d*d/4.0);
    
    BL_FLOAT a2[2];
    int numSol = SecondOrderEqSolve(A0, B0, C0, a2);
    if (numSol == 0)
    {
        return false;
    }
    
    BL_FLOAT a;
    if (numSol == 1)
        a = a2[0];
    if (numSol == 2)
    {
        if ((a2[0] >= 0.0) && (a2[1] < 0.0))
            a = a2[0];
        else
            if ((a2[0] < 0.0) && (a2[1] >= 0.0))
                a = a2[1];
        else
        {
            // See: https://dsp.stackexchange.com/questions/30736/how-to-calculate-an-azimuth-of-a-sound-source-in-stereo-signal
            // Condition: a + b > r
            
            if (a2[0] + b > sourceR)
            {
                a = a2[0];
            }
            else if (a2[1] + b > sourceR)
            {
                a = a2[1];
            } else
            {
                // No solution !
                return false;
            }
            
        }
    }
    
    
    // Compute phaseDiff
    BL_FLOAT timeDelay0 = a/c;
    
    BL_FLOAT denomG = (timeDelay0*c + b);
    if (std::fabs(denomG) < BL_EPS8)
    {
        return false;
    }
    
    BL_FLOAT G = b/denomG;
    
    // Phase diff
    BL_FLOAT phaseDiff = timeDelay0*freq*2.0*M_PI;
    
#if INVERT_ALGO
    if (invertFlag)
    {
        if (G > 0.0)
            G = 1.0/G;
        
        phaseDiff = 2.0*M_PI - phaseDiff;
    }
#endif
    
    // Added after volume rendering tests
    phaseDiff = BLUtilsPhases::MapToPi(phaseDiff);
    phaseDiff += 2.0*M_PI;
    
    // GOOD ... (old)
#if 0 // Method 1: take the left, and adjust for the right
    const BL_FLOAT origMagnL = *ioMagnL;
    const BL_FLOAT origPhaseL = *ioPhaseL;
    
    // Method "1"
    *ioMagnR = origMagnL/G;
    *ioPhaseR = origPhaseL + phaseDiff;
    
    *ioPhaseR = BLUtils::MapToPi(*ioPhaseR);
#endif
    
    // GOOD: for identity
#if 1 // Method 2: take the middle, and adjust for left and right
    
    // For magns, take the middle and compute the extremities
    BL_FLOAT middleMagn = (*ioMagnL + *ioMagnR)/2.0;
    
    *ioMagnR = 2.0*middleMagn/(G + 1.0);
    *ioMagnL = *ioMagnR*G;
    
    // For phases, take the left and compute the right
#if !INVERT_ALGO
    *ioPhaseR = *ioPhaseL + phaseDiff;
#else
    
    // NEW
    *ioPhaseR = *ioPhaseL + phaseDiff;
    
#if 0 //OLD
    if (invertFlag)
    {
        *ioPhaseR = *ioPhaseL + phaseDiff;
    }
    else
    {
        *ioPhaseL = *ioPhaseR + phaseDiff;
    }
#endif
    
    *ioPhaseL = BLUtilsPhases::MapToPi(*ioPhaseL);
    *ioPhaseR = BLUtilsPhases::MapToPi(*ioPhaseR);
#endif
    
#endif
    
    return true;
}

BL_FLOAT
StereoWidthProcess2::ComputeFactor(BL_FLOAT normVal, BL_FLOAT maxVal)
{
    BL_FLOAT res = 0.0;
    if (normVal < 0.0)
        res = normVal + 1.0;
    else
        res = normVal*(maxVal - 1.0) + 1.0;

    return res;
}

void
StereoWidthProcess2::ComputeTimeDelays(WDL_TypedBuf<BL_FLOAT> *timeDelays,
                                      const WDL_TypedBuf<BL_FLOAT> &phasesL,
                                      const WDL_TypedBuf<BL_FLOAT> &phasesR)
{
    if (phasesL.GetSize() != phasesR.GetSize())
        // R can be empty if we are in mono
        return;
    
    timeDelays->Resize(phasesL.GetSize());
    
    WDL_TypedBuf<BL_FLOAT> samplesIdsL;
    BLUtilsFft::FftIdsToSamplesIdsFloat(phasesL, &samplesIdsL);
    
    WDL_TypedBuf<BL_FLOAT> samplesIdsR;
    BLUtilsFft::FftIdsToSamplesIdsFloat(phasesR, &samplesIdsR);
    
    for (int i = 0; i < timeDelays->GetSize(); i++)
    {
        BL_FLOAT sampIdL = samplesIdsL.Get()[i];
        BL_FLOAT sampIdR = samplesIdsR.Get()[i];
        
        BL_FLOAT diff = sampIdR - sampIdL;
        
        BL_FLOAT delay = diff/mSampleRate;
        
        timeDelays->Get()[i] = delay;
    }
}

void
StereoWidthProcess2::ModifyPhase(BL_FLOAT *phaseL,
                                BL_FLOAT *phaseR,
                                BL_FLOAT factor)
{
    BL_FLOAT phaseDiff = *phaseR - *phaseL;
    
    *phaseR = *phaseL + factor*phaseDiff;
}

void
StereoWidthProcess2::TemporalSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
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
StereoWidthProcess2::SpatialSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
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
StereoWidthProcess2::NormalizeRs(WDL_TypedBuf<BL_FLOAT> *sourceRs,
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
StereoWidthProcess2::ComputeColorWeightsMagns(WDL_TypedBuf<BL_FLOAT> *outWeights,
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
StereoWidthProcess2::ComputeColorWeightsFreqs(WDL_TypedBuf<BL_FLOAT> *outWeights,
                                             const WDL_TypedBuf<BL_FLOAT> &freqs)
{
    outWeights->Resize(freqs.GetSize());
    
    for (int i = 0; i < outWeights->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/outWeights->GetSize();
    
        // Logical to use log since frequency preception is logarithmic
        t = BLUtils::AmpToDBNorm(t, (BL_FLOAT)1e-15, (BL_FLOAT)-120.0);
        
        BL_FLOAT w = (t + 1.0)/2.0;
        
        //w = BLUtils::ApplyParamShape(w, 4.0);
        w *= 2.0;
        
        outWeights->Get()[i] = w;
    }
}

int
StereoWidthProcess2::SecondOrderEqSolve(BL_FLOAT a, BL_FLOAT b, BL_FLOAT c,
                                        BL_FLOAT res[2])
{
    // See: http://math.lyceedebaudre.net/premiere-sti2d/second-degre/resoudre-une-equation-du-second-degre
    //
    BL_FLOAT delta = b*b - 4.0*a*c;
    
    if (delta > 0.0)
    {
      res[0] = (-b - std::sqrt(delta))/(2.0*a);
      res[1] = (-b + std::sqrt(delta))/(2.0*a);
        
        return 2;
    }
    
    //#define EPS 1e-15
    if (std::fabs(delta) < BL_EPS)
    {
        res[0] = -b/(2.0*a);
        
        return 1;
    }
    
    return 0;
}

void
StereoWidthProcess2::InitHRTF(IGraphics *pGraphics)
{
    // Get resources dir
    //WDL_String wdlResDir;
    //pGraphics->GetResourceDir(&wdlResDir);
    //const char *resDir = wdlResDir.Get();
    
    const char *resDir = pGraphics->GetSharedResourcesSubPath();
    
    // For fft convolution
    KemarHRTF::Load(resDir, &mHrtf);
    
    for (int i = 0; i < NUM_HRTF_SLICES; i++)
    {
        BL_FLOAT azimuth = (((BL_FLOAT)i)/NUM_HRTF_SLICES - 0.5)*180.0;
        
        if (azimuth < 0.0)
            azimuth += 360.0;
        azimuth = std::fmod(azimuth, 360.0);
        
        BL_FLOAT elevation = 0.0;
        
        for (int j = 0; j < 2; j++)
        {
	  //mConvolvers[i][j] = new FftConvolver7(BUFFER_SIZE, true, false, false);
	  mConvolvers[i][j] = new FftConvolver6(STEREOWIDTH_BUFFER_SIZE,
                                            true, false, false);
            
            WDL_TypedBuf<BL_FLOAT> impulse;
            bool found = mHrtf->GetImpulseResponse(&impulse, elevation, azimuth, j);
            if (found)
            {
                mConvolvers[i][j]->SetResponse(&impulse);
            }
        }
    }
}

void
StereoWidthProcess2::SimpleStereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples, BL_FLOAT widthChange)
{
    BL_FLOAT width = ComputeFactor(widthChange, MAX_WIDTH_CHANGE_SIMPLE_ALGO);
    
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
        BL_FLOAT tmp = 1.0/max(1.0 + width, 2.0);
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
