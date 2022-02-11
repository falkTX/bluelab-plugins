//
//  DNNModelKF.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef __BL_Rebalance__DNNModelKF__
#define __BL_Rebalance__DNNModelKF__

// DNN
#include <pt_model.h>
#include <pt_tensor.h>

// For PT
#include <string>
using namespace std;

#include <DNNModel.h>

// DNNModel exported from keras using kerasify
//
// Models are ".model" files exported with kerasify.py
//
class DNNModelKF : public DNNModel
{
public:
    DNNModelKF();
    
    virtual ~DNNModelKF();
    
    bool Load(const char *modelFileName,
              const char *resourcePath);
    
    // For WIN32
    bool LoadWin(IGraphics *pGraphics, int rcId);
    
    void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *output);
    
protected:
    std::unique_ptr<pt::Model> mModel;
};

#endif /* defined(__BL_Rebalance__DNNModelKF__) */
