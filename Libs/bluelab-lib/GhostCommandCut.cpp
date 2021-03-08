#include <BLUtils.h>

#include "GhostCommandCut.h"

GhostCommandCut::GhostCommandCut(BL_FLOAT sampleRate)
: GhostCommand(sampleRate) {}

GhostCommandCut::~GhostCommandCut() {}

void
GhostCommandCut::Apply(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                       vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    // Do not use phases
    
    WDL_TypedBuf<BL_FLOAT> selectedMagns;
    
    // Get the selected data, just for convenience
    GetSelectedDataY(*magns, &selectedMagns);
    
    // For the moment, do not use fade, just fill all with zeros
    BLUtils::FillAllZero(&selectedMagns);
    
    // And replace in the result
    ReplaceSelectedDataY(magns, selectedMagns);
}
