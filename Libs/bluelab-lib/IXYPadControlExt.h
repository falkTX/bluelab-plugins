#ifndef IXY_PAD_CONTROL_EXT_H
#define IXY_PAD_CONTROL_EXT_H

#include <vector>
using namespace std;

#include <IControl.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class IXYPadControlExtListener
{
public:
    virtual void OnHandleChanged(int handleNum)  = 0;
};

// Manage multiple handles
class IXYPadControlExt : public IControl
{
 public:
    IXYPadControlExt(Plugin *plug,
                     const IRECT& bounds,
                     const std::initializer_list<int>& params,
                     const IBitmap& trackBitmap,
                     float borderSize = 0.0,
                     bool reverseY = false);

    virtual ~IXYPadControlExt();

    void SetListener(IXYPadControlExtListener *listener);
    
    void AddHandle(IGraphics *pGraphics, const char *handleBitmapFname,
                   const std::initializer_list<int>& params);
    int GetNumHandles();

    void SetHandleEnabled(int handleNum, bool flag);
    bool IsHandleEnabled(int handleNum);
    
    void Draw(IGraphics& g) override;

    void OnMouseDown(float x, float y, const IMouseMod& mod) override;
    void OnMouseUp(float x, float y, const IMouseMod& mod) override;
    void OnMouseDrag(float x, float y, float dX, float dY,
                     const IMouseMod& mod) override;
  
 protected:
    void DrawTrack(IGraphics& g);
    void DrawHandles(IGraphics& g);

    // Ensure that the handle doesn't go out of the track at all 
    void PixelsToParams(float *x, float *y);
    void ParamsToPixels(float *x, float *y);

    // Return the handle number, or -1 if no handle
    int MouseOnHandle(float x, float y,
                      float *offsetX, float *offsetY);
        
    //
    IBitmap mTrackBitmap;
    IBitmap mHandleBitmap;

    // Border size, or "stroke width"
    float mBorderSize;

    bool mReverseY;
    
    bool mMouseDown;

    //
    Plugin *mPlug;

    //
    struct Handle
    {
        IBitmap mBitmap;
        int mParamIdx[2];

        float mOffsetX;
        float mOffsetY;
        
        float mPrevX;
        float mPrevY;

        bool mIsGrabbed;

        bool mIsEnabled;

        enum DrawState
        {
            NORMAL = 1,
            HIGHLIGHT,
            GRAY_OUT
        };

        DrawState mDrawState;
    };

    vector<Handle> mHandles;

    //
    IXYPadControlExtListener *mListener;
};

#endif
