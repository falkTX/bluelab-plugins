//
//  GUIHelpers.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/06/17.
//
//

#ifndef __Transient__GUIHelpers__
#define __Transient__GUIHelpers__

#include "IPlug_include_in_plug_hdr.h"

#include <VuMeterControl.h>

#define TEXT_SIZE 12
#define TEXT_OFFSET 3

#define TEXT_COLOR_R 182
#define TEXT_COLOR_G 255
#define TEXT_COLOR_B 85


#define TEXT_FONT "Tahoma"

class GUIHelpers
{
public:
    static ITextControl *CreateText(IPlug *plug, IGraphics *graphics, int width, int height, int x, int y,
                                    const char *label);
    
    static IKnobMultiControl *CreateKnobWithText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                                                int x, int y, int param, const char *label);
    
    static void UpdateText(IPlug *plug, int paramIdx);
    
    static IToggleButtonControl *CreateToggleButtonWithText(IPlug *plug,
                                                            IGraphics *graphics, IBitmap *bitmap,
                                                            int x, int y, int param, const char *label);
    
    static VuMeterControl *CreateVuMeterWithText(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                                                 int x, int y, int param, const char *label,
                                                 bool alignLeft);
    
    static int GetTextOffset();
    
private:
    static IColor mTextColor;
};

#endif /* defined(__Transient__GUIHelpers__) */
