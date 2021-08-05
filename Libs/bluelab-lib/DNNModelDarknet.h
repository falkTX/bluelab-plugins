//
//  DNNModelDarknet.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef __BL_Rebalance__DNNModelDarknet__
#define __BL_Rebalance__DNNModelDarknet__

#include <Rebalance_defs.h>
//#include <DNNModel.h>
#include <DNNModel2.h>

// Models trained directily inside Darknet
// DNNModelDarknetMc: from DNNModelDarknet
// - use mlutichannel (get all masks at once)
//
// DNNModelDarknet: from DNNModelDarknetMc
struct network;
class Scale;
class DNNModelDarknet : public DNNModel2
{
public:
    DNNModelDarknet();
    
    virtual ~DNNModelDarknet();
    
    bool Load(const char *modelFileName,
              const char *resourcePath) override;
    
    // For WIN32
    bool LoadWin(IGraphics *pGraphics,
                 const char* modelRcName,
                 const char* weightsRcName) override;
    
    void Predict(const WDL_TypedBuf<BL_FLOAT> &input,
                 vector<WDL_TypedBuf<BL_FLOAT> > *masks) override;
    
    // TESTS
    //void SetDbgThreshold(BL_FLOAT thrs);
    void SetMaskScale(int maskNum, BL_FLOAT scale) override;
    
protected:
    bool LoadWinTest(const char *modelFileName, const char *resourcePath);
    
    // Darknet model
    network *mNet;
    
    BL_FLOAT mDbgThreshold;
    
    BL_FLOAT mMaskScales[NUM_STEM_SOURCES];

    Scale *mScale;

private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<float> mTmpBuf1;
};

#endif /* defined(__BL_Rebalance__DNNModelDarknet__) */
