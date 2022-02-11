//
//  RebalanceTestPredictObj.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef __BL_Rebalance__RebalanceTestPredictObj__
#define __BL_Rebalance__RebalanceTestPredictObj__

#include <deque>
using namespace std;

// Include for defines
#include <Rebalance_defs.h>

#include <FftProcessObj16.h>
#include <DNNModel.h>

class RebalanceTestPredictObj : public ProcessObj
{
public:
    RebalanceTestPredictObj(int bufferSize,
                            IGraphics *graphics);
    
    virtual ~RebalanceTestPredictObj();
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    void Reset();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
protected:
    DNNModel *mModelVocal;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMixCols;
};

#endif /* defined(__BL_Rebalance__RebalanceTestPredictObj__) */
