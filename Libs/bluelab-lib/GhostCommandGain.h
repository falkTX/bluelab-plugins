#ifndef GHOST_COMMAND_GAIN_H
#define GHOST_COMMAND_GAIN_H

#include <GhostCommand.h>

#include "IPlug_include_in_plug_hdr.h"

class GhostCommandGain : public GhostCommand
{
public:
    GhostCommandGain(BL_FLOAT sampleRate, BL_FLOAT factor);
    
    virtual ~GhostCommandGain();
    
    void Apply(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
               vector<WDL_TypedBuf<BL_FLOAT> > *phases) override;
    
protected:
    BL_FLOAT mFactor;
};

#endif
