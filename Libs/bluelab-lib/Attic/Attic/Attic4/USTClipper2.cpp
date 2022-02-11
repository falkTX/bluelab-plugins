//
//  USTClipper2.cpp
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#include <USTClipperDisplay2.h>
#include <OversampProcessObj.h>
#include <BLUtils.h>

#include "USTClipper2.h"

// See: https://www.siraudiotools.com/StandardCLIP_manual.php
#define OVERSAMPLING 8 //4

class ClipperOverObj2 : public OversampProcessObj
{
public:
    ClipperOverObj2(int oversampling, BL_FLOAT sampleRate);
    
    virtual ~ClipperOverObj2();
    
    void Reset(BL_FLOAT sampleRate);
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void SetClipValue(BL_FLOAT clipValue);
    
protected:
    BL_FLOAT mClipValue;
    
    WDL_TypedBuf<BL_FLOAT> mCopyBuffer;
};

ClipperOverObj2::ClipperOverObj2(int oversampling, BL_FLOAT sampleRate)
: OversampProcessObj(oversampling, sampleRate)
{
    mClipValue = 2.0;
}

ClipperOverObj2::~ClipperOverObj2() {}

void
ClipperOverObj2::Reset(BL_FLOAT sampleRate)
{
    OversampProcessObj::Reset(sampleRate);
}

void
ClipperOverObj2::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mCopyBuffer.GetSize() != ioBuffer->GetSize())
        mCopyBuffer.Resize(ioBuffer->GetSize());
    
    for (int i = 0 ; i < ioBuffer->GetSize(); i++)
    {
        BL_FLOAT input = ioBuffer->Get()[i];
        BL_FLOAT output = 0.0;
        
        USTClipper2::ComputeClipping(input, &output, mClipValue);
        
        ioBuffer->Get()[i] = output;
    }
}

void
ClipperOverObj2::SetClipValue(BL_FLOAT clipValue)
{
    mClipValue = clipValue;
}


USTClipper2::USTClipper2(GraphControl11 *graph, BL_FLOAT sampleRate)
{
    mClipperDisplay = new USTClipperDisplay2(graph);
    
    for (int i = 0; i < 2; i++)
        mClipObjs[i] = new ClipperOverObj2(OVERSAMPLING, sampleRate);
}

USTClipper2::~USTClipper2()
{
    delete mClipperDisplay;
    
    for (int i = 0; i < 2; i++)
        delete mClipObjs[i];
}

void
USTClipper2::Reset(BL_FLOAT sampleRate)
{
    for (int i = 0; i < 2; i++)
        mClipObjs[i]->Reset(sampleRate);
}

void
USTClipper2::SetClipValue(BL_FLOAT clipValue)
{
    mClipValue = clipValue;
    
    mClipperDisplay->SetClipValue(clipValue);
    
    for (int i = 0; i < 2; i++)
        mClipObjs[i]->SetClipValue(clipValue);
}

void
USTClipper2::Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples)
{
    if (ioSamples->size() != 2)
        return;
    
    // Display
    // Mono in
    WDL_TypedBuf<BL_FLOAT> monoIn;
    BLUtils::StereoToMono(&monoIn, (*ioSamples)[0], (*ioSamples)[1]);
    mClipperDisplay->AddSamples(monoIn);
    
    // Process
    for (int i = 0; i < 2; i++)
    {
        if (i >= ioSamples->size())
            continue;
        
        mClipObjs[i]->ProcessSamplesBuffer(&(*ioSamples)[i]);
    }
    
    WDL_TypedBuf<BL_FLOAT> monoOut;
    BLUtils::StereoToMono(&monoOut, (*ioSamples)[0], (*ioSamples)[1]);
    mClipperDisplay->AddClippedSamples(monoOut);
}

void
USTClipper2::ComputeClipping(BL_FLOAT inSample, BL_FLOAT *outSample, BL_FLOAT clipValue)
{
    clipValue = 2.0 - clipValue;
    
    *outSample = inSample;
    
    if (inSample > clipValue)
        *outSample = clipValue;
    
    if (inSample < -clipValue)
        *outSample = -clipValue;
}
