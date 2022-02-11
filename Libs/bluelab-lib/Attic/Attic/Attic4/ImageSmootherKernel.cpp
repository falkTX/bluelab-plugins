//
//  ImageSmootherKernel.cpp
//  BL-DUET
//
//  Created by applematuer on 5/4/20.
//
//

#include <Window.h>

#include <BLUtils.h>

#include <PPMFile.h>

#include "ImageSmootherKernel.h"


ImageSmootherKernel::ImageSmootherKernel(int kernelSize)
{
    mKernelSize = kernelSize;
    
    Window::MakeHanningKernel2(mKernelSize, &mHanningKernel);
    
    //PPMFile::SavePPM("kernel.ppm", mHanningKernel.Get(),
    //                 kernelSize, kernelSize, 1, 255.0);
}

ImageSmootherKernel::~ImageSmootherKernel() {}

void
ImageSmootherKernel::SetKernelSize(int kernelSize)
{
    mKernelSize = kernelSize;
    
    Window::MakeHanningKernel2(mKernelSize, &mHanningKernel);
}

void
ImageSmootherKernel::SmoothImage(int imgWidth, int imgHeight,
                                 WDL_TypedBuf<BL_FLOAT> *imageData)
{
    WDL_TypedBuf<BL_FLOAT> result;
    result.Resize(imageData->GetSize());
    BLUtils::FillAllZero(&result);
    
    int halfWinSize = mKernelSize/2;
    
    for (int j = 0; j < imgHeight; j++)
    {
        for (int i = 0; i < imgWidth; i++)
        {
            // For each pixel, multiply by the window
            BL_FLOAT avg = 0.0;
            BL_FLOAT sum = 0.0;
            
            int index0 = i + j*imgWidth;
            
            for (int wi = -halfWinSize; wi <= halfWinSize; wi++)
            {
                for (int wj = -halfWinSize; wj <= halfWinSize; wj++)
                {
                    int x = i + wi;
                    if (x < 0)
                        continue;
                    if (x >= imgWidth)
                        continue;
                    
                    int y = j + wj;
                    if (y < 0)
                        continue;
                    if (y >= imgHeight)
                        continue;
                    
                    BL_FLOAT val = imageData->Get()[x + y*imgWidth];
                    BL_FLOAT kernelVal = mHanningKernel.Get()[(wi + halfWinSize) +
                                                            (wj + halfWinSize)*mKernelSize];
                    
                    avg += val*kernelVal;
                    sum += kernelVal;
                }
            }
            
            if (sum > 0.0)
                avg /= sum;
            
            // Set the result
            result.Get()[index0] = avg;
        }
    }
    
    *imageData = result;
}
