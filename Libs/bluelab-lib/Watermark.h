//
//  Watermark.h
//  SpectralDiff
//
//  Created by Pan on 05/03/18.
//
//

#ifndef SpectralDiff_Watermark_h
#define SpectralDiff_Watermark_h

#include "IPlug_include_in_plug_hdr.h"

#define WATERMARK_MODE 1

#if 1 // Clear blue
#define WATERMARK_TEXT_COLOR_R 0
#define WATERMARK_TEXT_COLOR_G 128
#define WATERMARK_TEXT_COLOR_B 255
#endif

#if 0 // Red
#define WATERMARK_TEXT_COLOR_R 255
#define WATERMARK_TEXT_COLOR_G 0
#define WATERMARK_TEXT_COLOR_B 0
#endif


#define WATERMARK_TEXT_SIZE 10
#define WATERMARK_TEXT_FONT "Tahoma"

#define WATERMARK_TEXT_X_OFFSET 2
#define WATERMARK_TEXT_Y_OFFSET 2

using namespace iplug;
using namespace iplug::igraphics;

class GUIHelper11;

class Watermark
{
public:
    static void SetWatermarkMessage(Plugin *plug, IGraphics *graphics, GUIHelper11 *helper,
                                    const char *message);
    
private:
    static IColor mTextColor;
};

#endif
