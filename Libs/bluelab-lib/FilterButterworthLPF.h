//
//  FilterButterworthLPF.h
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#ifndef __UST__ButterworthFilter__
#define __UST__ButterworthFilter__

#include <BLTypes.h>

class SO_BUTTERWORTH_LPF;
class FilterButterworthLPF
{
public:
    FilterButterworthLPF();
    
    virtual ~FilterButterworthLPF();
    
    void Init(BL_FLOAT cutFrec, BL_FLOAT sampleRate);
    
    BL_FLOAT Process(BL_FLOAT sample);
    
protected:
    SO_BUTTERWORTH_LPF *mFilter;
};

#endif /* defined(__UST__FilterButterworthLPF__) */
