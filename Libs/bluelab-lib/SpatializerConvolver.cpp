#include <SpatializerConvolver.h>

SpatializerConvolver::SpatializerConvolver(int bufferSize)
: FftConvolver6(bufferSize, true, true, false),
  mResponse(NULL) {}

void
SpatializerConvolver::Reset()
{
    FftConvolver6::Reset();
}

void
SpatializerConvolver::Reset(int oversampling, int freqRes)
{
    FftConvolver6::Reset();
}

void
SpatializerConvolver::SetBufferSize(int bufferSize)
{
#if TEST_CONVOLVER_VARIABLE_BUFFER
    FftConvolver6::Reset(bufferSize);
#endif
}

void
SpatializerConvolver::Flush()
{
    FftConvolver6::Flush();
}
       
void
SpatializerConvolver::SetParameter(void *param)
{
    WDL_TypedBuf<BL_FLOAT> *response = (WDL_TypedBuf<BL_FLOAT> *)param;
  
    if (mResponse != NULL)
        delete mResponse;
  
    // Stock a copy of the response in the memeber variable
    // So the pointer will still be valid
    mResponse = new WDL_TypedBuf<BL_FLOAT>();
    *mResponse = *response;
  
    FftConvolver6::SetResponse(mResponse);
}


void *
SpatializerConvolver::GetParameter()
{
    return (void *)mResponse;
}

bool
SpatializerConvolver::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    return FftConvolver6::Process(input, output, nFrames);
}
