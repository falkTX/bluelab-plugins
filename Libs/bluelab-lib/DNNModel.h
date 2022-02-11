//
//  DNNModel.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef BL_Rebalance_DNNModel_h
#define BL_Rebalance_DNNModel_h

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug::igraphics;

class DNNModel
{
public:
    DNNModel() {}
    
    virtual ~DNNModel() {};

    virtual bool Load(const char *modelFileName,
                      const char *resourcePath) = 0;
                
    // For WIN32
    virtual bool LoadWin(IGraphics *pGraphics, int rcId) = 0;
    
    //virtual void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
    //                     WDL_TypedBuf<BL_FLOAT> *output,
    //                     WDL_TypedBuf<BL_FLOAT> *mask = NULL) = 0;
    virtual void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                         WDL_TypedBuf<BL_FLOAT> *mask) = 0;
};

#endif
