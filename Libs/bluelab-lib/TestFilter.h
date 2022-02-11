//
//  TestFilter.h
//  UST
//
//  Created by applematuer on 8/26/19.
//
//

#ifndef __UST__TestFilter__
#define __UST__TestFilter__

#include "IPlug_include_in_plug_hdr.h"

class CrossoverSplitterNBands2;
class CrossoverSplitter3Bands;
class CrossoverSplitterNBands;
class CrossoverSplitterNBands3;
class FilterRBJNX;

// TODO: test LinkwitzRileyTwo ?
class TestFilter
{
public:
    TestFilter(BL_FLOAT sampleRate);
    
    virtual ~TestFilter();
    
    // CrossoverN2 IR + dump bands
    // Error: 1e-7 (curve)
    void Test0();
    
    // CrossoverN2 White noise
    void Test1();
    
    // Crossover3Bands IR + dump bands
    //
    // Error: 1.5e-9 (noisy)
    void Test2();
    
    // CrossoverN IR + dump bands
    void Test3();
    
    // CrossoverNBands3
    void Test4();
    
    // CrossoverNBands3
    void Test5();
    
    // RBJ + shelves
    void Test6();
    
    void GetFft(const WDL_TypedBuf<BL_FLOAT> &samples,
                WDL_TypedBuf<BL_FLOAT> *fft);
    
protected:
    void InitTest0();
    void ShutdownTest0();
    
    void InitTest1();
    void ShutdownTest1();
    
    void InitTest2();
    void ShutdownTest2();
    
    void InitTest3();
    void ShutdownTest3();

    void InitTest4();
    void ShutdownTest4();

    void InitTest5();
    void ShutdownTest5();
   
    void InitTest6();
    void ShutdownTest6();

    
    void GenImpulse(WDL_TypedBuf<BL_FLOAT> *impulse);
    
    //
    BL_FLOAT mSampleRate;
    
    // TEST 0
    CrossoverSplitterNBands2 *mTest0Splitter;
    
    // TEST 1
    CrossoverSplitterNBands2 *mTest1Splitter;
    
    // TEST 2
    CrossoverSplitter3Bands *mTest2Splitter;
    
    // TEST 3
    CrossoverSplitterNBands *mTest3Splitter;
    
    // TEST 4
    CrossoverSplitterNBands3 *mTest4Splitter;
    
    // TEST 5
    CrossoverSplitterNBands3 *mTest5Splitter;
    
    // TEST 6
    FilterRBJNX *mTest6Filter;
};

#endif /* defined(__UST__TestFilter__) */
