//
//  DNNModelCaffe.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef __BL_Rebalance__DNNModelKF__
#define __BL_Rebalance__DNNModelKF__

#include <caffe/caffe.hpp>

#include <DNNModel.h>

// Models are converted from Keras to Caffe, using mmdnn
class DNNModelCaffe : public DNNModel
{
public:
    DNNModelCaffe();
    
    virtual ~DNNModelCaffe();
    
    bool Load(const char *modelFileName,
              const char *resourcePath);
    
    // For WIN32
    bool LoadWin(IGraphics *pGraphics, int rcId);
    
    void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *output);
    
protected:
    void DBG_PrintNetData();

    //
    caffe::Net<float> *mNet;
};

#endif /* defined(__BL_Rebalance__DNNModelCaffe__) */
