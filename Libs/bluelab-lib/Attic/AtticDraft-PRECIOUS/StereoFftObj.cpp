//
//  StereoFftObj.cpp
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#include "StereoFftObj.h"

StereoFftObj::StereoFftObj(FftObj *objs[2])
{
    mObjs[0] = objs[0];
    mObjs[1] = objs[1];
}

StereoFftObj::~StereoFftObj() {}

void
StereoFftObj::Reset(int oversampling, int freqRes)
{
    for (int i = 0; i < 2; i++)
        mObjs[i]->Reset(oversampling, freqRes);
}

bool
StereoFftObj::Process(double *input, double *output, double *inSc, int nFrames)
{
    
}

void
StereoFftObj::PreProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer)
{
    
}

void
StereoFftObj::PostProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer)
{
    
}


//virtual void SetTrackIO(int maxNumPoints, double decimFactor,
//                        bool trackInput, bool trackOutput);

//virtual void GetCurrentInput(WDL_TypedBuf<double> *outInput);

//virtual void GetCurrentOutput(WDL_TypedBuf<double> *outOutput);
