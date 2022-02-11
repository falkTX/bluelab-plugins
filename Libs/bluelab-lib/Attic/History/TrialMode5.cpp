//
//  TrialMode5.cpp
//  SpectralDiff
//
//  Created by Pan on 05/03/18.
//
//

#include "TrialMode5.h"

#include "GUIHelper8.h"

IColor TrialMode5::mTextColor = IColor(255,
                                      TRIAL_TEXT_COLOR_R,
                                      TRIAL_TEXT_COLOR_G,
                                      TRIAL_TEXT_COLOR_B);

void
TrialMode5::SetTrialMessage(IPlug *plug, IGraphics *graphics, GUIHelper8 *helper)
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
