//
//  WavesRender.cpp
//  BL-Waves
//
//  Created by applematuer on 10/13/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <LinesRender.h>
#include <Axis3D.h>

#include <BLUtils.h>

// Include for flag for partials debug
#include <SASViewerProcess.h>

#include "SASViewerRender.h"

// Axis drawing
#define AXIS_OFFSET_Z 0.06

// Artificially modify the coeff, to increase the spread on the grid
#define MEL_COEFF 4.0

// NOTE TESTED
#define FIX_FREEZE_GUI 1

SASViewerRender::SASViewerRender(SASViewerPluginInterface *plug,
                                 GraphControl12 *graphControl,
                                 BL_FLOAT sampleRate, int bufferSize)
{
    mPlug = plug;
    
    mSampleRate = sampleRate;
    mBufferSize = bufferSize;
    
    mLinesRenderWaves = new LinesRender2();
    mLinesRenderWaves->SetMode(LinesRender2::LINES_FREQ);
    
    mLinesRenderPartials = NULL;
    
    SetGraph(graphControl);
    
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials = new LinesRender2();
    mGraph->AddCustomDrawer(mLinesRenderPartials);
    mLinesRenderPartials->SetMode(LinesRender2::POINTS);
    mLinesRenderPartials->SetDensePointsFlag(false);
    
    mLinesRenderPartials->DBG_SetDisplayAllSlices(DEBUG_PARTIAL_TRACKING);
#endif
    
#if DEBUG_PARTIAL_TRACKING
    CreateFreqsAxis();
    
    //mLinesRenderPartials->SetShowAxis(true);
#endif
    
    //
    mMouseIsDown = false;
    mPrevDrag[0] = 0;
    mPrevDrag[1] = 0;
    
    mPrevMouseDrag = false;
    
    // Rotation of the camera
    mCamAngle0 = 0.0;
    mCamAngle1 = MIN_CAM_ANGLE_1;
    
    mLinesRenderWaves->SetCameraAngles(mCamAngle0, mCamAngle1);
    
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials->SetCameraAngles(mCamAngle0, mCamAngle1);
#endif
    
    mAddNum = 0;
}

SASViewerRender::~SASViewerRender()
{
    delete mLinesRenderWaves;
    
#if DEBUG_PARTIAL_TRACKING
    delete mLinesRenderPartials;
    
    delete mFreqsAxis;
#endif
}

void
SASViewerRender::SetGraph(GraphControl12 *graphControl)
{
    mGraph = graphControl;
    
    if (mGraph != NULL)
    {
        mGraph->AddCustomControl(this);
        mGraph->AddCustomDrawer(mLinesRenderWaves);
    }
}

void
SASViewerRender::Clear()
{
    mLinesRenderWaves->ClearSlices();

    // Not sure, where is called Clear() ?
#if !FIX_FREEZE_GUI
    mGraph->SetDirty(true);
#else
    //mGraph->SetDirty(false);
    mGraph->SetDataChanged();
#endif

}

void
SASViewerRender::AddMagns(const WDL_TypedBuf<BL_FLOAT> &magns)
{
    if (magns.GetSize() == 0)
        return;
    
    vector<LinesRender2::Point> points;
    MagnsToPoints(&points, magns);
    
    mLinesRenderWaves->AddSlice(points);
    
    mAddNum++;
    
    // Without that, the volume rendering is not displayed
#if !FIX_FREEZE_GUI
    mGraph->SetDirty(true);
#else
    //mGraph->SetDirty(false);
    mGraph->SetDataChanged();
#endif
}

void
SASViewerRender::AddPoints(const vector<LinesRender2::Point> &points)
{
#if DEBUG_PARTIAL_TRACKING
      // Set to 1 if displaying debug partial peaks
    mLinesRenderPartials->AddSlice(points);
#else
    mLinesRenderWaves->AddSlice(points);
    
    mAddNum++;
#endif
    
    // Without that, the volume rendering is not displayed
#if !FIX_FREEZE_GUI
    mGraph->SetDirty(true);
#else
    //mGraph->SetDirty(false);
    mGraph->SetDataChanged();
#endif
}

void
SASViewerRender::SetLineMode(LinesRender2::Mode mode)
{
    mLinesRenderWaves->SetMode(mode);
}

void
SASViewerRender::OnMouseDown(float x, float y, const IMouseMod &mod)
{
    mMouseIsDown = true;
    
    mPrevDrag[0] = x;
    mPrevDrag[1] = y;
    
    mPrevMouseDrag = false;
}

void
SASViewerRender::OnMouseUp(float x, float y, const IMouseMod &mod)
{
    if (!mMouseIsDown)
        return;
    
    mMouseIsDown = false;
}

void
SASViewerRender::OnMouseDrag(float x, float y, float dX, float dY,
                             const IMouseMod &mod)
{
    if (mod.A)
        // Alt-drag => zoom
    {
        if (!mPrevMouseDrag)
        {
            mPrevMouseDrag = true;
            
            mPrevDrag[0] = y;
            
            return;
        }
        
#define DRAG_WHEEL_COEFF 0.2
        
        BL_FLOAT dY2 = y - mPrevDrag[0];
        mPrevDrag[0] = y;
        
        dY2 *= -1.0;
        dY2 *= DRAG_WHEEL_COEFF;
        
        OnMouseWheel(mPrevDrag[0], mPrevDrag[1], mod, dY2);
        
        return;
    }
    
    mPrevMouseDrag = true;
    
    // Move camera
    
    int dragX = x - mPrevDrag[0];
    int dragY = y - mPrevDrag[1];
    
    mPrevDrag[0] = x;
    mPrevDrag[1] = y;
    
    mCamAngle0 += dragX;
    mCamAngle1 += dragY;
    
    // Bounds
    if (mCamAngle0 < -MAX_CAM_ANGLE_0)
        mCamAngle0 = -MAX_CAM_ANGLE_0;
    if (mCamAngle0 > MAX_CAM_ANGLE_0)
        mCamAngle0 = MAX_CAM_ANGLE_0;
    
    if (mCamAngle1 < MIN_CAM_ANGLE_1)
        mCamAngle1 = MIN_CAM_ANGLE_1;
    if (mCamAngle1 > MAX_CAM_ANGLE_1)
        mCamAngle1 = MAX_CAM_ANGLE_1;
    
    mLinesRenderWaves->SetCameraAngles(mCamAngle0, mCamAngle1);
    
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials->SetCameraAngles(mCamAngle0, mCamAngle1);
#endif
    
    // Camera changed
    //
    // Without that, the camera point of view is not modified if
    // the sound is not playing
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
    
    mPlug->SetCameraAngles(mCamAngle0, mCamAngle1);
}

void //bool
SASViewerRender::OnMouseDblClick(float x, float y, const IMouseMod &mod)
{
    // Reset the view
    mCamAngle0 = 0.0;
    mCamAngle1 = MIN_CAM_ANGLE_1;
    
    mLinesRenderWaves->SetCameraAngles(mCamAngle0, mCamAngle1);
    
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials->SetCameraAngles(mCamAngle0, mCamAngle1);
#endif
    
    // Reset the fov
    mLinesRenderWaves->ResetZoom();
    
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials->ResetZoom();
#endif
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
    
    mPlug->SetCameraAngles(mCamAngle0, mCamAngle1);
    
    BL_FLOAT fov = mLinesRenderWaves->GetCameraFov();
    mPlug->SetCameraFov(fov);
    
    //return true;
}

void
SASViewerRender::OnMouseWheel(float x, float y,
                              const IMouseMod &mod, float d)
{
#define WHEEL_ZOOM_STEP 0.025
    
    BL_FLOAT zoomChange = 1.0 + d*WHEEL_ZOOM_STEP;
    
    mLinesRenderWaves->ZoomChanged(zoomChange);
    
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials->ZoomChanged(zoomChange);
#endif
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
    
    BL_FLOAT angle = mLinesRenderWaves->GetCameraFov();
    mPlug->SetCameraFov(angle);
}

void
SASViewerRender::SetSpeed(BL_FLOAT speed)
{
    mLinesRenderWaves->SetSpeed(speed);
    
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials->SetSpeed(speed);
#endif
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SASViewerRender::SetDensity(BL_FLOAT density)
{
    mLinesRenderWaves->SetDensity(density);
    
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials->SetDensity(density);
#endif
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SASViewerRender::SetScale(BL_FLOAT scale)
{
    mLinesRenderWaves->SetScale(scale);
    
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials->SetScale(scale);
#endif
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SASViewerRender::SetCamAngle0(BL_FLOAT angle)
{
    mCamAngle0 = angle;
    
    mLinesRenderWaves->SetCameraAngles(mCamAngle0, mCamAngle1);
    
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials->SetCameraAngles(mCamAngle0, mCamAngle1);
#endif
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SASViewerRender::SetCamAngle1(BL_FLOAT angle)
{
    mCamAngle1 = angle;
    
    mLinesRenderWaves->SetCameraAngles(mCamAngle0, mCamAngle1);
    
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials->SetCameraAngles(mCamAngle0, mCamAngle1);
#endif
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
SASViewerRender::SetCamFov(BL_FLOAT angle)
{
    mLinesRenderWaves->SetCameraFov(angle);
 
#if DEBUG_PARTIAL_TRACKING
    mLinesRenderPartials->SetCameraFov(angle);
#endif
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

int
SASViewerRender::GetNumSlices()
{
    int numSlices = mLinesRenderWaves->GetNumSlices();
    
    return numSlices;
}

int
SASViewerRender::GetSpeed()
{
    int speed = mLinesRenderWaves->GetSpeed();
    
    return speed;
}

void
SASViewerRender::SetAdditionalLines(const vector<LinesRender2::Line> &lines,
                                    BL_FLOAT lineWidth)
{
    mLinesRenderWaves->SetAdditionalLines(lines, lineWidth);
}

void
SASViewerRender::ClearAdditionalLines()
{
    mLinesRenderWaves->ClearAdditionalLines();
}

void
SASViewerRender::ShowAdditionalLines(bool flag)
{
    mLinesRenderWaves->ShowAdditionalLines(flag);
}

void
SASViewerRender::MagnsToPoints(vector<LinesRender2::Point> *points,
                               const WDL_TypedBuf<BL_FLOAT> &magns)
{
    if (magns.GetSize() == 0)
        return;
    
    // Convert to points
    points->resize(magns.GetSize());
    
    // Optim
    BL_FLOAT xCoeff = 0.0;
    if (magns.GetSize() > 0)
    {
        if (magns.GetSize() <= 1)
            xCoeff = 1.0/magns.GetSize();
        else
            xCoeff = 1.0/(magns.GetSize() - 1);
    }
    
    // Loop
    for (int i = 0; i < magns.GetSize(); i++)
    {
        BL_FLOAT magn = magns.Get()[i];
        
        LinesRender2::Point &p = (*points)[i];
        
#if 0 // Original
        if (magns.GetSize() <= 1)
            p.mX = ((BL_FLOAT)i)/magns.GetSize() - 0.5;
        else
            p.mX = ((BL_FLOAT)i)/(magns.GetSize() - 1) - 0.5;
#endif
        
#if 1 // Optim
        p.mX = xCoeff*i - 0.5;
#endif
        
        p.mY = magn;
        
        // Fill with dummy Z (to avoid undefined value)
        p.mZ = 0.0;
        
        // Fill with dummy color (to avoid undefined value)
        p.mR = 0;
        p.mG = 0;
        p.mB = 0;
        p.mA = 0;
    }
}

void
SASViewerRender::CreateFreqsAxis()
{
    // Create axis
#define NUM_AXIS_DATA 7 //8
    char *labels[NUM_AXIS_DATA] =
    {
        /*"1Hz",*/ "100Hz", "500Hz", "1KHz", "2KHz", "5KHz", "10KHz", "20KHz"
    };
    
    // Scale the axis normalized values
    BL_FLOAT freqs[NUM_AXIS_DATA] =
    {
        /*1.0,*/ 100.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0
    };
    
    BL_FLOAT normPos[NUM_AXIS_DATA];
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        BL_FLOAT freq = freqs[i];
        freq = FreqToMelNorm(freq);
        
        normPos[i] = freq;
    }
    
    
    // 3d extremities of the axis
    BL_FLOAT p0[3] = { -0.5, 0.0, 0.5 + AXIS_OFFSET_Z };
    BL_FLOAT p1[3] = { 0.5, 0.0, 0.5 + AXIS_OFFSET_Z };
    
    // Create the axis
    mFreqsAxis = new Axis3D(labels, normPos, NUM_AXIS_DATA, p0, p1);
    mFreqsAxis->SetDoOverlay(true);
    mFreqsAxis->SetPointProjector(mLinesRenderPartials);
    
    mLinesRenderPartials->AddAxis(mFreqsAxis);
}

BL_FLOAT
SASViewerRender::FreqToMelNorm(BL_FLOAT freq)
{
    // Convert to Mel
    BL_FLOAT hzPerBin = mSampleRate/(mBufferSize/2.0);
    hzPerBin *= MEL_COEFF;
    
    // Hack: something is not really correct here...
    freq *= 2.0;
    
    BL_FLOAT result = BLUtils::FreqToMelNorm((BL_FLOAT)(freq*MEL_COEFF), hzPerBin, mBufferSize);
    
    return result;
}

#endif // IGRAPHICS_NANOVG
