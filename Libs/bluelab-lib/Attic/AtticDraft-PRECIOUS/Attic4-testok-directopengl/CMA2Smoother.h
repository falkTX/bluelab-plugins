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
    bool Process(const double *data, double *smoothedData, int nFrames);
    
    // NOT WELL TESTED !
    
    // Process one buffer, without managing streaming to next buffers
    // Fill the missing input data with zeros
    static bool ProcessOne(const double *data, double *smoothedData, int nFrames, int windowSize);

    
    void Reset();
    
protected:
    CMASmoother mSmoother0;
    CMASmoother mSmoother1;
};

#endif /* defined(__Transient__CMA2Smoother__) */
