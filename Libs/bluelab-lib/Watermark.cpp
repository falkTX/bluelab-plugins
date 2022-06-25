/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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
                                                   WATERMARK_TEXT_FONT,
                                                   mTextColor,
                                                   EAlign::Near);

    // Avoids that the trial message masks the interaction on the help button for example
    textControl->SetInteractionDisabled(true);
#endif
}
