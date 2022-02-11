//
//  USTBrillance.h
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#ifndef __UST__USTBrillance__
#define __UST__USTBrillance__

#define MAX_NUM_CHANNELS 2

class CrossoverSplitterNBands4;

class USTBrillance
{
public:
    USTBrillance(BL_FLOAT sampleRate);
    
    virtual ~USTBrillance();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > *samples,
                 BL_FLOAT brillance);
    
    void BypassProcess(vector<WDL_TypedBuf<BL_FLOAT> > *samples);
    
protected:
    CrossoverSplitterNBands4 *mSplitters[MAX_NUM_CHANNELS];
    
    CrossoverSplitterNBands4 *mBypassSplitters[MAX_NUM_CHANNELS];
};

#endif /* defined(__UST__USTBrillance__) */
