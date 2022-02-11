//
//  USTClipper.cpp
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <USTClipperDisplay.h>
#include <OversampProcessObj.h>
#include <BLUtils.h>

#include "USTClipper.h"

#define OVERSAMPLING 4

class ClipperOverObj : public OversampProcessObj
{
public:
    ClipperOverObj(int oversampling, BL_FLOAT sampleRate);
    
    virtual ~ClipperOverObj();
    
    void Reset(BL_FLOAT sampleRate);
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void SetClipValue(BL_FLOAT clipValue);
    
protected:
    BL_FLOAT mClipValue;
    
    WDL_TypedBuf<BL_FLOAT> mCopyBuffer;
};

ClipperOverObj::ClipperOverObj(int oversampling, BL_FLOAT sampleRate)
: OversampProcessObj(oversampling, sampleRate)
{
    mClipValue = 2.0;
}

ClipperOverObj::~ClipperOverObj() {}

void
ClipperOverObj::Reset(BL_FLOAT sampleRate)
{
    OversampProcessObj::Reset(sampleRate);
}

void
ClipperOverObj::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mCopyBuffer.GetSize() != ioBuffer->GetSize())
        mCopyBuffer.Resize(ioBuffer->GetSize());
    
    for (int i = 0 ; i < ioBuffer->GetSize(); i++)
    {
        BL_FLOAT input = ioBuffer->Get()[i];
        BL_FLOAT output = 0.0;
        
        USTClipper::ComputeClipping(input, &output, mClipValue);
        
        ioBuffer->Get()[i] = output;
    }
}

void
ClipperOverObj::SetClipValue(BL_FLOAT clipValue)
{
    mClipValue = clipValue;
}


USTClipper::USTClipper(GraphControl11 *graph, BL_FLOAT sampleRate)
{
    mClipperDisplay = new USTClipperDisplay(graph);
    
    for (int i = 0; i < 2; i++)
        mClipObjs[i] = new ClipperOverObj(OVERSAMPLING, sampleRate);
}

USTClipper::~USTClipper()
{
    delete mClipperDisplay;
    
    for (int i = 0; i < 2; i++)
        delete mClipObjs[i];
}

void
USTClipper::Reset(BL_FLOAT sampleRate)
{
    for (int i = 0; i < 2; i++)
        mClipObjs[i]->Reset(sampleRate);
}

void
USTClipper::SetClipValue(BL_FLOAT clipValue)
{
    mClipValue = clipValue;
    
    mClipperDisplay->SetClipValue(clipValue);
    
    for (int i = 0; i < 2; i++)
        mClipObjs[i]->SetClipValue(clipValue);
}

void
USTClipper::Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples)
{
    if (ioSamples->size() != 2)
        return;
    
    // Display
    WDL_TypedBuf<BL_FLOAT> mono;
    BLUtils::StereoToMono(&mono, (*ioSamples)[0], (*ioSamples)[1]);
    mClipperDisplay->AddSamples(mono);
    
    // Process
    for (int i = 0; i < 2; i++)
    {
        if (i >= ioSamples->size())
            continue;
        
        mClipObjs[i]->ProcessSamplesBuffer(&(*ioSamples)[i]);
    }
}

void
USTClipper::ComputeClipping(BL_FLOAT inSample, BL_FLOAT *outSample, BL_FLOAT clipValue)
{
    clipValue = 2.0 - clipValue;
    
    *outSample = inSample;
    
    if (inSample > clipValue)
        *outSample = clipValue;
    
    if (inSample < -clipValue)
        *outSample = -clipValue;
}

#endif // IGRAPHICS_NANOVG
