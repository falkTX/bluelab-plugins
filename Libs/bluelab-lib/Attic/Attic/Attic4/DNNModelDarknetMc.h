//
//  DNNModelDarknetMc.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef __BL_Rebalance__DNNModelDarknetMc__
#define __BL_Rebalance__DNNModelDarknetMc__

#include <DNNModelMc.h>

// Models trained directily inside Darknet
// DNNModelDarknetMc: from DNNModelDarknet
// - use mlutichannel (get all masks at once)
struct network;
class DNNModelDarknetMc : public DNNModelMc
{
public:
    DNNModelDarknetMc();
    
    virtual ~DNNModelDarknetMc();
    
    bool Load(const char *modelFileName,
              const char *resourcePath);
    
    // For WIN32
    bool LoadWin(IGraphics *pGraphics, int modelRcId, int weightsRcId);
    
    void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                 vector<WDL_TypedBuf<BL_FLOAT> > *masks);
    
protected:
    bool LoadWinTest(const char *modelFileName, const char *resourcePath);
    
    // Darknet model
    network *mNet;
};

#endif /* defined(__BL_Rebalance__DNNModelDarknetMc__) */
