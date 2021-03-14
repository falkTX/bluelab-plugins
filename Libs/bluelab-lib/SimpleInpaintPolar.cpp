#include <BLUtils.h>

#include <BLDebug.h>
// For debugging
#include <FftProcessObj16.h>

#include "SimpleInpaintPolar.h"

// TODO: in BLUtils, put TWO_PI in BLUtils.h
// and remove this
#define TWO_PI 6.28318530717959

// THIS WAS JUST A TEST...
//
// Do not touch the first phase, it correcponds to the DC offset
// NOTE: this should be set somewhere else, in GhostCommand!!
// NOTE; this only works here if we select the full spectro height
#define TMP_HACK_DC_OFFSET 0 //1

SimpleInpaintPolar::SimpleInpaintPolar(bool processHorizontal, bool processVertical)
{
    mProcessHorizontal = processHorizontal;
    mProcessVertical = processVertical;
}

SimpleInpaintPolar::~SimpleInpaintPolar() {}

void
SimpleInpaintPolar::Process(WDL_TypedBuf<BL_FLOAT> *magns,
                            WDL_TypedBuf<BL_FLOAT> *phases,
                            int width, int height)
{
    if ((width <= 2) || (height <= 2))
        return;

#define DBG_INDEX width/2
    
    DBG_DumpSignal("signal0.txt", magns, phases, width, height, DBG_INDEX);
    
    if (mProcessHorizontal && !mProcessVertical)
    {
        ProcessHorizontal(magns, width, height);

        //UnwrapPhasesHorizontal(phases, width, height);

        //ProcessPhasesHorizontal(phases, width, height);

        // NOTE: the current problem comes from here!
        ProcessPhasesHorizontal(phases, width, height);
        
        DBG_DumpSignal("signal1.txt", magns, phases, width, height, DBG_INDEX);
        
        return;
    }

    if (!mProcessHorizontal && mProcessVertical)
    {
        ProcessVertical(magns, width, height);
        ProcessVertical(phases, width, height);
        
        return;
    }

    // else...
    ProcessBothDir(magns, width, height);   
    ProcessBothDir(phases, width, height);   
}

void
SimpleInpaintPolar::ProcessHorizontal(WDL_TypedBuf<BL_FLOAT> *values,
                                      int width, int height)
{
#if !TMP_HACK_DC_OFFSET
    for (int j = 0; j < height; j++)
#else
    for (int j = 1; j < height; j++)
#endif
    {
        BL_FLOAT m0 = values->Get()[0 + j*width];
        BL_FLOAT m1 = values->Get()[(width - 1) + j*width];
        
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(width - 1);

            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            values->Get()[i + j*width] = m;
        }
    }
}

void
SimpleInpaintPolar::ProcessVertical(WDL_TypedBuf<BL_FLOAT> *values,
                                    int width, int height)
{
    for (int i = 0; i < width ; i++)
    {
        BL_FLOAT m0 = values->Get()[i + 0*width];
        BL_FLOAT m1 = values->Get()[i + (height - 1)*width];
        
        for (int j = 1; j < height - 1; j++)
        {
            BL_FLOAT t = ((BL_FLOAT)j)/(height - 1);

            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            values->Get()[i + j*width] = m;
        }
    }
}

void
SimpleInpaintPolar::ProcessBothDir(WDL_TypedBuf<BL_FLOAT> *values,
                                   int width, int height)
{
    WDL_TypedBuf<BL_FLOAT> horiz = *values;
    ProcessHorizontal(&horiz, width, height);

    WDL_TypedBuf<BL_FLOAT> vert = *values;
    ProcessHorizontal(&vert, width, height);

    for (int i = 0; i < values->GetSize(); i++)
    {
        BL_FLOAT h = horiz.Get()[i];
        BL_FLOAT v = vert.Get()[i];

        BL_FLOAT m = (h + v)*0.5;
        values->Get()[i] = m;
    }
}

#if 0
void
SimpleInpaintPolar::UnwrapPhasesHorizontal(WDL_TypedBuf<BL_FLOAT> *phases,
                                           int width, int height)
{
    for (int j = 0; j < height; j++)
    {
        BL_FLOAT m0 = phases->Get()[0 + j*width];
        BL_FLOAT m1 = phases->Get()[(width - 1) + j*width];

        BL_FLOAT uw = m1;
        BLUtils::FindNextPhase(&uw, m0);

        phases->Get()[(width - 1) + j*width] = uw;
    }
}
#endif

#if 0
void
SimpleInpaintPolar::ProcessPhasesHorizontal(WDL_TypedBuf<BL_FLOAT> *phases,
                                            int width, int height)
{
    if (width < 2)
        return;
    
    for (int j = 0; j < height; j++)
    {
        BL_FLOAT m0 = phases->Get()[0 + j*width];
        BL_FLOAT m01 = phases->Get()[1 + j*width];
        BL_FLOAT dphase = m01 - m0;
        
        BL_FLOAT m1 = phases->Get()[(width - 1) + j*width];
        BL_FLOAT uw = m1;
        //BLUtils::FindNextPhase(&uw, m0);

        //uw += 0.25*dphase*width;
        uw += dphase*width;
            
        phases->Get()[(width - 1) + j*width] = uw;
    }
}
#endif

void
SimpleInpaintPolar::ProcessPhasesHorizontal(WDL_TypedBuf<BL_FLOAT> *phases,
                                            int width, int height)
{
    //DBG_DumpPhaseRow("0-phases0.txt", 0, phases, width, height);
    //DBG_DumpPhaseRow("0-phases1.txt", 1, phases, width, height);
    //DBG_DumpPhaseRow("0-phases-end.txt", width - 2, phases, width, height);

#if !TMP_HACK_DC_OFFSET
    for (int j = 0; j < height; j++)
#else
    for (int j = 1; j < height; j++)
#endif
    {
        // First point
        BL_FLOAT m0 = phases->Get()[0 + j*width];

        // Compute phse derivative at point 0
        BL_FLOAT m01 = phases->Get()[1 + j*width];
#if 0 //1 // ORIGIN: works well (most of the time..)
        BL_FLOAT dphase = m01 - m0;
#endif
#if 1 //0// NEW: test
        BL_FLOAT m01Close = m01;
        FindClosestPhase(&m01Close, m0);
        BL_FLOAT dphase = m01Close - m0;
#endif
        
        // Last point
        BL_FLOAT m1 = phases->Get()[(width - 1) + j*width];

        // Extrapolated point
        //BL_FLOAT extra = m0 + dphase*width;
        //BL_FLOAT extra = m01 + dphase*(width - 1);
        BL_FLOAT extra = m0 + dphase*(width - 1); // TEST
        
        // Last point, wrapped to the extrapolated point
        //BLUtils::FindNextPhase(&m1, extra);
        FindClosestPhase(&m1, extra); // TEST
        
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(width - 1);

            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            phases->Get()[i + j*width] = m;
        }
    }

    //DBG_DumpPhaseRow("phases1.txt", 0, phases, width, height);
    //DBG_DumpPhaseRow("phases1.txt", width - 2, phases, width, height);
    //DBG_DumpPhaseRow("phases-end1.txt", width - 2, phases, width, height);

    //DBG_DumpPhaseRow("1-phases-end.txt", width - 2, phases, width, height);
}

#if 0 // ORIGIN: correct
void
SimpleInpaintPolar::FindClosestPhase(BL_FLOAT *phase, BL_FLOAT refPhase)
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
    
    //*phase = refPhase + (pMod - refMod);
    *phase = (refPhase - refMod) + pMod;
}
#endif

#if 1 // TEST: correct
void
SimpleInpaintPolar::FindClosestPhase(BL_FLOAT *phase, BL_FLOAT refPhase)
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
    
    //*phase = refPhase + (pMod - refMod);
    *phase = (refPhase - refMod) + pMod;
}
#endif

void
SimpleInpaintPolar::FindClosestPhase180(BL_FLOAT *phase, BL_FLOAT refPhase)
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
    
    //*phase = refPhase + (pMod - refMod);
    *phase = (refPhase - refMod) + pMod;
}

#if 0 // Correct, but makes some jumps
      // because we asjut a lot several successive times
// TODO: put this in BLUtils
// We must use "closest", not "next"
// See: https://ccrma.stanford.edu/~jos/fp/Phase_Unwrapping.html
void
SimpleInpaintPolar::UnwrapPhases2(WDL_TypedBuf<BL_FLOAT> *phases)
{
    if (phases->GetSize() == 0)
        // Empty phases
        return;
    
    BL_FLOAT prevPhase = phases->Get()[0];
    //FindNextPhase(&prevPhase, (BL_FLOAT)0.0);
    //FindClosestPhase(&prevPhase, (BL_FLOAT)0.0);
    
    int phasesSize = phases->GetSize();
    BL_FLOAT *phasesData = phases->Get();
    for (int i = 0; i < phasesSize; i++)
    {
        BL_FLOAT phase = phasesData[i];
        
        //FindNextPhase(&phase, prevPhase);
        FindClosestPhase(&phase, prevPhase);
        
        phasesData[i] = phase;
        
        prevPhase = phase;
    }
}
#endif


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
void
SimpleInpaintPolar::UnwrapPhases2(WDL_TypedBuf<BL_FLOAT> *phases,
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
SimpleInpaintPolar::DBG_DumpPhaseRow(const char *fileName, int index,
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
SimpleInpaintPolar::DBG_DumpSignal(const char *fileName,
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
    UnwrapPhases2(&phases0, true); // false
    
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
