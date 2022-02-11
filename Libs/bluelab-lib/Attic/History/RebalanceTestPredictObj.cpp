//
//  RebalanceTestPredictObj.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#include <RebalanceMaskPredictorComp.h>

#include <BLUtils.h>

#include "RebalanceTestPredictObj.h"


RebalanceTestPredictObj::RebalanceTestPredictObj(int bufferSize,
                                                 IGraphics *graphics)
: ProcessObj(bufferSize)
{
    WDL_String resPath;
    graphics->GetResourceDir(&resPath);
    const char *resourcePath = resPath.Get();
    
    // Vocal
    RebalanceMaskPredictorComp::CreateModel(MODEL_VOCAL, resourcePath, &mModelVocal);
    
    for (int i = 0; i < NUM_INPUT_COLS; i++)
    {
        WDL_TypedBuf<BL_FLOAT> col;
        BLUtils::ResizeFillZeros(&col, bufferSize/(2*RESAMPLE_FACTOR));
        
        mMixCols.push_back(col);
    }
}

RebalanceTestPredictObj::~RebalanceTestPredictObj()
{
    delete mModelVocal;
}

void
RebalanceTestPredictObj::Reset(int bufferSize, int oversampling,
                               int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
}

void
RebalanceTestPredictObj::Reset()
{
    ProcessObj::Reset();
}

void
RebalanceTestPredictObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Mix
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBuffer = *ioBuffer;
    BLUtils::TakeHalf(&mixBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, mixBuffer);
    
    // Compute the masks
    WDL_TypedBuf<BL_FLOAT> magnsDown = magns;
    RebalanceMaskPredictorComp::Downsample(&magnsDown, mSampleRate);
    
    // mMixCols is filled with zeros at the origin
    mMixCols.push_back(magnsDown);
    mMixCols.pop_front();
    
    WDL_TypedBuf<BL_FLOAT> mixBufHisto;
    RebalanceMaskPredictorComp::ColumnsToBuffer(&mixBufHisto, mMixCols);
    
    WDL_TypedBuf<BL_FLOAT> predictedMask;
    RebalanceMaskPredictorComp::PredictMask(&predictedMask, mixBufHisto,
                                            mModelVocal, mSampleRate);
    
    // TEST
    //BLDebug::DumpData("mask.txt", predictedMask);
    
#if 0 //1 //0 //1 // Mask
    // Fill the result
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    fftSamples.Resize(predictedMask.GetSize());
    for (int i = 0; i < fftSamples.GetSize(); i++)
    {
        BL_FLOAT maskVal = predictedMask.Get()[i];
        fftSamples.Get()[i].re = maskVal;
        fftSamples.Get()[i].im = 0.0;
    }
#endif
    
#if 1 //0 //1 //0 // Apply mask
    // TEST
    //BLUtils::MultValues(&predictedMask, 4.0);
    //BLUtils::AddValues(&predictedMask, -1.0);
    //BLUtils::ClipMin(&predictedMask, 0.0);
    //for (int i = 0; i < predictedMask.GetSize(); i++)
    //{
    //    BL_FLOAT val = predictedMask.Get()[i];
    //    //val = 0.5 - val;
    //
    //    val *= 2.0;
    //    if (val < 0.6)
    //        val = 0.0;
    //
    //    predictedMask.Get()[i] = val;
    //}
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = mixBuffer;
    for (int i = 0; i < fftSamples.GetSize(); i++)
    {
        BL_FLOAT maskVal = predictedMask.Get()[i];
        fftSamples.Get()[i].re *= maskVal;
        fftSamples.Get()[i].im *= maskVal;
        
        // TEST: try to avoid the effect of phases
        //BL_FLOAT magn = COMP_MAGN(fftSamples.Get()[i]);
        //fftSamples.Get()[i].re = magn*maskVal;
        //fftSamples.Get()[i].im = 0.0;
    }
#endif
    
    fftSamples.Resize(fftSamples.GetSize()*2);
    BLUtils::FillSecondFftHalf(&fftSamples);
    
    // Result
    *ioBuffer = fftSamples;
}
