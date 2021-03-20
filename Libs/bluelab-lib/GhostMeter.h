#ifndef GHOST_METER_H
#define GHOST_METER_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class GUIHelper12;
class GhostMeter
{
public:
    enum Mode
    {
        GHOST_METER_SAMPLES,
        GHOST_METER_HMS
    };
    
    GhostMeter(BL_FLOAT x, BL_FLOAT y, BL_FLOAT sampleRate);
    virtual ~GhostMeter();

    void Reset(BL_FLOAT sampleRate);
    
    void GenerateUI(GUIHelper12 *guiHelper, IGraphics *graphics);
    void ClearUI();
    
    void SetCursorPosition(BL_FLOAT x, BL_FLOAT y);
    void ResetCursorPosition();
    
    void SetSelectionValues(BL_FLOAT x, BL_FLOAT y,
                            BL_FLOAT w, BL_FLOAT h);
    void ResetSelectionValues();
    
protected:
    void UpdateTextBGColor();

    void ConvertToHMS(BL_FLOAT timeSec,
                      int *h, int *m, int *s, int *ms);

    void TimeToStr(BL_FLOAT timeSec, char buf[256]);
    void HMSStr(BL_FLOAT timeSec, char buf[256]);
    void SamplesStr(BL_FLOAT timeSec, char buf[256]);
        
    BL_FLOAT AdjustFreq(BL_FLOAT freq);
    
    //
    Mode mMode;

    BL_FLOAT mSampleRate;
    
    BL_FLOAT mX;
    BL_FLOAT mY;
    
    ITextControl *mCursorPosTexts[2];
    ITextControl *mSelPosTexts[2];
    ITextControl *mSelSizeTexts[2];
};

#endif
