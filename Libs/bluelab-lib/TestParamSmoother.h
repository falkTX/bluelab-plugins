#ifndef TEST_PARAM_SMOOTHER_H
#define TEST_PARAM_SMOOTHER_H

#include <ParamSmoother.h>
#include <ParamSmoother2.h>

class TestParamSmoother
{
public:
    static void RunTest()
    {
        //int numIters = 4096;
        int numIters = 44100;

        // ParamSmoother
        //
        WDL_TypedBuf<BL_FLOAT> smootherBuf;
        smootherBuf.Resize(numIters);
        
        ParamSmoother smoother(0.0, 0.999);
        smoother.Update(); // Hack
        smoother.SetNewValue(1.0);
        for (int i = 0; i < numIters; i++)
        {
            BL_FLOAT val = smoother.GetCurrentValue();
            smoother.Update();
            
            smootherBuf.Get()[i] = val;
        }

        BLDebug::DumpData("data0.txt", smootherBuf);
        
        // ParamSmoother2
        //
        WDL_TypedBuf<BL_FLOAT> smoother2Buf;
        smoother2Buf.Resize(numIters);
        
        ParamSmoother2 smoother2(44100.0, 0.0);
        smoother2.SetTargetValue(1.0);
        for (int i = 0; i < numIters; i++)
        {
            BL_FLOAT val = smoother2.Process();
            
            smoother2Buf.Get()[i] = val;
        }

        BLDebug::DumpData("data1.txt", smoother2Buf);
    }
};

#endif
