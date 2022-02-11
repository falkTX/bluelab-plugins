//
//  SMVVolRender3.cpp
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

// #bl-iplug2
//#include "nanovg.h"

#include <RayCaster2.h>

#include <BLUtils.h>

//#include <SoundMetaViewer.h>

#include "SMVVolRender3.h"

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

#define TEST_SPECTROGRAM3 1

// Selection
#define SELECTION_BORDER_SIZE 8

// NOT TESTED
#define FIX_FREEZE_GUI 1

// 46ms per clices at 44100Hz
//#define MIN_TIME_WINDOW 0.371 // 8 slices at 44100Hz
#define MIN_TIME_WINDOW 4.8 // Minimum value to avoid having holes between slices
//#define MAX_TIME_WINDOW 4.6 // 100 slices at 44100Hz
#define MAX_TIME_WINDOW 9.2 // 200 slices at 44100Hz

#define MIN_TIME_QUALITY 0.1
#define MAX_TIME_QUALITY 1.0

// Alway use max time quality
#define FORCE_MAX_QUALITY_T 1

//#define FIX_PLAY_SELECTION 1

// When we drag the 3D volume with ctrl or alt
#define VOLUME_DRAG_COEFF 0.0025 //0.01

// Fix selection strange incorrect state
// FIX: draw a selection, release Cmd, click somewhere = the selection disappears
// Then make a new selection => the previous selection reappears
//
// => Only disable selection if Cmd is pressed
// = Avoid loosing selection when manipulating the view
#define FIX_SELECTION_MESS 1

// Automatically adapt quality T when speed is changed
#define ADAPTIVE_QUALITY_T 1

// BUG: press several times the Cmd key => the step animation freezes
// Other step to reproduce: press Cmd, release it during animation
// => we stay at the end animation step
// VERY GOOD: now the animation can be canceled if the Cmd key is release
// just after pressing
#define FIX_STEP_ANIM_FREEZE 1

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
        *resultAngle0 = -MAX_CAMERA_ANGLE_0;
        *resultAngle1 = 0.0;
    }
    else if ((currentAngle0 < 0.0) && (-currentAngle0 < currentAngle1))
    {
        *resultAngle0 = 0.0;
        *resultAngle1 = MAX_CAMERA_ANGLE_1;
    }
    else if ((currentAngle0 > 0.0) && (currentAngle0 > currentAngle1))
    {
        *resultAngle0 = MAX_CAMERA_ANGLE_0;
        *resultAngle1 = 0.0;
    }
    else if ((currentAngle0 > 0.0) && (currentAngle0 < currentAngle1))
    {
        *resultAngle0 = 0.0;
        *resultAngle1 = MAX_CAMERA_ANGLE_1;
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
    
    if (currentAngle0 < -MAX_CAMERA_ANGLE_0/2.0)
        *resultAngle0 = -MAX_CAMERA_ANGLE_0;
    else
        if (currentAngle0 > MAX_CAMERA_ANGLE_0/2.0)
            *resultAngle0 = MAX_CAMERA_ANGLE_0;
    else // between -MAX_ANGLE_0/2.0 and MAX_ANGLE_0/2.0
        *resultAngle0 = 0.0;
    
    
    if (currentAngle1 > MAX_CAMERA_ANGLE_1/2.0)
        *resultAngle1 = MAX_CAMERA_ANGLE_1;
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


SMVVolRender3::SMVVolRender3(SoundMetaViewerPluginInterface *plug,
                             GraphControl11 *graphControl,
                             int bufferSize, BL_FLOAT sampleRate)
{
    mPlug = plug;
    
    mGraph = graphControl;
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mRayCaster = new RayCaster2(NUM_SLICES0, NUM_POINTS_SLICE0);
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
    mAngle0 = DEFAULT_ANGLE_0; //0.0;
    mAngle1 = DEFAULT_ANGLE_1; //0.0;
    
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
    
#if FORCE_MAX_QUALITY_T
    SetQualityT(1.0);
#endif
}

SMVVolRender3::~SMVVolRender3()
{
    delete mRayCaster;
    
    delete mStartOrientation;
    delete mEndOrientation;
    delete mPrevUserOrientation;
}

void
SMVVolRender3::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

void
SMVVolRender3::AddCurveValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
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
    
    vector<RayCaster2::Point> newPoints;

    for (int i = 0; i < xValues.GetSize(); i++)
    {
        RayCaster2::Point p;
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
        
        //p.mSize[0] = 1.0;
        //p.mSize[1] = 1.0;
        
        //p.mIsSelected = false;
        
        // Add the point
        newPoints.push_back(p);
    }
    
    mRayCaster->AddSlice(newPoints, skipDisplay);
    
    if (!skipDisplay)
    {
        // Without that, the volume rendering is not displayed
#if !FIX_FREEZE_GUI
        mGraph->SetDirty(true);
#else
        //mGraph->SetDirty(false);
        mGraph->SetDataChanged();
#endif
    }
}

void
SMVVolRender3::Clear()
{
    mRayCaster->ClearSlices();
}

void
SMVVolRender3::OnMouseDown(int x, int y, IMouseMod* pMod)
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
SMVVolRender3::OnMouseUp(int x, int y, IMouseMod* pMod)
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
#if FIX_SELECTION_MESS
        if (pMod->Cmd)
        {
#endif
        if (!mPrevMouseInsideSelect && !BorderSelected())
        {
            mSelectionActive = false;
        
            mRayCaster->ResetSelection();
            
            // Must do that to draw the selection when the volume is not updated
            //mGraph->SetDirty(true);
            mGraph->SetDataChanged();
        }
#if FIX_SELECTION_MESS
        }
#endif
    }
    
    for (int i = 0; i < 4; i++)
        mBorderSelected[i] = false;
}

void
SMVVolRender3::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    if (mIsSteppingOrientation)
        // Don't manage the mouse if we are currently playing a camera animation
        return;
    
    mPrevMouseDrag = true;
    
    if (pMod->A)
        // Alt-drag => zoom
    {
#define DRAG_WHEEL_COEFF 0.2
        
        dY *= -1.0;
        dY *= DRAG_WHEEL_COEFF;
        
        OnMouseWheel(mPrevDrag[0], mPrevDrag[1], pMod, dY);
        
        return;
    }
    
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
            //mGraph->SetDirty(true);
            mGraph->SetDataChanged();
        
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
            //mGraph->SetDirty(true);
            mGraph->SetDataChanged();
            
            return;
        }
    }
    
    if (pMod->C || pMod->S)
        // Control or shift => Move the selection volume
    {
        if (IsVolumeSelectionEnabled())
        {
         
            BL_FLOAT drag = dX;
            bool useX = true;
            if (std::fabs(dY) > std::fabs(drag))
            {
                drag = dY;
                useX = false;
            }
            
            drag *= VOLUME_DRAG_COEFF;
            
            RC_FLOAT trans[3] = { 0.0, 0.0, 0.0 };
            
            if (pMod->C && !pMod->S)
                // Ctrl only => move X
            {
                trans[0] = drag;
                
                if (!useX)
                    trans[0] = -trans[0];
            }
            
            if (pMod->C && pMod->S)
                // Ctrl + shift => move Y
            {
                trans[1] = -drag;
                
                if (useX)
                    trans[1] = -trans[1];
            }
            
            if (!pMod->C && pMod->S)
                // Shift only => move Z
            {
                trans[2] = drag;
                
                if (useX)
                    trans[2] = -trans[2];
            }
            
            mRayCaster->TranslateVolumeSelection(trans);
            
            // Refresh the view
            //mGraph->SetDirty(true);
            mGraph->SetDataChanged();
            
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
    if (mAngle0 < -MAX_CAMERA_ANGLE_0)
        mAngle0 = -MAX_CAMERA_ANGLE_0;
    if (mAngle0 > MAX_CAMERA_ANGLE_0)
        mAngle0 = MAX_CAMERA_ANGLE_0;
    
    if (mAngle1 < MIN_CAMERA_ANGLE_1)
        mAngle1 = MIN_CAMERA_ANGLE_1;
    if (mAngle1 > MAX_CAMERA_ANGLE_1)
        mAngle1 = MAX_CAMERA_ANGLE_1;
    
    mRayCaster->SetCameraAngles(mAngle0, mAngle1);
    
    mPlug->SetCameraAngles(mAngle0, mAngle1);
    
    // Camera changed
    //
    // Without that, the camera point of view is not modified if
    // the sound is not playing
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

bool
SMVVolRender3::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
    if (mIsSteppingOrientation)
        // Don't manage the mouse if we are currently playing a camera animation
        return true;
    
    if (InsideSelection(x, y))
        // Do not reset the camera view if BL_FLOAT-click inside selection
        return true;
    
    // Reset the view
    mAngle0 = DEFAULT_ANGLE_0; //0.0;
    mAngle1 = DEFAULT_ANGLE_1; //0.0;
    
    mRayCaster->SetCameraAngles(mAngle0, mAngle1);

    mPlug->SetCameraAngles(mAngle0, mAngle1);
    
    mRayCaster->SetCameraFov(DEFAULT_CAMERA_FOV);
    mPlug->SetCameraFov(DEFAULT_CAMERA_FOV/*fov*/);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
    
    return true;
}

bool
SMVVolRender3::OnKeyDown(int x, int y, int key, IMouseMod* pMod)
{    
    bool cmdPressed = false;
    bool cmdReleased = false;
    
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
    
#if !FIX_STEP_ANIM_FREEZE
    if (cmdReleased && !mIsSteppingOrientation)
#else
    if (cmdReleased)
    // Cmd release during animation => start reverse animation
#endif
    {
        BL_FLOAT prevAngle0;
        BL_FLOAT prevAngle1;
        mPrevUserOrientation->GetValue(&prevAngle0, &prevAngle1);
        
        mStartOrientation->SetValue(mAngle0, mAngle1);
        mEndOrientation->SetValue(prevAngle0, prevAngle1);
        
        mIsSteppingOrientation = true;
        
        // Cmd released, disable selection
        mIsSelecting = false;
        
        // Disable selection 2D
        mRayCaster->DisableSelection();
    }
    
    if (cmdPressed || cmdReleased)
        return true;
    
    return false;
}

void
SMVVolRender3::OnMouseWheel(int x, int y, IMouseMod* pMod, BL_FLOAT d)
{
#define WHEEL_ZOOM_STEP 0.025
    
    BL_FLOAT zoomChange = 1.0 + d*WHEEL_ZOOM_STEP;
    
    mRayCaster->ZoomChanged(zoomChange);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
    
    BL_FLOAT angle = mRayCaster->GetCameraFov();
    mPlug->SetCameraFov(angle);
}


void
SMVVolRender3::OnGUIIdle()
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
    
    mPlug->SetCameraAngles(mAngle0, mAngle1);
    
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
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

// Not used anymore
void
SMVVolRender3::SetQualityXY(BL_FLOAT quality)
{
}

void
SMVVolRender3::SetSpeedT(BL_FLOAT speed)
{
    BL_FLOAT timeNorm = 1.0 - speed;
    BL_FLOAT timeWindow = (1.0 - timeNorm)*MIN_TIME_WINDOW + timeNorm*MAX_TIME_WINDOW;
    
    BL_FLOAT sliceDuration = ((BL_FLOAT)mBufferSize)/mSampleRate;
    
    int numSlices = timeWindow/sliceDuration;
    
    mRayCaster->SetNumSlices(numSlices); // TEST
    
#if ADAPTIVE_QUALITY_T
    AdaptQualityT(speed);
#endif
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetQualityT(BL_FLOAT quality)
{
    BL_FLOAT qualityT = (1.0 - quality)*MIN_TIME_QUALITY + quality*MAX_TIME_QUALITY;
    mRayCaster->SetQualityT(qualityT);
    
    //int numSlices = (1.0 - quality)*NUM_SLICES0 + quality*NUM_SLICES1;
    //mRayCaster->SetQualityT(numSlices);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetQuality(BL_FLOAT quality)
{
    mRayCaster->SetQuality(quality);
    
#if !FORCE_MAX_QUALITY_T
    //
    SetQualityT(quality);
#endif
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetSpeed(BL_FLOAT speed)
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
SMVVolRender3::SetColormap(int colormapId)
{
    mRayCaster->SetColorMap(colormapId);
    
    // Without that, the colormap is not applied until we play
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetInvertColormap(bool flag)
{
    mRayCaster->SetInvertColormap(flag);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetColormapRange(BL_FLOAT range)
{
    mRayCaster->SetColormapRange(range);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetColormapContrast(BL_FLOAT contrast)
{
    mRayCaster->SetColormapContrast(contrast);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

// Not used anymore
void
SMVVolRender3::SetPointSize(BL_FLOAT size)
{
    mRayCaster->SetPointSize(size);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

//#if 0
// Not used anymore
void
SMVVolRender3::SetAlphaCoeff(BL_FLOAT coeff)
{
    //mRayCaster->SetAlphaScale(coeff/100.0);
    mRayCaster->SetAlphaCoeff(coeff/100.0);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}
//#endif

void
SMVVolRender3::SetThreshold(BL_FLOAT threshold)
{
    mRayCaster->SetThreshold(threshold);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetThresholdCenter(BL_FLOAT thresholdCenter)
{
    mRayCaster->SetThresholdCenter(thresholdCenter);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetClipFlag(bool flag)
{
    mRayCaster->SetClipFlag(flag);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetDisplayRefreshRate(int displayRefreshRate)
{
    mRayCaster->SetDisplayRefreshRate(displayRefreshRate);
}

long
SMVVolRender3::GetNumSlices()
{
    long numSlices = mRayCaster->GetNumSlices();
    
    return numSlices;
}

//bool
//SMVVolRender3::SelectionActive()
//{
//    return mSelectionActive;
//}

bool
SMVVolRender3::IsSelectionEnabled()
{
    bool enabled = mRayCaster->IsSelectionEnabled();
    
    return enabled;
}

bool
SMVVolRender3::IsVolumeSelectionEnabled()
{
    bool enabled = mRayCaster->IsVolumeSelectionEnabled();
    
    return enabled;
}

long
SMVVolRender3::GetCurrentSlice()
{
    long result = mRayCaster->GetCurrentSlice();
    
    return result;
}

bool
SMVVolRender3::GetSliceSelection(vector<bool> *selection,
                                 vector<BL_FLOAT> *xCoords,
                                 long sliceNum)
{
    vector<RC_FLOAT> resXCoords;
    bool res = mRayCaster->GetSliceSelection(selection, &resXCoords, sliceNum);
    
    xCoords->resize(resXCoords.size());
    for (int i = 0; i < resXCoords.size(); i++)
    {
        RC_FLOAT val = resXCoords[i];
        (*xCoords)[i] = val;
    }
    
    return res;
}

bool
SMVVolRender3::GetSelectionBoundsLines(int *startLine, int *endLine)
{
    bool res = mRayCaster->GetSelectionBoundsLines(startLine, endLine);
    
    return res;
}

void
SMVVolRender3::SetPlayBarPos(BL_FLOAT t)
{
    mRayCaster->SetPlayBarPos(t);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetAxis(int idx, Axis3D *axis)
{
    mRayCaster->SetAxis(idx, axis);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

RayCaster2 *
SMVVolRender3::GetRayCaster()
{
    return mRayCaster;
}

void
SMVVolRender3::SetRenderAlgo(int algo)
{
    mRayCaster->SetRenderAlgo((RayCaster2::RenderAlgo)algo);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetRenderAlgoParam(BL_FLOAT renderParam)
{
    mRayCaster->SetRenderAlgoParam(renderParam);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetAutoQuality(bool flag)
{
    mRayCaster->SetAutoQuality(flag);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetCamAngle0(BL_FLOAT angle)
{
    mAngle0 = angle;
    
    mRayCaster->SetCameraAngles(mAngle0, mAngle1);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetCamAngle1(BL_FLOAT angle)
{
    mAngle1 = angle;
    
    mRayCaster->SetCameraAngles(mAngle0, mAngle1);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::SetCamFov(BL_FLOAT angle)
{
    mRayCaster->SetCameraFov(angle);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SMVVolRender3::DistToDbScale(const WDL_TypedBuf<BL_FLOAT> &xValues,
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
            BL_FLOAT lengthInv = 1.0/length;
            dir[0] *= lengthInv;
            dir[1] *= lengthInv;
            
            //dir[0] /= length;
            //dir[1] /= length;
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

void
SMVVolRender3::SelectBorders(int x, int y)
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
SMVVolRender3::InsideSelection(int x, int y)
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
SMVVolRender3::BorderSelected()
{
    for (int i = 0; i < 4; i++)
    {
        if (mBorderSelected[i])
            return true;
    }
    
    return false;
}

void
SMVVolRender3::UpdateSelection()
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
SMVVolRender3::UpdateSelectionRC()
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

void
SMVVolRender3::AdaptQualityT(BL_FLOAT speed)
{
    //BL_FLOAT minQualityT = 0.7;
    BL_FLOAT minQualityT = 0.47; // With this value we keep constant number of slices
    BL_FLOAT maxQualityT = 1.0;
    
    BL_FLOAT qualityT = (1.0 - speed)*minQualityT + speed*maxQualityT;
    
    SetQualityT(qualityT);
}
