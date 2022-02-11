//
//  Watermark.cpp
//  SpectralDiff
//
//  Created by Pan on 05/03/18.
//
//

#include "Watermark.h"

#include "GUIHelper11.h"

IColor Watermark::mTextColor = IColor(255,
                                      WATERMARK_TEXT_COLOR_R,
                                      WATERMARK_TEXT_COLOR_G,
                                      WATERMARK_TEXT_COLOR_B);

void
Watermark::SetWatermarkMessage(Plugin *plug, IGraphics *graphics,
                               GUIHelper11 *helper,
                               const char *watermarkMessage)
{
#if WATERMARK_MODE
    int x = WATERMARK_TEXT_X_OFFSET;
    int y = graphics->Height() - WATERMARK_TEXT_SIZE - WATERMARK_TEXT_Y_OFFSET;
    
    ITextControl *textControl = helper->CreateText(graphics,
                                                   x, y,
                                                   watermarkMessage,
                                                   WATERMARK_TEXT_SIZE,
                                                   mTextColor,
                                                   EAlign::Near);

    // Avoids that the trial message masks the interaction on the help button for example
    textControl->SetInteractionDisabled(true);
#endif
}
