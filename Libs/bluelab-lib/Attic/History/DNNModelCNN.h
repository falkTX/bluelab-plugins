//
//  DNNModelCNN.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef __BL_Rebalance__DNNModelKF__
#define __BL_Rebalance__DNNModelKF__

#include <CompiledNN.h>

#include <DNNModel.h>

// DNNModel exported from keras in h5 format
// For CompiledNN
//
// Models are .hd5 files exported directly from python
//
class DNNModelCNN : public DNNModel
{
public:
    DNNModelCNN();
    
    virtual ~DNNModelCNN();
    
    bool Load(const char *modelFileName,
              const char *resourcePath);
    
    // For WIN32
    bool LoadWin(IGraphics *pGraphics, int rcId);
    
    void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *output);
    
protected:
    NeuralNetwork::Model mModel;
    NeuralNetwork::CompiledNN mNN;
};

#endif /* defined(__BL_Rebalance__DNNModelCNN__) */
