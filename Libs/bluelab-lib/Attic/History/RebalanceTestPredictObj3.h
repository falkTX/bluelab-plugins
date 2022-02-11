//
//  RebalanceTestPredictObj3.h
//  BL-Rebalance
//
//  Created by applematuer on 5/24/20.
//
//

#ifndef __BL_Rebalance__RebalanceTestPredictObj3__
#define __BL_Rebalance__RebalanceTestPredictObj3__

#include <deque>
using namespace std;

// Include for defines
#include <Rebalance_defs.h>

#include <FftProcessObj16.h>
#include <DNNModel.h>

// RebalanceTestPredictObj2: from RebalanceTestPredictObj
// - added side chain for expected vocal mask
// RebalanceTestPredictObj3: for darknet models

class RebalanceMaskStack;
class RebalanceTestPredictObj3 : public ProcessObj
{
public:
    RebalanceTestPredictObj3(int bufferSize,
                            IGraphics *graphics);
    
    virtual ~RebalanceTestPredictObj3();
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    void Reset();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
protected:
    void ResetHistory();

    void SaveImage(const char *fileName,
                   const deque<WDL_TypedBuf<BL_FLOAT> > &data,
                   BL_FLOAT coeff);
    
    void BufferToQue(deque<WDL_TypedBuf<BL_FLOAT> > *que,
                     const WDL_TypedBuf<BL_FLOAT> &buffer,
                     int width);
    
    void MultQue(deque<WDL_TypedBuf<BL_FLOAT> > *result,
                 const deque<WDL_TypedBuf<BL_FLOAT> > &que);
    
    void AddColsImg(deque<WDL_TypedBuf<BL_FLOAT> > *img,
                    const deque<WDL_TypedBuf<BL_FLOAT> > &cols);

    
    //
    DNNModel *mModel;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMixCols;
    deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mMixColsComp;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMixDownCols;
    
    // Real vocal data for example
    deque<WDL_TypedBuf<BL_FLOAT> > mStemCols;
    deque<WDL_TypedBuf<BL_FLOAT> > mStemDownCols;
    
    //
    deque<WDL_TypedBuf<BL_FLOAT> > mMixColsImg;
    deque<WDL_TypedBuf<BL_FLOAT> > mStemColsImg;
    deque<WDL_TypedBuf<BL_FLOAT> > mMaskColsImg;
    deque<WDL_TypedBuf<BL_FLOAT> > mMultColsImg;
    
    int mDbgCount;
    
    //
    RebalanceMaskStack *mMaskStack;
};

#endif /* defined(__BL_Rebalance__RebalanceTestPredictObj3__) */
