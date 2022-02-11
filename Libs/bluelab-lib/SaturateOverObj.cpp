#include <SaturateOverObj.h>

SaturateOverObj::SaturateOverObj(int oversampling, BL_FLOAT sampleRate)
: OversampProcessObj(oversampling, sampleRate)
{
    mRatio = 0.0;
}

SaturateOverObj::~SaturateOverObj() {}

void
SaturateOverObj::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mCopyBuffer.GetSize() != ioBuffer->GetSize())
        mCopyBuffer.Resize(ioBuffer->GetSize());
    
    for (int i = 0 ; i < ioBuffer->GetSize(); i++)
    {
        BL_FLOAT input = ioBuffer->Get()[i];
        BL_FLOAT output = 0.0;
        
        ComputeSaturation(input, &output, mRatio);
        
        ioBuffer->Get()[i] = output;
    }
}

void
SaturateOverObj::SetRatio(BL_FLOAT ratio)
{
    mRatio = ratio;
}

// From Saturate plugin
void
SaturateOverObj::ComputeSaturation(BL_FLOAT inSample,
                                   BL_FLOAT *outSample,
                                   BL_FLOAT ratio)
{
    BL_FLOAT cut = 1.0 - ratio;
    BL_FLOAT factor = 1.0/(1.0 - ratio);
    
    *outSample = inSample;
    
    if (inSample > cut)
        *outSample = cut;
    
    if (inSample < -cut)
        *outSample = -cut;
    
    *outSample *= factor;
}
