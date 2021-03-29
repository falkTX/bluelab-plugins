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

#define RESIZE_NETWORK 0 //1
#if RESIZE_NETWORK
#include <stb_image_resize.h>
#endif

// Output is not normalized at all, it has negative values, and values > 1.
//#define FIX_OUTPUT_NORM 1

// Was a test (interesting, but needs more testing)
//#define OTHER_IS_REST2 0

// Was a test
//#define SET_OTHER_TO_ZERO 0

// From bl-darknet/rebalance.c
void
amp_to_db(float *buf, int size)
{
    for (int i = 0; i < size; i++)
    {
        float v = buf[i];
        v = my_amp_to_db_norm(v);
        buf[i] = v;
    }
}

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

#if RESIZE_NETWORK
    int res = resize_network(mNet,
                             REBALANCE_NUM_SPECTRO_FREQS/2,
                             REBALANCE_NUM_SPECTRO_COLS/2);
#endif
    
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
    
    WDL_TypedBuf<float> X;
    X.Resize(input0.GetSize()*NUM_STEMS);
    for (int i = 0; i < X.GetSize(); i++)
    {
        BL_FLOAT val = input0.Get()[i % input0.GetSize()];
        
        X.Get()[i] = val;
    }

#define DEBUG_DATA 0 //1
#if DEBUG_DATA
    // DEBUG: load .bin example
#define DBG_INPUT_IMAGE_FNAME "/home/niko/Documents/Dev/plugs-dev/bluelab/BL-Plugins/BL-Rebalance/DNNPack/training/mix" //mix-sum"

#define IMAGE_WIDTH REBALANCE_NUM_SPECTRO_FREQS
#define IMAGE_HEIGHT REBALANCE_NUM_SPECTRO_COLS
    
    // Load
    WDL_TypedBuf<float> dbgMix;
    dbgMix.Resize(IMAGE_WIDTH*IMAGE_HEIGHT);
    float *dbgMixData = dbgMix.Get();
    my_load_image_bin(IMAGE_WIDTH, IMAGE_HEIGHT, 1,
                      dbgMixData, DBG_INPUT_IMAGE_FNAME);

    for (int i = 0; i < X.GetSize(); i++)
    {
        BL_FLOAT val = dbgMix.Get()[i % dbgMix.GetSize()];
        
        X.Get()[i] = val;
    }
#endif   

    // Process
    my_normalize(X.Get(), X.GetSize());
    amp_to_db(X.Get(), X.GetSize());
    my_normalize(X.Get(), X.GetSize());

#if RESIZE_NETWORK
#define IMAGE_WIDTH REBALANCE_NUM_SPECTRO_FREQS
#define IMAGE_HEIGHT REBALANCE_NUM_SPECTRO_COLS
    
    WDL_TypedBuf<float> XResize;
    XResize.Resize(IMAGE_WIDTH*IMAGE_HEIGHT*NUM_STEMS/4);
    for (int i = 0; i < NUM_STEMS; i++)
    {
        stbir_resize_float(&X.Get()[i*IMAGE_WIDTH*IMAGE_HEIGHT],
                           IMAGE_WIDTH, IMAGE_HEIGHT, 0,
                           &XResize.Get()[i*IMAGE_WIDTH*IMAGE_HEIGHT/4],
                           IMAGE_WIDTH/2, IMAGE_HEIGHT/2, 0,
                           1);
    }
    X = XResize;
#endif
    
     // ?
    srand(2222222);
    // Prediction
    float *pred = network_predict(mNet, X.Get());

#if RESIZE_NETWORK
#define IMAGE_WIDTH REBALANCE_NUM_SPECTRO_FREQS
#define IMAGE_HEIGHT REBALANCE_NUM_SPECTRO_COLS
    
    WDL_TypedBuf<float> predResize;
    predResize.Resize(IMAGE_WIDTH*IMAGE_HEIGHT*NUM_STEMS);
    for (int i = 0; i < NUM_STEMS; i++)
    {
        stbir_resize_float(&pred[i*IMAGE_WIDTH*IMAGE_HEIGHT/4],
                           IMAGE_WIDTH/2, IMAGE_HEIGHT/2, 0,
                           &predResize.Get()[i*IMAGE_WIDTH*IMAGE_HEIGHT],
                           IMAGE_WIDTH, IMAGE_HEIGHT, 0,
                           1);
    }
    // TODO: check memory
    pred = predResize.Get();
#endif

#if DEBUG_DATA
#define IMAGE_WIDTH REBALANCE_NUM_SPECTRO_FREQS
#define IMAGE_HEIGHT REBALANCE_NUM_SPECTRO_COLS

#define SAVE_IMG_FNAME "/home/niko/Documents/BlueLabAudio-Debug/X"
    //my_save_image(IMAGE_WIDTH, IMAGE_HEIGHT, 1, X.Get(), SAVE_IMG_FNAME, 1);
    my_save_image_mc(IMAGE_WIDTH, IMAGE_HEIGHT, 4, X.Get(), SAVE_IMG_FNAME);
    my_save_image_txt(IMAGE_WIDTH, IMAGE_HEIGHT, 1, X.Get(), SAVE_IMG_FNAME);
    
#define SAVE_PRED_FNAME "/home/niko/Documents/BlueLabAudio-Debug/pred"
    my_save_image_mc(IMAGE_WIDTH, IMAGE_HEIGHT, 4, pred, SAVE_PRED_FNAME);
    my_save_image_txt(IMAGE_WIDTH, IMAGE_HEIGHT, 4, pred, SAVE_PRED_FNAME);
 
    exit(0);
#endif
        
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

    for (int i = 0; i < NUM_STEMS; i++)
    {
        BLUtils::ClipMin(&(*masks)[i], (BL_FLOAT)0.0);
    }
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
