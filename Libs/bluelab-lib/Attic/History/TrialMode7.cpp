//
//  TrialMode7.cpp
//  SpectralDiff
//
//  Created by Pan on 05/03/18.
//
//

#include <GUIHelper11.h>

#include "TrialMode7.h"

IColor TrialMode7::mTextColor = IColor(255,
                                       TRIAL_TEXT_COLOR_R,
                                       TRIAL_TEXT_COLOR_G,
                                       TRIAL_TEXT_COLOR_B);

void
TrialMode7::SetTrialMessage(IGraphics *graphics, GUIHelper11 *helper)
{
#if TRIAL_MODE
    int x = TRIAL_TEXT_X_OFFSET;
    int y = graphics->Height() - TRIAL_TEXT_SIZE - TRIAL_TEXT_Y_OFFSET;
    
    ITextControl *textControl = helper->CreateText(graphics,
                                                   x, y,
                                                   TRIAL_MESSAGE,
                                                   TRIAL_TEXT_SIZE,
                                                   mTextColor, EAlign::Near);

    // Avoids that the trial message masks the interaction on the help button for example
    textControl->SetInteractionDisabled(true);
#endif
}
