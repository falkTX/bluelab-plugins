//
//  StereoVizVolRender2.cpp
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

// #bl-iplug2
//#include "nanovg.h"

#include <BLUtils.h>

#include "StereoVizVolRender2.h"

#define NUM_POINTS_SLICE0 32
//#define NUM_POINTS_SLICE1 2048

// Set to maximum, this way, we are sur no point is missing
#define NUM_POINTS_SLICE1 1024*1024

// TEST: 500x500 for GPU slices
//#define NUM_POINTS_SLICE1 250000 // TEST !

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

// Max: 90 = >but we will see slices too much

// Sides
//#define MAX_ANGLE_0 70.0

// Set to 90 for debugging
#define MAX_ANGLE_0 90.0

// Above
#define MAX_ANGLE_1 70.0

// Below
#define MIN_ANGLE_1 -20.0

#define TEST_SPECTROGRAM3 1


#if 0
TEST: discard points with a too low intensity => doesn't work well
#endif

StereoVizVolRender2::StereoVizVolRender2(GraphControl11 *graphControl)
{
    mGraph = graphControl;
    
    mVolRender = new SparseVolRender(NUM_SLICES0, NUM_POINTS_SLICE0);
    mGraph->AddCustomDrawer(mVolRender);
    mGraph->AddCustomControl(this);
    
    mMouseIsDown = false;
    mPrevDrag[0] = 0;
    mPrevDrag[1] = 0;
    
    mIsSelecting = false;
    mSelectionActive = false;
    
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
StereoVizVolRender2::AddCurveValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                          const WDL_TypedBuf<BL_FLOAT> &yValues,
                                          const WDL_TypedBuf<BL_FLOAT> &colorWeights)
{
    // Tests just in case
    if (xValues.GetSize() != yValues.GetSize())
        return;
    if (xValues.GetSize() != colorWeights.GetSize())
        return;
    
    //if (mAddNum++ % mSpeed != 0)
    //    return;
    bool skipDisplay = (mAddNum++ % mSpeed != 0);
    
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
        
        p.mSize = 1.0;
        
#if !TEST_SPECTROGRAM3
        
        
        // NOTE: that was an old expe to choose weight as norm value id
#if (!TEST_SPECTROGRAM && !TEST_SPECTROGRAM2)
        // Normalized frequency
        p.mColorMapId = ((BL_FLOAT)i)/xValues.GetSize();
#else
        p.mColorMapId = weight;
#endif
#else
        // GOOD !
        //
        // Should be that for all modes !
        p.mColorMapId = weight;
#endif
        
        p.mIsSelected = false;
        
        // Add the point
        newPoints.push_back(p);
    }
    
    mVolRender->AddSlice(newPoints, skipDisplay);
    
    if (!skipDisplay)
    {
        // Without that, the volume rendering is not displayed
        //mGraph->SetDirty(true);
        mGraph->SetDataChanged();
    }
}

void
StereoVizVolRender2::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    mMouseIsDown = true;
    
    mPrevDrag[0] = x;
    mPrevDrag[1] = y;
    
    // Selection
    mSelectionActive = false;
    mVolRender->DisableSelection();
    
    if (pMod->S)
        mIsSelecting = true;
    else
        mIsSelecting = false;
}

void
StereoVizVolRender2::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    mMouseIsDown = false;
    
    if (mIsSelecting)
    {
        mSelectionActive = true;
        mIsSelecting = false;
    }
}

void
StereoVizVolRender2::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    if ((pMod->S) && mIsSelecting)
        // Grow Selection
    {
        mSelection[0] = mPrevDrag[0];
        mSelection[1] = mPrevDrag[1];
        
        mSelection[2] = x;
        mSelection[3] = y;
        
        mSelectionActive = true;
        
        int width = mGraph->GetRECT().W();
        int height = mGraph->GetRECT().H();
        
        BL_FLOAT selection[4];
        selection[0] = ((BL_FLOAT)mSelection[0])/width;
        selection[1] = ((BL_FLOAT)mSelection[1])/height;
        selection[2] = ((BL_FLOAT)mSelection[2])/width;
        selection[3] = ((BL_FLOAT)mSelection[3])/height;
        
        mVolRender->SetSelection(selection);
        // Must do that to draw the selection when the volume is not updated
        //mGraph->SetDirty(true);
        mGraph->SetDataChanged();
        
        return;
    }
    
    // Else move camera
    
    int dragX = x - mPrevDrag[0];
    int dragY = y - mPrevDrag[1];
    
    mPrevDrag[0] = x;
    mPrevDrag[1] = y;
    
    mAngle0 += dragX;
    mAngle1 += dragY;
    
    // Bounds
    if (mAngle0 < -MAX_ANGLE_0)
        mAngle0 = -MAX_ANGLE_0;
    if (mAngle0 > MAX_ANGLE_0)
        mAngle0 = MAX_ANGLE_0;
    
    if (mAngle1 < MIN_ANGLE_1)
        mAngle1 = MIN_ANGLE_1;
    if (mAngle1 > MAX_ANGLE_1)
        mAngle1 = MAX_ANGLE_1;
    
    mVolRender->SetCameraAngles(mAngle0, mAngle1);
    
    // Camera changed
    //
    // Without that, the camera point of view is not modified if
    // the sound is not playing
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

bool
StereoVizVolRender2::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
    // Reset the view
    mAngle0 = 0.0;
    mAngle1 = 0.0;
    
    mVolRender->SetCameraAngles(mAngle0, mAngle1);

    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
    
    return true;
}

void
StereoVizVolRender2::SetQualityXY(BL_FLOAT quality)
{
    int numPointsSlice = (1.0 - quality)*NUM_POINTS_SLICE0 + quality*NUM_POINTS_SLICE1;
    
    mVolRender->SetNumPointsSlice(numPointsSlice);
}

void
StereoVizVolRender2::SetQualityT(BL_FLOAT quality)
{
    int numSlices = (1.0 - quality)*NUM_SLICES0 + quality*NUM_SLICES1;
    mVolRender->SetNumSlices(numSlices);
}

void
StereoVizVolRender2::SetQuality(BL_FLOAT quality)
{
    mVolRender->SetQuality(quality);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
StereoVizVolRender2::SetSpeed(BL_FLOAT speed)
{
    // Speed is in fact a step
    speed = 1.0 - speed;
    
    //speed = std::pow(speed, 4.0);
    
    mSpeed = (1.0 - speed)*SPEED0 + speed*SPEED1;
}

void
StereoVizVolRender2::SetColormap(int colormapId)
{
    mVolRender->SetColorMap(colormapId);
    
    // Without that, the colormap is not applied until we play
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
StereoVizVolRender2::SetInvertColormap(bool flag)
{
    mVolRender->SetInvertColormap(flag);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
StereoVizVolRender2::SetColormapRange(BL_FLOAT range)
{
    mVolRender->SetColormapRange(range);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
StereoVizVolRender2::SetColormapContrast(BL_FLOAT contrast)
{
    mVolRender->SetColormapContrast(contrast);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
StereoVizVolRender2::SetPointSizeCoeff(BL_FLOAT coeff)
{
    mVolRender->SetPointSizeCoeff(coeff);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
StereoVizVolRender2::SetAlphaCoeff(BL_FLOAT coeff)
{
    mVolRender->SetAlphaCoeff(coeff);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
StereoVizVolRender2::GetPointsSelection(vector<bool> *pointFlags)
{
    mVolRender->GetPointsSelection(pointFlags);
}

void
StereoVizVolRender2::SetDisplayRefreshRate(int displayRefreshRate)
{
    mVolRender->SetDisplayRefreshRate(displayRefreshRate);
}

void
StereoVizVolRender2::DistToDbScale(const WDL_TypedBuf<BL_FLOAT> &xValues,
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
