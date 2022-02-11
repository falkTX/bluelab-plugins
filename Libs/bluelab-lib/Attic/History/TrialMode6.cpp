//
//  TrialMode6.cpp
//  SpectralDiff
//
//  Created by Pan on 05/03/18.
//
//

#include <GUIHelper11.h>

#include "TrialMode6.h"

IColor TrialMode6::mTextColor = IColor(255,
                                      TRIAL_TEXT_COLOR_R,
                                      TRIAL_TEXT_COLOR_G,
                                      TRIAL_TEXT_COLOR_B);

void
TrialMode6::SetTrialMessage(Plugin *plug, IGraphics *graphics, GUIHelper9 *helper)
{
#if TRIAL_MODE
    int x = TRIAL_TEXT_X_OFFSET;
    int y = graphics->Height() - TRIAL_TEXT_SIZE - TRIAL_TEXT_Y_OFFSET;
    
    ITextControl *textControl = helper->CreateText(plug, graphics,
                                                   TRIAL_MESSAGE,
                                                   x, y,
                                                   TRIAL_TEXT_SIZE, TRIAL_TEXT_FONT,
                                                   IText::kStyleBold,
                                                   mTextColor,
                                                   IText::kAlignNear);

    // Avoids that the trial message masks the interaction on the help button for example
    textControl->SetInteractionDisabled(true);
#endif
}
