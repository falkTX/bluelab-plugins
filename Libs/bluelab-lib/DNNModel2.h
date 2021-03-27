//
//  DNNModel2.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef BL_Rebalance_DNNModel2_h
#define BL_Rebalance_DNNModel2_h

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug::igraphics;

class DNNModel2
{
public:
    DNNModel2() {}
    
    virtual ~DNNModel2() {};

    virtual bool Load(const char *modelFileName,
                      const char *resourcePath) = 0;
                
    // For WIN32
    //virtual bool LoadWin(IGraphics *pGraphics,
    //                     int modelRcId, int weightsRcId) = 0;
    virtual bool LoadWin(IGraphics &pGraphics, 
                         const char* modelRcName,
                         const char* weightsRcName) = 0;

    // Returns several masks at once
    virtual void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                         vector<WDL_TypedBuf<BL_FLOAT> > *masks) = 0;
    
    // TESTS
    //virtual void SetDbgThreshold(BL_FLOAT thrs) = 0;
    virtual void SetMaskScale(int maskNum, BL_FLOAT scale) = 0;
};

#endif
