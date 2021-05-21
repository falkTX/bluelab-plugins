#ifndef IXY_PAD_CONTROL_H
#define IXY_PAD_CONTROL_H

#include <IControl.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class IXYPadControl : public IControl
{
 public:
    IXYPadControl(const IRECT& bounds,
                  const std::initializer_list<int>& params,
                  const IBitmap& trackBitmap,
                  const IBitmap& handleBitmap,
                  float borderSize = 0.0,
                  bool reverseY = false);

    virtual ~IXYPadControl();
    
    void Draw(IGraphics& g) override;

    void OnMouseDown(float x, float y, const IMouseMod& mod) override;
    void OnMouseUp(float x, float y, const IMouseMod& mod) override;
    void OnMouseDrag(float x, float y, float dX, float dY,
                     const IMouseMod& mod) override;
  
 protected:
    void DrawTrack(IGraphics& g);
    void DrawHandle(IGraphics& g);

    // Ensure that the handle doesn't go out of the track at all 
    void PixelsToParams(float *x, float *y);
    void ParamsToPixels(float *x, float *y);

    bool MouseOnHandle(float x, float y,
                       float *offsetX, float *offsetY);
        
    //
    IBitmap mTrackBitmap;
    IBitmap mHandleBitmap;

    // Border size, or "stroke width"
    float mBorderSize;

    bool mReverseY;
    
    bool mMouseDown;

    float mOffsetX;
    float mOffsetY;

    float mPrevX;
    float mPrevY;
};

#endif
