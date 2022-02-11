//
//  DNNModelKF.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef __BL_Rebalance__DNNModelKF__
#define __BL_Rebalance__DNNModelKF__

#include <fdeep/fdeep.hpp>

#include <DNNModel.h>

// DNNModel exported from keras using frugally-deep
//
// Must have C++14 to compile, due to Functional Plus...
//
// Models are .json files exported from .hd5 using the provided
// python3 script
//
class DNNModelFD : public DNNModel
{
public:
    DNNModelFD();
    
    virtual ~DNNModelFD();
    
    bool Load(const char *modelFileName,
              const char *resourcePath);
    
    // For WIN32
    bool LoadWin(IGraphics *pGraphics, int rcId);
    
    void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *output);
    
protected:
    fdeep::model mModel;
};

#endif /* defined(__BL_Rebalance__DNNModelKF__) */
