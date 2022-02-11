//
//  DNNModelKF.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

// For define LSTM
#include <Rebalance_defs.h>

#include <BLUtils.h>

#include <PPMFile.h>

#include "DNNModelCNN.h"


DNNModelCNN::DNNModelCNN() {}

DNNModelCNN::~DNNModelCNN() {}

bool
DNNModelCNN::Load(const char *modelFileName,
                  const char *resourcePath)
{
#ifdef WIN32
    return false;
#endif
    
    char modelFullFileName[2048];
    sprintf(modelFullFileName, "%s/%s", resourcePath, modelFileName);
    mModel.load(modelFullFileName);
    
    // Optionally, indicate which input tensors should be converted from unsigned chars to floats in the beginning.
    //mModel.setInputUInt8(0);
    
    mNN.compile(mModel);
    
    return true;
}

// For WIN32
bool
DNNModelCNN::LoadWin(IGraphics *pGraphics, int rcId)
{
#ifdef WIN32
    void *rcBuf;
	long rcSize;
	bool loaded = ((IGraphicsWin *)pGraphics)->LoadWindowsResource(rcId, "RCDATA", &rcBuf, &rcSize);
	if (!loaded)
		return false;
    
    TODO
    
	return true;
#endif

    return false;
}

void
DNNModelCNN::Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                     WDL_TypedBuf<BL_FLOAT> *output)
{
#if 0 //RESHAPE_32_X_32
#define IN_COEFF 64.0
    
    PPMFile::SavePPM("input0.ppm", input.Get(),
                     256, 4,/*4, 256,*/ 1, 255.0*IN_COEFF);
    
    WDL_TypedBuf<BL_FLOAT> &input0 = (WDL_TypedBuf<BL_FLOAT> &)input;
    BLUtils::Reshape(&input0, /*4, 256,*/256, 4, 32, 32);
    
    PPMFile::SavePPM("input1.ppm", input.Get(),
                     32, 32, 1, 255.0*IN_COEFF);
    
    // Revert transform
    //WDL_TypedBuf<BL_FLOAT> input1 = input;
    //BLUtils::Reshape(&input1, 32, 32, 256, 4);
    
    //PPMFile::SavePPM("input2.ppm", input1.Get(),
    //                 256, 4, 1, 255.0*IN_COEFF);
    
    BLDebug::DumpData("input.txt", input);
#endif
    
#if 0 //!RESHAPE_32_X_32 //

#define IN_COEFF 64.0
    
    PPMFile::SavePPM("input0.ppm", input.Get(),
                     1024/RESAMPLE_FACTOR, NUM_INPUT_COLS, 1, 255.0*IN_COEFF);
    
    //WDL_TypedBuf<BL_FLOAT> &input0 = (WDL_TypedBuf<BL_FLOAT> &)input;
    //BLUtils::Reshape(&input0, 1024/RESAMPLE_FACTOR, NUM_INPUT_COLS, 32, 32);
    //PPMFile::SavePPM("input1.ppm", input.Get(),
    //                 32, 32, 1, 255.0*IN_COEFF);
    
    BLDebug::DumpData("input.txt", input);
#endif
    
    if (mNN.numOfInputs() < 1)
        return;

    if (mNN.numOfOutputs() < 1)
        return;
    
    NeuralNetwork::TensorXf& inTensor = mNN.input(0);
    float *inData = inTensor.data();
    for (int i = 0; i < input.GetSize(); i++)
    {
        BL_FLOAT val = input.Get()[i];
        inData[i] = val;
    }
    
    // Compute
    mNN.apply();
    
    NeuralNetwork::TensorXf& outTensor = mNN.output(0);
    float *outData = outTensor.data();
    output->Resize(NUM_OUTPUT_COLS*BUFFER_SIZE/(RESAMPLE_FACTOR*2));
    for (int i = 0; i < output->GetSize(); i++)
    {
        float val = outData[i];
        output->Get()[i] = val;
    }
    
#if 0 //RESHAPE_32_X_32
    
#if 1
    BLDebug::DumpData("output.txt", *output);
    
#define OUT_COEFF 1.0
    PPMFile::SavePPM("output0.ppm", output->Get(),
                     32, 32, 1, 255.0*OUT_COEFF);
#endif
    
    BLUtils::Reshape(output, 32, 32, 256, 4);
    //BLUtils::Reshape(output, 32, 32, 4, 256);
    
#if 1
    PPMFile::SavePPM("output1.ppm", output->Get(),
                     256, 4, /*4, 256,*/ 1, 255.0*OUT_COEFF);
#endif

    //BLDebug::DumpData("output.txt", *output);
    
#endif
    
#if 0 //!RESHAPE_32_X_32 //
    BLDebug::DumpData("output.txt", *output);
    
#define OUT_COEFF 1.0
    PPMFile::SavePPM("output1.ppm", output->Get(),
                     1024/RESAMPLE_FACTOR, NUM_OUTPUT_COLS, 1, 255.0*OUT_COEFF);
#endif
}
