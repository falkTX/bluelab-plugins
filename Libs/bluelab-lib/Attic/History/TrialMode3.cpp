//
//  TrialMode.cpp
//  SpectralDiff
//
//  Created by Pan on 05/03/18.
//
//

#include "TrialMode3.h"

#include "GUIHelper6.h"

IColor TrialMode3::mTextColor = IColor(255,
                                      TRIAL_TEXT_COLOR_R,
                                      TRIAL_TEXT_COLOR_G,
                                      TRIAL_TEXT_COLOR_B);

void
TrialMode3::SetTrialMessage(IPlug *plug, IGraphics *graphics, GUIHelper6 *helper)
{
#if TRIAL_MODE
    int x = TRIAL_TEXT_X_OFFSET;
    int y = graphics->Height() - TRIAL_TEXT_SIZE - TRIAL_TEXT_Y_OFFSET;
    
    helper->CreateText(plug, graphics,
                       TRIAL_MESSAGE,
                       x, y,
                       TRIAL_TEXT_SIZE, TRIAL_TEXT_FONT,
                       IText::kStyleBold,
                       mTextColor,
                       IText::kAlignNear);

#endif
}
