//
//  SourceComputer.cpp
//  UST
//
//  Created by applematuer on 8/21/19.
//
//

#include <BLVectorscopeProcess.h>

#include <FftProcessObj16.h>
#include <StereoWidthProcessDisp.h>

#include <BLUtils.h>

#include "SourceComputer.h"

// CHECK: maybe it could work well with 1024 buffer ?
#define BUFFER_SIZE 2048

#define OVERSAMPLING 4
#define FREQ_RES 1
#define KEEP_SYNTHESIS_ENERGY 0

// When set to 0, we are accurate for frequencies
#define VARIABLE_HANNING 0

#define OPTIM_SKIP_IFFT 1


SourceComputer::SourceComputer(BL_FLOAT sampleRate)
{
    // Init WDL FFT
    FftProcessObj16::Init();
    
    mFftObj = NULL;
    mStereoWidthProcessDisp = NULL;
    
    Init(OVERSAMPLING, FREQ_RES, sampleRate);
}

SourceComputer::~SourceComputer()
{
    if (mFftObj != NULL)
        delete mFftObj;
    
    if (mStereoWidthProcessDisp != NULL)
        delete mStereoWidthProcessDisp;
}

void
SourceComputer::Reset()
{
    mFftObj->Reset();
    mStereoWidthProcessDisp->Reset();
}

void
SourceComputer::Reset(BL_FLOAT sampleRate)
{
    mFftObj->Reset(BUFFER_SIZE, OVERSAMPLING, FREQ_RES, sampleRate);
    mStereoWidthProcessDisp->Reset(BUFFER_SIZE, OVERSAMPLING, FREQ_RES, sampleRate);
}

void
SourceComputer::ComputePoints(WDL_TypedBuf<BL_FLOAT> samplesIn[2],
                              WDL_TypedBuf<BL_FLOAT> points[2])
{
    vector<WDL_TypedBuf<BL_FLOAT> > in;
    in.resize(2);
    in[0] = samplesIn[0];
    in[1] = samplesIn[1];
    
    vector<WDL_TypedBuf<BL_FLOAT> > scIn;
    vector<WDL_TypedBuf<BL_FLOAT> > out;
    
    out = in;
    
    mFftObj->Process(in, scIn, &out);
    
    WDL_TypedBuf<BL_FLOAT> polarX;
    WDL_TypedBuf<BL_FLOAT> polarY;
    WDL_TypedBuf<BL_FLOAT> colorWeights;
    mStereoWidthProcessDisp->GetWidthValues(&polarX, &polarY, &colorWeights);
    
    points[0] = polarX;
    points[1] = polarY;
}

void
SourceComputer::Init(int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    if (mFftObj == NULL)
    {
        int numChannels = 2;
        int numScInputs = 0;
        
        vector<ProcessObj *> processObjs;
        mFftObj = new FftProcessObj16(processObjs,
                                      numChannels, numScInputs,
                                      BUFFER_SIZE, oversampling, freqRes,
                                      sampleRate);
        
#if OPTIM_SKIP_IFFT
        mFftObj->SetSkipIFft(-1, true);
#endif
        
#if !VARIABLE_HANNING
        mFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                   FftProcessObj16::WindowHanning);
        mFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                    FftProcessObj16::WindowHanning);
#else
        mFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                   FftProcessObj16::WindowVariableHanning);
        mFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                    FftProcessObj16::WindowVariableHanning);
#endif
        
        mFftObj->SetKeepSynthesisEnergy(FftProcessObj16::ALL_CHANNELS,
                                        KEEP_SYNTHESIS_ENERGY);
        
        mStereoWidthProcessDisp = new StereoWidthProcessDisp(BUFFER_SIZE,
                                                             oversampling, freqRes,
                                                             sampleRate);
        
        mFftObj->AddMultichannelProcess(mStereoWidthProcessDisp);
    }
    else
    {
        mFftObj->Reset(BUFFER_SIZE, oversampling, freqRes, sampleRate);
    }
}
