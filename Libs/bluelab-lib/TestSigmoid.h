#ifndef TEST_SIGMOID_H
#define TEST_SIGMOID_H

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

class TestSigmoid
{
 public:
    static void RunTest()
    {
        int numValues = 1024;
        //BL_FLOAT a = 0.5;
        //BL_FLOAT a = 0.2;
        BL_FLOAT a = 0.05;
        
        WDL_TypedBuf<BL_FLOAT> tVec;
        tVec.Resize(numValues);

        for (int i = 0; i < tVec.GetSize(); i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(numValues + 1);
            tVec.Get()[i] = t;
        }

        BLDebug::DumpData("data0.txt", tVec);

        WDL_TypedBuf<BL_FLOAT> tVecSigmo = tVec;
        for (int i = 0; i < tVecSigmo.GetSize(); i++)
        {
            BL_FLOAT t = tVec.Get()[i];

            t = BLUtilsMath::ApplySigmoid(t, a);

            tVecSigmo.Get()[i] = t;
        }

        BLDebug::DumpData("data1.txt", tVecSigmo);
    }
};

#endif
