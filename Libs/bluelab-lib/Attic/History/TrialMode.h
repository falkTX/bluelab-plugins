//
//  TrialMode.h
//  SpectralDiff
//
//  Created by Pan on 05/03/18.
//
//

#ifndef SpectralDiff_TrialMode_h
#define SpectralDiff_TrialMode_h

#include "IPlug_include_in_plug_hdr.h"

#define TRIAL_MODE 0

#define TRIAL_MESSAGE "[TRIAL] Please consider buying if you like it !"

#define TRIAL_TEXT_COLOR_R 200
#define TRIAL_TEXT_COLOR_G 0
#define TRIAL_TEXT_COLOR_B 0

#define TRIAL_TEXT_SIZE 10
#define TRIAL_TEXT_FONT "Tahoma"

#define TRIAL_TEXT_X_OFFSET 2
#define TRIAL_TEXT_Y_OFFSET 2

class GUIHelper4;

class TrialMode
{
public:
    static void SetTrialMessage(IPlug *plug, IGraphics *graphics, GUIHelper4 *helper);
    
private:
    static IColor mTextColor;
};

#endif
