//
//  BLVectorscope.cpp
//  UST
//
//  Created by applematuer on 7/27/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <math.h>

#include <GraphControl12.h>
#include <GraphCurve5.h>

#include <BLCircleGraphDrawer.h>
#include <BLLissajousGraphDrawer.h>
#include <BLUpmixGraphDrawer.h>
#include <StereoWidthGraphDrawer3.h>

#include <BLVectorscopeProcess.h>

#include <BLFireworks.h>

#include <SourceComputer.h>

#include <BLUtils.h>

#include "BLVectorscope.h"

#ifndef M_PI
#define M_PI 3.141592653589
#endif

// NOTE: can gain 10% CPU by setting to 1024
// (but the display will be a bit too quick)
#define NUM_POINTS 4096
// Keep a sliding point set, for animation fluidity independently of block size
#define NUM_POINTS_TO_CONSUME 512

// Bounds
//

// Graph0
#define GRAPH0_MIN_X -1.0
#define GRAPH0_MAX_X 1.0

#define GRAPH0_MIN_Y 0.0
#define GRAPH0_MAX_Y 1.0

// Graph1
#define GRAPH1_MIN_X -1.0
#define GRAPH1_MAX_X 1.0

#define GRAPH1_MIN_Y 0.0
#define GRAPH1_MAX_Y 1.0


// Graph2
#define GRAPH2_MIN_X -1.0
#define GRAPH2_MAX_X 1.0

#define GRAPH2_MIN_Y -1.0
#define GRAPH2_MAX_Y 1.0

// Graph4
#define GRAPH4_MIN_X -1.0
#define GRAPH4_MAX_X 1.0

#define GRAPH4_MIN_Y 0.0
#define GRAPH4_MAX_Y 1.0

//
#define POINT_SIZE_MODE0 5.0
#define POINT_SIZE_MODE1 2.0
#define LINE_SIZE_MODE2 2.0 //1.0
#define LINE_SIZE2_MODE2 3.0
#define POINT_SIZE_MODE4 5.0

#define POINT_ALPHA 0.25 //1.0

// Curve to display points
#define CURVE_POINTS0 0
#define CURVE_POINTS1 1

#define SQR2_INV 0.70710678118655

// Same as BLVectorscopeProcess::CLIP_DISTANCE
// So when saturate, the saturation line will be exactly on the circle drawer
//#define SCALE_POLAR_Y 0.95
// New design:
#define SCALE_POLAR_X 0.96 //0.95
#define SCALE_POLAR_Y 0.91


//
#define LISSAJOUS_SCALE 0.8

#define OPTIM_QUAD_SAME_COLOR 1

// Avoid re-sending many points for small buffer size
// => simply wait that we have enough points
#define OPTIM_SMALL_BLOCK_SIZE 1

BLVectorscope::BLVectorscope(BLVectorscopePlug *plug,
                             BL_FLOAT sampleRate)
{
    mPlug = plug;
    
    for (int i = 0; i < NUM_MODES; i++)
        mGraphs[i] = NULL;
    
    for (int i = 0; i < NUM_MODES; i++)
        mCurves[i] = new GraphCurve5(NUM_POINTS);

    mCircleDrawerPolarSamples = NULL;
    mCircleDrawerFireworks = NULL;
    mLissajousDrawer = NULL;
    mUpmixDrawer = NULL;
    mCircleDrawerSource = NULL;
    
    mFireworks = new BLFireworks(sampleRate);
    
    mSourceComputer = new SourceComputer(sampleRate);
    
    mMode = POLAR_SAMPLE;
}

BLVectorscope::~BLVectorscope()
{
    delete mFireworks;
    
    delete mSourceComputer;
    
    for (int i = 0; i < NUM_MODES; i++)
        delete mCurves[i];
}

void
BLVectorscope::Reset(BL_FLOAT sampleRate)
{
    mFireworks->Reset(sampleRate);
    mSourceComputer->Reset(sampleRate);
}

void
BLVectorscope::Reset()
{
    mFireworks->Reset();
    mSourceComputer->Reset();
}

void
BLVectorscope::SetMode(Mode mode)
{
    mMode = mode;
    
    for (int i = 0; i < NUM_MODES; i++)
    {
        if (mGraphs[i] != NULL)
        {
            bool enabled = (i == mode);
            mGraphs[i]->SetEnabled(enabled);
            
            if (enabled)
            {
                mGraphs[i]->SetInteractionDisabled(false);
                mGraphs[i]->SetDirty(false);//
            }
            else
            {
                mGraphs[i]->SetInteractionDisabled(true);
                mGraphs[i]->SetClean();
            }
        }
    }
    
    mFireworks->Reset();
    mSourceComputer->Reset();
}

void
BLVectorscope::OnMouseDown(float x, float y, const IMouseMod &mod)
{
    if (mod.C)
    {
        // Reset parameters
        mPlug->VectorscopeResetAllParamsCB();
    
        return;
    }
    
    if (mMode == UPMIX)
    {
        // Upmix graph drawer also manage control
        mUpmixDrawer->OnMouseDown(x, y, mod);
    }
}

void
BLVectorscope::OnMouseUp(float x, float y, const IMouseMod &mod)
{
    if (mMode == UPMIX)
    {
        // Upmix graph drawer also manage control
        mUpmixDrawer->OnMouseUp(x, y, mod);
        
        return;
    }
}

void
BLVectorscope::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod &mod)
{
    if (mMode == UPMIX)
    {
        // Upmix graph drawer also manage control
        mUpmixDrawer->OnMouseDrag(x, y, dX, dY, mod);
        
        return;
    }
    
//#define COEFF 0.01
//#define FINE_COEFF 0.1
    
#define COEFF 0.005
#define FINE_COEFF 0.2
    
    BL_FLOAT dpan = 0.0;
    BL_FLOAT dwidth = 0.0;
    
    if (!mod.A)
        dpan = ((BL_FLOAT)dX)*COEFF;
    else
        dwidth = -((BL_FLOAT)dY)*COEFF;
    
    if (mod.S)
    {
        dpan *= FINE_COEFF;
        dwidth *= FINE_COEFF;
    }
    
    mPlug->VectorscopeUpdateDPanCB(dpan);
    mPlug->VectorscopeUpdateDWidthCB(dwidth);
}

void
BLVectorscope::SetGraphs(GraphControl12 *graph0,
                         GraphControl12 *graph1,
                         GraphControl12 *graph2,
                         GraphControl12 *graph3,
                         GraphControl12 *graph4,
                         GUIHelper12 *guiHelper)
{
    // Assign
    mGraphs[POLAR_SAMPLE_MODE_ID] = graph0;
    mGraphs[LISSAJOUS_MODE_ID] = graph1;
    mGraphs[FIREWORKS_MODE_ID] = graph2;
    mGraphs[UPMIX_MODE_ID] = graph3;
    mGraphs[SOURCE_MODE_ID] = graph4;
    
    // Fireworks has 2 curves.
    // And Upmix has no curve
    if (mGraphs[POLAR_SAMPLE_MODE_ID] != NULL)
        mGraphs[POLAR_SAMPLE_MODE_ID]->AddCurve(mCurves[POLAR_SAMPLE_CURVE_ID]);
    
    if (mGraphs[LISSAJOUS_MODE_ID] != NULL)
        mGraphs[LISSAJOUS_MODE_ID]->AddCurve(mCurves[LISSAJOUS_CURVE_ID]);
    
    if (mGraphs[FIREWORKS_MODE_ID] != NULL)
    {
        mGraphs[FIREWORKS_MODE_ID]->AddCurve(mCurves[FIREWORKS_CURVE0_ID]);
        mGraphs[FIREWORKS_MODE_ID]->AddCurve(mCurves[FIREWORKS_CURVE1_ID]);
    }
    
    if (mGraphs[SOURCE_MODE_ID] != NULL)
        mGraphs[SOURCE_MODE_ID]->AddCurve(mCurves[SOURCE_CURVE_ID]);
    
    //
    if (mGraphs[POLAR_SAMPLE_MODE_ID] != NULL)
        mGraphs[POLAR_SAMPLE_MODE_ID]->AddCustomControl(this);
    if (mGraphs[LISSAJOUS_MODE_ID] != NULL)
        mGraphs[LISSAJOUS_MODE_ID]->AddCustomControl(this);
    if (mGraphs[FIREWORKS_MODE_ID] != NULL)
        mGraphs[FIREWORKS_MODE_ID]->AddCustomControl(this);
    if (mGraphs[UPMIX_MODE_ID] != NULL)
        mGraphs[UPMIX_MODE_ID]->AddCustomControl(this);
    if (mGraphs[SOURCE_MODE_ID] != NULL)
        mGraphs[SOURCE_MODE_ID]->AddCustomControl(this);
    
    // Graph0
    if (mGraphs[POLAR_SAMPLE_MODE_ID] != NULL)
    {
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetClearColor(0, 0, 0, 255);
    
        mCircleDrawerPolarSamples = new BLCircleGraphDrawer(guiHelper,
                                                            "POLAR SAMPLES");
        mGraphs[POLAR_SAMPLE_MODE_ID]->AddCustomDrawer(mCircleDrawerPolarSamples);
    
        // Style
        //
        int pointColor[3] = { 113, 130, 182 };
        
        // To display lines instead of points
        bool pointsAsLines = false;
        BL_FLOAT alpha = POINT_ALPHA;
        bool overlay = true;
        
        bool bevelFlag = false;
        
        SetCurveStyle(mGraphs[POLAR_SAMPLE_MODE_ID],
                      POLAR_SAMPLE_CURVE_ID,
                      GRAPH0_MIN_X, GRAPH0_MAX_X,
                      GRAPH0_MIN_Y, GRAPH0_MAX_Y,
                      true, POINT_SIZE_MODE0,
                      bevelFlag,
                      pointColor[0], pointColor[1], pointColor[2],
                      false, alpha, pointsAsLines, overlay, false);

#if OPTIM_QUAD_SAME_COLOR
        mCurves[POLAR_SAMPLE_CURVE_ID]->SetOptimSameColor(true);
#endif
        
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    // Graph for Lissajouss
    if (mGraphs[LISSAJOUS_MODE_ID] != NULL)
    {
        mGraphs[LISSAJOUS_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraphs[LISSAJOUS_MODE_ID]->SetClearColor(0, 0, 0, 255);
        
        mLissajousDrawer = new BLLissajousGraphDrawer(LISSAJOUS_SCALE,
                                                      guiHelper,
                                                      "LISSAJOUS");
        mGraphs[LISSAJOUS_MODE_ID]->AddCustomDrawer(mLissajousDrawer);
        
        // Style
        //
        int pointColor[3] = { 113, 130, 182 };
        
        // To display lines instead of points
        bool pointsAsLines = false;
        BL_FLOAT alpha = POINT_ALPHA;
        bool overlay = true;
        bool bevelFlag = false;
        
        SetCurveStyle(mGraphs[LISSAJOUS_MODE_ID],
                      LISSAJOUS_CURVE_ID,
                      GRAPH2_MIN_X, GRAPH2_MAX_X,
                      GRAPH2_MIN_Y, GRAPH2_MAX_Y,
                      true, POINT_SIZE_MODE1,
                      bevelFlag,
                      pointColor[0], pointColor[1], pointColor[2],
                      false, alpha, pointsAsLines, overlay, false);

#if OPTIM_QUAD_SAME_COLOR
        mCurves[LISSAJOUS_CURVE_ID]->SetOptimSameColor(true);
#endif
        
        mGraphs[LISSAJOUS_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    // Graph for fireworks
    if (mGraphs[FIREWORKS_MODE_ID] != NULL)
    {
        mGraphs[FIREWORKS_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraphs[FIREWORKS_MODE_ID]->SetClearColor(0, 0, 0, 255);
        
        mCircleDrawerFireworks = new BLCircleGraphDrawer(guiHelper, "FLAME");
        mGraphs[FIREWORKS_MODE_ID]->AddCustomDrawer(mCircleDrawerFireworks);
        
        // Style
        //
        int pointColor0[3] = { 153, 176, 246 };
        int pointColor1[3] = { 255, 255, 255 };
        
        // To display lines instead of points
        bool pointsAsLines = true;
        BL_FLOAT alpha = 1.0;
        bool overlayFlag = false;
        
        bool fillFlag0 = false;
        bool linesPolarFlag0 = true;
        bool linesPolarFlag1 = false;
        bool bevelFlag0 = false;
        
        SetCurveStyle(mGraphs[FIREWORKS_MODE_ID],
                      FIREWORKS_CURVE0_ID,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true, LINE_SIZE_MODE2,
                      bevelFlag0,
                      pointColor0[0], pointColor0[1], pointColor0[2],
                      fillFlag0, alpha, pointsAsLines, overlayFlag, linesPolarFlag0);
        
        bool fillFlag1 = false;
        bool bevelFlag1 = true;
        SetCurveStyle(mGraphs[FIREWORKS_MODE_ID],
                      FIREWORKS_CURVE1_ID,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true, LINE_SIZE2_MODE2,
                      bevelFlag1,
                      pointColor1[0], pointColor1[1], pointColor1[2],
                      fillFlag1, alpha, pointsAsLines, overlayFlag, linesPolarFlag1);
        
        mGraphs[FIREWORKS_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    // Graph for upmix
    if (mGraphs[UPMIX_MODE_ID] != NULL)
    {
        mGraphs[UPMIX_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraphs[UPMIX_MODE_ID]->SetClearColor(0, 0, 0, 255);
        
        mUpmixDrawer = new BLUpmixGraphDrawer(mPlug, mGraphs[UPMIX_MODE_ID],
                                              guiHelper, "GRID");
        mGraphs[UPMIX_MODE_ID]->AddCustomDrawer(mUpmixDrawer);
        
        mGraphs[UPMIX_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    // Graph4
    if (mGraphs[SOURCE_MODE_ID] != NULL)
    {
        mGraphs[SOURCE_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraphs[SOURCE_MODE_ID]->SetClearColor(0, 0, 0, 255);
        
        mCircleDrawerSource = new StereoWidthGraphDrawer3(guiHelper, "SOURCE");
        
        mGraphs[SOURCE_MODE_ID]->AddCustomDrawer(mCircleDrawerSource);
        
        // Style
        //
        int pointColor[3] = { 113, 130, 182 };
        
        // To display lines instead of points
        bool pointsAsLines = false;
        BL_FLOAT alpha = POINT_ALPHA;
        bool overlay = true;
        
        bool bevelFlag = false;
        
        SetCurveStyle(mGraphs[SOURCE_MODE_ID],
                      SOURCE_CURVE_ID,
                      GRAPH4_MIN_X, GRAPH4_MAX_X,
                      GRAPH4_MIN_Y, GRAPH4_MAX_Y,
                      true, POINT_SIZE_MODE4,
                      bevelFlag,
                      pointColor[0], pointColor[1], pointColor[2],
                      false, alpha, pointsAsLines, overlay, false);
        
        mGraphs[SOURCE_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    SetMode(mMode);
}

void
BLVectorscope::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples)
{
    if (samples.size() != 2)
        return;

#if !OPTIM_SMALL_BLOCK_SIZE
    for (int i = 0; i < 2; i++)
    {
        mSamples[i].Add(samples[i].Get(), samples[i].GetSize());
        
        //int numToConsume = mSamples[i].GetSize() - NUM_POINTS;
        int numToConsume = mSamples[i].Available() - NUM_POINTS;
        if (numToConsume > 0)
        {
            BLUtils::ConsumeLeft(&mSamples[i], numToConsume);
        }
    }
#else
    bool enoughPoints = true;
    for (int i = 0; i < 2; i++)
    {
        mSamples[i].Add(samples[i].Get(), samples[i].GetSize());
        if (mSamples[i].Available() < NUM_POINTS + NUM_POINTS_TO_CONSUME)
            enoughPoints = false;
    }

    if (!enoughPoints)
        // Not enough points, do nothing and wait next times...
        return;

    // Check for too many points (would make crash with big block size
    for (int i = 0; i < 2; i++)
    {
        if (mSamples[i].Available() > NUM_POINTS)
        {
            int numToConsume = mSamples[i].Available() - NUM_POINTS;
            if (numToConsume > 0)
            {
                BLUtils::ConsumeLeft(&mSamples[i], numToConsume);
            }
        }
    }
#endif
    
    if (mMode == POLAR_SAMPLE)
    {
        if (mGraphs[POLAR_SAMPLE_MODE_ID] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> *polarSamples = mTmpBuf0;
            
            WDL_TypedBuf<BL_FLOAT> *samplesIn = mTmpBuf1;
            BLUtils::FastQueueToBuf(mSamples[0], &samplesIn[0]);
            BLUtils::FastQueueToBuf(mSamples[1], &samplesIn[1]);
            
            BLVectorscopeProcess::ComputePolarSamples(samplesIn/*mSamples*/,
                                                      polarSamples);
            
            // Adjust to the circle graph drawer
            // New
            BLUtils::MultValues(&polarSamples[0], (BL_FLOAT)SCALE_POLAR_X);

            // Origin
            BLUtils::MultValues(&polarSamples[1], (BL_FLOAT)SCALE_POLAR_Y);
            
            mCurves[POLAR_SAMPLE_MODE_ID]->
                SetValuesPoint(polarSamples[0], polarSamples[1]);
        }
    }
    
    if (mMode == LISSAJOUS)
    {
        if (mGraphs[LISSAJOUS_MODE_ID] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> *lissajousSamples = mTmpBuf2;

            WDL_TypedBuf<BL_FLOAT> *samples2 = mTmpBuf3;
            BLUtils::FastQueueToBuf(mSamples[0], &samples2[0]);
            BLUtils::FastQueueToBuf(mSamples[1], &samples2[1]);
            
            BLVectorscopeProcess::ComputeLissajous(samples2/*mSamples*/,
                                                   lissajousSamples, true);
            
            // Scale so that we stay in the square (even in diagonal)
            BLUtils::MultValues(&lissajousSamples[0], (BL_FLOAT)LISSAJOUS_SCALE);
            BLUtils::MultValues(&lissajousSamples[1], (BL_FLOAT)LISSAJOUS_SCALE);
            
            mCurves[LISSAJOUS_MODE_ID]->SetValuesPointEx(lissajousSamples[0],
                                                         lissajousSamples[1],
                                                         true, false, true);
        }
    }
    
    if (mMode == FIREWORKS)
    {
        if (mGraphs[FIREWORKS_MODE_ID] != NULL)
        {
            //WDL_TypedBuf<BL_FLOAT> samplesIn[2] = { mSamples[0], mSamples[1] };
            WDL_TypedBuf<BL_FLOAT> *samplesIn = mTmpBuf4;
            BLUtils::FastQueueToBuf(mSamples[0], &samplesIn[0]);
            BLUtils::FastQueueToBuf(mSamples[1], &samplesIn[1]);
            
            WDL_TypedBuf<BL_FLOAT> *polarSamples = mTmpBuf5;
            WDL_TypedBuf<BL_FLOAT> *polarSamplesMax = mTmpBuf6;
            mFireworks->ComputePoints(samplesIn, polarSamples, polarSamplesMax);
            
            // Adjust to the circle graph drawer
            // New
            BLUtils::MultValues(&polarSamples[0], (BL_FLOAT)SCALE_POLAR_X);
            BLUtils::MultValues(&polarSamplesMax[0], (BL_FLOAT)SCALE_POLAR_X);

            // Origin
            BLUtils::MultValues(&polarSamples[1], (BL_FLOAT)SCALE_POLAR_Y);
            BLUtils::MultValues(&polarSamplesMax[1], (BL_FLOAT)SCALE_POLAR_Y);
            
            mCurves[FIREWORKS_MODE_ID]->
                SetValuesPoint(polarSamples[0], polarSamples[1]);
            
            mCurves[FIREWORKS_MODE_ID + 1]->SetValuesPoint(polarSamplesMax[0],
                                                           polarSamplesMax[1]);
        }
    }
    
    if (mMode == UPMIX)
    {
        // Nothing to do
    }
    
    if (mMode == SOURCE)
    {
        if (mGraphs[SOURCE_MODE_ID] != NULL)
        {
            //WDL_TypedBuf<BL_FLOAT> samplesIn[2] = { mSamples[0], mSamples[1] };
            WDL_TypedBuf<BL_FLOAT> *samplesIn = mTmpBuf7;
            BLUtils::FastQueueToBuf(mSamples[0], &samplesIn[0]);
            BLUtils::FastQueueToBuf(mSamples[1], &samplesIn[1]);
            
            WDL_TypedBuf<BL_FLOAT> *polarSamples = mTmpBuf8;
            mSourceComputer->ComputePoints(samplesIn, polarSamples);
            
            // Adjust to the circle graph drawer
            BLUtils::MultValues(&polarSamples[1], (BL_FLOAT)SCALE_POLAR_Y);
            
            mCurves[SOURCE_MODE_ID]->SetValuesPoint(polarSamples[0], polarSamples[1]);
        }
    }

    // Here, we are sure we had enough points
    // We used them, now clear
    for (int i = 0; i < 2; i++)
    {
        int numToConsume = NUM_POINTS_TO_CONSUME; //NUM_POINTS;
        if (mSamples[i].Available() < numToConsume)
            numToConsume = mSamples[i].Available();
        if (numToConsume > 0)
        {
            BLUtils::ConsumeLeft(&mSamples[i], numToConsume);
        }
    }
}

BLUpmixGraphDrawer *
BLVectorscope::GetUpmixGraphDrawer()
{
    return mUpmixDrawer;
}

void
BLVectorscope::SetCurveStyle(GraphControl12 *graph,
                             int curveNum,
                             BL_FLOAT minX, BL_FLOAT maxX,
                             BL_FLOAT minY, BL_FLOAT maxY,
                             bool pointFlag,
                             BL_FLOAT strokeSize,
                             bool bevelFlag,
                             int r, int g, int b,
                             bool curveFill, BL_FLOAT curveFillAlpha,
                             bool pointsAsLines,
                             bool pointOverlay, bool linesPolarFlag)
{
    if (graph == NULL)
        return;
    
    mCurves[curveNum]->SetXScale(Scale::LINEAR, minX, maxX);
    mCurves[curveNum]->SetYScale(Scale::LINEAR, minY, maxY);
    
    if (!pointFlag)
        mCurves[curveNum]->SetLineWidth(strokeSize);
    
    //
    if (pointFlag)
    {
        mCurves[curveNum]->SetPointSize(strokeSize);
        mCurves[curveNum]->SetPointStyle(true, linesPolarFlag, pointsAsLines);
        mCurves[curveNum]->SetPointOverlay(pointOverlay);
    }
    
    mCurves[curveNum]->SetBevel(bevelFlag);
    
    mCurves[curveNum]->SetColor(r, g, b);
    
    if (curveFill)
    {
        mCurves[curveNum]->SetFill(curveFill);
    }
            
    mCurves[curveNum]->SetFillAlpha(curveFillAlpha);
}

#endif // IGRAPHICS_NANOVG
