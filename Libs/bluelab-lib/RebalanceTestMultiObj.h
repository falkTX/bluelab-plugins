//
//  RebalanceTestMultiObj.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#ifndef RebalanceTestMultiObj_hpp
#define RebalanceTestMultiObj_hpp

#include <FftProcessObj16.h>

#include "IPlug_include_in_plug_hdr.h"

#if TEST_DNN_INPUT

class RebalanceDumpFftObj;
class RebalanceTestMultiObj : public MultichannelProcess
{
public:
    RebalanceTestMultiObj(RebalanceDumpFftObj *rebalanceDumpFftObjs[4],
                          double sampleRate);
    
    virtual ~RebalanceTestMultiObj();
    
    void Reset() override;
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, double sampleRate) override;
    
    void
    ProcessResultFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                     const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer) override;
    
protected:
    double mSampleRate;
    
    RebalanceDumpFftObj *mRebalanceDumpFftObjs[4];
    
    int mDumpCount;
};

#endif

#endif /* RebalanceTestMultiObj_hpp */
