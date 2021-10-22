#ifndef IICON_LABEL_CONTROL_H
#define IICON_LABEL_CONTROL_H

#include <IControl.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class IIconLabelControl : public IBitmapControl
{
public:
    IIconLabelControl(float x, float y,
                      float iconOffsetX, float iconOffsetY,
                      float textOffsetX, float textOffsetY,
                      const IBitmap &bgBitmap,
                      const IBitmap &iconBitmap,
                      const IText &text = DEFAULT_TEXT,
                      int paramIdx = kNoParameter,
                      EBlend blend = EBlend::Default);

    virtual ~IIconLabelControl();
    
    virtual void Draw(IGraphics& g) override;

    void SetLabelText(const char *text);
    void SetIconNum(int iconNum);
    
 protected:
    ITextControl *mTextControl;
    IBitmap mIconBitmap;

    float mIconOffsetX;
    float mIconOffsetY;

    float mTextOffsetX;
    float mTextOffsetY;
    
    int mIconNum;
};

#endif
