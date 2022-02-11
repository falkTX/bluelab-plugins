#ifndef GHOST_COMMAND_CUT_H
#define GHOST_COMMAND_CUT_H

#include <GhostCommand.h>

#include "IPlug_include_in_plug_hdr.h"

class GhostCommandCut : public GhostCommand
{
public:
    GhostCommandCut(BL_FLOAT sampleRate);
    
    virtual ~GhostCommandCut();
    
    void Apply(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
               vector<WDL_TypedBuf<BL_FLOAT> > *phases) override;
};

#endif
