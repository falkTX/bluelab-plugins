//
//  StereoVizVolRender.cpp
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

// #bl-iplug2
//#include "nanovg.h"

#include <BLUtils.h>

#include "StereoVizVolRender.h"

#define MAX_NUM_SLICES 32 //64 //32 //32 //8

#define POINT_COLOR_R 0.25
#define POINT_COLOR_G 0.25
#define POINT_COLOR_B 1.0
#define POINT_COLOR_A 0.02

#define THRESHOLD_SMALL_MAGNS 0

#define MAX_NUM_POINTS_SLICE 512 //32 //256 //512 //1024 //128

#define TEST_SPECTROGRAM 0
#define TEST_DIST_DB 0

#define TEST_SPECTROGRAM2 0
#define TEST_DIST_DB 0

#if 0
TEST: discard points with a too low intensity => doesn't work well
#endif

StereoVizVolRender::StereoVizVolRender(GraphControl11 *graphControl)
{
    mGraph = graphControl;
    
    // #bl-iplug2
    //mVolRender = new SparseVolRender();
    mVolRender = new SparseVolRender(MAX_NUM_SLICES, MAX_NUM_SLICES);
    
    mGraph->AddCustomDrawer(mVolRender);
    mGraph->AddCustomControl(this);
    
    mMouseIsDown = false;
    mPrevDrag[0] = 0;
    mPrevDrag[1] = 0;
    
    mAngle0 = 0.0;
    mAngle1 = 0.0;
    
    mAddNum = 0;
}

StereoVizVolRender::~StereoVizVolRender()
{
    delete mVolRender;
}

void
StereoVizVolRender::AddCurveValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                           const WDL_TypedBuf<BL_FLOAT> &yValues,
                                           const WDL_TypedBuf<BL_FLOAT> &colorWeights)
{
    // Tests just in case
    if (xValues.GetSize() != yValues.GetSize())
        return;
    if (xValues.GetSize() != colorWeights.GetSize())
        return;
    
#if 0
    // Skip add, to slow the animation, and increase the performances
    if (mAddNum++ % 4/*16*/ != 0)
        return;
#endif
    
#if TEST_SPECTROGRAM
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        // Compute the normalized direction
        BL_FLOAT dir[2] = { xValues.Get()[i], yValues.Get()[i] };
        BL_FLOAT dirLength = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1]);
        
#define EPS 1e-15
        if (dirLength > EPS)
        {
            dir[0] /= dirLength;
            dir[1] /= dirLength;
        }
        
        // Normalized frequency
        BL_FLOAT freq = ((BL_FLOAT)i)/xValues.GetSize();
        
        // TEST
        //freq = BLUtils::AmpToDBNorm(freq, 1e-15, -60.0);
        
        // Compute position
        BL_FLOAT posX = dir[0] * freq;
        BL_FLOAT posY = dir[1] * freq;
        
        // Modify position
        xValues.Get()[i] = posX;
        yValues.Get()[i] = posY;
        
        // TEST
        //BL_FLOAT weight = colorWeights.Get()[i];
        
        //weight = 1.0 - weight;
        
        //colorWeights.Get()[i] = weight;
    }
#endif

#if TEST_SPECTROGRAM2
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        // Compute the normalized direction
        BL_FLOAT pos[2] = { xValues.Get()[i], yValues.Get()[i] };
        
        BL_FLOAT azim = std::atan2(pos[1], pos[0]);
        azim = fmod(azim, M_PI);
        azim -= M_PI/2.0;
        azim /= M_PI/2.0;
        
        // Normalized frequency
        BL_FLOAT freq = ((BL_FLOAT)i)/xValues.GetSize();
        
#if TEST_DIST_DB2
        freq = BLUtils::AmpToDBNorm(freq, 1e-15, -60.0);
#endif
        
        // Modify position
        xValues.Get()[i] = azim;
        yValues.Get()[i] = freq;
    }
#endif
    
#if TEST_DIST_DB
    DistToDbScale(xValues, yValues);
#endif
    
    vector<SparseVolRender::Point> newPoints;

    for (int i = 0; i < xValues.GetSize(); i++)
    {
        SparseVolRender::Point p;
        p.mX = xValues.Get()[i];
        p.mY = yValues.Get()[i];
        //p.mZ
        
        BL_FLOAT weight = colorWeights.Get()[i];
        
#if THRESHOLD_SMALL_MAGNS

#define EPS 2.0 // With 2, we keep 3000 points from 30000 at the beginning
                // (the weights are in log scale, and 2 is for very small magns)
        if (weight < EPS)
            continue;
#endif
        
        p.mWeight = weight;
        
        p.mR = POINT_COLOR_R;
        p.mG = POINT_COLOR_G;
        p.mB = POINT_COLOR_B;
        p.mA = POINT_COLOR_A;
        
        SparseVolRender::ComputePackedPointColor(&p, 1.0);
        
        p.mSize = 1.0;
        
#if (!TEST_SPECTROGRAM && !TEST_SPECTROGRAM2) 
        // Normalized frequency
        p.mColorMapId = ((BL_FLOAT)i)/xValues.GetSize();
#else
        p.mColorMapId = weight;
#endif
        
        // Add the point
        newPoints.push_back(p);
    }
    
    BL_FLOAT alphaCoeff = 1.0;
    SparseVolRender::DecimateSlice(&newPoints, MAX_NUM_POINTS_SLICE, alphaCoeff);
    
    // Add the slice to the whole set
    mPoints.push_back(newPoints);
    if (mPoints.size() > MAX_NUM_SLICES)
        mPoints.pop_front();
    
    // Adjust z for all the history
    for (int i = 0; i < mPoints.size(); i++)
    {
        // Compute time step
        BL_FLOAT z = 1.0 - ((BL_FLOAT)i)/mPoints.size();
        
        // Center
        z -= 0.5;
        
        // Get the slice
        vector<SparseVolRender::Point> &points = mPoints[i];
        
        // Set the same time step for all the points of the slice
        for (int j = 0; j < points.size(); j++)
        {
            points[j].mZ = z;
        }
    }
    
#if 0 // #bl-iplug2
    // Add all the points to the renderer
    // Add them back to front (better for blending !)
    mVolRender->ClearPoints();
    for (int i = mPoints.size() - 1; i >= 0; i--)
    {
        // Get the slice
        vector<SparseVolRender::Point> &points = mPoints[i];
        
        mVolRender->AddPoints(points);
    }
#else
    // Add all the points to the renderer
    // Add them back to front (better for blending !)
    mVolRender->ClearSlices();
    for (int i = mPoints.size() - 1; i >= 0; i--)
    {
        // Get the slice
        vector<SparseVolRender::Point> &points = mPoints[i];
        
        bool skipDisplay = false;
        mVolRender->AddSlice(points, skipDisplay);
    }
#endif
    
    // The following lines are not tested with StereoWidth (only StereoViz in another class)
    
    // Without that, the volume rendering is not displayed
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
StereoVizVolRender::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    mMouseIsDown = true;
    
    mPrevDrag[0] = x;
    mPrevDrag[1] = y;
}

void
StereoVizVolRender::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    mMouseIsDown = false;
}

void
StereoVizVolRender::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    int dragX = x - mPrevDrag[0];
    int dragY = y - mPrevDrag[1];
    
    mPrevDrag[0] = x;
    mPrevDrag[1] = y;
    
    mAngle0 += dragX;
    mAngle1 += dragY;
    
    mVolRender->SetCameraAngles(mAngle0, mAngle1);
}

void
StereoVizVolRender::DistToDbScale(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                    const WDL_TypedBuf<BL_FLOAT> &yValues)
{
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        // Compute the normalized direction
        BL_FLOAT dir[2] = { xValues.Get()[i], yValues.Get()[i] };
        BL_FLOAT length = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1]);
        
#define EPS 1e-15
        if (length > EPS)
        {
            dir[0] /= length;
            dir[1] /= length;
        }
        
        length = BLUtils::AmpToDBNorm(length, (BL_FLOAT)1e-15, (BL_FLOAT)-60.0);
        
        // Compute position
        BL_FLOAT posX = dir[0] * length;
        BL_FLOAT posY = dir[1] * length;
        
        // Modify position
        xValues.Get()[i] = posX;
        yValues.Get()[i] = posY;
    }
}
