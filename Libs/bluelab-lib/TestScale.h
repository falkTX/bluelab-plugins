#ifndef TEST_SCALE_H
#define TEST_SCALE_H

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <Scale.h>

#include <BLDebug.h>

class TestScale
{
 public:
    // Scale
    static void RunTest0()
    {
        Scale scale;
        BL_FLOAT minValue = 0.0;
        BL_FLOAT maxValue = 22050.0;
            
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

        WDL_TypedBuf<BL_FLOAT> tVecScale = tVec;
        for (int i = 0; i < tVecScale.GetSize(); i++)
        {
            BL_FLOAT t = tVec.Get()[i];

            t = scale.NormalizedToLowZoom(t, minValue, maxValue);

            tVecScale.Get()[i] = t;
        }

        BLDebug::DumpData("data1.txt", tVecScale);

        WDL_TypedBuf<BL_FLOAT> tVecScaleInv = tVecScale;
        for (int i = 0; i < tVecScaleInv.GetSize(); i++)
        {
            BL_FLOAT t = tVecScale.Get()[i];

            t = scale.NormalizedToLowZoomInv(t, minValue, maxValue);

            tVecScaleInv.Get()[i] = t;
        }

        BLDebug::DumpData("data2.txt", tVecScaleInv);
    }

    // Scale
    static void RunTest1()
    {
        Scale scale;
        BL_FLOAT minValue = 0.0;
        BL_FLOAT maxValue = 22050.0;
            
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

        WDL_TypedBuf<BL_FLOAT> tVecScale = tVec;
        for (int i = 0; i < tVecScale.GetSize(); i++)
        {
            BL_FLOAT t = tVec.Get()[i];

            t = scale.NormalizedToLowZoomInv(t, minValue, maxValue);

            tVecScale.Get()[i] = t;
        }

        BLDebug::DumpData("data1.txt", tVecScale);

        WDL_TypedBuf<BL_FLOAT> tVecScaleInv = tVecScale;
        for (int i = 0; i < tVecScaleInv.GetSize(); i++)
        {
            BL_FLOAT t = tVecScale.Get()[i];

            t = scale.NormalizedToLowZoom(t, minValue, maxValue);

            tVecScaleInv.Get()[i] = t;
        }

        BLDebug::DumpData("data2.txt", tVecScaleInv);
    }
};

#endif
