//
//  FftConvolverSmooth4.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__BufProcessObjSmooth__
#define __Spatializer__BufProcessObjSmooth__

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

//#include "../../WDL/IPlug/Containers.h"

class BufProcessObj;


// FftProcessObjSmooth5: same as FftConvolverSmooth4, but for FftProcessObj
//
// BufProcessObjSmooth: from FftProcessObjSmooth5
class BufProcessObjSmooth
{
public:
    BufProcessObjSmooth(BufProcessObj *obj0, BufProcessObj *obj1,
                        int bufSize, int oversampling, int freqRes);
    
    virtual ~BufProcessObjSmooth();
    
    BufProcessObj *GetBufObj(int index);
    
    void Reset(int oversampling, int freqRes);
    
    // Parameter behavior depends on the derived type of FftObj
    void SetParameter(void *param);
    
    // Return true if nFrames were provided
    bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
protected:
    static void MakeFade(const WDL_TypedBuf<BL_FLOAT> &buf0,
                         const WDL_TypedBuf<BL_FLOAT> &buf1,
                         BL_FLOAT *resultBuf);
    
    BufProcessObj *mBufObjs[2];
    
    // 2 because we have 2 process objects
    WDL_TypedBuf<BL_FLOAT> mResultBuf[2];
    
    // Parameter
    void *mParameter;
    bool mParameterChanged;
    
    bool mReverseFade;
    
    // Set after Reset was called.
    // So we know everything went back to zero
    bool mHasJustReset;
    
    //
    // Stuff fors flushing the buffers before switching
    // back to only one obj processor
    //
    
    int mFlushBufferCount;
    
    // Bufs size and oversampling are used to compute a size
    // to finish to flush the buffers
    // when the parameter doesn't change anymore
    int mBufSize;
    
    int mOversampling;
    
    // Not used
    int mFreqRes;
};

#endif /* defined(__Spatializer__BufProcessObjSmooth__) */

