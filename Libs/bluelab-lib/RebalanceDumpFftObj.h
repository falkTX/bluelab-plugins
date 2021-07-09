//
//  RebalanceDumpFftObj.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#ifndef RebalanceDumpFftObj_hpp
#define RebalanceDumpFftObj_hpp

#include <deque>
using namespace std;

#include <FftProcessObj16.h>

#include <Rebalance_defs.h>

// RebalanceDumpFftObj
class RebalanceDumpFftObj : public ProcessObj
{
public:
    /*class Slice
     {
     public:
     Slice();
     
     virtual ~Slice();
     
     void SetData(const deque<WDL_TypedBuf<double> > &mixCols,
     const deque<WDL_TypedBuf<double> > &sourceCols);
     
     void GetData(vector<WDL_TypedBuf<double> > *mixCols,
     vector<WDL_TypedBuf<double> > *sourceCols) const;
     
     protected:
     vector<WDL_TypedBuf<double> > mMixCols;
     vector<WDL_TypedBuf<double> > mSourceCols;
     };*/
    
    //
    
    RebalanceDumpFftObj(int bufferSize);
    
    virtual ~RebalanceDumpFftObj();
    
    virtual void
    ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    // Addition, for normalized masks
    
    // Get spectrogram slices
    //void GetSlices(vector<Slice> *slices);
    
    //void ResetSlices();
    
    bool HasEnoughData();
    
    void GetData(WDL_TypedBuf<double> mixCols[REBALANCE_NUM_SPECTRO_COLS],
                 WDL_TypedBuf<double> sourceCols[REBALANCE_NUM_SPECTRO_COLS]);
    
protected:
    deque<WDL_TypedBuf<double> > mMixCols;
    deque<WDL_TypedBuf<double> > mSourceCols;
    
    //vector<Slice> mSlices;
};

#endif /* RebalanceDumpFftObj_hpp */
