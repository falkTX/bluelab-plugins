//
//  TestSimd.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/17/20.
//
//

#ifndef __BL_SoundMetaViewer__TestSimd__
#define __BL_SoundMetaViewer__TestSimd__

class TestSimd
{
public:
    TestSimd(bool activateSimd);
    
    virtual ~TestSimd();
    
    void Test();
    
protected:
    // Performance test
    void TestPerfs();
    
    bool Compare(const WDL_TypedBuf<BL_FLOAT> results[2]);
    
    // Unit tests
    void Test0(WDL_TypedBuf<BL_FLOAT> *result);
};

#endif /* defined(__BL_SoundMetaViewer__TestSimd__) */
