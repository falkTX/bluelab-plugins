//
//  CMA2Smoother.h
//  Transient
//
//  Created by Apple m'a Tuer on 24/05/17.
//
//

#ifndef __Transient__CMA2Smoother__
#define __Transient__CMA2Smoother__

#include <CMASmoother.h>

// Double central moving average smoother
class CMA2Smoother
{
public:
    CMA2Smoother(int bufferSize, int windowSize);
    
    virtual ~CMA2Smoother();
    
    // Return true if nFrames has been returned
    bool Process(const BL_FLOAT *data, BL_FLOAT *smoothedData, int nFrames);

    // NOTE: removed the static attribute
    // for memory optimization and buffers re-use
    
    // Process one buffer, without managing streaming to next buffers
    // Fill the missing input data with zeros
    //template <typename FLOAT_TYPE>
    ///*static*/ bool ProcessOne(const FLOAT_TYPE *data,
    //                           FLOAT_TYPE *smoothedData,
    //                           int nFrames, int windowSize);

    /*static*/ bool ProcessOne(const BL_FLOAT *data,
                               BL_FLOAT *smoothedData,
                               int nFrames, int windowSize);
    
    /*static*/ bool ProcessOne(const WDL_TypedBuf<BL_FLOAT> &inData,
                               WDL_TypedBuf<BL_FLOAT> *outSmoothedData,
                               int windowSize);
    
    void Reset();
    
protected:
    CMASmoother mSmoother0;
    CMASmoother mSmoother1;

    // For ProcessOne()
    CMASmoother mSmootherP1;

private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
};

#endif /* defined(__Transient__CMA2Smoother__) */
