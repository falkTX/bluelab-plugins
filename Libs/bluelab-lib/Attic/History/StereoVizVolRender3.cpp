//
//  StereoVizVolRender3.cpp
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#include "nanovg.h"

#include <RayCaster.h>

#include <BLUtils.h>

#include "StereoVizVolRender3.h"

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
#define MAX_ANGLE_1 90.0 //70.0

// Below
#define MIN_ANGLE_1 -20.0

#define TEST_SPECTROGRAM3 1

// Selection
#define SELECTION_BORDER_SIZE 8

// NOT TESTED
#define FIX_FREEZE_GUI 1


#if 0
TEST: discard points with a too low intensity => doesn't work well
#endif


class CameraOrientation
{
public:
    CameraOrientation();
    
    virtual ~CameraOrientation();
    
    void GetValue(BL_FLOAT *angle0, BL_FLOAT *angle1);
    
    void SetValue(BL_FLOAT angle0, BL_FLOAT angle1);
    
    BL_FLOAT GetAngle0();
    BL_FLOAT GetAngle1();
    
    static void FindNeasrestAlignedOrientation(BL_FLOAT currentAngle0,
                                               BL_FLOAT currentAngle1,
                                               BL_FLOAT *resultAngle0,
                                               BL_FLOAT *resultAngle1);
    
    // Return true if no step remains to do
    static bool StepTo(CameraOrientation *result,
                       const CameraOrientation &start, const CameraOrientation &end);
    
protected:
    BL_FLOAT mAngle0;
    BL_FLOAT mAngle1;
};

CameraOrientation::CameraOrientation()
{
    mAngle0 = 0.0;
    mAngle1 = 0.0;
}
    
CameraOrientation::~CameraOrientation() {}

void
CameraOrientation::GetValue(BL_FLOAT *angle0, BL_FLOAT *angle1)
{
    *angle0 = mAngle0;
    *angle1 = mAngle1;
}

void
CameraOrientation::SetValue(BL_FLOAT angle0, BL_FLOAT angle1)
{
    mAngle0 = angle0;
    mAngle1 = angle1;
}
    
BL_FLOAT
CameraOrientation::GetAngle0()
{
    return mAngle0;
}

BL_FLOAT
CameraOrientation::GetAngle1()
{
    return mAngle1;
}

#if 0 // A bit false...
void
CameraOrientation::FindNeasrestAlignedOrientation(BL_FLOAT currentAngle0,
                                                  BL_FLOAT currentAngle1,
                                                  BL_FLOAT *resultAngle0,
                                                  BL_FLOAT *resultAngle1)
{
    *resultAngle0 = 0.0;
    *resultAngle1 = 0.0;
    
    if ((currentAngle0 < 0.0) && (-currentAngle0 > currentAngle1))
    {
        *resultAngle0 = -MAX_ANGLE_0;
        *resultAngle1 = 0.0;
    }
    else if ((currentAngle0 < 0.0) && (-currentAngle0 < currentAngle1))
    {
        *resultAngle0 = 0.0;
        *resultAngle1 = MAX_ANGLE_1;
    }
    else if ((currentAngle0 > 0.0) && (currentAngle0 > currentAngle1))
    {
        *resultAngle0 = MAX_ANGLE_0;
        *resultAngle1 = 0.0;
    }
    else if ((currentAngle0 > 0.0) && (currentAngle0 < currentAngle1))
    {
        *resultAngle0 = 0.0;
        *resultAngle1 = MAX_ANGLE_1;
    }
}
#endif

void
CameraOrientation::FindNeasrestAlignedOrientation(BL_FLOAT currentAngle0,
                                                  BL_FLOAT currentAngle1,
                                                  BL_FLOAT *resultAngle0,
                                                  BL_FLOAT *resultAngle1)
{
    *resultAngle0 = 0.0;
    *resultAngle1 = 0.0;
    
    if (currentAngle0 < -MAX_ANGLE_0/2.0)
        *resultAngle0 = -MAX_ANGLE_0;
    else
        if (currentAngle0 > MAX_ANGLE_0/2.0)
            *resultAngle0 = MAX_ANGLE_0;
    else // between -MAX_ANGLE_0/2.0 and MAX_ANGLE_0/2.0
        *resultAngle0 = 0.0;
    
    
    if (currentAngle1 > MAX_ANGLE_1/2.0)
        *resultAngle1 = MAX_ANGLE_1;
    else
        *resultAngle1 = 0.0;
}


bool
CameraOrientation::StepTo(CameraOrientation *result,
                          const CameraOrientation &start, const CameraOrientation &end)
{
    // Step in degrees
#define STEP 10.0
 
#define EPS 1e-15
    
    // Compute directions
    BL_FLOAT dirAngle0 = end.mAngle0 - start.mAngle0;
    if (dirAngle0 < 0.0)
        dirAngle0 = -1.0;
    if (dirAngle0 > 0.0)
        dirAngle0 = 1.0;
    
    BL_FLOAT dirAngle1 = end.mAngle1 - start.mAngle1;
    if (dirAngle1 < 0.0)
        dirAngle1 = -1.0;
    if (dirAngle1 > 0.0)
        dirAngle1 = 1.0;
    
    // Check if we direction is null
    //if ((std::fabs(dirAngle0) < EPS) && (std::fabs(dirAngle1) < EPS))
    //    // No additional moves to do, stop.
    //    return true;
    
    // Increase one step
    result->mAngle0 = start.mAngle0 + STEP*dirAngle0;
    result->mAngle1 = start.mAngle1 + STEP*dirAngle1;
    
    bool maxAngle0 = false;
    bool maxAngle1 = false;
    
    // Check the bounds for angle 0
    if ((dirAngle0 >= 0.0) && (result->mAngle0 >= end.mAngle0))
    {
        result->mAngle0 = end.mAngle0;
        
        maxAngle0 = true;
    }
    
    if ((dirAngle0 <= 0.0) && (result->mAngle0 <= end.mAngle0))
    {
        result->mAngle0 = end.mAngle0;
        
        maxAngle0 = true;
    }
    
    // Check the bounds for angles
    if ((dirAngle1 >= 0.0) && (result->mAngle1 >= end.mAngle1))
    {
        result->mAngle1 = end.mAngle1;
        
        maxAngle1 = true;
    }
    
    if ((dirAngle1 <= 0.0) && (result->mAngle1 <= end.mAngle1))
    {
        result->mAngle1 = end.mAngle1;
        
        maxAngle1 = true;
    }
    
    if (maxAngle0 && maxAngle1)
        return true;
    
    return false;
}


StereoVizVolRender3::StereoVizVolRender3(GraphControl11 *graphControl)
{
    mGraph = graphControl;
    
    mRayCaster = new RayCaster(NUM_SLICES0, NUM_POINTS_SLICE0);
    mGraph->AddCustomDrawer(mRayCaster);
    mGraph->AddCustomControl(this);
    
    // Selection
    mMouseIsDown = false;
    mPrevDrag[0] = 0;
    mPrevDrag[1] = 0;
    
    mIsSelecting = false;
    mSelectionActive = false;
    
    mPrevMouseInsideSelect = false;
    //mSelectionActive = false;
    
    mPrevMouseDrag = false;
    
    // Rotation of the camera
    mAngle0 = 0.0;
    mAngle1 = 0.0;
    
    mAddNum = 0;
    
    // Quality
    mSpeed = SPEED0;
    
    // Cmd key
    mPrevCmdPressed = false;
    
    // Automatic orientation change (step by step)
    mIsSteppingOrientation = false;
    
    mStartOrientation = new CameraOrientation();
    mEndOrientation = new CameraOrientation();
    mPrevUserOrientation = new CameraOrientation();;
}

StereoVizVolRender3::~StereoVizVolRender3()
{
    delete mRayCaster;
    
    delete mStartOrientation;
    delete mEndOrientation;
    delete mPrevUserOrientation;
}

void
StereoVizVolRender3::AddCurveValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
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
        
        // Compute position
        BL_FLOAT posX = dir[0] * freq;
        BL_FLOAT posY = dir[1] * freq;
        
        // Modify position
        xValues.Get()[i] = posX;
        yValues.Get()[i] = posY;
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
    
    vector<RayCaster::Point> newPoints;

    for (int i = 0; i < xValues.GetSize(); i++)
    {
        RayCaster::Point p;
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
        
        p.mSize[0] = 1.0;
        p.mSize[1] = 1.0;
        
        p.mIsSelected = false;
        
        // Add the point
        newPoints.push_back(p);
    }
    
    mRayCaster->AddSlice(newPoints, skipDisplay);
    
    if (!skipDisplay)
    {
        //mGraph->SetMyDirty(true);
    
        // Without that, the volume rendering is not displayed
#if !FIX_FREEZE_GUI
        mGraph->SetDirty(true);
#else
        mGraph->SetDirty(false);
#endif
    }
}

void
StereoVizVolRender3::Clear()
{
    mRayCaster->ClearSlices();
}

void
StereoVizVolRender3::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    if (mIsSteppingOrientation)
        // Don't manage the mouse if we are currently playing a camera animation
        return;
    
    mMouseIsDown = true;
    
    mPrevDrag[0] = x;
    mPrevDrag[1] = y;
    
    mPrevMouseDrag = false;
    
    // Check that if is the beginning of a drag from inside the selection.
    // In this case, move the selection.
    //
    // We can move selection even if shift is not pressed anymore
    mPrevMouseInsideSelect = InsideSelection(x, y);

    // Try to select borders
    //
    // We can select borders even if shift is not pressed anymore
    SelectBorders(x, y);
    
    if (mPrevCmdPressed)//(pMod->S)
        mIsSelecting = true;
    else
    {
        if (!BorderSelected() && !mPrevMouseInsideSelect)
            mIsSelecting = false;
    }
    
    // Selection
    if (!mIsSelecting)
    {
        mSelectionActive = false;
        mRayCaster->DisableSelection();
    }
}

void
StereoVizVolRender3::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    if (mIsSteppingOrientation)
        // Don't manage the mouse if we are currently playing a camera animation
        return;
    
    if (!mMouseIsDown)
        return;
    
    mMouseIsDown = false;
    
    if (mIsSelecting)
    {
        mSelectionActive = true;
        //mIsSelecting = false;
    }
    
    if (!mPrevMouseDrag)
        // Pure mouse up
    {
        if (!mPrevMouseInsideSelect && !BorderSelected())
        {
            mSelectionActive = false;
        
            mRayCaster->ResetSelection();
            
            // Must do that to draw the selection when the volume is not updated
            //mGraph->SetMyDirty(true);
            mGraph->SetDirty(true);
        }
    }
    
    for (int i = 0; i < 4; i++)
        mBorderSelected[i] = false;
}

void
StereoVizVolRender3::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    if (mIsSteppingOrientation)
        // Don't manage the mouse if we are currently playing a camera animation
        return;
    
    mPrevMouseDrag = true;
    
    if (mIsSelecting && mPrevCmdPressed)
        // Grow Selection
    {
        if (!mPrevMouseInsideSelect && !BorderSelected())
            // Not moving selection, and not moving border
        {
            mSelection[0] = mPrevDrag[0];
            mSelection[1] = mPrevDrag[1];
        
            mSelection[2] = x;
            mSelection[3] = y;
        
            mSelectionActive = true;
        
            UpdateSelection();
            
            // Must do that to draw the selection when the volume is not updated
            //mGraph->SetMyDirty(true);
            mGraph->SetDirty(true);
        
            return;
        }
        else
            // Moving slection or moving a border
        {
            if (!BorderSelected())
            {
                // Move the selection
                mSelection[0] += dX;
                mSelection[1] += dY;
                
                mSelection[2] += dX;
                mSelection[3] += dY;
            }
            else
                // Modify the selection
            {
                if (mBorderSelected[0])
                    mSelection[0] += dX;
                
                if (mBorderSelected[1])
                    mSelection[1] += dY;
                
                if (mBorderSelected[2])
                    mSelection[2] += dX;
                
                if (mBorderSelected[3])
                    mSelection[3] += dY;
            }
            
            UpdateSelection();
            
            // Must do that to redraw the selection when the volume is not updated
            //mGraph->SetMyDirty(true);
            mGraph->SetDirty(true);
            
            return;
        }
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
    
    mRayCaster->SetCameraAngles(mAngle0, mAngle1);
    
    // Camera changed
    //
    // Without that, the camera point of view is not modified if
    // the sound is not playing
    //mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

bool
StereoVizVolRender3::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
    if (mIsSteppingOrientation)
        // Don't manage the mouse if we are currently playing a camera animation
        return true;
    
    if (InsideSelection(x, y))
        // Do not reset the camera view if BL_FLOAT-click inside selection
        return true;
    
    // Reset the view
    mAngle0 = 0.0;
    mAngle1 = 0.0;
    
    mRayCaster->SetCameraAngles(mAngle0, mAngle1);

    // mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
    
    return true;
}

bool
StereoVizVolRender3::OnKeyDown(int x, int y, int key, IMouseMod* pMod)
{
    bool cmdPressed = false;
    bool cmdReleased = false;
    
    // bl-iplug2
#if 0
    // Detect "cmd" pressed or released
    if (pMod->Cmd && !mPrevCmdPressed)
    {
        cmdPressed = true;
        mPrevCmdPressed = true;
    }
    
    if (!pMod->Cmd && mPrevCmdPressed)
    {
        cmdReleased = true;
        mPrevCmdPressed = false;
    }
#endif
    
    if (cmdPressed && !mIsSteppingOrientation)
    {
        mPrevUserOrientation->SetValue(mAngle0, mAngle1);
        
        mStartOrientation->SetValue(mAngle0, mAngle1);
        
        BL_FLOAT angle0;
        BL_FLOAT angle1;
        CameraOrientation::FindNeasrestAlignedOrientation(mAngle0, mAngle1,
                                                          &angle0, &angle1);
        mEndOrientation->SetValue(angle0, angle1);
        
        mIsSteppingOrientation = true;
    }
    
    if (cmdReleased && !mIsSteppingOrientation)
    {
        BL_FLOAT prevAngle0;
        BL_FLOAT prevAngle1;
        mPrevUserOrientation->GetValue(&prevAngle0, &prevAngle1);
        
        mStartOrientation->SetValue(mAngle0, mAngle1);
        mEndOrientation->SetValue(prevAngle0, prevAngle1);
        
        mIsSteppingOrientation = true;
        
        // Cmd released, disable selection
        mIsSelecting = false;
        mRayCaster->DisableSelection();
    }
    
    if (cmdPressed || cmdReleased)
        return true;
    
    return false;
}

void
StereoVizVolRender3::OnGUIIdle()
{
    if (!mIsSteppingOrientation)
        // Not currently automatic moving the camera
        return;
    
    // Step the camera orientation
    CameraOrientation result;
    bool finished = CameraOrientation::StepTo(&result,
                                              *mStartOrientation, *mEndOrientation);
    
    // Update
    *mStartOrientation = result;
    
    // Update the camera
    mAngle0 = result.GetAngle0();
    mAngle1 = result.GetAngle1();
    
    mRayCaster->SetCameraAngles(mAngle0, mAngle1);
    
    // Check if animation is finished
    if (finished)
    {
        mIsSteppingOrientation = false;
        
        if (mPrevCmdPressed)
        {
            mRayCaster->VolumeToSelection();
            
            //if (mIsSelecting)
            mRayCaster->EnableSelection();
            
            UpdateSelectionRC();
            
            mSelectionActive = true;
            
            // Touch selection, in order to hilight the voxels inside the selection
            // in the RayCaster
            RC_FLOAT selection[4];
            bool selectionValid = mRayCaster->GetSelection(selection);
            if (selectionValid)
                mRayCaster->SetSelection(selection);
        }
    }
    
    // Force graph update
    //mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

// Not used anymore
void
StereoVizVolRender3::SetQualityXY(BL_FLOAT quality)
{
}

void
StereoVizVolRender3::SetQualityT(BL_FLOAT quality)
{
    int numSlices = (1.0 - quality)*NUM_SLICES0 + quality*NUM_SLICES1;
    mRayCaster->SetNumSlices(numSlices);
}

void
StereoVizVolRender3::SetQuality(BL_FLOAT quality)
{
    mRayCaster->SetQuality(quality);
    
    //mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

void
StereoVizVolRender3::SetSpeed(BL_FLOAT speed)
{
    // Disabled !!
    // (for implementing selection)
    return;
    
    // Speed is in fact a step
    speed = 1.0 - speed;
    
    //speed = std::pow(speed, 4.0);
    
    mSpeed = (1.0 - speed)*SPEED0 + speed*SPEED1;
}

void
StereoVizVolRender3::SetColormap(int colormapId)
{
    mRayCaster->SetColorMap(colormapId);
    
    // Without that, the colormap is not applied until we play
    // mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

void
StereoVizVolRender3::SetInvertColormap(bool flag)
{
    mRayCaster->SetInvertColormap(flag);
    
    // mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

void
StereoVizVolRender3::SetColormapRange(BL_FLOAT range)
{
    mRayCaster->SetColormapRange(range);
    
    //mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

void
StereoVizVolRender3::SetColormapContrast(BL_FLOAT contrast)
{
    mRayCaster->SetColormapContrast(contrast);
    
    // mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

// Not used anymore
void
StereoVizVolRender3::SetPointSizeCoeff(BL_FLOAT coeff)
{
    // bl-iplug2
    //mRayCaster->SetPointScale(coeff);
    mRayCaster->SetPointSize(coeff);
    
    //mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

// Not used anymore
void
StereoVizVolRender3::SetAlphaCoeff(BL_FLOAT coeff)
{
    mRayCaster->SetAlphaScale(coeff/100.0);
    
    // mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

void
StereoVizVolRender3::SetThreshold(BL_FLOAT threshold)
{
    mRayCaster->SetThreshold(threshold);
    
    // mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

void
StereoVizVolRender3::SetClipFlag(bool flag)
{
    mRayCaster->SetClipFlag(flag);
    
    // mGraph->SetMyDirty(true);
    mGraph->SetDirty(true);
}

void
StereoVizVolRender3::SetDisplayRefreshRate(int displayRefreshRate)
{
    mRayCaster->SetDisplayRefreshRate(displayRefreshRate);
}

long
StereoVizVolRender3::GetNumSlices()
{
    long numSlices = mRayCaster->GetNumSlices();
    
    return numSlices;
}

bool
StereoVizVolRender3::SelectionEnabled()
{
    bool enabled = mRayCaster->IsSelectionEnabled();
    
    return enabled;
}

bool
StereoVizVolRender3::GetCenterSliceSelection(vector<bool> *selection,
                                             vector<BL_FLOAT> *xCoords,
                                             long *sliceNum)
{
    bool res = true;
    
    // bl-iplug2
#if 0
    res = mRayCaster->GetCenterSliceSelection(selection, xCoords, sliceNum);
#endif
    
    return res;
}

void
StereoVizVolRender3::DistToDbScale(const WDL_TypedBuf<BL_FLOAT> &xValues,
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
        
        length = BLUtils::AmpToDBNorm(length, 1e-15, -60.0);
        
        // Compute position
        BL_FLOAT posX = dir[0] * length;
        BL_FLOAT posY = dir[1] * length;
        
        // Modify position
        xValues.Get()[i] = posX;
        yValues.Get()[i] = posY;
    }
}

void
StereoVizVolRender3::SelectBorders(int x, int y)
{
    for (int i = 0; i < 4; i++)
        mBorderSelected[i] = false;
    
    int dist0 = abs(x - (int)mSelection[0]);
    if ((dist0 < SELECTION_BORDER_SIZE) && // near x
        (y > mSelection[1]) && (y < mSelection[3])) // on the interval of y
        mBorderSelected[0] = true;
    
    int dist1 = abs(y - (int)mSelection[1]);
    if ((dist1 < SELECTION_BORDER_SIZE) && // near y
        (x > mSelection[0]) && (x < mSelection[2])) // on the interval of x
        mBorderSelected[1] = true;
    
    int dist2 = abs(x - (int)mSelection[2]);
    if ((dist2 < SELECTION_BORDER_SIZE) && // near x
        (y > mSelection[1]) && (y < mSelection[3])) // on the interval of y
        mBorderSelected[2] = true;
    
    int dist3 = abs(y - (int)mSelection[3]);
    if ((dist3 < SELECTION_BORDER_SIZE) && // near y
        (x > mSelection[0]) && (x < mSelection[2])) // on the interval of x
        mBorderSelected[3] = true;
}

bool
StereoVizVolRender3::InsideSelection(int x, int y)
{
    if (!mSelectionActive)
        return false;
    
    if (x < mSelection[0])
        return false;
    
    if (y < mSelection[1])
        return false;
    
    if (x > mSelection[2])
        return false;
    
    if (y > mSelection[3])
        return false;
    
    return true;
}

bool
StereoVizVolRender3::BorderSelected()
{
    for (int i = 0; i < 4; i++)
    {
        if (mBorderSelected[i])
            return true;
    }
    
    return false;
}

void
StereoVizVolRender3::UpdateSelection()
{
    int width = mGraph->GetRECT().W();
    int height = mGraph->GetRECT().H();
    
    RC_FLOAT selection[4];
    selection[0] = ((BL_FLOAT)mSelection[0])/width;
    selection[1] = ((BL_FLOAT)mSelection[1])/height;
    selection[2] = ((BL_FLOAT)mSelection[2])/width;
    selection[3] = ((BL_FLOAT)mSelection[3])/height;
    
    mRayCaster->SetSelection(selection);
}

void
StereoVizVolRender3::UpdateSelectionRC()
{
    RC_FLOAT selection[4];
    bool selectionValid = mRayCaster->GetSelection(selection);
    
    if (selectionValid)
    {
        int width = mGraph->GetRECT().W();
        int height = mGraph->GetRECT().H();
    
        mSelection[0] = selection[0]*width;
        mSelection[1] = selection[1]*height;
        mSelection[2] = selection[2]*width;
        mSelection[3] = selection[3]*height;
    }
}
