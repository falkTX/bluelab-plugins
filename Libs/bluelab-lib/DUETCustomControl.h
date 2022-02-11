//
//  DUETCustomControl.h
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

#ifndef __BL_Panogram__DUETCustomControl__
#define __BL_Panogram__DUETCustomControl__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>
#include <DUETPlugInterface.h>

using namespace iplug;

class SpectrogramDisplayScroll;

class DUETCustomControl : public GraphCustomControl
{
public:
    DUETCustomControl(DUETPlugInterface *plug);
    
    virtual ~DUETCustomControl() {}
    
    void Reset();
    
    void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod &mod) override;
    
    bool OnKeyDown(float x, float y, const IKeyPress& key) override;
    
protected:
    DUETPlugInterface *mPlug;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Panogram__DUETCustomControl__) */
