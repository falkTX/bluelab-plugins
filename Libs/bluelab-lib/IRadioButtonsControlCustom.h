//
//  IRadioButtonControls.h
//  BL-GhostViewer-macOS
//
//  Created by applematuer on 10/29/20.
//
//

#ifndef IRadioButtonControlsCustom_h
#define IRadioButtonControlsCustom_h

#include <stdlib.h>

#include <vector>
using namespace std;

#include <IControl.h>
#include <IPlugConstants.h>

using namespace iplug;
using namespace iplug::igraphics;

// Can take 1 differrent bitmap by button,
// and can make rollover
class IRadioButtonsControlCustom : public IControl
{
public:
    IRadioButtonsControlCustom(IRECT pR, int paramIdx,
                               int nButtons, const IBitmap bitmaps[],
                               EDirection direction = EDirection::Vertical,
                               bool reverse = false);
    virtual ~IRadioButtonsControlCustom() {}
    
    virtual void OnMouseDown(float x, float y, const IMouseMod &mod) override;

    // For rollover
    virtual void OnMouseOver(float x, float y, const IMouseMod &mod) override;
    virtual void OnMouseOut() override;
    
    virtual void Draw(IGraphics &g) override;
    
    virtual void GetRects(vector<IRECT> *rects)
    {
        *rects = mRECTs;
    }
    
protected:
    vector<IRECT> mRECTs;
    vector<IBitmap> mBitmaps;


    vector<bool> mMouseOver;
};

#endif /* IRadioButtonControlsCustom_h */
