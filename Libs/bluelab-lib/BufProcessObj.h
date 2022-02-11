//
//  BufProcessObj.h
//  PitchShift
//
//  Created by Pan on 06/01/18.
//
//

#ifndef PitchShift_BufProcessObj_h
#define PitchShift_BufProcessObj_h

#include <BLTypes.h>

class BufProcessObj
{
public:
    BufProcessObj() {}
    
    virtual ~BufProcessObj() {}
    
    virtual void Reset(int oversampling, int freqRes) = 0;
    
    virtual void SetParameter(void *param) = 0;
    
    virtual bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames) = 0;
};
#endif
