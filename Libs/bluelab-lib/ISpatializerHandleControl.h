#ifndef ISPATIALIZER_HANDLE_CONTROL_H
#define ISPATIALIZER_HANDLE_CONTROL_H

#include <IControl.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class ISpatializerHandleControl : public IControl
{
 public:
    ISpatializerHandleControl(const IRECT& bounds,
                              float minAngle, float maxAngle, bool reverseY,
                              const IBitmap& handleBitmap,
                              int paramIdx = kNoParameter);

    virtual ~ISpatializerHandleControl();
    
    void Draw(IGraphics& g) override;

    void OnMouseDown(float x, float y, const IMouseMod& mod);
    
    void OnMouseDrag(float x, float y, float dX, float dY,
                     const IMouseMod& mod) override;
        
 protected:
    void DrawHandle(IGraphics& g);

    void PixelsToParam(float *y);
    void ParamToPixels(float val, float *x, float *y);
    
    //
    IBitmap mHandleBitmap;

    float mMinAngle;
    float mMaxAngle;

    // For fine ma,nip with shift
    float mPrevX;
    float mPrevY;

    bool mReverseY;
};

#endif
