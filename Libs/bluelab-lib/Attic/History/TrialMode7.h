//
//  TrialMode7.h
//  SpectralDiff
//
//  Created by Pan on 05/03/18.
//
//

#ifndef SpectralDiff_TrialMode7_h
#define SpectralDiff_TrialMode7_h

#include "IPlug_include_in_plug_hdr.h"

#if DEMO_VERSION
#define TRIAL_MODE 1
#endif

#ifndef TRIAL_MODE
#define TRIAL_MODE 0 // 1
#endif

#define TRIAL_MESSAGE "[DEMO] Please consider buying if you like it !"

#define TRIAL_TEXT_COLOR_R 200
#define TRIAL_TEXT_COLOR_G 0
#define TRIAL_TEXT_COLOR_B 0

#define TRIAL_TEXT_SIZE 10

#define TRIAL_TEXT_X_OFFSET 2
#define TRIAL_TEXT_Y_OFFSET 2


using namespace iplug::igraphics;

class GUIHelper11;
class TrialMode7
{
public:
    static void SetTrialMessage(IGraphics *graphics, GUIHelper11 *helper);
    
private:
    static IColor mTextColor;
};

#endif
