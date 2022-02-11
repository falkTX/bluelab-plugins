//
//  RebalanceTestMultiObj.cpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#include <RebalanceDumpFftObj.h>

#include "RebalanceTestMultiObj.h"

#if TEST_DNN_INPUT

RebalanceTestMultiObj::RebalanceTestMultiObj(RebalanceDumpFftObj *rebalanceDumpFftObjs[4],
                                             double sampleRate)
{
    for (int i = 0; i < 4; i++)
        mRebalanceDumpFftObjs[i] = rebalanceDumpFftObjs[i];
    
    mSampleRate = sampleRate;
    
    mDumpCount = 0;
}

RebalanceTestMultiObj::~RebalanceTestMultiObj() {}

void
RebalanceTestMultiObj::Reset()
{
    MultichannelProcess::Reset();
    
    mDumpCount = 0;
}

void
RebalanceTestMultiObj::Reset(int bufferSize, int overlapping, int oversampling, double sampleRate)
{
    MultichannelProcess::Reset(bufferSize, overlapping, oversampling, sampleRate);
    
    mSampleRate = sampleRate;
    
    mDumpCount = 0;
}

void
RebalanceTestMultiObj::ProcessResultFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                                        const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
#if 0 //DONT_DUMP_EVERY_STEP
    // Don't dump every step.
    // Take an overlap of 50%
    
    // Maybe buggy, see: Rebalance::ComputeAndDump() for fixed version
    mDumpCount++;
    if (mDumpCount < NUM_INPUT_COLS/2)
    {
        for (int k = 0; k < 4; k++)
        {
            mRebalanceDumpFftObjs[k]->ResetSlices();
        }
        
        return;
    }
    mDumpCount = 0;
#endif
    
    //#if TEST_DNN_INPUT
#define NUM_SOURCES 4
    //#endif
#if TEST_DNN_INPUT2
#define NUM_SOURCES 1
#endif
    
    // Similar to ComputeAndDump (a bit)
    vector<RebalanceDumpFftObj::Slice> slices[NUM_SOURCES];
    for (int k = 0; k < NUM_SOURCES; k++)
    {
        // Get spectrogram slice for source
        mRebalanceDumpFftObjs[k]->GetSlices(&slices[k]);
        mRebalanceDumpFftObjs[k]->ResetSlices();
    }
    
    int sliceIdx = slices[0].size() - 1;
    
    vector<WDL_TypedBuf<double> > sourceData[4/*NUM_SOURCES*/];
    vector<WDL_TypedBuf<double> > mixData;
    for (int k = 0; k < NUM_SOURCES; k++)
    {
        const RebalanceDumpFftObj::Slice &slice = slices[k][sliceIdx];
        slice.GetData(&mixData, &sourceData[k]);
    }
    
    Rebalance::Downsample(&mixData, mSampleRate);
    for (int k = 0; k < NUM_SOURCES; k++)
        Rebalance::Downsample(&sourceData[k], mSampleRate);
    
    // Compute binary mask (normalized vs binary)
    // ORIGIN (very raw)
    //Rebalance::BinarizeSourceSlices(sourceData);
    
    Rebalance::NormalizeSourceSlices2(sourceData, mixData);
    
    // Now, recale up the binarized data and set it as output
    //
    
    //WIP: test Rebalance::NormalizeSourceSlicesDB with DB inv
    //(maybe there is a bug in bormalization...) this is strange...
    //TODO: test better the masks computing, maybe normalize before downsample...
    //=> must have very clean masks !
    // TODO: mult by mix and see if we get the correct result
    int vocalIdx = 0;
    int lastIdx = sourceData[vocalIdx].size() - 1;
    RebalanceMaskPredictorComp5::Upsample(&sourceData[vocalIdx][lastIdx], mSampleRate);
    
    if (ioFftSamples->empty())
        return;
    
#if 1 //0 // Output mask
    // NOTE: masks look a bit strange, but the result is good
    // when multiplied by the mix signal
    WDL_TypedBuf<WDL_FFT_COMPLEX> resultData;
    resultData.Resize((*ioFftSamples)[0]->GetSize()/2);
    BLUtils::FillAllZero(&resultData);
    
    for (int i = 0; i < resultData.GetSize(); i++)
    {
        double val = sourceData[vocalIdx][lastIdx].Get()[i];
        
        // "set magn" and ignore phase
        resultData.Get()[i].re = val;
        // .im is 0
    }
#endif
    
#if 0 //1 // Apply mask
    WDL_TypedBuf<WDL_FFT_COMPLEX> resultData = *(*ioFftSamples)[0];
    resultData.Resize(resultData.GetSize()/2);
    BLUtils::MultValues(&resultData, sourceData[vocalIdx][lastIdx]);
#endif
    
    // Fill the result
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = resultData;
    fftSamples.Resize(fftSamples.GetSize()*2);
    BLUtils::FillSecondFftHalf(&fftSamples);
    
    // Result
    *(*ioFftSamples)[0] = fftSamples;
}

#endif
