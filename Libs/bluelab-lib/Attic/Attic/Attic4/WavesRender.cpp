//
//  WavesRender.cpp
//  BL-Waves
//
//  Created by applematuer on 10/13/18.
//
//

// #bl-iplug2
//#include "nanovg.h"

#include <LinesRender2.h>

#include <BLUtils.h>

//#include <Wav3s.h>
#include <Axis3D.h>

#include "WavesRender.h"


#define ENABLE_ORXX_MODE 1

// Artificially modify the coeff, to increase the spread on the grid
#define MEL_COEFF 4.0

// Axis drawing
#define AXIS_OFFSET_Z 0.06

#define AMPS_AXIS 1

// On Ableton 10, Sierra, AU format, sometimes it freezes
// (create, delete, re-create, manipulate)
//
// This stays locked in the mutex IPlugAU::IPlugAUEntry(),
// and on the other side on BLUtils::TouchPlugParam() from SetCameraFov()
//
// In IControl::SetDirty(bool pushParamToPlug), if pushParamToPlug is true then
// SetParameterFromGUI() is called, which is not correct here, because AddMagns is called
// from the audio thread
#define FIX_ABLETON_AU_FREEZE 1

//const char *_ORXXKey = "izwal";
const char *_ORXXKey = "yoko";

WavesRender::WavesRender(WavesPluginInterface *plug, GraphControl11 *graphControl,
                         int bufferSize, BL_FLOAT sampleRate)
{
    mPlug = plug;
    
    mGraph = graphControl;
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mLinesRender = new LinesRender2();
    mGraph->AddCustomDrawer(mLinesRender);
    mGraph->AddCustomControl(this);

    mFreqsAxis = NULL;
    CreateFreqsAxis();

    mAmpsAxis = NULL;
#if AMPS_AXIS
    UpdateAmpsAxis(false);
#endif
    
    //
    mMouseIsDown = false;
    mPrevDrag[0] = 0;
    mPrevDrag[1] = 0;
    
    mPrevMouseDrag = false;
    
    // Rotation of the camera
    mCamAngle0 = 0.0;
    mCamAngle1 = MIN_CAM_ANGLE_1;
    
    mLinesRender->SetCameraAngles(mCamAngle0, mCamAngle1);
    
    mORXXMode = false;
    mORXXKeyGuessStep = 0;
    
    mAddNum = 0;
}

WavesRender::~WavesRender()
{
    delete mLinesRender;
    
    delete mFreqsAxis;
    
    if (mAmpsAxis != NULL)
        delete mAmpsAxis;
}

void
WavesRender::Reset(BL_FLOAT sampleRate)
{
    if (sampleRate != mSampleRate)
    {
        mSampleRate = sampleRate;
        
        mLinesRender->RemoveAxis(mFreqsAxis);
        CreateFreqsAxis();
        
#if !FIX_ABLETON_AU_FREEZE
        // Force refresh, for the freq axis to be updated
        mGraph->SetDirty(true);
#else
        // Don't push param to plug because Reset() is called from the audio thread
        //mGraph->SetDirty(false);
        mGraph->SetDataChanged();
#endif
    }
}

void
WavesRender::AddMagns(const WDL_TypedBuf<BL_FLOAT> &magns)
{
    vector<LinesRender2::Point> points;
    MagnsToPoints(&points, magns);
    
    mLinesRender->AddSlice(points);
    
    mAddNum++;
    
#if !FIX_ABLETON_AU_FREEZE
    // Without that, the volume rendering is not displayed
    mGraph->SetDirty(true);
#else
    // Don't push param to plug because AddMagns() is called from the audio thread
    //mGraph->SetDirty(false);
    mGraph->SetDataChanged();
#endif
}

void
WavesRender::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    mMouseIsDown = true;
    
    mPrevDrag[0] = x;
    mPrevDrag[1] = y;
    
    mPrevMouseDrag = false;
}

void
WavesRender::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    if (!mMouseIsDown)
        return;
    
    mMouseIsDown = false;
}

void
WavesRender::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    if (pMod->A)
        // Alt-drag => zoom
    {
        if (!mPrevMouseDrag)
        {
            mPrevMouseDrag = true;
            
            mPrevDrag[0] = y;
            
            return;
        }
        
#define DRAG_WHEEL_COEFF 0.2
        
        BL_FLOAT dY = y - mPrevDrag[0];
        mPrevDrag[0] = y;
        
        dY *= -1.0;
        dY *= DRAG_WHEEL_COEFF;
        
        OnMouseWheel(mPrevDrag[0], mPrevDrag[1], pMod, dY);
        
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
    
    mLinesRender->SetCameraAngles(mCamAngle0, mCamAngle1);
    
    // Camera changed
    //
    // Without that, the camera point of view is not modified if
    // the sound is not playing
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
    
    mPlug->SetCameraAngles(mCamAngle0, mCamAngle1);
}

bool
WavesRender::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
    // Reset the view
    mCamAngle0 = 0.0;
    mCamAngle1 = MIN_CAM_ANGLE_1;
    
    mLinesRender->SetCameraAngles(mCamAngle0, mCamAngle1);

    // Reset the fov
    mLinesRender->ResetZoom();
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
    
    mPlug->SetCameraAngles(mCamAngle0, mCamAngle1);
    
    BL_FLOAT fov = mLinesRender->GetCameraFov();
    mPlug->SetCameraFov(fov);
    
    return true;
}

void
WavesRender::OnMouseWheel(int x, int y, IMouseMod* pMod, BL_FLOAT d)
{
#define WHEEL_ZOOM_STEP 0.025
    
    BL_FLOAT zoomChange = 1.0 + d*WHEEL_ZOOM_STEP;
    
    mLinesRender->ZoomChanged(zoomChange);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
    
    BL_FLOAT angle = mLinesRender->GetCameraFov();
    mPlug->SetCameraFov(angle);
}

bool
WavesRender::OnKeyDown(int x, int y, int key, IMouseMod* pMod)
{
// #bl-iplug2
#if 0
    
    char c = key - KEY_ALPHA_A + 'a';
    
    int keyLen = strlen(_ORXXKey);
    if ((mORXXKeyGuessStep < keyLen) &&
        (_ORXXKey[mORXXKeyGuessStep] == c))
    {
        mORXXKeyGuessStep++;
    }
    else
    {
        mORXXKeyGuessStep = 0;
    }
    
    if (mORXXKeyGuessStep >= keyLen)
    {
        // We have the key, open ORXX mode !
        mORXXMode = !mORXXMode;
        
        mORXXKeyGuessStep = 0;
    }
#endif
    
    return false;
}

void
WavesRender::SetMode(LinesRender2::Mode mode)
{
    mLinesRender->SetMode(mode);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
WavesRender::SetSpeed(BL_FLOAT speed)
{
    mLinesRender->SetSpeed(speed);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
WavesRender::SetDensity(BL_FLOAT density)
{
    mLinesRender->SetDensity(density);

    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
WavesRender::SetScale(BL_FLOAT scale)
{
    mLinesRender->SetScale(scale);
    
    if (mAmpsAxis != NULL)
    {
        mAmpsAxis->SetScale(scale);
    }
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
WavesRender::SetScrollDirection(LinesRender2::ScrollDirection dir)
{
    mLinesRender->SetScrollDirection(dir);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
WavesRender::SetShowAxis(bool flag)
{
    mLinesRender->SetShowAxis(flag);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
WavesRender::SetDBScale(bool flag, BL_FLOAT minDB)
{
    mLinesRender->SetDBScale(flag, minDB);
 
    UpdateAmpsAxis(flag);
    
    mAmpsAxis->SetDBScale(flag, minDB);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
WavesRender::SetCamAngle0(BL_FLOAT angle)
{
    mCamAngle0 = angle;
    
    mLinesRender->SetCameraAngles(mCamAngle0, mCamAngle1);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
WavesRender::SetCamAngle1(BL_FLOAT angle)
{
    mCamAngle1 = angle;
    
    mLinesRender->SetCameraAngles(mCamAngle0, mCamAngle1);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
WavesRender::SetCamFov(BL_FLOAT angle)
{
    mLinesRender->SetCameraFov(angle);
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
WavesRender::SetDirty(bool pushParamToPlug)
{
    //mGraph->SetDirty(pushParamToPlug);
    mGraph->SetDataChanged();
    
    // Force recomputing projection
    mLinesRender->SetDirty();
}

void
WavesRender::SetColors(unsigned char color0[4], unsigned char color1[4])
{
    mLinesRender->SetColors(color0, color1);
    
    //mGraph->SetDirty(false);
    mGraph->SetDataChanged();
    
    // Force recomputing projection
    mLinesRender->SetDirty();
}

void
WavesRender::MagnsToPoints(vector<LinesRender2::Point> *points,
                           const WDL_TypedBuf<BL_FLOAT> &magns)
{
    if (magns.GetSize() == 0)
        return;
    
    // Convert to Mel
    BL_FLOAT hzPerBin = mSampleRate/magns.GetSize();

    hzPerBin *= MEL_COEFF;
    
    WDL_TypedBuf<BL_FLOAT> magnsMel;
    BLUtils::FreqsToMelNorm(&magnsMel, magns, hzPerBin);
    
    // NOTE: Changes the display spread, but may not be so cool
#if 0 // Convert to dB
#define EPS 1e-15
#define MIN_DB -45.0
    WDL_TypedBuf<BL_FLOAT> magnsDB;
    BLUtils::AmpToDBNorm(&magnsDB, magnsMel, EPS, MIN_DB);
#else
    WDL_TypedBuf<BL_FLOAT> magnsDB = magnsMel;
#endif
    
#if ENABLE_ORXX_MODE
    if (mORXXMode)
        TransformORXX(&magnsDB);
#endif
    
    if (magnsDB.GetSize() == 0)
        return;
    
    // Convert to points
    points->resize(magnsDB.GetSize());
    for (int i = 0; i < magnsDB.GetSize(); i++)
    {
        BL_FLOAT magn = magnsDB.Get()[i];
        
        LinesRender2::Point &p = (*points)[i];
        
        p.mX = 0.0;
        if (magnsDB.GetSize() > 1)
            p.mX = ((BL_FLOAT)i)/(magnsDB.GetSize() - 1) - 0.5;
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
WavesRender::TransformORXX(WDL_TypedBuf<BL_FLOAT> *magns)
{
#define PERCENT_KEEP 0.4
#define ONDULATION_FREQ 0.5
#define ONDULATION_AMP 0.25
    
#define HIGH_SCALE 3.0
    
    // Take the first half
    WDL_TypedBuf<BL_FLOAT> firstHalf;
    firstHalf.Resize(magns->GetSize()*PERCENT_KEEP);
    for (int i = 0; i < firstHalf.GetSize(); i++)
    {
        BL_FLOAT val = magns->Get()[i];
    
        firstHalf.Get()[i] = val;
    }
    
    // Resize it to 50%
    BLUtils::ResizeLinear2(&firstHalf, magns->GetSize()/2);
    
    // Mirror it
    WDL_TypedBuf<BL_FLOAT> secondHalf = firstHalf;
    BLUtils::Reverse(&secondHalf);
    
    // Compute the step and left/right ratio
    BL_FLOAT t = (mAddNum*magns->GetSize()/mSampleRate);
    BL_FLOAT step = std::sin(t*ONDULATION_FREQ);
    
    BL_FLOAT ratio = 0.5 + step*ONDULATION_AMP;
    
    BLUtils::ResizeLinear2(&firstHalf, ratio*magns->GetSize());
    BLUtils::ResizeLinear2(&secondHalf, (1.0 - ratio)*magns->GetSize());
    
    // Collapse the result
    BLUtils::FillAllZero(magns);
    
    for (int i = 0; i < magns->GetSize(); i++)
    {
        BL_FLOAT val = 0.0;
        
        if (i < firstHalf.GetSize() - 1)
        {
            val = firstHalf.Get()[i];
        }
        else
        {
            if ((i - firstHalf.GetSize() > 0) &&
                (i - firstHalf.GetSize() < secondHalf.GetSize() - 1))
            {
                val = secondHalf.Get()[i - firstHalf.GetSize()];
            }
        }
        
        magns->Get()[i] = val;
    }
    
    // Increase the hight
    BLUtils::MultValues(magns, (BL_FLOAT)HIGH_SCALE);
}

BL_FLOAT
WavesRender::FreqToMelNorm(BL_FLOAT freq)
{
    // Convert to Mel
    BL_FLOAT hzPerBin = mSampleRate/(mBufferSize/2.0);
    hzPerBin *= MEL_COEFF;
    
    // Hack: something is not really correct here...
    freq *= 2.0;
    
    BL_FLOAT result = BLUtils::FreqToMelNorm((BL_FLOAT)(freq*MEL_COEFF), hzPerBin, mBufferSize);
    
    return result;
}

void
WavesRender::CreateFreqsAxis()
{
    if (mFreqsAxis != NULL)
        delete mFreqsAxis;
    mFreqsAxis = NULL;
    
    // Create axis
#define NUM_AXIS_DATA 9 //7 //8
    char *labels[NUM_AXIS_DATA] =
    {
        /*"1Hz",*/ "", "100Hz", "500Hz", "1KHz", "2KHz", "5KHz", "10KHz", "20KHz", ""
    };
    
    // Scale the axis normalized values
    BL_FLOAT freqs[NUM_AXIS_DATA] =
    {
        1.0, 100.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0, 40000.0
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
    mFreqsAxis->SetPointProjector(mLinesRender);
    
    mLinesRender->AddAxis(mFreqsAxis);
}

void
WavesRender::UpdateAmpsAxis(bool dBScale)
{
    // Create axis
    
    // For Db scale
    //
#define NUM_AXIS_DATA_DBSCALE 7
    char *labelsDBScale[NUM_AXIS_DATA_DBSCALE] =
    {
        ""/*"-60dB"*/, "-50dB", "-40dB", "-30dB", "-20dB", "-10dB", "0dB"
    };
    
    // Scale the axis normalized values
    BL_FLOAT ampsDBDBScale[NUM_AXIS_DATA_DBSCALE] =
    {
        -60.0, -50.0, -40.0, -30.0, -20.0, -10.0, 0.0
    };
    
    BL_FLOAT normPosDBScale[NUM_AXIS_DATA_DBSCALE];
    for (int i = 0; i < NUM_AXIS_DATA_DBSCALE; i++)
    {
        BL_FLOAT ampDB = ampsDBDBScale[i];
        BL_FLOAT amp = DBToAmp(ampDB);
        
        //amp = BLUtils::AmpToDBNorm(amp, 1e-15, -60.0); // TEST
        
        normPosDBScale[i] = amp;
    }
    
    // For linear scale
    //
#define NUM_AXIS_DATA 5
    char *labels[NUM_AXIS_DATA] =
    {
        "-20dB", "-15dB", "-10dB", "-5dB", "0dB"
    };
    
    // Scale the axis normalized values
    BL_FLOAT ampsDB[NUM_AXIS_DATA] =
    {
        -20.0, -15.0, -10.0, -5.0, 0.0
    };

    BL_FLOAT normPos[NUM_AXIS_DATA];
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        BL_FLOAT ampDB = ampsDB[i];
        BL_FLOAT amp = DBToAmp(ampDB);
        
        normPos[i] = amp;
    }
    
    
    // 3D extremities of the axis
    BL_FLOAT p0[3] = { -0.5, 0.0, 0.5 + AXIS_OFFSET_Z };
    BL_FLOAT p1[3] = { -0.5, 0.5, 0.5 + AXIS_OFFSET_Z };
    
    // Create the axis
    if (mAmpsAxis == NULL)
    {
        if (!dBScale)
            mAmpsAxis = new Axis3D(labels, normPos, NUM_AXIS_DATA, p0, p1);
        else
            mAmpsAxis = new Axis3D(labelsDBScale, normPosDBScale, NUM_AXIS_DATA_DBSCALE, p0, p1);
        
        mAmpsAxis->SetDoOverlay(true);
        mAmpsAxis->SetPointProjector(mLinesRender);
    
        mLinesRender->AddAxis(mAmpsAxis);
    }
    else
    {
        if (!dBScale)
            mAmpsAxis->UpdateLabels(labels, normPos, NUM_AXIS_DATA, p0, p1);
        else
            mAmpsAxis->UpdateLabels(labelsDBScale, normPosDBScale, NUM_AXIS_DATA_DBSCALE, p0, p1);
    }
}
