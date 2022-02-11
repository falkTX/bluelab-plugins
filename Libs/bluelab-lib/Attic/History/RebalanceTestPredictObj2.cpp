//
//  RebalanceTestPredictObj2.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#include <RebalanceMaskPredictorComp.h>

#include <BLUtils.h>

#include <PPMFile.h>

#include "RebalanceTestPredictObj2.h"


RebalanceTestPredictObj2::RebalanceTestPredictObj2(int bufferSize,
                                                   IGraphics *graphics)
: ProcessObj(bufferSize)
{
    WDL_String resPath;
    graphics->GetResourceDir(&resPath);
    const char *resourcePath = resPath.Get();
    
    // Vocal
    RebalanceMaskPredictorComp::CreateModel(MODEL_VOCAL, resourcePath, &mModel);
    
    for (int i = 0; i < NUM_INPUT_COLS; i++)
    {
        WDL_TypedBuf<BL_FLOAT> col;
        BLUtils::ResizeFillZeros(&col, bufferSize/(2*RESAMPLE_FACTOR));
        
        mMixCols.push_back(col);
        mStemCols.push_back(col);
    }
}

RebalanceTestPredictObj2::~RebalanceTestPredictObj2()
{
    delete mModel;
}

void
RebalanceTestPredictObj2::Reset(int bufferSize, int oversampling,
                               int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
}

void
RebalanceTestPredictObj2::Reset()
{
    ProcessObj::Reset();
}

void
RebalanceTestPredictObj2::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Mix
    //
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
    
    // Stem
    //
    WDL_TypedBuf<WDL_FFT_COMPLEX> stemBuffer;
    if ((scBuffer != NULL) && (scBuffer->GetSize() == ioBuffer->GetSize()))
    {
        stemBuffer = *scBuffer;
        BLUtils::TakeHalf(&stemBuffer);
    
        WDL_TypedBuf<BL_FLOAT> stemMagns;
        WDL_TypedBuf<BL_FLOAT> stemPhases;
        BLUtils::ComplexToMagnPhase(&stemMagns, &stemPhases, stemBuffer);
    
        // Compute the masks
        WDL_TypedBuf<BL_FLOAT> stemMagnsDown = stemMagns;
        RebalanceMaskPredictorComp::Downsample(&stemMagnsDown, mSampleRate);
    
        // mStemCols is filled with zeros at the origin
        mStemCols.push_back(stemMagnsDown);
        mStemCols.pop_front();
    }
    
    //
    WDL_TypedBuf<BL_FLOAT> mixBuf;
    RebalanceMaskPredictorComp::ColumnsToBuffer(&mixBuf, mMixCols);
    
    WDL_TypedBuf<BL_FLOAT> stemBuf;
    RebalanceMaskPredictorComp::ColumnsToBuffer(&stemBuf, mStemCols);
    
#if 1
    BLDebug::LoadData("00_mix.txt", &mixBuf);
    
    //BLUtils::Transpose(&mixBuf, 256, 32);
    
    //BLUtils::MultValues(&mixBuf, 22.0);
#endif
    
    // Predicted downsample mask
    WDL_TypedBuf<BL_FLOAT> maskBuf;
    mModel->Predict(mixBuf, &maskBuf);
    
    // TEST => threshold mask
    //BLUtils::AddValues(&maskBuf, -0.49204);
    //BLUtils::ClipMin(&maskBuf, 0.0);
    //BLUtils::Normalize(&maskBuf);
    
    WDL_TypedBuf<BL_FLOAT> applyBuf = mixBuf;
    BLUtils::MultValues(&applyBuf, maskBuf);
    
#if 1 //0 //1 // Dump ppm
    WDL_TypedBuf<BL_FLOAT> mixBuf0 = mixBuf;
    WDL_TypedBuf<BL_FLOAT> stemBuf0 = stemBuf;
    WDL_TypedBuf<BL_FLOAT> maskBuf0 = maskBuf;
    WDL_TypedBuf<BL_FLOAT> applyBuf0 = applyBuf;
    
    BLDebug::DumpData("mix.txt", mixBuf0);
    
    //BLUtils::Transpose(&maskBuf0, 256, 32);
    BLDebug::DumpData("mask.txt", maskBuf0);
    
    BLUtils::Normalize(&mixBuf0);
    BLUtils::Normalize(&stemBuf0);
    BLUtils::Normalize(&maskBuf0);
    BLUtils::Normalize(&applyBuf0);
    
    PPMFile::SavePPM("mix.ppm", mixBuf0.Get(),
                     1024/RESAMPLE_FACTOR, NUM_OUTPUT_COLS, 1, 255.0/**DBG_PPM_COLOR_COEFF*/);
    
    PPMFile::SavePPM("stem.ppm", stemBuf0.Get(),
                     1024/RESAMPLE_FACTOR, NUM_OUTPUT_COLS, 1, 255.0/**DBG_PPM_COLOR_COEFF*/);
    
    PPMFile::SavePPM("mask.ppm", maskBuf0.Get(),
                     1024/RESAMPLE_FACTOR, NUM_OUTPUT_COLS, 1, 255.0/**DBG_PPM_COLOR_COEFF*/);
    
    PPMFile::SavePPM("apply.ppm", applyBuf0.Get(),
                     1024/RESAMPLE_FACTOR, NUM_OUTPUT_COLS, 1, 255.0/**DBG_PPM_COLOR_COEFF*/);
#endif
   
    // Predict real size mask
    WDL_TypedBuf<BL_FLOAT> predictedMask;
    RebalanceMaskPredictorComp::PredictMask(&predictedMask, mixBuf,
                                            mModel, mSampleRate);
    
    // TEST => threshold mask
    //BLUtils::AddValues(&predictedMask, -0.35);
    //BLUtils::ClipMin(&predictedMask, 0.0);
    //BLUtils::Normalize(&predictedMask);
    //BLDebug::DumpData("mask2.txt", predictedMask);
    
    // Output
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = mixBuffer;
    //WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = stemBuffer;
    
#if 0 //1 //0 // Dump mask
    // Fill the result
    fftSamples.Resize(predictedMask.GetSize());
    for (int i = 0; i < fftSamples.GetSize(); i++)
    {
        BL_FLOAT maskVal = predictedMask.Get()[i];
        fftSamples.Get()[i].re = maskVal;
        fftSamples.Get()[i].im = 0.0;
    }
#endif
    
#if 1 //0 // Apply mask
    fftSamples = mixBuffer;
    for (int i = 0; i < fftSamples.GetSize(); i++)
    {
        BL_FLOAT maskVal = predictedMask.Get()[i];
        fftSamples.Get()[i].re *= maskVal;
        fftSamples.Get()[i].im *= maskVal;
    }
#endif
    
    fftSamples.Resize(fftSamples.GetSize()*2);
    BLUtils::FillSecondFftHalf(&fftSamples);
    
    // Result
    *ioBuffer = fftSamples;
}
