//
//  DNNModelDarknet.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#include <darknet.h>

extern "C" {
#include <bl_utils.h>
}

extern "C" {
#include <fmem.h>

#ifdef WIN32
#include <fmemopen-win.h>
#endif
}

#include <BLUtils.h>
#include <BLDebug.h>

#include <PPMFile.h>

#include <Scale.h>
#include <Rebalance_defs.h>

#include "DNNModelDarknet.h"

// Was just a test to avoid writing the temporary file to disk (failed)
#define USE_FMEMOPEN_WIN 0 //1

//#define NORMALIZE_INPUT 1

#define NUM_STEMS 4

// Output is not normalized at all, it has negative values, and values > 1.
//#define FIX_OUTPUT_NORM 1

// Was a test (interesting, but needs more testing)
//#define OTHER_IS_REST2 0

// Was a test
//#define SET_OTHER_TO_ZERO 0

DNNModelDarknet::DNNModelDarknet()
{
    mNet = NULL;
    
    mDbgThreshold = 0.0;
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        mMaskScales[i] = 1.0;

    mScale = new Scale();
}

DNNModelDarknet::~DNNModelDarknet()
{
    if (mNet != NULL)
        free_network(mNet);

    delete mScale;
}

bool
DNNModelDarknet::Load(const char *modelFileName,
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
DNNModelDarknet::LoadWin(IGraphics &pGraphics,
                           const char *modelRcName, const char *weightsRcName)
{
#ifdef WIN32

#if 0 // iPlug1
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
#endif->

#if 1 // iPlug2
    WDL_TypedBuf<uint8_t> modelRcBuf = pGraphics.LoadResource(modelRcName, "CFG");
    long modelRcSize = modelRcBuf.GetSize();
    if (modelRcSize == 0)
        return false;
    
    WDL_TypedBuf<uint8_t> weightsRcBuf = pGraphics.LoadResource(weightsRcName, "WEIGHTS");
    long weightsRcSize = weightsRcBuf.GetSize();
    if (weightsRcSize == 0)
        return false;
#endif

    // Model
#if !USE_FMEMOPEN_WIN
    fmem fmem0;
    fmem_init(&fmem0);
    FILE *file0 = fmem_open(&fmem0, "w+");
    //fmem_mem(&fmem0, &modelRcBuf.Get(), modelRcSize);
    fwrite(modelRcBuf.Get(), 1, modelRcSize, file0);
    fflush(file0);
    fseek(file0, 0L, SEEK_SET);
#else
    FILE* file0 = fmemopen_win(modelRcBuf.Get(), modelRcSize, "w+");
#endif

    // Weights
#if !USE_FMEMOPEN_WIN
    fmem fmem1;
    fmem_init(&fmem1);
    FILE *file1 = fmem_open(&fmem1, "wb+");
    //fmem_mem(&fmem1, &weightsRcBuf, &weightsRcSize);
    fwrite(weightsRcBuf.Get(), 1, weightsRcSize, file1);
    fflush(file1);
    fseek(file1, 0L, SEEK_SET);
#else
    FILE* file1 = fmemopen_win(weightsRcBuf.Get(), weightsRcSize, "wb+");
#endif

    // Load network
    mNet = load_network_file(file0, file1, 0);
    set_batch_network(mNet, 1);
    
    // Model
    fclose(file0);
#if !USE_FMEMOPEN_WIN
    fmem_term(&fmem0);
#endif

    // Weights
    fclose(file1);
#if !USE_FMEMOPEN_WIN
    fmem_term(&fmem1);
#endif

	return true;
#endif

    return false;
}

void
DNNModelDarknet::SetMaskScale(int maskNum, BL_FLOAT scale)
{
    if (maskNum >= NUM_STEM_SOURCES)
        return;
    
    mMaskScales[maskNum] = scale;
}

void
DNNModelDarknet::Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                         vector<WDL_TypedBuf<BL_FLOAT> > *masks)
{
#define EPS 1e-15
    
    WDL_TypedBuf<BL_FLOAT> input0 = input;
    
    /*#if NORMALIZE_INPUT
    // Normalize!
    BL_FLOAT normMin;
    BL_FLOAT normMax;
    BLUtils::Normalize(&input0, &normMin, &normMax);
    #endif*/
    
    WDL_TypedBuf<float> X;
    X.Resize(input0.GetSize()*NUM_STEMS);
    for (int i = 0; i < X.GetSize(); i++)
    {
        BL_FLOAT val = input0.Get()[i % input0.GetSize()];
        
        /*#if PROCESS_SIGNAL_DB
        // Used when model is traind in dB
        val = mScale->ApplyScale(Scale::DB, val,
        (BL_FLOAT)PROCESS_SIGNAL_MIN_DB, (BL_FLOAT)0.0);
        #endif*/
        
        X.Get()[i] = val;
    }

    
    // TODO: load .bin example

    
    /*#define DBG_DUMP 0 // 1
#if DBG_DUMP
    PPMFile::SavePPM("data.ppm", input0.Get(), 256, 32, 1, 255);
#endif
    */
    
     // ?
    srand(2222222);
    // Prediction
    float *pred = network_predict(mNet, X.Get());
    
    /*#if OTHER_IS_REST2
      for (int i = 0; i < input0.GetSize(); i++)
      {
      float vals[NUM_STEMS];
      for (int j = 0; j < NUM_STEMS; j++)
      {
      vals[j] = pred[i + j*input0.GetSize()];
      }
      
      vals[3] = 1.0 - (vals[0] + vals[1] + vals[2]);
      if (vals[3] < 0.0)
      vals[3] = 0.0;
      pred[i + 3*input0.GetSize()] = vals[3];
      }
      #endif*/
    
    /*#if SET_OTHER_TO_ZERO
      for (int i = 0; i < input0.GetSize(); i++)
      {
      pred[i + 3*input0.GetSize()] = 0.0;
      }
      #endif*/
    
    /*#if DBG_DUMP
      BLDebug::DumpData("pred0.txt", pred, X.GetSize());
      #endif*/
    
    /*#if SENSIVITY_IS_SCALE
      for (int i = 0; i < X.GetSize(); i++)
      {
      int maskIndex = i/input0.GetSize();
      
      float val = pred[i];
      val *= mMaskScales[maskIndex];
      pred[i] = val;
      }
      #endif
    */
    
    /*#if FIX_OUTPUT_NORM
    // Exactly like the process done in darknet, to multiply masks
    BLUtils::Normalize(pred, input0.GetSize()*NUM_STEMS);
    //my_normalize_chan2(pred, NUM_STEMS, input0.GetSize());
    #endif
    */
    
    /*#if USE_DBG_PREDICT_MASK_THRESHOLD
      for (int i = 0; i < X.GetSize(); i++)
      {
      int maskIndex = i/input0.GetSize();
      if (maskIndex != 3)
      continue;
      
      float val = pred[i];
      if (val > mDbgThreshold)
      val = 0.0;
      pred[i] = val;
      }
      #endif
    */

    /*#if SET_OTHER_TO_ZERO
    // Set to 0 again to avoid a floor effect after normalization
    // (mask would be a constant value instead of 0
    for (int i = 0; i < input0.GetSize(); i++)
    {
    pred[i + 3*input0.GetSize()] = 0.0;
    }
#endif
    */
    
    /*#if DBG_DUMP
      BLDebug::DumpData("pred1.txt", pred, X.GetSize());
      #endif*/
    
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

    /*#if DBG_DUMP //0// DEBUG
      BLDebug::DumpData("pred-mask0.txt", (*masks)[0]);
      BLDebug::DumpData("pred-mask1.txt", (*masks)[1]);
      BLDebug::DumpData("pred-mask2.txt", (*masks)[2]);
      BLDebug::DumpData("pred-mask3.txt", (*masks)[3]);
      #endif*/
    
    //
    for (int i = 0; i < NUM_STEMS; i++)
    {
        BLUtils::ClipMin(&(*masks)[i], (BL_FLOAT)0.0);
    }
    
    /*#if DBG_DUMP
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
      #endif*/
}

#if 0
void
DNNModelDarknet::SetDbgThreshold(BL_FLOAT thrs)
{
    mDbgThreshold = thrs;
}
#endif

// To test on Mac, the mechanisme using fmem
bool
DNNModelDarknet::LoadWinTest(const char *modelFileName, const char *resourcePath)
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
