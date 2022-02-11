//
//  ImageSmootherKernel.h
//  BL-DUET
//
//  Created by applematuer on 5/4/20.
//
//

#ifndef __BL_DUET__ImageSmootherKernel__
#define __BL_DUET__ImageSmootherKernel__

#include "IPlug_include_in_plug_hdr.h"

class ImageSmootherKernel
{
public:
    ImageSmootherKernel(int kernelSize);
    
    virtual ~ImageSmootherKernel();
    
    void SetKernelSize(int kernelSize);
    
    void SmoothImage(int imgWidth, int imgHeight, WDL_TypedBuf<BL_FLOAT> *imageData);
    
protected:
    int mKernelSize;
    
    WDL_TypedBuf<BL_FLOAT> mHanningKernel;

private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
};

#endif /* defined(__BL_DUET__ImageSmootherKernel__) */
