//
//  RebalanceMaskPredictor.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#include <string>
using namespace std;

#include <SoftMaskingN.h>

#include <BLUtils.h>

#include "RebalanceMaskPredictor.h"

RebalanceMaskPredictor::RebalanceMaskPredictor(int bufferSize,
                                               BL_FLOAT overlapping, BL_FLOAT oversampling,
                                               BL_FLOAT sampleRate,
                                               IGraphics *graphics)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
#if FORCE_SAMPLE_RATE
    mPlugSampleRate = sampleRate;
#endif
    
    // DNN
    
#ifndef WIN32
    WDL_String resPath;
    graphics->GetResourceDir(&resPath);
    const char *resourcePath = resPath.Get();
    
    // Vocal
    char modelVocalFileName[2048];
    sprintf(modelVocalFileName, "%s/%s", resourcePath, MODEL_VOCAL);
    mModelVocal = pt::Model::create(modelVocalFileName);
    
    // Bass
    char modelBassFileName[2048];
    sprintf(modelBassFileName, "%s/%s", resourcePath, MODEL_BASS);
    mModelBass = pt::Model::create(modelBassFileName);
    
    // Drums
    char modelDrumsFileName[2048];
    sprintf(modelDrumsFileName, "%s/%s", resourcePath, MODEL_DRUMS);
    mModelDrums = pt::Model::create(modelDrumsFileName);
    
    // Other
    char modelOtherFileName[2048];
    sprintf(modelOtherFileName, "%s/%s", resourcePath, MODEL_OTHER);
    mModelOther = pt::Model::create(modelOtherFileName);
#else // WIN32
    LoadModelWin(graphics, MODEL0_ID, &mModelVocal);
    LoadModelWin(graphics, MODEL1_ID, &mModelBass);
    LoadModelWin(graphics, MODEL2_ID, &mModelDrums);
    LoadModelWin(graphics, MODEL3_ID, &mModelOther);
#endif
    
    for (int i = 0; i < NUM_INPUT_COLS; i++)
    {
        WDL_TypedBuf<BL_FLOAT> col;
        BLUtils::ResizeFillZeros(&col, bufferSize/(2*RESAMPLE_FACTOR));
        
        mMixCols.push_back(col);
    }
    
#if FORCE_SAMPLE_RATE
    InitResamplers();
    
    mRemainingSamples[0] = 0.0;
    mRemainingSamples[1] = 0.0;
#endif
    
#if USE_SOFT_MASK_N
    mSoftMasking = new SoftMaskingN(SOFT_MASK_HISTO_SIZE);
    mUseSoftMasks = true;
#endif
}

RebalanceMaskPredictor::~RebalanceMaskPredictor()
{
#if USE_SOFT_MASK_N
    delete mSoftMasking;
#endif
}

void
RebalanceMaskPredictor::Reset()
{
    Reset(mBufferSize, mOverlapping, mOversampling, mSampleRate);
    
#if FORCE_SAMPLE_RATE
    for (int i = 0; i < 2; i++)
    {
        mResamplers[i].Reset();
        // set input and output samplerates
        mResamplers[i].SetRates(mPlugSampleRate, SAMPLE_RATE);
    }
    
    mRemainingSamples[0] = 0.0;
    mRemainingSamples[1] = 0.0;
#endif
    
#if USE_SOFT_MASK_N
    mSoftMasking->Reset();
#endif
}

void
RebalanceMaskPredictor::Reset(int bufferSize, int overlapping,
                              int oversampling, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
#if FORCE_SAMPLE_RATE
    mRemainingSamples[0] = 0.0;
    mRemainingSamples[1] = 0.0;
#endif
    
#if USE_SOFT_MASK_N
    mSoftMasking->Reset();
#endif
}

#if 1 // GOOD (even for 192000Hz, block size 64)
#if FORCE_SAMPLE_RATE
void
RebalanceMaskPredictor::ProcessInputSamplesPre(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                               const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer)
{
    if (mPlugSampleRate == SAMPLE_RATE)
        return;
    
    BL_FLOAT sampleRate = mPlugSampleRate;
    for (int i = 0; i < 2; i++)
    {
        if (i >= ioSamples->size())
            break;
        
        WDL_ResampleSample *resampledAudio = NULL;
        int desiredSamples = (*ioSamples)[i]->GetSize(); // Input driven
        
        // GOOD
        // Without: spectrogram scalled horizontally
#if FIX_ADJUST_IN_RESAMPLING
        // Adjust
        if (mRemainingSamples[i] >= 1.0)
        {
            int subSamples = floor(mRemainingSamples[i]);
            mRemainingSamples[i] -= subSamples;
            
            desiredSamples -= subSamples;
        }
#endif
        
        int numSamples = mResamplers[i].ResamplePrepare(desiredSamples, 1, &resampledAudio);
        
        for (int j = 0; j < numSamples; j++)
        {
            if (j >= (*ioSamples)[i]->GetSize())
                break;
            resampledAudio[j] = (*ioSamples)[i]->Get()[j];
        }
        
        WDL_TypedBuf<BL_FLOAT> outSamples;
        outSamples.Resize(desiredSamples);
        int numResampled = mResamplers[i].ResampleOut(outSamples.Get(),
                                                      // Must be exactly the value returned by ResamplePrepare
                                                      // Otherwise the spectrogram could be scaled horizontally
                                                      // (or clicks)
                                                      // Due to flush of the resampler
                                                      numSamples,
                                                      outSamples.GetSize(), 1);
        
        // GOOD !
        // Avoid clicks sometimes (for example with 88200Hz and buffer size 447)
        // The numResampled varies around a value, to keep consistency of the stream
        outSamples.Resize(numResampled);
        
        *((*ioSamples)[i]) = outSamples;
        
#if FIX_ADJUST_IN_RESAMPLING
        // Adjust
        //BL_FLOAT remaining = desiredSamples - numResampled*sampleRate/SAMPLE_RATE;
        BL_FLOAT remaining = numResampled*sampleRate/SAMPLE_RATE - desiredSamples;
        mRemainingSamples[i] += remaining;
#endif
    }
}
#endif
#endif

void
RebalanceMaskPredictor::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                                        const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() < 1)
        return;
    
    // Take only the left channel...
    //
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = *(*ioFftSamples)[0];
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    
    BLUtils::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
#if USE_SOFT_MASK_N
    mCurrentMagns = magns;
#endif
    
    // Compute the masks
    WDL_TypedBuf<BL_FLOAT> magnsDown = magns;
    Downsample(&magnsDown, mSampleRate);
    
    // mMixCols is filled with zeros at the origin
    mMixCols.push_back(magnsDown);
    mMixCols.pop_front();
    
    WDL_TypedBuf<BL_FLOAT> mixBufHisto;
    ColumnsToBuffer(&mixBufHisto, mMixCols);
    
    ComputeMasks(&mMaskVocal, &mMaskBass, &mMaskDrums, &mMaskOther, mixBufHisto);
}

void
RebalanceMaskPredictor::GetMaskVocal(WDL_TypedBuf<BL_FLOAT> *maskVocal)
{
    *maskVocal = mMaskVocal;
}

void
RebalanceMaskPredictor::GetMaskBass(WDL_TypedBuf<BL_FLOAT> *maskBass)
{
    *maskBass = mMaskBass;
}

void
RebalanceMaskPredictor::GetMaskDrums(WDL_TypedBuf<BL_FLOAT> *maskDrums)
{
    *maskDrums = mMaskDrums;
}

void
RebalanceMaskPredictor::GetMaskOther(WDL_TypedBuf<BL_FLOAT> *maskOther)
{
    *maskOther = mMaskOther;
}

void
RebalanceMaskPredictor::Downsample(WDL_TypedBuf<BL_FLOAT> *ioBuf,
                                   BL_FLOAT sampleRate)
{
#if FORCE_SAMPLE_RATE
    sampleRate = SAMPLE_RATE;
#endif
    
    BL_FLOAT hzPerBin = sampleRate/(BUFFER_SIZE/2);
    WDL_TypedBuf<BL_FLOAT> bufMel;
    BLUtils::FreqsToMelNorm(&bufMel, *ioBuf, hzPerBin);
    *ioBuf = bufMel;
    
    if (RESAMPLE_FACTOR == 1)
        // No need to downsample
        return;
    
    int newSize = ioBuf->GetSize()/RESAMPLE_FACTOR;
    
    BLUtils::ResizeLinear(ioBuf, newSize);
}

void
RebalanceMaskPredictor::Upsample(WDL_TypedBuf<BL_FLOAT> *ioBuf,
                                 BL_FLOAT sampleRate)
{
#if FORCE_SAMPLE_RATE
    sampleRate = SAMPLE_RATE;
#endif
    
    if (RESAMPLE_FACTOR != 1)
    {
        int newSize = ioBuf->GetSize()*RESAMPLE_FACTOR;
        
        BLUtils::ResizeLinear(ioBuf, newSize);
    }
    
    BL_FLOAT hzPerBin = sampleRate/(BUFFER_SIZE/2);
    WDL_TypedBuf<BL_FLOAT> bufMel;
    BLUtils::MelToFreqsNorm(&bufMel, *ioBuf, hzPerBin);
    *ioBuf = bufMel;
}

#if FORCE_SAMPLE_RATE
void
RebalanceMaskPredictor::SetPlugSampleRate(BL_FLOAT sampleRate)
{
    mPlugSampleRate = sampleRate;
    
    InitResamplers();
}
#endif

#if USE_SOFT_MASK_N
void
RebalanceMaskPredictor::SetUseSoftMasks(bool flag)
{
    mUseSoftMasks = flag;
    
    Reset();
}
#endif

#ifdef WIN32
bool
RebalanceMaskPredictor::LoadModelWin(IGraphics *pGraphics, int rcId,
                                     std::unique_ptr<pt::Model> *model)
{
    void *rcBuf;
	long rcSize;
	bool loaded = ((IGraphicsWin *)pGraphics)->LoadWindowsResource(rcId, "RCDATA", &rcBuf, &rcSize);
	if (!loaded)
		return false;
    
    *model = pt::Model::create(rcBuf, rcSize);
    
	return true;
}
#endif

// Split the mask cols, get each mask and upsample it
void
RebalanceMaskPredictor::UpsamplePredictedMask(WDL_TypedBuf<BL_FLOAT> *ioBuf)
{
    if (RESAMPLE_FACTOR == 1)
        // No need to upsample
        return;
    
    WDL_TypedBuf<BL_FLOAT> result;
    
    for (int j = 0; j < NUM_OUTPUT_COLS; j++)
    {
        WDL_TypedBuf<BL_FLOAT> mask;
        mask.Resize(BUFFER_SIZE/(2*RESAMPLE_FACTOR));
        
        // Optim
        memcpy(mask.Get(),
               &ioBuf->Get()[j*mask.GetSize()],
               mask.GetSize()*sizeof(BL_FLOAT));
        
        Upsample(&mask, mSampleRate);
        
        result.Add(mask.Get(), mask.GetSize());
    }
    
    *ioBuf = result;
}

void
RebalanceMaskPredictor::PredictMask(WDL_TypedBuf<BL_FLOAT> *result,
                                    const WDL_TypedBuf<BL_FLOAT> &mixBufHisto,
                                    std::unique_ptr<pt::Model> &model)
{
#if !LSTM
    pt::Tensor in(mixBufHisto.GetSize());
    for (int i = 0; i < mixBufHisto.GetSize(); i++)
    {
        in(i) = mixBufHisto.Get()[i];
    }
#else
    pt::Tensor in(1, mixBufHisto.GetSize());
    for (int i = 0; i < mixBufHisto.GetSize(); i++)
    {
        in(0, i) = mixBufHisto.Get()[i];
    }
#endif
    
    pt::Tensor out;
    model->predict(in, out);
    
    WDL_TypedBuf<BL_FLOAT> maskBuf;
    maskBuf.Resize(NUM_OUTPUT_COLS*BUFFER_SIZE/(RESAMPLE_FACTOR*2));
    
    for (int i = 0; i < maskBuf.GetSize(); i++)
    {
        maskBuf.Get()[i] = out(i);
    }
    
    BLUtils::ClipMin(&maskBuf, 0.0);
    
    UpsamplePredictedMask(&maskBuf);
    
    BLUtils::ClipMin(&maskBuf, 0.0);
    
    *result = maskBuf;
}

void
RebalanceMaskPredictor::ColumnsToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                                        const deque<WDL_TypedBuf<BL_FLOAT> > &cols)
{
    if (cols.empty())
        return;
    
    buf->Resize(cols.size()*cols[0].GetSize());
    
    for (int j = 0; j < cols.size(); j++)
    {
        const WDL_TypedBuf<BL_FLOAT> &col = cols[j];
        for (int i = 0; i < col.GetSize(); i++)
        {
            int bufIndex = i + j*col.GetSize();
            
            buf->Get()[bufIndex] = col.Get()[i];
        }
    }
}

void
RebalanceMaskPredictor::ComputeMasks(WDL_TypedBuf<BL_FLOAT> *maskVocal,
                                     WDL_TypedBuf<BL_FLOAT> *maskBass,
                                     WDL_TypedBuf<BL_FLOAT> *maskDrums,
                                     WDL_TypedBuf<BL_FLOAT> *maskOther,
                                     const WDL_TypedBuf<BL_FLOAT> &mixBufHisto)
{
    WDL_TypedBuf<BL_FLOAT> maskVocalFull;
    PredictMask(&maskVocalFull, mixBufHisto, mModelVocal);
    
    WDL_TypedBuf<BL_FLOAT> maskBassFull;
    PredictMask(&maskBassFull, mixBufHisto, mModelBass);
    
    WDL_TypedBuf<BL_FLOAT> maskDrumsFull;
    PredictMask(&maskDrumsFull, mixBufHisto, mModelDrums);
    
    WDL_TypedBuf<BL_FLOAT> maskOtherFull;
    PredictMask(&maskOtherFull, mixBufHisto, mModelOther);
    
#if USE_SOFT_MASK_N
    if (mUseSoftMasks)
    {
        ComputeLineMasksSoft(maskVocal, maskVocalFull, maskBass, maskBassFull,
                             maskDrums, maskDrumsFull, maskOther, maskOtherFull);
        
        return;
    }
#endif
    
    // ORIGIN: Not using soft masks
    ComputeLineMask(maskVocal, maskVocalFull);
    ComputeLineMask(maskBass, maskBassFull);
    ComputeLineMask(maskDrums, maskDrumsFull);
    ComputeLineMask(maskOther, maskOtherFull);
}

void
RebalanceMaskPredictor::ComputeLineMask(WDL_TypedBuf<BL_FLOAT> *maskResult,
                                        const WDL_TypedBuf<BL_FLOAT> &maskSource)
{
    int numFreqs = BUFFER_SIZE/2;
    int numCols = NUM_OUTPUT_COLS;
    
    maskResult->Resize(numFreqs);
    
    for (int i = 0; i < numFreqs; i++)
    {
        BL_FLOAT avg = 0.0;
        for (int j = 0; j < numCols; j++)
        {
            int idx = i + j*numFreqs;
            
            BL_FLOAT m = maskSource.Get()[idx];
            avg += m;
        }
        
        avg /= numCols;
        
        maskResult->Get()[i] = avg;
    }
}

#if USE_SOFT_MASK_N
void
RebalanceMaskPredictor::ComputeLineMask2(WDL_TypedBuf<BL_FLOAT> *maskResult,
                                         const WDL_TypedBuf<BL_FLOAT> &maskSource)
{
    int numFreqs = BUFFER_SIZE/2;
    int numCols = NUM_OUTPUT_COLS;
    
    maskResult->Resize(numFreqs);
    
    for (int i = 0; i < numFreqs; i++)
    {
        int idx = i + (numCols - 1)*numFreqs;
        
        BL_FLOAT m = maskSource.Get()[idx];
        
        maskResult->Get()[i] = m;
    }
}

void
RebalanceMaskPredictor::ComputeLineMasksSoft(WDL_TypedBuf<BL_FLOAT> *maskVocal,
                                             const WDL_TypedBuf<BL_FLOAT> &maskVocalFull,
                                             WDL_TypedBuf<BL_FLOAT> *maskBass,
                                             const WDL_TypedBuf<BL_FLOAT> &maskBassFull,
                                             WDL_TypedBuf<BL_FLOAT> *maskDrums,
                                             const WDL_TypedBuf<BL_FLOAT> &maskDrumsFull,
                                             WDL_TypedBuf<BL_FLOAT> *maskOther,
                                             const WDL_TypedBuf<BL_FLOAT> &maskOtherFull)
{
    // Compute the lines
    ComputeLineMask2(maskVocal, maskVocalFull);
    ComputeLineMask2(maskBass, maskBassFull);
    ComputeLineMask2(maskDrums, maskDrumsFull);
    ComputeLineMask2(maskOther, maskOtherFull);
    
    if (mMixCols.empty())
        return;
    
    const WDL_TypedBuf<BL_FLOAT> &mix = mCurrentMagns;
    
    vector<WDL_TypedBuf<BL_FLOAT> > estimMagns;
    estimMagns.push_back(*maskVocal);
    estimMagns.push_back(*maskBass);
    estimMagns.push_back(*maskDrums);
    estimMagns.push_back(*maskOther);
    
    // Multiply the masks by the magnitudes, to have the estimation
    for (int i = 0; i < 4; i++)
    {
        BLUtils::MultValues(&estimMagns[i], mix);
    }
    
    vector<WDL_TypedBuf<BL_FLOAT> > softMasks;
    mSoftMasking->Process(mix, estimMagns, &softMasks);
    
    *maskVocal = softMasks[0];
    *maskBass = softMasks[1];
    *maskDrums = softMasks[2];
    *maskOther = softMasks[3];
}
#endif

#if FORCE_SAMPLE_RATE
void
RebalanceMaskPredictor::InitResamplers()
{
    // In
    
    for (int i = 0; i < 2; i++)
    {
        mResamplers[i].Reset(); //
        
        mResamplers[i].SetMode(true, 1, false, 0, 0);
        mResamplers[i].SetFilterParms();
        
        // GOOD !
        // Set input driven
        // (because output driven has a bug when downsampling:
        // the first samples are bad)
        mResamplers[i].SetFeedMode(true);
        
        // set input and output samplerates
        mResamplers[i].SetRates(mPlugSampleRate, SAMPLE_RATE);
    }
}
#endif
