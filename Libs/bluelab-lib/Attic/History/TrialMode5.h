//
//  TrialMode5.h
//  SpectralDiff
//
//  Created by Pan on 05/03/18.
//
//

#ifndef SpectralDiff_TrialMode5_h
#define SpectralDiff_TrialMode5_h

#include "IPlug_include_in_plug_hdr.h"

// TrialMode2: for GUIHelper5
// TrialMode3: for GUIHelper6
// TrialMode4: for GUIHelper7
// TrialMode5: for GUIHelper8

#define TRIAL_MODE 0

#define TRIAL_MESSAGE "[TRIAL] Please consider buying if you like it !"

#define TRIAL_TEXT_COLOR_R 200
#define TRIAL_TEXT_COLOR_G 0
#define TRIAL_TEXT_COLOR_B 0

#define TRIAL_TEXT_SIZE 10
#define TRIAL_TEXT_FONT "Tahoma"

#define TRIAL_TEXT_X_OFFSET 2
#define TRIAL_TEXT_Y_OFFSET 2

class GUIHelper8;

class TrialMode5
{
public:
    static void SetTrialMessage(IPlug *plug, IGraphics *graphics, GUIHelper8 *helper);
    
private:
    static IColor mTextColor;
};

#endif
