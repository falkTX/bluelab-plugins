#include <BLUtils.h>

#include <BLReverb.H>

#define DIRAC_VALUE 1.0


BLReverb::~BLReverb() {}

void
BLReverb::GetIRs(WDL_TypedBuf<BL_FLOAT> irs[2], int numSamples)
{
    if (numSamples == 0)
        return;
    
    // Generate impulse
    WDL_TypedBuf<BL_FLOAT> impulse;
    BLUtils::ResizeFillZeros(&impulse, numSamples);
    
    impulse.Get()[0] = DIRAC_VALUE;
    
    // NOTE: sometimes this can crash (we may break the stack)
    //
    // Clone the reverb (to keep the original untouched)
    //BLReverb revClone = *this;
    
    // Use dynamic alloc to not break the stack
    BLReverb *revClone = Clone();
    
    // Apply the reverb to the impulse
    revClone->Process(impulse, &irs[0], &irs[1]);
    
    delete revClone;
}
