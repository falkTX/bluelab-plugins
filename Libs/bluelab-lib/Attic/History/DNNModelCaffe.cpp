//
//  DNNModelCaffe.cpp
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#include <string>

// For define LSTM
#include <Rebalance_defs.h>

#include <BLUtils.h>

#include <PPMFile.h>

#include "DNNModelCaffe.h"

using namespace caffe;

#define DBG_MODEL_FILE "caffe-dnn-model-source0.prototxt"
#define DBG_TRAINED_FILE "caffe-dnn-model-source0.caffemodel"

DNNModelCaffe::DNNModelCaffe()
{
    //NetParameter param;
    //mNet = new Net<float>(param);
    mNet = NULL;
}

DNNModelCaffe::~DNNModelCaffe()
{
    if (mNet != NULL)
        delete mNet;
}

bool
DNNModelCaffe::Load(const char *modelFileName,
                    const char *resourcePath)
{
#ifdef WIN32
    return false;
#endif
    
    if (mNet != NULL)
        delete mNet;
    
    char modelFullFileName[2048];
    sprintf(modelFullFileName, "%s/%s", resourcePath, DBG_MODEL_FILE);
    
    char trainedFileFullFileName[2048];
    sprintf(trainedFileFullFileName, "%s/%s", resourcePath, DBG_TRAINED_FILE);
    
    mNet = new Net<float>(modelFullFileName, caffe::TEST);
    
    mNet->set_debug_info(true);
    
    //DBG_PrintNetData(); // No Nan (all zeros)
    
    mNet->CopyTrainedLayersFrom(trainedFileFullFileName);
    
#if 0
    // TEST
    Blob<float>* input_layer = mNet->input_blobs()[0];
    int w = input_layer->width();
    int h = input_layer->height();
    int num_channels = 1;
    input_layer->Reshape(1, num_channels, h, w);
    mNet->Reshape();
#endif
    
    //DBG_PrintNetData(); // TEST with reshape => NaN
    
    //DBG_PrintNetData(); // got some NaN
    
    return true;
}

// For WIN32
bool
DNNModelCaffe::LoadWin(IGraphics *pGraphics, int rcId)
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
DNNModelCaffe::Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                       WDL_TypedBuf<BL_FLOAT> *output)
{
    output->Resize(input.GetSize());
    BLUtils::FillAllZero(output);
    
    // DEBUG
    //int numIn = mNet->num_inputs();
    //int numOut = mNet->num_outputs();
    BLDebug::DumpData("input.txt", input);
    
    Blob<float>* input_layer = mNet->input_blobs()[0];
    float* input_data = input_layer->mutable_cpu_data();
    
    // Debug
    //int inW = input_layer->width();
    //int inH = input_layer->height();
    
    for (int i = 0; i < input.GetSize(); i++)
    {
        BL_FLOAT val = input.Get()[i];
        
#if 0
        // Avoid NaN in network
        if (std::fabs(val) < 1e-15)
            val = 0.0;
        if (val < -1e+15)
            val = -1e+15;
        if (val > 1e+15)
            val = 1e+15;
#endif
        
        input_data[i] = val;
    }
    
    //float testab[] = {0, 0, 0, 1, 1, 0, 1, 1};
    //float testc[] = {0, 1, 1, 0};
    //caffe::MemoryDataLayer<float> *dataLayer =
    //        (caffe::MemoryDataLayer<float> *)(mNet->layer_by_name("input_1").get());
    //dataLayer->Reset(testab, testc, 4);
    
    //const vector<Blob<float>*>& outputs = mNet->Forward();
    
    //DBG_PrintNetData();
    
    mNet->Forward();
    
    //DBG_PrintNetData();
    
    //Blob<float>* output_layer = outputs[0];
    Blob<float>* output_layer = mNet->output_blobs()[0];
    
    const float* output_data = output_layer->cpu_data();
    
    // Debug
    //int outW = output_layer->width();
    //int outH = output_layer->height();
    
    for (int i = 0; i < output->GetSize(); i++)
    {
        BL_FLOAT val = output_data[i];
        output->Get()[i] = val;
    }
    
    BLDebug::DumpData("output.txt", *output);
}

void
DNNModelCaffe::DBG_PrintNetData()
{
    const vector<shared_ptr<Blob<float> > > &blobs = mNet->blobs();
    const vector<shared_ptr<Layer<float> > > &layers = mNet->layers();
    
    for (int i = 0; i < blobs.size(); i++)
    {
        const shared_ptr<Layer<float> > &layer = layers[i];
        const LayerParameter &param = layer->layer_param();
        const string layerName = param.name();
        
        char fileName[512];
        sprintf(fileName, "layer-%d-%s.txt", i, layerName.c_str());
        
        const shared_ptr<Blob<float> > &blob = blobs[i];
        const float *blobData = blob->cpu_data();
        
#define DATA_SIZE 32*256
        WDL_TypedBuf<BL_FLOAT> data;
        data.Resize(DATA_SIZE);
        BLUtils::FillAllZero(&data);
        
        for (int i = 0; i < DATA_SIZE; i++)
        {
            data.Get()[i] = blobData[i];
        }
        
        Debug::DumpData(fileName, data);
    }
}
