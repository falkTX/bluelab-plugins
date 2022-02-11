#ifndef GHOST_COMMAND_REPLACE_H
#define GHOST_COMMAND_REPLACE_H

#include <vector>
using namespace std;

#include <GhostCommand.h>

#include "IPlug_include_in_plug_hdr.h"

class GhostCommandReplace : public GhostCommand
{
public:
    GhostCommandReplace(BL_FLOAT sampleRate,
                        bool processHorizontal, bool processVertical);
    
    virtual ~GhostCommandReplace();
    
    void Apply(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
               vector<WDL_TypedBuf<BL_FLOAT> > *phases) override;
    
protected:
    bool mProcessHorizontal;
    bool mProcessVertical;
};


#endif 
