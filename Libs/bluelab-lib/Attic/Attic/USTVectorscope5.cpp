//
//  USTVectorscope5.cpp
//  UST
//
//  Created by applematuer on 7/27/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <math.h>

#include <GraphControl11.h>

#include <USTCircleGraphDrawer.h>
#include <USTLissajousGraphDrawer.h>
#include <USTUpmixGraphDrawer.h>

#include <USTProcess.h>

#include <USTFireworks.h>

#include <BLUtils.h>

#include <USTPluginInterface.h>

#include "USTVectorscope5.h"

#ifndef M_PI
#define M_PI 3.141592653589
#endif

#define POLAR_SAMPLES_INSERT_POINTS 1
#if POLAR_SAMPLES_INSERT_POINTS
#define INSERT_POINT_FACTOR 2 //8//1 //4 //8 //2
#else
#define INSERT_POINT_FACTOR 1
#endif

// Smaller points, moure points
#define HD_POLAR_SAMPLES 0 //1

#if !HD_POLAR_SAMPLES
#define NUM_POINTS_POLAR_SAMPLES 4096
#else
#define NUM_POINTS_POLAR_SAMPLES 4096 //4096*4
#endif

// NOTE: can gain 10% CPU by setting to 1024
// (but the display will be a bit too quick)
#define NUM_POINTS_LISSAJOUS 4096

#define NUM_POINTS_FIREWORKS 4096

#define NUM_CURVES_GRAPH0 1
#define NUM_CURVES_GRAPH1 1
#define NUM_CURVES_GRAPH2 2
#define NUM_CURVES_GRAPH3 0

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


#if !HD_POLAR_SAMPLES
#define POINT_SIZE_MODE0 5.0
#else
#define POINT_SIZE_MODE0 2.0 //1.0 //2.0
#endif

// DEBUG: 10. ORIGIN: 2
#if POLAR_SAMPLES_INSERT_POINTS // DEBUG: 10
#define POINT_SIZE_MODE0 2.0 //10.0 //2.0 //1.0 //2.0
#endif


#define POINT_SIZE_MODE1 2.0
#define LINE_SIZE_MODE2 2.0
#define LINE_SIZE2_MODE2 3.0

#if !HD_POLAR_SAMPLES
#define POINT_ALPHA_POLAR_SAMPLES 0.25
#else
#define POINT_ALPHA_POLAR_SAMPLES 1.0 //0.5
#endif

#define POINT_ALPHA_LISSAJOUS 0.25

#define ORANGE_COLOR_SCHEME 0
#define BLUE_COLOR_SCHEME 1

// Curve to display points
#define CURVE_POINTS0 0
#define CURVE_POINTS1 1


#define LISSAJOUS_SCALE 0.8

#define SQR2_INV 0.70710678118655

// Send data to to graph less often, but jitter animation
#define OPTIM_ANIM_VECTORSCOPE 0 //1


USTVectorscope5::USTVectorscope5(USTPluginInterface *plug, BL_FLOAT sampleRate)
{
    mPlug = plug;
    
    mIsEnabled = false;
    
    for (int i = 0; i < VECTORSCOPE_NUM_GRAPHS; i++)
        mGraphs[i] = NULL;
    
    mFireworks = new USTFireworks(sampleRate);

    mMode = POLAR_SAMPLE; // NEW (the mode was not initialized)
}

USTVectorscope5::~USTVectorscope5()
{
    delete mFireworks;
}

void
USTVectorscope5::OnMouseDown(float x, float y, const IMouseMod &mod)
{
    if (mMode == UPMIX)
    {
        // Upmix graph drawer also manage control
        mUpmixDrawer->OnMouseDown(x, y, mod);
    }
}

void
USTVectorscope5::OnMouseUp(float x, float y, const IMouseMod &mod)
{
    if (mMode == UPMIX)
    {
        // Upmix graph drawer also manage control
        mUpmixDrawer->OnMouseUp(x, y, mod);
        
        return;
    }
}

void
USTVectorscope5::OnMouseDrag(float x, float y, float dX, float dY,
                             const IMouseMod &mod)
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
    
    BL_GUI_FLOAT dpan = 0.0;
    BL_GUI_FLOAT dwidth = 0.0;
    
    if (!mod.A)
        dpan = ((BL_GUI_FLOAT)dX)*COEFF;
    else
        dwidth = -((BL_GUI_FLOAT)dY)*COEFF;
    
    if (mod.S)
    {
        dpan *= FINE_COEFF;
        dwidth *= FINE_COEFF;
    }
    
    mPlug->UpdateVectorscopePanWidth(dpan, dwidth);
}

void
USTVectorscope5::SetEnabled(bool flag)
{
    mIsEnabled = flag;
    
    for (int i = 0; i < VECTORSCOPE_NUM_GRAPHS; i++)
    {
        if (mGraphs[i] != NULL)
        {
            bool enabled = false;
            
            if (mIsEnabled && (i == mMode))
                enabled = true;
            
            mGraphs[i]->SetEnabled(enabled);
            
            if (enabled)
            {
                mGraphs[i]->SetInteractionDisabled(false);
                //mGraphs[i]->SetDirty(false);
                mGraphs[i]->SetDataChanged();
            }
            else
            {
                mGraphs[i]->SetInteractionDisabled(true);
                mGraphs[i]->SetClean();
            }
        }
    }
    
    mFireworks->Reset();
}

void
USTVectorscope5::Reset(BL_FLOAT sampleRate)
{
    mFireworks->Reset(sampleRate);
}

void
USTVectorscope5::SetMode(Mode mode)
{
    mMode = mode;
    
    for (int i = 0; i < VECTORSCOPE_NUM_GRAPHS; i++)
    {
        if (mGraphs[i] != NULL)
        {
            bool enabled = (i == mode);
            mGraphs[i]->SetEnabled(enabled);
            
            if (enabled)
            {
                mGraphs[i]->SetInteractionDisabled(false);
                //mGraphs[i]->SetDirty(false);
                mGraphs[i]->SetDataChanged();
            }
            else
            {
                mGraphs[i]->SetInteractionDisabled(true);
                mGraphs[i]->SetClean();
            }
        }
    }
    
    mFireworks->Reset();
}

int
USTVectorscope5::GetNumCurves(int graphNum)
{
    if (graphNum == POLAR_SAMPLE_MODE_ID)
        return NUM_CURVES_GRAPH0;
    
    if (graphNum == LISSAJOUS_MODE_ID)
        return NUM_CURVES_GRAPH1;
    
    if (graphNum == FIREWORKS_MODE_ID)
        return NUM_CURVES_GRAPH2;
    
    if (graphNum == UPMIX_MODE_ID)
        return NUM_CURVES_GRAPH3;
    
    return -1;
}

int
USTVectorscope5::GetNumPoints(int graphNum)
{
    if (graphNum == POLAR_SAMPLE_MODE_ID)
        return NUM_POINTS_POLAR_SAMPLES*INSERT_POINT_FACTOR;
    
    if (graphNum == LISSAJOUS_MODE_ID)
        return NUM_POINTS_LISSAJOUS;
    
    if (graphNum == FIREWORKS_MODE_ID)
        return NUM_POINTS_FIREWORKS;
    
    return 0;
}

void
USTVectorscope5::SetGraphs(GraphControl11 *graph0,
                           GraphControl11 *graph1,
                           GraphControl11 *graph2,
                           GraphControl11 *graph3)
{
    mGraphs[POLAR_SAMPLE_MODE_ID] = graph0;
    mGraphs[LISSAJOUS_MODE_ID] = graph1;
    mGraphs[FIREWORKS_MODE_ID] = graph2;
    mGraphs[UPMIX_MODE_ID] = graph3;
    
    //
    mGraphs[POLAR_SAMPLE_MODE_ID]->AddCustomControl(this);
    mGraphs[LISSAJOUS_MODE_ID]->AddCustomControl(this);
    mGraphs[FIREWORKS_MODE_ID]->AddCustomControl(this);
    //
    mGraphs[UPMIX_MODE_ID]->AddCustomControl(this); // ??
    
    // Graph0
    if (mGraphs[POLAR_SAMPLE_MODE_ID] != NULL)
    {
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraphs[0]->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetClearColor(0, 0, 0, 255);
    
        mCircleDrawerPolarSamples = new USTCircleGraphDrawer();
        mGraphs[POLAR_SAMPLE_MODE_ID]->AddCustomDrawer(mCircleDrawerPolarSamples);
    
        // Style
        //
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
        int pointColor[3] = { 64, 64, 255 };
#endif
      
#if ORANGE_COLOR_SCHEME
        int pointColor[3] = { 232, 110, 36 };
#endif

        BL_GUI_FLOAT alpha = POINT_ALPHA_POLAR_SAMPLES; //0.25;
        
#if BLUE_COLOR_SCHEME
        //int pointColor[3] = { 170, 202, 209 };
        //int pointColor[3] = { 78, 90, 126 };
        
#if !HD_POLAR_SAMPLES
        int pointColor[3] = { 113, 130, 182 };
#else
        //int pointColor[3] = { 113, 130, 182 };
        int pointColor[3] = { 255, 0, 0 };
        
        //int weightTargetColor[4] = { 143, 209, 255, 255 };
        int weightTargetColor[4] = { 143, 209, 255, 192 };
#endif
        
#endif

        // To display lines instead of points
#if !HD_POLAR_SAMPLES
        bool pointsAsLines = false;
#else
        //bool pointsAsLines = true;
        bool pointsAsLines = false;
#endif
        
#if !HD_POLAR_SAMPLES
        bool overlay = true;
#else
        //bool overlay = false;
        bool overlay = true;
#endif
        
        bool bevelFlag = false;
        
        bool pointFlag = true;
        //bool pointFlag = false;
        
        SetCurveStyle(mGraphs[POLAR_SAMPLE_MODE_ID],
                      CURVE_POINTS0,
                      GRAPH0_MIN_X, GRAPH0_MAX_X,
                      GRAPH0_MIN_Y, GRAPH0_MAX_Y,
                      pointFlag, POINT_SIZE_MODE0, //0.5, //1.0, // point size
                      bevelFlag,
                      pointColor[0], pointColor[1], pointColor[2],
                      false, alpha, pointsAsLines, overlay, false);
        
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetDisablePointOffsetHack(true);
        
#if HD_POLAR_SAMPLES
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetCurveWeightMultAlpha(CURVE_POINTS0, false);
        
        //int weightTargetColor[4] = { 255, 0, 0, 255 };
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetCurveWeightTargetColor(CURVE_POINTS0, weightTargetColor);
        
        // Red and trasparent as starting color
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetCurveAlpha(CURVE_POINTS0, 0.0);
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetCurveFillAlpha(CURVE_POINTS0, 0.0);
#endif
    }
    
    // Graph for Lissajouss
    if (mGraphs[LISSAJOUS_MODE_ID] != NULL)
    {
        mGraphs[LISSAJOUS_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraphs[1]->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraphs[LISSAJOUS_MODE_ID]->SetClearColor(0, 0, 0, 255);
        
        mLissajousDrawer = new USTLissajousGraphDrawer(LISSAJOUS_SCALE);
        mGraphs[LISSAJOUS_MODE_ID]->AddCustomDrawer(mLissajousDrawer);
        
        // Style
        //
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
        int pointColor[3] = { 64, 64, 255 };
#endif
        
#if ORANGE_COLOR_SCHEME
        int pointColor[3] = { 232, 110, 36 };
#endif

#if BLUE_COLOR_SCHEME
        //int pointColor[3] = { 170, 202, 209 };
        int pointColor[3] = { 113, 130, 182 };
#endif

        // To display lines instead of points
        bool pointsAsLines = false;
        BL_GUI_FLOAT alpha = POINT_ALPHA_LISSAJOUS; //0.25;
        bool overlay = true;
        bool bevelFlag = false;
        
        SetCurveStyle(mGraphs[LISSAJOUS_MODE_ID],
                      CURVE_POINTS0,
                      GRAPH2_MIN_X, GRAPH2_MAX_X,
                      GRAPH2_MIN_Y, GRAPH2_MAX_Y,
                      true, POINT_SIZE_MODE1, //0.5, //1.0, // point size
                      bevelFlag,
                      pointColor[0], pointColor[1], pointColor[2],
                      false, alpha, pointsAsLines, overlay, false);
        
        mGraphs[LISSAJOUS_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    // Graph for fireworks
    if (mGraphs[FIREWORKS_MODE_ID] != NULL)
    {
        mGraphs[FIREWORKS_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraphs[1]->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraphs[FIREWORKS_MODE_ID]->SetClearColor(0, 0, 0, 255);
        
        mCircleDrawerFireworks = new USTCircleGraphDrawer();
        mGraphs[FIREWORKS_MODE_ID]->AddCustomDrawer(mCircleDrawerFireworks);
        
        // Style
        //
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
        int pointColor[3] = { 64, 64, 255 };
#endif
        
#if ORANGE_COLOR_SCHEME
        //int pointColor0[3] = { 252, 79, 36 };
        int pointColor0[3] = { 252, 228, 205/*, 64*/ };
        int pointColor1[3] = { 232, 110, 36 };
#endif

#if BLUE_COLOR_SCHEME
        //int pointColor0[3] = { 193, 229, 237 };
        //int pointColor1[3] = { 170, 202, 209 };
        
        //int pointColor0[3] = { 193, 229, 237 };
        //int pointColor1[3] = { 113, 130, 182 };
        
        int pointColor0[3] = { 113, 130, 182 };
        //int pointColor1[3] = { 193, 229, 237 };
        
        // Blue
        int pointColor1[3] = { 255, 255, 255 };
        // Orange (like Malik likes)
        //int pointColor1[3] = { 234, 101, 0 };
#endif
        
        // To display lines instead of points
        bool pointsAsLines = true; //false;
        BL_GUI_FLOAT alpha = 1.0; //POINT_ALPHA; //0.25;
        bool overlayFlag = false; //true;
        
#if 0
        bool fillFlag0 = true; //false; //true; //GraphControl11 Fill is buggy
        bool linesPolarFlag = true;
#endif
        
#if 1
        bool fillFlag0 = false;
        bool linesPolarFlag0 = true;
        bool linesPolarFlag1 = false;
        bool bevelFlag0 = false;
#endif
        
        SetCurveStyle(mGraphs[FIREWORKS_MODE_ID],
                      CURVE_POINTS0,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true/*false*/, LINE_SIZE_MODE2, //0.5, //1.0, // point size
                      bevelFlag0,
                      pointColor0[0], pointColor0[1], pointColor0[2],
                      fillFlag0, alpha, pointsAsLines, overlayFlag, linesPolarFlag0);
        
        bool fillFlag1 = false;
        bool bevelFlag1 = true;
        SetCurveStyle(mGraphs[FIREWORKS_MODE_ID],
                      CURVE_POINTS1,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true/*false*/, LINE_SIZE2_MODE2, //0.5, //1.0, // point size
                      bevelFlag1,
                      pointColor1[0], pointColor1[1], pointColor1[2],
                      fillFlag1, alpha, pointsAsLines, overlayFlag, linesPolarFlag1);
        
        mGraphs[FIREWORKS_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    // Graph for upmix
    if (mGraphs[UPMIX_MODE_ID] != NULL)
    {
        mGraphs[UPMIX_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraphs[1]->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraphs[UPMIX_MODE_ID]->SetClearColor(0, 0, 0, 255);
        
        mUpmixDrawer = new USTUpmixGraphDrawer(mPlug, mGraphs[UPMIX_MODE_ID]);
        mGraphs[UPMIX_MODE_ID]->AddCustomDrawer(mUpmixDrawer);
        
        // Style
        //
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
        int pointColor[3] = { 64, 64, 255 };
#endif
        
#if ORANGE_COLOR_SCHEME
        //int pointColor0[3] = { 252, 79, 36 };
        int pointColor0[3] = { 252, 228, 205/*, 64*/ };
        int pointColor1[3] = { 232, 110, 36 };
#endif

#if BLUE_COLOR_SCHEME
        //int pointColor0[3] = { 193, 229, 237 };
        //int pointColor1[3] = { 170, 202, 209 };
        
        // Orange ??
        int pointColor0[3] = { 193, 229, 237 };
        int pointColor1[3] = { 113, 130, 182 };
#endif

        // To display lines instead of points
        bool pointsAsLines = false;
        BL_GUI_FLOAT alpha = 1.0; //POINT_ALPHA; //0.25;
        bool overlayFlag = false; //true;
        
#if 0
        bool fillFlag0 = true; //false; //true; //GraphControl11 Fill is buggy
        bool linesPolarFlag = true;
#endif
        
#if 1
        bool fillFlag0 = false;
        bool linesPolarFlag0 = true;
        bool linesPolarFlag1 = false;
        bool bevelFlag0 = false;
#endif
        
        SetCurveStyle(mGraphs[UPMIX_MODE_ID],
                      CURVE_POINTS0,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true/*false*/, LINE_SIZE_MODE2, //0.5, //1.0, // point size
                      bevelFlag0,
                      pointColor0[0], pointColor0[1], pointColor0[2],
                      fillFlag0, alpha, pointsAsLines, overlayFlag, linesPolarFlag0);
        
        bool fillFlag1 = false;
        bool bevelFlag1 = true;
        SetCurveStyle(mGraphs[UPMIX_MODE_ID],
                      CURVE_POINTS1,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true/*false*/, LINE_SIZE2_MODE2, //0.5, //1.0, // point size
                      bevelFlag1,
                      pointColor1[0], pointColor1[1], pointColor1[2],
                      fillFlag1, alpha, pointsAsLines, overlayFlag, linesPolarFlag1);
        
        mGraphs[UPMIX_MODE_ID]->SetDisablePointOffsetHack(true);
    }
}

void
USTVectorscope5::SetGraphsNull()
{
    mGraphs[POLAR_SAMPLE_MODE_ID] = NULL;
    mGraphs[LISSAJOUS_MODE_ID] = NULL;
    mGraphs[FIREWORKS_MODE_ID] = NULL;
    mGraphs[UPMIX_MODE_ID] = NULL;
}

void
USTVectorscope5::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples)
{
    if (samples.size() != 2)
        return;
    
    WDL_TypedBuf<BL_FLOAT> samplesArray[2] = { samples[0], samples[1] };
    
    if (mMode == POLAR_SAMPLE)
    {
        if (mGraphs[POLAR_SAMPLE_MODE_ID] != NULL)
        {
            //WDL_TypedBuf<BL_GUI_FLOAT> polarSamples[2];
            USTProcess::ComputePolarSamples(samplesArray, mTmpPolarSamples);
            
            //WDL_TypedBuf<BL_GUI_FLOAT> polarSamplesResult[2];
            // Bufferize
            for (int k = 0; k < 2; k++)
            {
                mPolarSampleSamples[k].Add(mTmpPolarSamples[k].Get(), mTmpPolarSamples[k].GetSize());
                
                if (mPolarSampleSamples[k].GetSize() >= NUM_POINTS_POLAR_SAMPLES)
                {
                    //mTmpPolarSamplesResult[k].Add(mPolarSampleSamples[k].Get(), NUM_POINTS_POLAR_SAMPLES);
                    mTmpPolarSamplesResult[k].Resize(NUM_POINTS_POLAR_SAMPLES);
                    memcpy(mTmpPolarSamplesResult[k].Get(),
                           mPolarSampleSamples[k].Get(),
                           NUM_POINTS_POLAR_SAMPLES*sizeof(BL_GUI_FLOAT));
                    
#if !OPTIM_ANIM_VECTORSCOPE
                    int numToConsume = samples[0].GetSize();
                    if (mPolarSampleSamples[k].GetSize() < numToConsume)
                        numToConsume = mPolarSampleSamples[k].GetSize();
#else
                    int numToConsume = NUM_POINTS_POLAR_SAMPLES;
#endif
                    BLUtils::ConsumeLeft(&mPolarSampleSamples[k], numToConsume);
                }
                
                // Avoid overfitting, in case nFrames is too big
                if (samplesArray[k].GetSize() > NUM_POINTS_POLAR_SAMPLES)
                    mPolarSampleSamples[k].Resize(0);
            }
           
            WDL_TypedBuf<BL_FLOAT> *polarSamplesResult[2];
            polarSamplesResult[0] = &mTmpPolarSamplesResult[0];
            polarSamplesResult[1] = &mTmpPolarSamplesResult[1];
#if POLAR_SAMPLES_INSERT_POINTS
            int step = INSERT_POINT_FACTOR;
            int stepNum = 0;
            while(step > 1)
            {
                if (mTmpBuffersInsertPoints.size() < (stepNum + 1)*2)
                    mTmpBuffersInsertPoints.resize((stepNum + 1)*2);
                
                WDL_TypedBuf<BL_FLOAT> &newX = mTmpBuffersInsertPoints[stepNum*2];
                WDL_TypedBuf<BL_FLOAT> &newY = mTmpBuffersInsertPoints[stepNum*2 + 1];
                
                BLUtils::AddIntermediatePoints(*polarSamplesResult[0], *polarSamplesResult[1],
                                             &newX, &newY);
                
                polarSamplesResult[0] = &newX;
                polarSamplesResult[1] = &newY;
                
                step /= 2;
                stepNum++;
            }
#endif
            
            if (polarSamplesResult[0]->GetSize() >= NUM_POINTS_POLAR_SAMPLES)
            {
                mGraphs[POLAR_SAMPLE_MODE_ID]->SetCurveValuesPoint(CURVE_POINTS0,
                                                                   *polarSamplesResult[0], *polarSamplesResult[1]);
            
#if !HD_POLAR_SAMPLES
                mGraphs[POLAR_SAMPLE_MODE_ID]->SetCurveOptimSameColor(CURVE_POINTS0, true);
#else
                // Color weights
                WDL_TypedBuf<BL_GUI_FLOAT> colorWeights;
                colorWeights.Resize(polarSamplesResult[0].GetSize());
            
#define MIN_WEIGHT 0.0 //0.5
#define MAX_WEIGHT 1.0
            
                BL_FLOAT weightIncr = (MAX_WEIGHT - MIN_WEIGHT)/colorWeights.GetSize();
                BL_FLOAT currentWeight = MIN_WEIGHT;
                for (int i = 0; i < colorWeights.GetSize(); i++)
                {
                    BL_FLOAT weight = currentWeight;
                    //weight = sqrt(weight);
                
                    colorWeights.Get()[i] = weight;
                
                    currentWeight += weightIncr;
                }
            
                mGraphs[POLAR_SAMPLE_MODE_ID]->SetCurveColorWeight(CURVE_POINTS0, colorWeights);
#endif
            }
        }
    }
    
    if (mMode == LISSAJOUS)
    {
        if (mGraphs[LISSAJOUS_MODE_ID] != NULL)
        {
            WDL_TypedBuf<BL_GUI_FLOAT> lissajousSamples[2];
            USTProcess::ComputeLissajous(samplesArray, lissajousSamples, true);
            
            // Scale so that we stay in the square (even in diagonal)
            //BLUtils::MultValues(&lissajousSamples[0], SQR2_INV);
            //BLUtils::MultValues(&lissajousSamples[1], SQR2_INV);
            
            BLUtils::MultValues(&lissajousSamples[0], (BL_GUI_FLOAT)LISSAJOUS_SCALE);
            BLUtils::MultValues(&lissajousSamples[1], (BL_GUI_FLOAT)LISSAJOUS_SCALE);
            
            WDL_TypedBuf<BL_GUI_FLOAT> lissajousSamplesResult[2];
            // Bufferize
            for (int k = 0; k < 2; k++)
            {
                mLissajousSamples[k].Add(lissajousSamples[k].Get(), lissajousSamples[k].GetSize());
                
                if (mLissajousSamples[k].GetSize() >= NUM_POINTS_LISSAJOUS)
                {
                    lissajousSamplesResult[k].Add(mLissajousSamples[k].Get(), NUM_POINTS_LISSAJOUS);
                    
#if !OPTIM_ANIM_VECTORSCOPE
                    int numToConsume = samples[0].GetSize();
                    if (mLissajousSamples[k].GetSize() < numToConsume)
                        numToConsume = mLissajousSamples[k].GetSize();
#else
                    int numToConsume = NUM_POINTS_LISSAJOUS;
#endif
                    BLUtils::ConsumeLeft(&mLissajousSamples[k], numToConsume);
                }
                
                // Avoid overfitting, in case nFrames is too big
                if (samplesArray[k].GetSize() > NUM_POINTS_POLAR_SAMPLES)
                    mLissajousSamples[k].Resize(0);
            }
            
            if (lissajousSamplesResult[0].GetSize() >= NUM_POINTS_LISSAJOUS)
            {
#if 1
                mGraphs[LISSAJOUS_MODE_ID]->SetCurveValuesPointEx(CURVE_POINTS0,
                                              lissajousSamplesResult[0],
                                              lissajousSamplesResult[1],
                                              true, false, true);
#endif
            
#if 0
                mGraphs[LISSAJOUS_MODE_ID]->SetCurveValuesPoint(CURVE_POINTS,
                                                                lissajousSamplesResult[0],
                                                                lissajousSamplesResult[1]);
#endif
            
                mGraphs[LISSAJOUS_MODE_ID]->SetCurveOptimSameColor(CURVE_POINTS0, true);
            }
        }
    }
    
    if (mMode == FIREWORKS)
    {
        if (mGraphs[FIREWORKS_MODE_ID] != NULL)
        {
            // Warning, fireworks take constant input buffer size
            // (for smoothing)
            
            WDL_TypedBuf<BL_FLOAT> fireworksInSamples[2];
            
            for (int k = 0; k < 2; k++)
            {
                mFireworksInSamples[k].Add(samplesArray[k].Get(), samplesArray[k].GetSize());
                
                if (mFireworksInSamples[k].GetSize() >= NUM_POINTS_FIREWORKS)
                {
                    fireworksInSamples[k].Add(mFireworksInSamples[k].Get(), NUM_POINTS_FIREWORKS);
                    
#if !OPTIM_ANIM_VECTORSCOPE
                    int numToConsume = samples[0].GetSize();
                    if (mFireworksInSamples[k].GetSize() < numToConsume)
                        numToConsume = mFireworksInSamples[k].GetSize();
#else
                    int numToConsume = NUM_POINTS_FIREWORKS;
#endif
                    BLUtils::ConsumeLeft(&mFireworksInSamples[k], numToConsume);
                }
                
                // Avoid overfitting, in case nFrames is too big
                if (samplesArray[k].GetSize() > NUM_POINTS_FIREWORKS)
                    mFireworksInSamples[k].Resize(0);
            }
            
            if (fireworksInSamples[0].GetSize() >= NUM_POINTS_FIREWORKS)
            {
                WDL_TypedBuf<BL_GUI_FLOAT> polarSamples[2];
                WDL_TypedBuf<BL_GUI_FLOAT> polarSamplesMax[2];
                mFireworks->ComputePoints(fireworksInSamples, polarSamples, polarSamplesMax);
            
                mGraphs[FIREWORKS_MODE_ID]->SetCurveValuesPoint(CURVE_POINTS0,
                                                                polarSamples[0], polarSamples[1]);
            
                mGraphs[FIREWORKS_MODE_ID]->SetCurveValuesPoint(CURVE_POINTS1,
                                                                polarSamplesMax[0], polarSamplesMax[1]);
            }
        }
    }
    
    if (mMode == UPMIX)
    {
        // Nothing to do
    }
}

USTUpmixGraphDrawer *
USTVectorscope5::GetUpmixGraphDrawer()
{
    return mUpmixDrawer;
}

void
USTVectorscope5::SetCurveStyle(GraphControl11 *graph,
                               int curveNum,
                               BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX,
                               BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY,
                               bool pointFlag,
                               BL_GUI_FLOAT strokeSize,
                               bool bevelFlag,
                               int r, int g, int b,
                               bool curveFill, BL_GUI_FLOAT curveFillAlpha,
                               bool pointsAsLines,
                               bool pointOverlay, bool linesPolarFlag)
{
    if (graph == NULL)
        return;
    
    graph->SetCurveXScale(curveNum, false, minX, maxX);
    graph->SetCurveYScale(curveNum, false, minY, maxY);
    
    if (!pointFlag)
        graph->SetCurveLineWidth(curveNum, strokeSize);
    
    //
    if (pointFlag)
    {
        graph->SetCurvePointSize(curveNum, strokeSize);
        graph->SetCurvePointStyle(curveNum, true, linesPolarFlag, pointsAsLines);
        graph->SetCurvePointOverlay(curveNum, pointOverlay);
    }
    
    graph->SetCurveBevel(curveNum, bevelFlag);
                          
    graph->SetCurveColor(curveNum, r, g, b);
    
    if (curveFill)
    {
        graph->SetCurveFill(curveNum, curveFill);
    }
            
    graph->SetCurveFillAlpha(curveNum, curveFillAlpha);
}

#endif // IGRAPHICS_NANOVG
