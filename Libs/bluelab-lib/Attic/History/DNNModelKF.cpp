//
//  DNNModelKF.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

// For define LSTM
#include <Rebalance_defs.h>

#include "DNNModelKF.h"

DNNModelKF::DNNModelKF() {}

DNNModelKF::~DNNModelKF() {}

bool
DNNModelKF::Load(const char *modelFileName,
                 const char *resourcePath)
{
#ifdef WIN32
    return false;
#endif
    
    char modelFullFileName[2048];
    sprintf(modelFullFileName, "%s/%s", resourcePath, modelFileName);
    mModel = pt::Model::create(modelFullFileName);
    
    return true;
}

// For WIN32
bool
DNNModelKF::LoadWin(IGraphics *pGraphics, int rcId)
{
#ifdef WIN32
    void *rcBuf;
	long rcSize;
	bool loaded = ((IGraphicsWin *)pGraphics)->LoadWindowsResource(rcId, "RCDATA", &rcBuf, &rcSize);
	if (!loaded)
		return false;
    
    mModel = pt::Model::create(rcBuf, rcSize);
    
	return true;
#endif

    return false;
}

void
DNNModelKF::Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                    WDL_TypedBuf<BL_FLOAT> *output)
{
#if !LSTM
    pt::Tensor in(input.GetSize());
    for (int i = 0; i < input.GetSize(); i++)
    {
        in(i) = input.Get()[i];
    }
#else
    pt::Tensor in(1, input.GetSize());
    for (int i = 0; i < input.GetSize(); i++)
    {
        in(0, i) = input.Get()[i];
    }
#endif
    
    pt::Tensor out;
    mModel->predict(in, out);
    
    output->Resize(NUM_OUTPUT_COLS*BUFFER_SIZE/(RESAMPLE_FACTOR*2));
    
    for (int i = 0; i < output->GetSize(); i++)
    {
        output->Get()[i] = out(i);
    }
}
