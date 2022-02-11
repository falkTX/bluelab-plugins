//
//  FilterFftLowPass.cpp
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#include <FftProcessObj16.h>

#include <BLUtils.h>

#include "FilterFftLowPass.h"

#define OVERSAMPLING 4

class FftLowPassProcess : public ProcessObj
{
public:
    FftLowPassProcess(int bufferSize,
                      BL_FLOAT overlapping, BL_FLOAT oversampling,
                      BL_FLOAT sampleRate);
    
    virtual ~FftLowPassProcess();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping, int oversampling, BL_FLOAT sampleRate);
    
    // Set cut freq
    void SetCutFreq(BL_FLOAT cutFreq);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
protected:
    int mBufferSize;
    BL_FLOAT mOverlapping;
    BL_FLOAT mOversampling;
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mCutFreq;
};

FftLowPassProcess::FftLowPassProcess(int bufferSize,
                                     BL_FLOAT overlapping, BL_FLOAT oversampling,
                                     BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    mSampleRate = sampleRate;
    
    //
    mCutFreq = 0.0;
}

FftLowPassProcess::~FftLowPassProcess() {}

void
FftLowPassProcess::Reset()
{
    Reset(mBufferSize, mOverlapping, mOversampling, mSampleRate);
}

void
FftLowPassProcess::Reset(int bufferSize, int overlapping,
                         int oversampling, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    mSampleRate = sampleRate;
}

void
FftLowPassProcess::SetCutFreq(BL_FLOAT cutFreq)
{
    mCutFreq = cutFreq;
}

void
FftLowPassProcess::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                    const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    BLUtils::TakeHalf(ioBuffer);
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    // Last bin to be kept, after, fill with zeros
    int lastBin = ceil(mCutFreq/hzPerBin);
    
    if (lastBin < ioBuffer->GetSize())
    {
        for (int i = lastBin; i < ioBuffer->GetSize(); i++)
        {
            WDL_FFT_COMPLEX &c = ioBuffer->Get()[i];
            c.re = 0.0;
            c.im = 0.0;
        }
    }
    
    ioBuffer->Resize(ioBuffer->GetSize()*2);
    BLUtils::FillSecondFftHalf(ioBuffer);
}

//
//

FilterFftLowPass::FilterFftLowPass()
{
    mFftObj = NULL;
    mProcessObj = NULL;
    
    mPrevSampleRate = -1.0;
    
    mBlockSize = 1024;
}

FilterFftLowPass::~FilterFftLowPass()
{
    if (mFftObj != NULL)
        delete mFftObj;
    
    if (mProcessObj != NULL)
        delete mProcessObj;
}

void
FilterFftLowPass::Init(BL_FLOAT fc, BL_FLOAT sampleRate, int fftSize)
{
    mFC = fc;
    mFftSize = fftSize;
    
    if (mFftObj != NULL)
        delete mFftObj;
    
    if (mProcessObj != NULL)
        delete mProcessObj;
    
    int numChannels = 1;
    int numScInputs = 0;
        
    vector<ProcessObj *> processObjs;
    mProcessObj = new FftLowPassProcess(fftSize, OVERSAMPLING, 1, sampleRate);
    
    mProcessObj->SetCutFreq(fc);
            
    processObjs.push_back(mProcessObj);
    
    //
    mFftObj = new FftProcessObj16(processObjs,
                                  numChannels, numScInputs,
                                  fftSize, OVERSAMPLING, 1, sampleRate);

    mFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                               FftProcessObj16::WindowHanning);
    
    mFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                FftProcessObj16::WindowHanning);
    
    //mFftObj->SetKeepSynthesisEnergy(FftProcessObj16::ALL_CHANNELS,
    //                                KEEP_SYNTHESIS_ENERGY);
    
    mPrevSampleRate = sampleRate;
}

void
FilterFftLowPass::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mBlockSize = blockSize;
    
    if (sampleRate != mPrevSampleRate)
        Init(mFC, sampleRate, mFftSize);
    else
        mFftObj->Reset();
}

int
FilterFftLowPass::GetLatency()
{
    if (mFftObj != NULL)
    {
        int latency = mFftObj->ComputeLatency(mBlockSize);
        
        return latency;
    }
    
    return 0;
}

void
FilterFftLowPass::Process(WDL_TypedBuf<BL_FLOAT> *result,
                          const WDL_TypedBuf<BL_FLOAT> &samples)
{
    vector<WDL_TypedBuf<BL_FLOAT> > inBufs;
    inBufs.push_back(samples);
    
    vector<WDL_TypedBuf<BL_FLOAT> > scInBufs;
    
    vector<WDL_TypedBuf<BL_FLOAT> > outBufs = inBufs;
    mFftObj->Process(inBufs, scInBufs, &outBufs);
    
    if (outBufs.size() >= 1)
        *result = outBufs[0];
}
