//
//  StereoVizVolRender2.cpp
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#include "nanovg.h"

#include <Utils.h>
#include <Debug.h>

#include "StereoVizVolRender2.h"

#define NUM_POINTS_SLICE0 32
#define NUM_POINTS_SLICE1 2048

//#define MAX_NUM_POINTS_SLICE 512 //32 //256 //512 //1024 //128

#define NUM_SLICES0 4
#define NUM_SLICES1 256

//#define MAX_NUM_SLICES 32 //64 //32 //32 //8

#define SPEED0 1
#define SPEED1 8

#define POINT_COLOR_R 0.25
#define POINT_COLOR_G 0.25
#define POINT_COLOR_B 1.0
#define POINT_COLOR_A 0.02

#define THRESHOLD_SMALL_MAGNS 0

#define TEST_SPECTROGRAM 0
#define TEST_DIST_DB 0

#define TEST_SPECTROGRAM2 0
#define TEST_DIST_DB 0

#if 0
TEST: discard points with a too low intensity => doesn't work well
#endif

StereoVizVolRender2::StereoVizVolRender2(GraphControl10 *graphControl)
{
    mGraph = graphControl;
    
    mVolRender = new SparseVolRender(NUM_SLICES0, NUM_POINTS_SLICE0);
    mGraph->AddCustomDrawer(mVolRender);
    mGraph->AddCustomControl(this);
    
    mMouseIsDown = false;
    mPrevDrag[0] = 0;
    mPrevDrag[1] = 0;
    
    mAngle0 = 0.0;
    mAngle1 = 0.0;
    
    mAddNum = 0;
    
    // Quality
    mSpeed = SPEED0;
}

StereoVizVolRender2::~StereoVizVolRender2()
{
    delete mVolRender;
}

void
StereoVizVolRender2::AddCurveValuesWeight(const WDL_TypedBuf<double> &xValues,
                                          const WDL_TypedBuf<double> &yValues,
                                          const WDL_TypedBuf<double> &colorWeights)
{
    // Tests just in case
    if (xValues.GetSize() != yValues.GetSize())
        return;
    if (xValues.GetSize() != colorWeights.GetSize())
        return;
    
    if (mAddNum++ % mSpeed != 0)
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
        double dir[2] = { xValues.Get()[i], yValues.Get()[i] };
        double dirLength = sqrt(dir[0]*dir[0] + dir[1]*dir[1]);
        
#define EPS 1e-15
        if (dirLength > EPS)
        {
            dir[0] /= dirLength;
            dir[1] /= dirLength;
        }
        
        // Normalized frequency
        double freq = ((double)i)/xValues.GetSize();
        
        // TEST
        //freq = Utils::AmpToDBNorm(freq, 1e-15, -60.0);
        
        // Compute position
        double posX = dir[0] * freq;
        double posY = dir[1] * freq;
        
        // Modify position
        xValues.Get()[i] = posX;
        yValues.Get()[i] = posY;
        
        // TEST
        //double weight = colorWeights.Get()[i];
        
        //weight = 1.0 - weight;
        
        //colorWeights.Get()[i] = weight;
    }
#endif

#if TEST_SPECTROGRAM2
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        // Compute the normalized direction
        double pos[2] = { xValues.Get()[i], yValues.Get()[i] };
        
        double azim = atan2(pos[1], pos[0]);
        azim = fmod(azim, M_PI);
        azim -= M_PI/2.0;
        azim /= M_PI/2.0;
        
        // Normalized frequency
        double freq = ((double)i)/xValues.GetSize();
        
#if TEST_DIST_DB2
        freq = Utils::AmpToDBNorm(freq, 1e-15, -60.0);
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
        
        double weight = colorWeights.Get()[i];
        
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
        
#if 0
        SparseVolRender::ComputePackedPointColor(&p);
#endif
        
        p.mSize = 1.0;
        
#if (!TEST_SPECTROGRAM && !TEST_SPECTROGRAM2) 
        // Normalized frequency
        p.mColorMapId = ((double)i)/xValues.GetSize();
#else
        p.mColorMapId = weight;
#endif
        
        // Add the point
        newPoints.push_back(p);
    }
    
#if 0
    // Do not decimate if the quality is maximum
    //if (mNumPointsSlice < NUM_POINTS_SLICE1) // BUG: white point in the middle/down
        SparseVolRender::DecimateSlice(&newPoints, mNumPointsSlice);
#endif
    
#if 0
    // Add the slice to the whole set
    mPoints.push_back(newPoints);
    while (mPoints.size() > mNumSlices)
        mPoints.pop_front();
#endif
    
    // Add all the points to the renderer
    // Add them back to front (better for blending !)
    //mVolRender->ClearSlices();
    
#if 0
    for (int i = mPoints.size() - 1; i >= 0; i--)
    {
        // Get the slice
        vector<SparseVolRender::Point> &points = mPoints[i];
        
        mVolRender->AddPoints(points);
    }
#endif
    
    mVolRender->AddSlice(newPoints);
    
    mGraph->SetMyDirty(true);
    
    // Without that, the volume rendering is not displayed
    mGraph->SetDirty(true);
}

void
StereoVizVolRender2::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    mMouseIsDown = true;
    
    mPrevDrag[0] = x;
    mPrevDrag[1] = y;
}

void
StereoVizVolRender2::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    mMouseIsDown = false;
}

void
StereoVizVolRender2::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    int dragX = x - mPrevDrag[0];
    int dragY = y - mPrevDrag[1];
    
    mPrevDrag[0] = x;
    mPrevDrag[1] = y;
    
    mAngle0 += dragX;
    mAngle1 += dragY;
    
    // Bounds
    if (mAngle0 < -90.0)
        mAngle0 = -90.0;
    if (mAngle0 > 90.0)
        mAngle0 = 90.0;
    
    if (mAngle1 < -20.0)
        mAngle1 = -20.0;
    if (mAngle1 > 90.0)
        mAngle1 = 90.0;
    
    mVolRender->SetCameraAngles(mAngle0, mAngle1);
    
    // Camera changed
    //
    // Without that, the camera point of view is not modified if
    // the sound is not playing
    mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

bool
StereoVizVolRender2::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
    // Reset the view
    mAngle0 = 0.0;
    mAngle1 = 0.0;
    
    mVolRender->SetCameraAngles(mAngle0, mAngle1);

    mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
    
    return true;
}

void
StereoVizVolRender2::SetQualityXY(double quality)
{
    int numPointsSlice = (1.0 - quality)*NUM_POINTS_SLICE0 + quality*NUM_POINTS_SLICE1;
    
    mVolRender->SetNumPointsSlice(numPointsSlice);
}

void
StereoVizVolRender2::SetQualityT(double quality)
{
    //mNumSlices = (1.0 - quality)*NUM_SLICES0 + quality*NUM_SLICES1;
    
    int numSlices = (1.0 - quality)*NUM_SLICES0 + quality*NUM_SLICES1;
    mVolRender->SetNumSlices(numSlices);
}

void
StereoVizVolRender2::SetSpeed(double speed)
{
    // Speed is in fact a step
    speed = 1.0 - speed;
    
    //speed = pow(speed, 4.0);
    
    mSpeed = (1.0 - speed)*SPEED0 + speed*SPEED1;
}

void
StereoVizVolRender2::SetColormap(int colormapId)
{
    mVolRender->SetColorMap(colormapId);
    
    // Without that, the colormap is not applied until we play
    mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

void
StereoVizVolRender2::SetInvertColormap(bool flag)
{
    mVolRender->SetInvertColormap(flag);
    
    mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

void
StereoVizVolRender2::DistToDbScale(const WDL_TypedBuf<double> &xValues,
                                    const WDL_TypedBuf<double> &yValues)
{
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        // Compute the normalized direction
        double dir[2] = { xValues.Get()[i], yValues.Get()[i] };
        double length = sqrt(dir[0]*dir[0] + dir[1]*dir[1]);
        
#define EPS 1e-15
        if (length > EPS)
        {
            dir[0] /= length;
            dir[1] /= length;
        }
        
        length = Utils::AmpToDBNorm(length, 1e-15, -60.0);
        
        // Compute position
        double posX = dir[0] * length;
        double posY = dir[1] * length;
        
        // Modify position
        xValues.Get()[i] = posX;
        yValues.Get()[i] = posY;
    }
}
