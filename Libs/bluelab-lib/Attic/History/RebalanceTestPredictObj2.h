//
//  RebalanceTestPredictObj2.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef __BL_Rebalance__RebalanceTestPredictObj2__
#define __BL_Rebalance__RebalanceTestPredictObj2__

#include <deque>
using namespace std;

// Include for defines
#include <Rebalance_defs.h>

#include <FftProcessObj16.h>
#include <DNNModel.h>

// RebalanceTestPredictObj2: from RebalanceTestPredictObj
// - added side chain for expected vocal mask

class RebalanceTestPredictObj2 : public ProcessObj
{
public:
    RebalanceTestPredictObj2(int bufferSize,
                            IGraphics *graphics);
    
    virtual ~RebalanceTestPredictObj2();
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    void Reset();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
protected:
    DNNModel *mModel;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMixCols;
    
    // Real vocal data for example
    deque<WDL_TypedBuf<BL_FLOAT> > mStemCols;
};

#endif /* defined(__BL_Rebalance__RebalanceTestPredictObj2__) */
