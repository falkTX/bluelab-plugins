//
//  IHelpButtonControl.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/5/20.
//
//

#ifndef IHelpButtonControl_h
#define IHelpButtonControl_h

#include <IControl.h>

using namespace iplug::igraphics;

class IHelpButtonControl : public IBitmapControl
{
public:
    IHelpButtonControl(float x, float y, const IBitmap &bitmap, int paramIdx,
                       const char *fileName,
                       EBlend blend = EBlend::Default)
    : IBitmapControl(x, y, bitmap, paramIdx, blend)
    {
        sprintf(mFileName, "%s", fileName);
    }
    
    virtual ~IHelpButtonControl() {}
    
    void OnMouseDown(float x, float y, const IMouseMod &mod) override;

    static void ShowManual(const char *fileName);
    
protected:
    char mFileName[1024];
};


#endif /* IHelpButtonControl_h */
