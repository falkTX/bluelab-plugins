//
//  IHelpButtonControl2.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/5/20.
//
//

#ifndef IHelpButtonControl2_h
#define IHelpButtonControl2_h

#include <IRolloverButtonControl.h>

using namespace iplug::igraphics;

class IHelpButtonControl2 : public IRolloverButtonControl
{
public:
    IHelpButtonControl2(float x, float y, const IBitmap &bitmap, int paramIdx,
                       const char *fileName,
                       EBlend blend = EBlend::Default)
    : IRolloverButtonControl(x, y, bitmap, paramIdx, false, blend)
    {
        sprintf(mFileName, "%s", fileName);
    }
    
    virtual ~IHelpButtonControl2() {}
    
    void OnMouseDown(float x, float y, const IMouseMod &mod) override;

    static void ShowManual(const char *fileName);
    
protected:
    char mFileName[1024];
};


#endif /* IHelpButtonControl2_h */
