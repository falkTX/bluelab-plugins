//
//  RebalanceTestPredictObj3.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#include <RebalanceMaskPredictorComp2.h>

#include <RebalanceMaskStack.h>

#include <BLUtils.h>

#include <PPMFile.h>

#include "RebalanceTestPredictObj3.h"

// Debug
#define DBG_DUMP_DATA 0 //1
#define DBG_DUMP_FULL_IMAGES 1 //0 //1 PREV TEST // Dump 1 row each time
#define DBG_DUMP_FULL_IMAGES2 0 // Dump 32 row each time
#define DBG_DUMP_TXT 0 // CURRENT 1 //0 //1

RebalanceTestPredictObj3::RebalanceTestPredictObj3(int bufferSize,
                                                   IGraphics *graphics)
: ProcessObj(bufferSize)
{
    WDL_String resPath;
    graphics->GetResourceDir(&resPath);
    const char *resourcePath = resPath.Get();
    
    // Vocal
    RebalanceMaskPredictorComp2::CreateModel(MODEL_VOCAL, resourcePath, &mModel);
    
    ResetHistory();
    
    mMaskStack = new RebalanceMaskStack(NUM_INPUT_COLS);
    
    mDbgCount = 0;
}

RebalanceTestPredictObj3::~RebalanceTestPredictObj3()
{
    delete mModel;
    
    delete mMaskStack;
}

void
RebalanceTestPredictObj3::Reset(int bufferSize, int oversampling,
                               int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    ResetHistory();
    
    mDbgCount = 0;
}

void
RebalanceTestPredictObj3::Reset()
{
    ProcessObj::Reset();
    
    ResetHistory();
    
    mDbgCount = 0;
}

void
RebalanceTestPredictObj3::ResetHistory()
{
    //
    mMixCols.clear();
    mMixColsComp.clear();
    
    mMixDownCols.clear();
    
    mStemCols.clear();
    mStemDownCols.clear();
    
    // TEST
    return;
    
    //
    WDL_TypedBuf<BL_FLOAT> col;
    BLUtils::ResizeFillZeros(&col, mBufferSize/2);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> colComp;
    BLUtils::ResizeFillZeros(&colComp, mBufferSize/2);
    
    WDL_TypedBuf<BL_FLOAT> colDown;
    BLUtils::ResizeFillZeros(&colDown, mBufferSize/(2*RESAMPLE_FACTOR));
    
    for (int i = 0; i < NUM_INPUT_COLS; i++)
    {
        mMixCols.push_back(col);
        mMixColsComp.push_back(colComp);
        
        mMixDownCols.push_back(colDown);
        mStemDownCols.push_back(colDown);
        mStemCols.push_back(col);
    }
    
#if DBG_DUMP_FULL_IMAGES
    mMixColsImg.clear();
    mStemColsImg.clear();
    mMaskColsImg.clear();
    mMultColsImg.clear();
#endif
    
#if DBG_DUMP_FULL_IMAGES2
    mMixColsImg.clear();
    mStemColsImg.clear();
    mResultColsImg.clear();
#endif
}

#if 0
void
RebalanceTestPredictObj3::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Mix
    //
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBuffer = *ioBuffer;
    BLUtils::TakeHalf(&mixBuffer);

    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, mixBuffer);
    
    mMixCols.push_back(magns);
    if (mMixCols.size() > NUM_INPUT_COLS)
        mMixCols.pop_front();
    
    mMixColsComp.push_back(mixBuffer);
    if (mMixColsComp.size() > NUM_INPUT_COLS)
        mMixColsComp.pop_front();
    
    // Compute the masks
    WDL_TypedBuf<BL_FLOAT> magnsDown = magns;
    RebalanceMaskPredictorComp2::Downsample(&magnsDown, mSampleRate);
    
    // mMixCols is filled with zeros at the origin
    mMixDownCols.push_back(magnsDown);
    if (mMixDownCols.size() > NUM_INPUT_COLS)
        mMixDownCols.pop_front();
    
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
    
        mStemCols.push_back(stemMagns);
        if (mStemCols.size() > NUM_INPUT_COLS)
            mStemCols.pop_front();
        
        // Compute the masks
        WDL_TypedBuf<BL_FLOAT> stemMagnsDown = stemMagns;
        RebalanceMaskPredictorComp2::Downsample(&stemMagnsDown, mSampleRate);
    
        // mStemCols is filled with zeros at the origin
        mStemDownCols.push_back(stemMagnsDown);
        if (mStemDownCols.size() > NUM_INPUT_COLS)
            mStemDownCols.pop_front();
    }
    
    if (mMixCols.size() < NUM_INPUT_COLS)
        return;
    
    //
    WDL_TypedBuf<BL_FLOAT> mixBuf;
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&mixBuf, mMixCols);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBufComp;
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&mixBufComp, mMixColsComp);
    
    WDL_TypedBuf<BL_FLOAT> mixDownBuf;
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&mixDownBuf, mMixDownCols);
    
    WDL_TypedBuf<BL_FLOAT> stemDownBuf;
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&stemDownBuf, mStemDownCols);
    
    WDL_TypedBuf<BL_FLOAT> stemBuf;
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&stemBuf, mStemCols);
    
#if DBG_DUMP_DATA
    WDL_TypedBuf<BL_FLOAT> normInput = mixBuf;
    BLUtils::Normalize(&normInput);
    PPMFile::SavePPM("tst-up-mix.ppm", normInput.Get(),
                     1024, NUM_OUTPUT_COLS, 1, 255.0);
#endif

#if DBG_DUMP_DATA
    //BLDebug::DumpData("mdl-input.txt", input);
    WDL_TypedBuf<BL_FLOAT> normDownStem = stemDownBuf;
    BLUtils::Normalize(&normDownStem);
    PPMFile::SavePPM("tst-stem.ppm", normDownStem.Get(),
                     1024/RESAMPLE_FACTOR, NUM_OUTPUT_COLS, 1, 255.0);
#endif
    
    // Predict real size mask
    WDL_TypedBuf<BL_FLOAT> predictedMask;
    RebalanceMaskPredictorComp2::PredictMask(&predictedMask, mixDownBuf,
                                             mModel, mSampleRate);
    
#if 1 // GOOD: sum the previous masks
    //PPMFile::SavePPM("mask0.ppm", predictedMask.Get(),
    //                 256, 32, 1, 255.0);
    //BLDebug::DumpData("line0.txt", predictedMask);
    
    deque<WDL_TypedBuf<BL_FLOAT> > predictQue;
    //BufferToQue(&predictQue, predictedMask, BUFFER_SIZE/2);
    BufferToQue(&predictQue, predictedMask, BUFFER_SIZE/(2*RESAMPLE_FACTOR));
    mMaskStack->AddMask(predictQue);
    // Avg is better than variance
    mMaskStack->GetMaskAvg(&predictQue);
    //mMaskStack->GetMaskWeightedAvg(&predictQue);
    //mMaskStack->GetMaskVariance(&predictQue);
    
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&predictedMask, predictQue);
    
    //PPMFile::SavePPM("mask1.ppm", predictedMask.Get(),
    //                 256, 32, 1, 255.0);
    //BLDebug::DumpData("line1.txt", predictedMask);
#endif
    
    //
    BLUtils::ClipMin(&predictedMask, 0.0);
    RebalanceMaskPredictorComp2::UpsamplePredictedMask(&predictedMask, mSampleRate);
    BLUtils::ClipMin(&predictedMask, 0.0);
    
#if DBG_DUMP_DATA
    WDL_TypedBuf<BL_FLOAT> normMaskOutput = predictedMask;
    BLUtils::Normalize(&normMaskOutput);
    PPMFile::SavePPM("tst-up-msk.ppm", normMaskOutput.Get(),
                     1024, NUM_OUTPUT_COLS, 1, 255.0);
#endif
   
#if DBG_DUMP_DATA
    WDL_TypedBuf<BL_FLOAT> multBuf = mixBuf;
    BLUtils::MultValues(&multBuf, predictedMask);
    
    WDL_TypedBuf<BL_FLOAT> normMult = multBuf;
    BLUtils::Normalize(&normMult);
    PPMFile::SavePPM("tst-up-mult.ppm", normMult.Get(),
                     1024, NUM_OUTPUT_COLS, 1, 255.0);
#endif
    
    int index = mMixColsComp.size()/2;
    
    // Compute output
    WDL_TypedBuf<WDL_FFT_COMPLEX> multBufComp = mixBufComp;
    BLUtils::MultValues(&multBufComp, predictedMask);
    
    // Directly multiply by stem => very good!
    //WDL_TypedBuf<WDL_FFT_COMPLEX> multBufComp = mixBufComp;
    //BLUtils::NormalizeMagns(&multBufComp);
    //BLUtils::MultValues(&multBufComp, stemBuf);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    fftSamples.Resize(ioBuffer->GetSize()/2);
    memcpy(fftSamples.Get(),
           &multBufComp.Get()[index*ioBuffer->GetSize()/2],
           fftSamples.GetSize()*sizeof(WDL_FFT_COMPLEX));
    
#if DBG_DUMP_FULL_IMAGES
    mMixColsImg.push_back(mMixCols[index]);
    mStemColsImg.push_back(mStemCols[index]);
    
    WDL_TypedBuf<BL_FLOAT> resultMagns;
    BLUtils::ComplexToMagn(&resultMagns, fftSamples);
    
    mResultColsImg.push_back(resultMagns);
    
    if (mMixColsImg.size() % 100 == 0)
    {
        SaveImage("img-mix.ppm", mMixColsImg);
        SaveImage("img-stem.ppm", mStemColsImg);
        SaveImage("img-result.ppm", mResultColsImg);
    }
#endif

#if DBG_DUMP_FULL_IMAGES2
    for (int i = 0; i < mMixCols.size(); i++)
        mMixColsImg.push_back(mMixCols[i]);
    
    for (int i = 0; i < mStemCols.size(); i++)
        mStemColsImg.push_back(mStemCols[i]);
    
    deque<WDL_TypedBuf<BL_FLOAT> > maskQue;
    BufferToQue(&maskQue, predictedMask, BUFFER_SIZE/2);
    
    deque<WDL_TypedBuf<BL_FLOAT> > resultQue = mMixCols;
    MultQue(&resultQue, maskQue);
    
    for (int i = 0; i < resultQue.size(); i++)
        mResultColsImg.push_back(resultQue[i]);
    
    if (mMixColsImg.size() % 100 == 0)
    {
        SaveImage("img-mix.ppm", mMixColsImg);
        SaveImage("img-stem.ppm", mStemColsImg);
        SaveImage("img-result.ppm", mResultColsImg);
    }
#endif

    fftSamples.Resize(fftSamples.GetSize()*2);
    BLUtils::FillSecondFftHalf(&fftSamples);
    
    // Result
    *ioBuffer = fftSamples;
}
#endif

void
RebalanceTestPredictObj3::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                           const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Mix
    //
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBuffer = *ioBuffer;
    BLUtils::TakeHalf(&mixBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, mixBuffer);
    
    //
    mMixColsComp.push_back(mixBuffer);
    if (mMixColsComp.size() > NUM_INPUT_COLS)
        mMixColsComp.pop_front();
    
    // Compute the masks
    WDL_TypedBuf<BL_FLOAT> magnsDown = magns;
    RebalanceMaskPredictorComp2::Downsample(&magnsDown, mSampleRate);
    
    // mMixCols is filled with zeros at the origin
    mMixDownCols.push_back(magnsDown);
    if (mMixDownCols.size() > NUM_INPUT_COLS)
        mMixDownCols.pop_front();
    
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
        RebalanceMaskPredictorComp2::Downsample(&stemMagnsDown, mSampleRate);
        
        // mStemCols is filled with zeros at the origin
        mStemDownCols.push_back(stemMagnsDown);
        if (mStemDownCols.size() > NUM_INPUT_COLS)
            mStemDownCols.pop_front();
    }
    
    if (mMixDownCols.size() < NUM_INPUT_COLS)
        return;
    
    //
    WDL_TypedBuf<WDL_FFT_COMPLEX> mixBufComp;
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&mixBufComp, mMixColsComp);
    
    WDL_TypedBuf<BL_FLOAT> mixDownBuf;
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&mixDownBuf, mMixDownCols);
    
    // Predict real size mask
    //WDL_TypedBuf<BL_FLOAT> predictedResultDown;
    WDL_TypedBuf<BL_FLOAT> predictedMaskDown;
    //RebalanceMaskPredictorComp2::PredictMask(&predictedResultDown, &predictedMaskDown,
    //                                         mixDownBuf, mModel, mSampleRate);
    RebalanceMaskPredictorComp2::PredictMask(&predictedMaskDown,
                                             mixDownBuf, mModel, mSampleRate);
                                             
#if DBG_DUMP_TXT
    WDL_TypedBuf<BL_FLOAT> stemDownBuf;
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&stemDownBuf, mStemDownCols);
    
    BLDebug::DumpData("rb-mix.txt", mixDownBuf);
    BLDebug::DumpData("rb-stem.txt", stemDownBuf);
    
    WDL_TypedBuf<BL_FLOAT> mixDownBufNorm = mixDownBuf;
    BLUtils::Normalize(&mixDownBufNorm);
    PPMFile::SavePPM("rb-mix.ppm", mixDownBufNorm.Get(),
                     256, 32, 1, 255.0);
    
    WDL_TypedBuf<BL_FLOAT> stemDownBufNorm = stemDownBuf;
    BLUtils::Normalize(&stemDownBufNorm);
    PPMFile::SavePPM("rb-stem.ppm", stemDownBufNorm.Get(),
                     256, 32, 1, 255.0);
#endif
    
    // => not sure it is very good,
    // the sound is better, but the separation may be worse
    
#if 1 // GOOD: sum the previous masks
    //PPMFile::SavePPM("mask0.ppm", predictedMask.Get(),
    //                 256, 32, 1, 255.0);
    
    deque<WDL_TypedBuf<BL_FLOAT> > predictQue;
    BufferToQue(&predictQue, predictedMaskDown, BUFFER_SIZE/(2*RESAMPLE_FACTOR));
    mMaskStack->AddMask(predictQue);
    // Avg is better than variance
    mMaskStack->GetMaskAvg(&predictQue);
    //mMaskStack->GetMaskWeightedAvg(&predictQue);
    //mMaskStack->GetMaskVariance(&predictQue);
    
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&predictedMaskDown, predictQue);
    
    //PPMFile::SavePPM("mask1.ppm", predictedMask.Get(),
    //                 256, 32, 1, 255.0);
#endif

    //deque<WDL_TypedBuf<BL_FLOAT> > resultQue;
    //BufferToQue(&resultQue, predictedResultDown, BUFFER_SIZE/(2*RESAMPLE_FACTOR));
    
#if 0 // GOOD: sum the previous masks
    //PPMFile::SavePPM("mask0.ppm", predictedMask.Get(),
    //                 256, 32, 1, 255.0);
    
    mMaskStack->AddMask(resultQue);
    // Avg is better than variance
    mMaskStack->GetMaskAvg(&resultQue);
    //mMaskStack->GetMaskWeightedAvg(&predictQue);
    //mMaskStack->GetMaskVariance(&predictQue);
    
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&predictedResultDown, resultQue);
    
    //PPMFile::SavePPM("mask1.ppm", predictedMask.Get(),
    //                 256, 32, 1, 255.0);
#endif

    //
    //BLUtils::ClipMin(&predictedMaskDown, 0.0);
    WDL_TypedBuf<BL_FLOAT> predictedMask = predictedMaskDown;
    RebalanceMaskPredictorComp2::UpsamplePredictedMask(&predictedMask, mSampleRate);
    BLUtils::ClipMin(&predictedMask, 0.0);
    
    int index = mMixColsComp.size()/2;
    
    // Compute output
    WDL_TypedBuf<WDL_FFT_COMPLEX> multBufComp = mixBufComp;
    BLUtils::MultValues(&multBufComp, predictedMask);
    
    //
    //WDL_TypedBuf<BL_FLOAT> predictedResult = predictedResultDown;
    //RebalanceMaskPredictorComp2::UpsamplePredictedMask(&predictedResult, mSampleRate);
    
    //WDL_TypedBuf<BL_FLOAT> resultMagns;
    //resultMagns.Resize(ioBuffer->GetSize()/2);
    //memcpy(resultMagns.Get(),
    //       &predictedResult.Get()[index*ioBuffer->GetSize()/2],
    //       resultMagns.GetSize()*sizeof(BL_FLOAT));
    
    //WDL_TypedBuf<WDL_FFT_COMPLEX> multBufComp;
    //multBufComp.Resize(magns.GetSize());
    //BLUtils::FillAllZero(&multBufComp);
    
    //WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    //BLUtils::MagnPhaseToComplex(&fftSamples, resultMagns, phases);
    
    //
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    fftSamples.Resize(ioBuffer->GetSize()/2);
    memcpy(fftSamples.Get(),
           &multBufComp.Get()[index*ioBuffer->GetSize()/2],
           fftSamples.GetSize()*sizeof(WDL_FFT_COMPLEX));
    
    fftSamples.Resize(fftSamples.GetSize()*2);
    BLUtils::FillSecondFftHalf(&fftSamples);
    
    // Result
    *ioBuffer = fftSamples;
    
#if DBG_DUMP_FULL_IMAGES
    deque<WDL_TypedBuf<BL_FLOAT> > maskQue;
    BufferToQue(&maskQue, predictedMaskDown, BUFFER_SIZE/(2*RESAMPLE_FACTOR));
    deque<WDL_TypedBuf<BL_FLOAT> > multQue = mMixDownCols;
    MultQue(&multQue, maskQue);
    
#if 1 //0
    mMixColsImg.push_back(mMixDownCols[index]);
    mStemColsImg.push_back(mStemDownCols[index]);
    //mMultColsImg.push_back(resultQue[index]);
    mMultColsImg.push_back(multQue[index]);
    mMaskColsImg.push_back(maskQue[index]);
#endif
    
#if 0 //1
    //if (mMixDownCols.size() % 32 == 0)
    if (mDbgCount++ % 8 == 0)
    {
        AddColsImg(&mMixColsImg, mMixDownCols);
        AddColsImg(&mStemColsImg, mStemDownCols);
        AddColsImg(&mMultColsImg, multQue);
        AddColsImg(&mMaskColsImg, maskQue);
    }
#endif
    
    if (mMixColsImg.size() % 8 == 0)
    {
        SaveImage("img-mix.ppm", mMixColsImg, 255.0);
        SaveImage("img-stem.ppm", mStemColsImg, 255.0);
        SaveImage("img-mask.ppm", mMaskColsImg, 1.0);
        SaveImage("img-mult.ppm", mMultColsImg, 255.0);
    }
#endif
}

void
RebalanceTestPredictObj3::SaveImage(const char *fileName,
                                    const deque<WDL_TypedBuf<BL_FLOAT> > &data,
                                    BL_FLOAT coeff)
{
    if (data.empty())
        return;
    
    int width = data[0].GetSize();
    int height = data.size();
    
    WDL_TypedBuf<BL_FLOAT> buf;
    RebalanceMaskPredictorComp2::ColumnsToBuffer(&buf, data);
    
    WDL_TypedBuf<BL_FLOAT> normBuf = buf;
    //BLUtils::Normalize(&normBuf);
    PPMFile::SavePPM(fileName, normBuf.Get(),
                     width, height, 1, coeff*255);
}

void
RebalanceTestPredictObj3::BufferToQue(deque<WDL_TypedBuf<BL_FLOAT> > *que,
                                      const WDL_TypedBuf<BL_FLOAT> &buffer,
                                      int width)
{
    int height = buffer.GetSize()/width;
    
    que->resize(height);
    for (int j = 0; j < height; j++)
    {
        WDL_TypedBuf<BL_FLOAT> line;
        line.Add(&buffer.Get()[j*width], width);
        (*que)[j] = line;
    }
}

void
RebalanceTestPredictObj3::MultQue(deque<WDL_TypedBuf<BL_FLOAT> > *result,
                                  const deque<WDL_TypedBuf<BL_FLOAT> > &que)
{
    if (result->empty())
        return;
    if (result->size() != que.size())
        return;
    if ((*result)[0].GetSize() != que[0].GetSize())
        return;
    
    for (int i = 0; i < result->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &r = (*result)[i];
        const WDL_TypedBuf<BL_FLOAT> &q = que[i];
        
        for (int j = 0; j < r.GetSize(); j++)
        {
            r.Get()[j] *= q.Get()[j];
        }
    }
}

void
RebalanceTestPredictObj3::AddColsImg(deque<WDL_TypedBuf<BL_FLOAT> > *img,
                                     const deque<WDL_TypedBuf<BL_FLOAT> > &cols)
{
    for (int i = 0; i < cols.size(); i++)
    {
        img->push_back(cols[i]);
    }
}
