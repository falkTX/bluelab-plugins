//
//  DUETCustomControl.h
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

#ifndef __BL_Panogram__DUETCustomControl__
#define __BL_Panogram__DUETCustomControl__

#include <GraphControl11.h>
#include <DUETPlugInterface.h>

using namespace iplug;

class SpectrogramDisplayScroll;

class DUETCustomControl : public GraphCustomControl
{
public:
    DUETCustomControl(DUETPlugInterface *plug);
    
    virtual ~DUETCustomControl() {}
    
    void Reset();
    
    void OnMouseDown(int x, int y, IMouseMod* pMod);
    void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);
    
    bool OnKeyDown(int x, int y, int key, IMouseMod* pMod);
    
protected:
    DUETPlugInterface *mPlug;
};

#endif /* defined(__BL_Panogram__DUETCustomControl__) */
