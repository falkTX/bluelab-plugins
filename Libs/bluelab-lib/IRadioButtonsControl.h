//
//  IRadioButtonControls.h
//  BL-GhostViewer-macOS
//
//  Created by applematuer on 10/29/20.
//
//

#ifndef IRadioButtonControls_h
#define IRadioButtonControls_h

#include <stdlib.h>

#include <vector>
using namespace std;

#include <IControl.h>
#include <IPlugConstants.h>

using namespace iplug;
using namespace iplug::igraphics;

class IRadioButtonsControl : public IControl
{
public:
    IRadioButtonsControl(IRECT pR, int paramIdx,
                         int nButtons, const IBitmap &bitmap,
                         EDirection direction = EDirection::Vertical,
                         bool reverse = false);
    virtual ~IRadioButtonsControl() {}
    
    virtual void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    
    virtual void Draw(IGraphics &g) override;
    
    virtual void GetRects(vector<IRECT> *rects)
    {
        *rects = mRECTs;
    }
    
protected:
    vector<IRECT> mRECTs;
    IBitmap mBitmap;
};

#endif /* IRadioButtonControls_h */
