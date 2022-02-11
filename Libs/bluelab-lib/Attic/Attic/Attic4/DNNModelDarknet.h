//
//  DNNModelDarknet.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef __BL_Rebalance__DNNModelKF__
#define __BL_Rebalance__DNNModelKF__

#include <DNNModel.h>

// Models trained directily inside Darknet
struct network;
class DNNModelDarknet : public DNNModel
{
public:
    DNNModelDarknet();
    
    virtual ~DNNModelDarknet();
    
    bool Load(const char *modelFileName,
              const char *resourcePath);
    
    // For WIN32
    bool LoadWin(IGraphics *pGraphics, int rcId);
    
    //void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
    //             WDL_TypedBuf<BL_FLOAT> *output,
    //             WDL_TypedBuf<BL_FLOAT> *mask = NULL);
    void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *mask);
    
protected:
    // Darknet model
    network *mNet;
};

#endif /* defined(__BL_Rebalance__DNNModelDarknet__) */
