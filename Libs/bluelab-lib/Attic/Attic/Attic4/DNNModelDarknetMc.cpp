//
//  DNNModelDarknetMc.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#include <darknet.h>

extern "C" {
#include <fmem.h>
}

#include <BLUtils.h>

#include <PPMFile.h>

#include "DNNModelDarknetMc.h"


#define NORMALIZE_INPUT 1

#define NUM_STEMS 4


DNNModelDarknetMc::DNNModelDarknetMc()
{
    mNet = NULL;
}

DNNModelDarknetMc::~DNNModelDarknetMc()
{
    free_network(mNet);
}

bool
DNNModelDarknetMc::Load(const char *modelFileName,
                        const char *resourcePath)
{
#ifdef WIN32
    return false;
#endif
    
    // Test fmem on Mac
    //bool res = LoadWinTest(modelFileName, resourcePath);
    //return res;
    
    char cfgFullFileName[2048];
    sprintf(cfgFullFileName, "%s/%s.cfg", resourcePath, modelFileName);
    
    char weightsFileFullFileName[2048];
    sprintf(weightsFileFullFileName, "%s/%s.weights", resourcePath, modelFileName);
    
    mNet = load_network(cfgFullFileName, weightsFileFullFileName, 0);
    set_batch_network(mNet, 1);
    
    return true;
}

// For WIN32
bool
DNNModelDarknetMc::LoadWin(IGraphics *pGraphics,
                           int modelRcId, int weightsRcId)
{
#ifdef WIN32
    void *modelRcBuf;
	long modelRcSize;
	bool loaded = ((IGraphicsWin *)pGraphics)->LoadWindowsResource(modelRcId,
                                                                   "RCDATA",
                                                                   &modelRcBuf,
                                                                   &modelRcSize);
	if (!loaded)
		return false;
    
    void *weightsRcBuf;
	long weightsRcSize;
	loaded = ((IGraphicsWin *)pGraphics)->LoadWindowsResource(weightsRcId,
                                                              "RCDATA",
                                                              &weightsRcBuf,
                                                              &weightsRcSize);
    
    if (!loaded)
		return false;
    
    
    // Model
    fmem fmem0;
    fmem_init(&fmem0);
    //FILE *file0 = fmem_open(&fmem0, "r");
    FILE *file0 = fmem_open(&fmem0, "w+");
    //fmem_mem(&fmem0, &modelRcBuf, &modelRcSize);
    fwrite(modelRcBuf, 1, modelRcSize, file0);
    fflush(file0);
    fseek(file0, 0L, SEEK_SET);
    
    // Weights
    fmem fmem1;
    fmem_init(&fmem1);
    //FILE *file1 = fmem_open(&fmem1, "rb");
    FILE *file1 = fmem_open(&fmem1, "wb+");
    //fmem_mem(&fmem1, &weightsRcBuf, &weightsRcSize);
    fwrite(weightsRcBuf, 1, weightsRcSize, file1);
    fflush(file1);
    fseek(file1, 0L, SEEK_SET);
    
    // Load network
    mNet = load_network_file(file0, file1, 0);
    set_batch_network(mNet, 1);
    
    // Model
    fclose(file0);
    fmem_term(&fmem0);
    
    // Weights
    fclose(file1);
    fmem_term(&fmem1);
    
	return true;
#endif

    return false;
}

void
DNNModelDarknetMc::Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                           vector<WDL_TypedBuf<BL_FLOAT> > *masks)
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
    
#define DBG_DUMP 0 // 1
    
#if DBG_DUMP
    PPMFile::SavePPM("data.ppm", input0.Get(), 256, 32, 1, 255);
#endif
    
    srand(2222222); // ?
    float *pred = network_predict(mNet, X.Get());
    
    masks->resize(NUM_STEMS);
    for (int i = 0; i < NUM_STEMS; i++)
    {
        (*masks)[i].Resize(input0.GetSize());
    }
    
    for (int i = 0; i < X.GetSize(); i++)
    {
        int maskIndex = i/input0.GetSize();
        
        float val = pred[i];
        
        (*masks)[maskIndex].Get()[i % input0.GetSize()] = val;
    }

    //
    for (int i = 0; i < NUM_STEMS; i++)
    {
        BLUtils::ClipMin(&(*masks)[i], (BL_FLOAT)0.0);
    }
    
#if DBG_DUMP
    PPMFile::SavePPM("mask0.ppm", (*masks)[0].Get(), 256, 32, 1, 255);
    PPMFile::SavePPM("mask1.ppm", (*masks)[1].Get(), 256, 32, 1, 255);
    PPMFile::SavePPM("mask2.ppm", (*masks)[2].Get(), 256, 32, 1, 255);
    PPMFile::SavePPM("mask3.ppm", (*masks)[3].Get(), 256, 32, 1, 255);
    
    static int count = 0;
    count++;
    if (count == 7)
    {
        int dummy = 0;
    }
#endif
}

// To test on Mac, the mechanisme using fmem
bool
DNNModelDarknetMc::LoadWinTest(const char *modelFileName, const char *resourcePath)
{
    // NOTE: with the fmem implementation "funopen", there is a memory problem
    // in darknet fgetl().
    //Detected with valgrind during plugin scan.
    // With the implementation "tmpfile", there is no memory problem.
    
    //fprintf(stderr, "modelFileName: %s\n", modelFileName);
    
    char cfgFullFileName[2048];
    sprintf(cfgFullFileName, "%s/%s.cfg", resourcePath, modelFileName);
    
    char weightsFileFullFileName[2048];
    sprintf(weightsFileFullFileName, "%s/%s.weights", resourcePath, modelFileName);
    
    // Read the two files in memory
    
    // File 0
    FILE *f0 = fopen(cfgFullFileName, "rb");
    fseek(f0, 0, SEEK_END);
    long fsize0 = ftell(f0);
    fseek(f0, 0, SEEK_SET);
    
    void *buf0 = malloc(fsize0 + 1);
    fread(buf0, 1, fsize0, f0);
    fclose(f0);
    
    // File 1
    FILE *f1 = fopen(weightsFileFullFileName, "rb");
    fseek(f1, 0, SEEK_END);
    long fsize1 = ftell(f1);
    fseek(f1, 0, SEEK_SET);
    
    void *buf1 = malloc(fsize1 + 1);
    fread(buf1, 1, fsize1, f1);
    fclose(f1);
    
    // Read with fmem
    //
    
    // Model
    fmem fmem0;
    fmem_init(&fmem0);
    //FILE *file0 = fmem_open(&fmem0, "r");
    FILE *file0 = fmem_open(&fmem0, "w+");
    fwrite(buf0, 1, fsize0, file0);
    fflush(file0);
    fseek(file0, 0L, SEEK_SET);
    
    // Weights
    fmem fmem1;
    fmem_init(&fmem1);
    //FILE *file1 = fmem_open(&fmem1, "rb");
    FILE *file1 = fmem_open(&fmem1, "wb+");
    fwrite(buf1, 1, fsize1, file1);
    fflush(file1);
    fseek(file1, 0L, SEEK_SET);
    
    // Load network
    mNet = load_network_file(file0, file1, 0);
    set_batch_network(mNet, 1);
    
    // Model
    fclose(file0);
    fmem_term(&fmem0);
    
    // Weights
    fclose(file1);
    fmem_term(&fmem1);
    
    free(buf0);
    free(buf1);
    
    return true;
}
