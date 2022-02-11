//
//  FftConvolverSmooth4.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__BufProcessObjSmooth__
#define __Spatializer__BufProcessObjSmooth__

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

class BufProcessObj;


// FftProcessObjSmooth5: same as FftConvolverSmooth4, but for FftProcessObj
//
// BufProcessObjSmooth: from FftProcessObjSmooth5
class BufProcessObjSmooth
{
public:
    BufProcessObjSmooth(BufProcessObj *obj0, BufProcessObj *obj1);
    
    virtual ~BufProcessObjSmooth();
    
    BufProcessObj *GetBufObj(int index);
    
    void Reset();
    
    // Parameter behavior depends on the derived type of FftObj
    void SetParameter(double param);
    
    // Return true if nFrames were provided
    bool Process(double *input, double *output, int nFrames);
    
protected:
    static void MakeFade(const WDL_TypedBuf<double> &buf0,
                         const WDL_TypedBuf<double> &buf1,
                         double *resultBuf);
    
    BufProcessObj *mBufObjs[2];
    
    // 2 because we have 2 process objects
    WDL_TypedBuf<double> mResultBuf[2];
    
    // Parameter
    double mParameter;
    bool mParameterChanged;
    
    bool mReverseFade;
};

#endif /* defined(__Spatializer__BufProcessObjSmooth__) */

