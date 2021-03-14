#include <BLUtils.h>

#include <BLDebug.h>

// For debug functions
#include <FftProcessObj16.h>

#include "SimpleInpaintPolar2.h"

// TODO: in BLUtils, put TWO_PI in BLUtils.h
// and remove this
#define TWO_PI 6.28318530717959

SimpleInpaintPolar2::SimpleInpaintPolar2(bool processHorizontal, bool processVertical)
{
    mProcessHorizontal = processHorizontal;
    mProcessVertical = processVertical;
}

SimpleInpaintPolar2::~SimpleInpaintPolar2() {}

void
SimpleInpaintPolar2::Process(WDL_TypedBuf<BL_FLOAT> *magns,
                             WDL_TypedBuf<BL_FLOAT> *phases,
                             int width, int height)
{
    if ((width <= 2) || (height <= 2))
        return;
    
    if (mProcessHorizontal && !mProcessVertical)
    {
        ProcessMagnsHorizontal(magns, width, height);
        ProcessPhasesHorizontal(phases, width, height);
        
        return;
    }

    if (!mProcessHorizontal && mProcessVertical)
    {
        ProcessMagnsVertical(magns, width, height);
        //ProcessVertical(phases, width, height);
        
        return;
    }

    // else...
    ProcessMagnsBothDir(magns, width, height);   
    //ProcessBothDir(phases, width, height);   
}

void
SimpleInpaintPolar2::ProcessMagnsHorizontal(WDL_TypedBuf<BL_FLOAT> *magns,
                                            int width, int height)
{
    for (int j = 0; j < height; j++)
    {
        BL_FLOAT m0 = magns->Get()[0 + j*width];
        BL_FLOAT m1 = magns->Get()[(width - 1) + j*width];
        
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(width - 1);

            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            magns->Get()[i + j*width] = m;
        }
    }
}

void
SimpleInpaintPolar2::ProcessMagnsVertical(WDL_TypedBuf<BL_FLOAT> *magns,
                                          int width, int height)
{
    for (int i = 0; i < width ; i++)
    {
        BL_FLOAT m0 = magns->Get()[i + 0*width];
        BL_FLOAT m1 = magns->Get()[i + (height - 1)*width];
        
        for (int j = 1; j < height - 1; j++)
        {
            BL_FLOAT t = ((BL_FLOAT)j)/(height - 1);

            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            magns->Get()[i + j*width] = m;
        }
    }
}

void
SimpleInpaintPolar2::ProcessMagnsBothDir(WDL_TypedBuf<BL_FLOAT> *magns,
                                         int width, int height)
{
    WDL_TypedBuf<BL_FLOAT> horiz = *magns;
    ProcessMagnsHorizontal(&horiz, width, height);

    WDL_TypedBuf<BL_FLOAT> vert = *magns;
    ProcessMagnsHorizontal(&vert, width, height);

    for (int i = 0; i < magns->GetSize(); i++)
    {
        BL_FLOAT h = horiz.Get()[i];
        BL_FLOAT v = vert.Get()[i];

        BL_FLOAT m = (h + v)*0.5;
        magns->Get()[i] = m;
    }
}

void
SimpleInpaintPolar2::ProcessPhasesHorizontal(WDL_TypedBuf<BL_FLOAT> *phases,
                                             int width, int height)
{
    for (int j = 0; j < height; j++)
    {
        // First point
        BL_FLOAT m0 = phases->Get()[0 + j*width];

        // Compute phse derivative at point 0
        BL_FLOAT m01 = phases->Get()[1 + j*width];
#if 0 //1 // ORIGIN: works well (most of the time..)
        BL_FLOAT dphase = m01 - m0;
#endif
#if 1 // NEW
        BL_FLOAT m01Close = m01;
        FindClosestPhase(&m01Close, m0);
        BL_FLOAT dphase = m01Close - m0;
#endif
        
        // Last point
        BL_FLOAT m1 = phases->Get()[(width - 1) + j*width];

        // Extrapolated point, from the left derivative
        BL_FLOAT extra = m0 + dphase*(width - 1);
        
        // Last point, wrapped to the extrapolated point
        FindClosestPhase(&m1, extra);
        
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(width - 1);

            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            phases->Get()[i + j*width] = m;
        }
    }
}

void
SimpleInpaintPolar2::FindClosestPhase(BL_FLOAT *phase, BL_FLOAT refPhase)
{
    BL_FLOAT refMod = BLUtils::fmod_negative(refPhase, (BL_FLOAT)TWO_PI);
    BL_FLOAT pMod = BLUtils::fmod_negative(*phase, (BL_FLOAT)TWO_PI);
    
    // Find closest
    if (std::fabs((pMod - (BL_FLOAT)TWO_PI) - refMod) <
        std::fabs(pMod - refMod))
        pMod -= TWO_PI;
    else
        if (std::fabs((pMod + (BL_FLOAT)TWO_PI) - refMod) <
            std::fabs(pMod - refMod))
            pMod += TWO_PI;
    
    *phase = (refPhase - refMod) + pMod;
}

// for debugging, use modulo PI insteaod of modulo TWO_PI
void
SimpleInpaintPolar2::FindClosestPhase180(BL_FLOAT *phase, BL_FLOAT refPhase)
{
    BL_FLOAT refMod = BLUtils::fmod_negative(refPhase, (BL_FLOAT)M_PI);
    BL_FLOAT pMod = BLUtils::fmod_negative(*phase, (BL_FLOAT)M_PI);
    
    // Find closest
    if (std::fabs((pMod - (BL_FLOAT)M_PI) - refMod) <
        std::fabs(pMod - refMod))
        pMod -= M_PI;
    else
        if (std::fabs((pMod + (BL_FLOAT)M_PI) - refMod) <
            std::fabs(pMod - refMod))
            pMod += M_PI;
    
    *phase = (refPhase - refMod) + pMod;
}

// TODO: put this in BLUtils
//
// We must use "closest", not "next"
// See: https://ccrma.stanford.edu/~jos/fp/Phase_Unwrapping.html
//
// Use smooth coeff to avoid several successive jumps
// that finally gets the data very far from the begining values
//
// NOTE: this is normal to have some jumps by PI
// See: https://ccrma.stanford.edu/~jos/fp/Example_Zero_Phase_Filter_Design.html#fig:remezexb
//
// Set dbgUnwrap180 to true in order to make module PI instead of TWO_PI
// (just for debugging)
void
SimpleInpaintPolar2::UnwrapPhases2(WDL_TypedBuf<BL_FLOAT> *phases,
                                   bool dbgUnwrap180)
{
#define UNWRAP_SMOOTH_COEFF 0.9 //0.5
    
    if (phases->GetSize() == 0)
        // Empty phases
        return;
    
    BL_FLOAT prevPhaseSmooth = phases->Get()[0];
    
    int phasesSize = phases->GetSize();
    BL_FLOAT *phasesData = phases->Get();
    for (int i = 0; i < phasesSize; i++)
    {
        BL_FLOAT phase = phasesData[i];

        if (!dbgUnwrap180)
            // Correct dehaviour
            FindClosestPhase(&phase, prevPhaseSmooth);
        else
            FindClosestPhase180(&phase, prevPhaseSmooth);
        
        phasesData[i] = phase;
        
        prevPhaseSmooth =
            (1.0 - UNWRAP_SMOOTH_COEFF)*phase + UNWRAP_SMOOTH_COEFF*prevPhaseSmooth;
    }
}

void
SimpleInpaintPolar2::DBG_DumpPhaseRow(const char *fileName, int index,
                                      WDL_TypedBuf<BL_FLOAT> *phases,
                                      int width, int height)
{
    WDL_TypedBuf<BL_FLOAT> buf;
    buf.Resize(height);

    for (int j = 0; j < height; j++)
    {
        BL_FLOAT val = phases->Get()[index + j*width];
        buf.Get()[j] = val;
    }

    //BLDebug::DumpData("raw.txt", buf);
    
    //UnwrapPhases2(&buf);
    //UnwrapPhases2(&buf, true);
        
    BLDebug::DumpData(fileName, buf);
}

void
SimpleInpaintPolar2::DBG_DumpSignal(const char *fileName,
                                    WDL_TypedBuf<BL_FLOAT> *magns,
                                    WDL_TypedBuf<BL_FLOAT> *phases,
                                    int width, int height,
                                    int index)
{
    //
    WDL_TypedBuf<BL_FLOAT> magns0;
    magns0.Resize(height);
    for (int j = 0; j < height; j++)
        magns0.Get()[j] = magns->Get()[index + j*width];

    //
    WDL_TypedBuf<BL_FLOAT> phases0;
    phases0.Resize(height);
    for (int j = 0; j < height; j++)
        phases0.Get()[j] = phases->Get()[index + j*width];

    //
    char magnsFileName[512];
    sprintf(magnsFileName, "magns-%s", fileName);
    BLDebug::DumpData(magnsFileName, magns0);

    //
    UnwrapPhases2(&phases0, false); // true
    
    char phasesFileName[512];
    sprintf(phasesFileName, "phases-%s", fileName);
    BLDebug::DumpData(phasesFileName, phases0);
    
    //
    WDL_TypedBuf<WDL_FFT_COMPLEX> comp0;
    BLUtils::MagnPhaseToComplex(&comp0, magns0, phases0);

    WDL_TypedBuf<WDL_FFT_COMPLEX> comp1;
    BLUtils::FillSecondFftHalf(comp0, &comp1);

    WDL_TypedBuf<BL_FLOAT> samples;
    FftProcessObj16::FftToSamples(comp1, &samples);

    BLDebug::DumpData(fileName, samples);
}
