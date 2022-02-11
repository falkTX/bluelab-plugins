//
//  DNNModelKF.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#include <string>
using namespace std;

// For define LSTM
#include <Rebalance_defs.h>

#include "DNNModelFD.h"

DNNModelFD::DNNModelFD() {}

DNNModelFD::~DNNModelFD() {}

bool
DNNModelFD::Load(const char *modelFileName,
                 const char *resourcePath)
{
#ifdef WIN32
    return false;
#endif
    
    char modelFullFileName[2048];
    sprintf(modelFullFileName, "%s/%s", resourcePath, modelFileName);
    mModel = fdeep::load_model(modelFullFileName);
    
    return true;
}

// For WIN32
bool
DNNModelFD::LoadWin(IGraphics *pGraphics, int rcId)
{
#ifdef WIN32
    void *rcBuf;
	long rcSize;
	bool loaded = ((IGraphicsWin *)pGraphics)->LoadWindowsResource(rcId, "RCDATA", &rcBuf, &rcSize);
	if (!loaded)
		return false;
    
    string modelString;
    modelString.resize(rcSize);
    memcpy(modelString.c_str(), rcBuf, rcSize);
    
    mModel = fdeep::read_model_from_string(modelString);
    
	return true;
#endif

    return false;
}

void
DNNModelFD::Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                    WDL_TypedBuf<BL_FLOAT> *output)
{
    fdeep::tensor inTensor(fdeep::tensor_shape(input.GetSize()));
    for (int i = 0; i < input.GetSize(); i++)
    {
        BL_FLOAT val = input.Get()[i];;
        inTensor.set(i, val);
    }
    
    fdeep::tensor outTensor = mModel->predict(inTensor);
    
    output->Resize(NUM_OUTPUT_COLS*BUFFER_SIZE/(RESAMPLE_FACTOR*2));
    for (int i = 0; i < output->GetSize(); i++)
    {
        float val = outTensor.get(i);
        output->Get()[i] = val;
    }
}
