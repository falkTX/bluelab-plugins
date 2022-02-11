//
//  StereoVizVolRender3.h
//  BL-Waves
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_Waves__WavesRender__
#define __BL_Waves__WavesRender__

#include <vector>
#include <deque>
using namespace std;

#include <GraphControl11.h>

//#include <LinesRender.h>
#include <LinesRender2.h>

#include <WavesPluginInterface.h>

// Sides
//#define MAX_ANGLE_0 70.0

// Set to 90 for debugging
#define MAX_CAM_ANGLE_0 90.0

// Above
#define MAX_CAM_ANGLE_1 90.0 //70.0

// Below
//#define MIN_ANGLE_1 -20.0

// Almost horizontal (a little above)
#define MIN_CAM_ANGLE_1 15.0


//class Wav3s;
class Axis3D;

class WavesRender : public GraphCustomControl
{
public:
    WavesRender(WavesPluginInterface *plug, GraphControl11 *graphControl,
                int bufferSize, BL_FLOAT sampleRate);
    
    virtual ~WavesRender();
    
    void Reset(BL_FLOAT sampleRate);
    
    virtual void AddMagns(const WDL_TypedBuf<BL_FLOAT> &magns);
    
    // Control
    virtual void OnMouseDown(int x, int y, IMouseMod* pMod);
    virtual void OnMouseUp(int x, int y, IMouseMod* pMod);
    virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);
    virtual bool OnMouseDblClick(int x, int y, IMouseMod* pMod);
    virtual void OnMouseWheel(int x, int y, IMouseMod* pMod, BL_FLOAT d);
    
    virtual bool OnKeyDown(int x, int y, int key, IMouseMod* pMod);
    
    // Parameters
    virtual void SetMode(LinesRender2::Mode mode);
    virtual void SetSpeed(BL_FLOAT speed);
    virtual void SetDensity(BL_FLOAT density);
    virtual void SetScale(BL_FLOAT scale);
    virtual void SetScrollDirection(LinesRender2::ScrollDirection dir);
    virtual void SetShowAxis(bool flag);
    virtual void SetDBScale(bool flag, BL_FLOAT minDB);

    
    // for parameter sent from plug (save state and automation)
    void SetCamAngle0(BL_FLOAT angle);
    void SetCamAngle1(BL_FLOAT angle);
    void SetCamFov(BL_FLOAT angle);
    
    // Force refresh
    void SetDirty(bool pushParamToPlug);
    
    void SetColors(unsigned char color0[4], unsigned char color1[4]);
    
protected:
    void MagnsToPoints(vector<LinesRender2::Point> *points,
                       const WDL_TypedBuf<BL_FLOAT> &magns);

    BL_FLOAT FreqToMelNorm(BL_FLOAT normFreq);
    
    void TransformORXX(WDL_TypedBuf<BL_FLOAT> *magns);

    // Axis
    void CreateFreqsAxis();
    void UpdateAmpsAxis(bool dbScale);
    
    //
    WavesPluginInterface *mPlug;
    
    GraphControl11 *mGraph;
    LinesRender2 *mLinesRender;
    
    Axis3D *mFreqsAxis;
    
    Axis3D *mAmpsAxis;
    
    // Selection
    bool mMouseIsDown;
    int mPrevDrag[2];
    
    // Used to detect pure mouse up, without drag
    bool mPrevMouseDrag;
    
    // Rotation
    BL_FLOAT mCamAngle0;
    BL_FLOAT mCamAngle1;
    
    BL_FLOAT mScale;
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    
    //
    unsigned long long int mAddNum;
    
    // Easter Egg
    bool mORXXMode;
    int mORXXKeyGuessStep;
};

#endif /* defined(__BL_Waves__WavesRender__) */
