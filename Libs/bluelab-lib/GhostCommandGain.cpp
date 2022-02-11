#include <BLUtils.h>

#include "GhostCommandGain.h"

GhostCommandGain::GhostCommandGain(BL_FLOAT sampleRate,
                                   BL_FLOAT factor)
: GhostCommand(sampleRate)
{
    mFactor = factor;
}

GhostCommandGain::~GhostCommandGain() {}

void
GhostCommandGain::Apply(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                        vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    // Do not use phases
    
    WDL_TypedBuf<BL_FLOAT> selectedMagns;
    
    // Get the selected data, just for convenience
    GetSelectedDataY(*magns, &selectedMagns);
    
    // For the moment, do not use fade, just fill all with zeros
    BLUtils::MultValues(&selectedMagns, mFactor);
    
    // And replace in the result
    ReplaceSelectedDataY(magns, selectedMagns);
}
