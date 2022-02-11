//
//  FftConvolverSmooth4.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__FftProcessObjSmooth5__
#define __Spatializer__FftProcessObjSmooth5__

#include "IPlug_include_in_plug_hdr.h"
#include "../../WDL/fft.h"
//#include "../../WDL/IPlug/Containers.h"

class FftProcessObj5;


// Same as FftConvolverSmooth4, but for FftProcessObj
class FftProcessObjSmooth5
{
public:
    FftProcessObjSmooth5(FftProcessObj5 *obj0, FftProcessObj5 *obj1,
                         int bufferSize);
    
    virtual ~FftProcessObjSmooth5();
    
    FftProcessObj5 *GetFftObj(int index);
    
    void Reset();
    
    // Parameter behavior depends on the derived type of FftObj
    void SetParameter(double param);
    
    // Return true if nFrames were provided
    bool Process(double *input, double *output, int nFrames);
    
protected:
    bool ComputeNewSamples(int nFrames);
    
    bool GetOutputSamples(double *output, int nFrames);
    
    bool ProcessFirstBuffer();
    
    bool ProcessMakeFade();
    
    static void MakeFade(const WDL_TypedBuf<double> &buf0,
                         const WDL_TypedBuf<double> &buf1,
                         double *resultBuf);
    
    FftProcessObj5 *mFftObjs[2];
    int mCurrentObjIndex;
    
    bool mStableState;
    bool mHasJustReset;
    bool mFirstNewBufferProcessed;
    
    // For buffering
    int mBufSize;
    
    WDL_TypedBuf<double> mSamplesBuf;
    
    // 2 because we have 2 process objects
    WDL_TypedBuf<double> mResultBuf[2];
    
    WDL_TypedBuf<double> mResultBufFade;
    
    // Parameter
    double mParameter;
    bool mParameterValid;
};

#endif /* defined(__Spatializer__FftProcessObjSmooth5__) */

