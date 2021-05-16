//
//  IRolloverButtonControl.hpp
//  BL-GhostViewer-macOS
//
//  Created by applematuer on 10/29/20.
//
//

#ifndef IRolloverButtonControl_h
#define IRolloverButtonControl_h

#include <stdlib.h>

#include <IControl.h>
#include <IPlugConstants.h>

using namespace iplug;
using namespace iplug::igraphics;

// WARNING: there was a modification in IPlugAAX for meta parameters
// (i.e the onces associated with RolloverButtons)
class IRolloverButtonControl : public IBitmapControl
{
public:
    IRolloverButtonControl(float x, float y,
                           const IBitmap &bitmap,
                           int paramIdx,
                           bool toggleFlag = true,
                           EBlend blend = EBlend::Default)
    : IBitmapControl(x, y, bitmap, paramIdx, blend),
      mToggleFlag(toggleFlag),
      mText(NULL) {}
    
    IRolloverButtonControl(float x, float y,
                           const IBitmap &bitmap,
                           bool toggleFlag = true,
                           EBlend blend = EBlend::Default)
    : IBitmapControl(x, y, bitmap, kNoParameter, blend),
      mToggleFlag(toggleFlag),
      mText(NULL),
      mPrevMouseOut(false) {}
    
    virtual ~IRolloverButtonControl() {}
    
    virtual void Draw(IGraphics& g) override;
    
    virtual void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    
    virtual void OnMouseUp(float x, float y, const IMouseMod& mod) override;
    
    virtual void OnMouseOver(float x, float y, const IMouseMod &mod) override;
    
    virtual void OnMouseOut() override;
    
    virtual void SetDisabled(bool disable) override;

    // Reference fo the associated text control, for hilighting
    void LinkText(ITextControl *textControl, const IColor &color,
                  const IColor &hilightColor);
    
protected:
    bool mToggleFlag;
    
    ITextControl *mText;
    IColor mTextColor;
    IColor mTextHilightColor;

    bool mPrevMouseOut;
};

#endif /* IRolloverButtonControl_h */
