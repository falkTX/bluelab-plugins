#ifndef ZOOM_CUSTOM_CONTROL_H
#define ZOOM_CUSTOM_CONTROL_H

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

class ZoomListener
{
public:
    virtual void UpdateZoom(BL_FLOAT zoomChange) = 0;
    virtual void ResetZoom() = 0;
};


class ZoomCustomControl : public GraphCustomControl
{
public:
    ZoomCustomControl(ZoomListener *listener);
    
    virtual ~ZoomCustomControl();
    
    virtual void OnMouseDown(float x, float y, const IMouseMod &pMod) override;
    virtual void OnMouseDrag(float x, float y, float dX, float dY,
                             const IMouseMod &pMod) override;
    
    virtual void OnMouseWheel(float x, float y, const IMouseMod &pMod,
                              float d) override;

protected:
    ZoomListener *mListener;

    int mStartDrag[2];
    
    BL_FLOAT mPrevMouseY;
};

#endif

#endif
