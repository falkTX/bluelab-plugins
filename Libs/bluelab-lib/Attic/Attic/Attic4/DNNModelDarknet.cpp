//
//  DNNModelDarknet.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#include <darknet.h>

// For define LSTM
#include <Rebalance_defs.h>

#include <BLUtils.h>

#include <PPMFile.h>

#include "DNNModelDarknet.h"

//#define DBG_CFG_FILE "dnn-model-source0.cfg"
//#define DBG_WEIGHTS_FILE "dnn-model-source0.weights"

#define DBG_CFG_FILE "rebalance-32x.cfg"
#define DBG_WEIGHTS_FILE "rebalance-32x.weights"

// Debug
#define DBG_DUMP_DATA 0 //CURRENT //1 //0
#define DUMMY_DATA 0 //1

#define NORMALIZE_INPUT 1

#define NUM_STEMS 4


DNNModelDarknet::DNNModelDarknet()
{
    mNet = NULL;
}

DNNModelDarknet::~DNNModelDarknet()
{
    free_network(mNet);
}

bool
DNNModelDarknet::Load(const char *modelFileName,
                      const char *resourcePath)
{
#ifdef WIN32
    return false;
#endif
    
    char cfgFullFileName[2048];
    sprintf(cfgFullFileName, "%s/%s", resourcePath, DBG_CFG_FILE);
    
    char weightsFileFullFileName[2048];
    sprintf(weightsFileFullFileName, "%s/%s", resourcePath, DBG_WEIGHTS_FILE);
    
    // TODO
    mNet = load_network(cfgFullFileName, weightsFileFullFileName, 0);
    set_batch_network(mNet, 1);
    
    return true;
}

// For WIN32
bool
DNNModelDarknet::LoadWin(IGraphics *pGraphics, int rcId)
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

#if 0 // Single channel
void
DNNModelDarknet::Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                         //WDL_TypedBuf<BL_FLOAT> *output,
                         WDL_TypedBuf<BL_FLOAT> *mask)
{
    // Init
    WDL_TypedBuf<BL_FLOAT> mask0;
    mask0.Resize(input.GetSize());
    BLUtils::FillAllZero(&mask0);
    
    //
    WDL_TypedBuf<BL_FLOAT> input0 = input;
    
#if DUMMY_DATA
    BLDebug::LoadData("test-0_input.txt", &input0);
#endif
    
#if NORMALIZE_INPUT
    // Normalize!
    BL_FLOAT normMin;
    BL_FLOAT normMax;
    BLUtils::Normalize(&input0, &normMin, &normMax);
#endif
    
    //BLDebug::DumpData("mdl-input.txt", input0);
#if DBG_DUMP_DATA
    //BLDebug::DumpData("mdl-input.txt", input0);
    WDL_TypedBuf<BL_FLOAT> normInput = input0;
    //BLUtils::Normalize(&normInput);
    PPMFile::SavePPM("mdl-input.ppm", normInput.Get(),
                     1024/RESAMPLE_FACTOR, NUM_OUTPUT_COLS, 1, 255.0);
#endif
    
    WDL_TypedBuf<float> X;
    X.Resize(input0.GetSize());
    for (int i = 0; i < X.GetSize(); i++)
    {
        BL_FLOAT val = input0.Get()[i];
        X.Get()[i] = val;
    }
    
    srand(2222222); // ?
    float *pred = network_predict(mNet, X.Get());
    //float *pred = network_eval(mNet, X.Get());
    
    for (int i = 0; i < mask0.GetSize(); i++)
    {
        float val = pred[i];
        mask0.Get()[i] = val;
    }
    
    BLUtils::ClipMin(&mask0, 0.0);
    
    // Result (mult)
    //*output = input0;
    //BLUtils::MultValues(output, mask0);
    
#if NORMALIZE_INPUT
    // De-normalize!
    //BLUtils::DeNormalize(output, normMin, normMax);
    //BLUtils::ClipMin(output, 0.0);
#endif
    
    //if (mask != NULL)
    *mask = mask0;
    
#if DBG_DUMP_DATA
    //BLDebug::DumpData("mdl-output.txt", *output);
    
    WDL_TypedBuf<BL_FLOAT> normOutput = *output;
    //BLUtils::Normalize(&normOutput);
    PPMFile::SavePPM("mdl-output.ppm", normOutput.Get(),
                     1024/RESAMPLE_FACTOR, NUM_OUTPUT_COLS, 1, 255.0);
#endif
    
#if DBG_DUMP_DATA
    //BLDebug::DumpData("mdl-mult.txt", *output);
    
    //WDL_TypedBuf<BL_FLOAT> normMult = input0;
    //BLUtils::MultValues(&normMult, *output);
    //BLUtils::Normalize(&normMult); //
    //PPMFile::SavePPM("mdl-mult.ppm", normMult.Get(),
    //                 1024/RESAMPLE_FACTOR, NUM_OUTPUT_COLS, 1, 255.0);
#endif
}
#endif

void
DNNModelDarknet::Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                         WDL_TypedBuf<BL_FLOAT> *mask)
{
#define EPS 1e-15
    
    WDL_TypedBuf<BL_FLOAT> input0 = input;
    
#if NORMALIZE_INPUT
    // Normalize!
    BL_FLOAT normMin;
    BL_FLOAT normMax;
    BLUtils::Normalize(&input0, &normMin, &normMax);
#endif
    
    WDL_TypedBuf<float> X;
    X.Resize(input0.GetSize()*NUM_STEMS);
    for (int i = 0; i < X.GetSize(); i++)
    {
        BL_FLOAT val = input0.Get()[i % input0.GetSize()];
        X.Get()[i] = val;
    }
    
    srand(2222222); // ?
    float *pred = network_predict(mNet, X.Get());
    
    WDL_TypedBuf<BL_FLOAT> masks[NUM_STEMS];
    for (int i = 0; i < NUM_STEMS; i++)
    {
        masks[i].Resize(input0.GetSize());
    }
    
    for (int i = 0; i < X.GetSize(); i++)
    {
        int maskIndex = i/input0.GetSize();
        
        float val = pred[i];
        
        masks[maskIndex].Get()[i % input0.GetSize()] = val;
    }
    
    // 0 => VOCAL
    WDL_TypedBuf<BL_FLOAT> mask0 = masks[0/*0*//*2*/];
    
#if 0 // TEST: normalize (does not change a lot)
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        BL_FLOAT sum = 0.0;
        for (int j = 0; j < NUM_STEMS; j++)
        {
            sum += masks[j].Get()[i];
        }
        
        if (sum > EPS)
            mask0.Get()[i] /= sum;
    }
#endif
    
#if 0 // TEST: try to threshold (just to see)
#define THRESHOLD 0.5
    BL_FLOAT maxValue = BLUtils::ComputeMax(mask0);
    BLUtils::ClipMin2(&mask0, maxValue*THRESHOLD, 0.0);
#endif

#if 0 // TEST: keep only max (interesting)
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        BL_FLOAT val = masks[0].Get()[i];
        for (int j = 1; j < NUM_STEMS; j++)
        {
            BL_FLOAT val0 = masks[j].Get()[i];
            if (val0 > val)
                val = 0.0;
        }
        
        mask0.Get()[i] = val;
    }
#endif
    
    BLUtils::ClipMin(&mask0, (BL_FLOAT)0.0);
    
    *mask = mask0;
}
