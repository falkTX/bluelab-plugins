#include <BLUtils.h>
#include <BLUtilsPhases.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <BLDebug.h>

// For debug functions
#include <FftProcessObj16.h>

#include "SimpleInpaintPolar3.h"

SimpleInpaintPolar3::SimpleInpaintPolar3(bool processHoriz, bool processVert)
{
    mProcessHoriz = processHoriz;
    mProcessVert = processVert;
}

SimpleInpaintPolar3::~SimpleInpaintPolar3() {}

void
SimpleInpaintPolar3::Process(WDL_TypedBuf<BL_FLOAT> *magns,
                             WDL_TypedBuf<BL_FLOAT> *phases,
                             int width, int height)
{
    if ((width <= 2) || (height <= 2))
        return;
    
    if (mProcessHoriz && !mProcessVert)
    {
        ProcessMagnsHoriz(magns, width, height);
        ProcessPhasesHorizCombined(*magns, phases, width, height);
        
        return;
    }

    if (!mProcessHoriz && mProcessVert)
    {
        ProcessMagnsVert(magns, width, height);
        ProcessPhasesVertCombined(*magns, phases, width, height);
        
        return;
    }    
        
    // else...
    ProcessBothDir(magns, phases, width, height);   
}

void
SimpleInpaintPolar3::ProcessMagnsHoriz(WDL_TypedBuf<BL_FLOAT> *magns,
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
SimpleInpaintPolar3::ProcessMagnsVert(WDL_TypedBuf<BL_FLOAT> *magns,
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
SimpleInpaintPolar3::ProcessBothDir(WDL_TypedBuf<BL_FLOAT> *magns,
                                    WDL_TypedBuf<BL_FLOAT> *phases,
                                    int width, int height)
{
    // Horiz
    WDL_TypedBuf<BL_FLOAT> magns0 = *magns;
    ProcessMagnsHoriz(&magns0, width, height);
    WDL_TypedBuf<BL_FLOAT> phases0 = *phases;
    ProcessPhasesHorizCombined(magns0, &phases0, width, height);

    // Vert
    WDL_TypedBuf<BL_FLOAT> magns1 = *magns;
    ProcessMagnsVert(&magns1, width, height);
    WDL_TypedBuf<BL_FLOAT> phases1 = *phases;
    ProcessPhasesVertCombined(magns1, &phases1, width, height);

    // Naive method: simply mix the two results
    BLUtilsComp::InterpComp(magns0, phases0, magns1, phases1, 0.5, magns, phases);
}

void
SimpleInpaintPolar3::ProcessPhasesHorizLToR(WDL_TypedBuf<BL_FLOAT> *phases,
                                            int width, int height)
{
    for (int j = 0; j < height; j++)
    {
        // First point
        BL_FLOAT m0 = phases->Get()[0 + j*width];

        // Compute phse derivative at point 0
        BL_FLOAT m01 = phases->Get()[1 + j*width];

        BL_FLOAT m01Close = m01;
        BLUtilsPhases::FindClosestPhase(&m01Close, m0);
        BL_FLOAT dphase = m01Close - m0;
        
        // Last point
        BL_FLOAT m1 = phases->Get()[(width - 1) + j*width];

        // Extrapolated point, from the left derivative
        BL_FLOAT extra = m0 + dphase*(width - 1);
        
        // Last point, wrapped to the extrapolated point
        BLUtilsPhases::FindClosestPhase(&m1, extra);
        
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(width - 1);

            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            phases->Get()[i + j*width] = m;
        }
    }
}

void
SimpleInpaintPolar3::ProcessPhasesHorizRToL(WDL_TypedBuf<BL_FLOAT> *phases,
                                            int width, int height)
{
    for (int j = 0; j < height; j++)
    {
        // First point
        BL_FLOAT m0 = phases->Get()[(width - 1) + j*width];

        // Compute phse derivative at point 0
        BL_FLOAT m01 = phases->Get()[(width - 2) + j*width];

        BL_FLOAT m01Close = m01;
        BLUtilsPhases::FindClosestPhase(&m01Close, m0);
        BL_FLOAT dphase = m01Close - m0;
        
        // Last point
        BL_FLOAT m1 = phases->Get()[0 + j*width];

        // Extrapolated point, from the left derivative
        BL_FLOAT extra = m0 + dphase*(width - 1);
        
        // Last point, wrapped to the extrapolated point
        BLUtilsPhases::FindClosestPhase(&m1, extra);
        
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(width - 1);
            
            BL_FLOAT m = (1.0 - t)*m1 + t*m0;

            phases->Get()[i + j*width] = m;
        }
    }
}

// TEST: pure sine, cut, try to inpaint over half of the hole
void
SimpleInpaintPolar3::
ProcessPhasesHorizCombined(const WDL_TypedBuf<BL_FLOAT> &magns,
                           WDL_TypedBuf<BL_FLOAT> *phases,
                           int width, int height)
{
    // Process L -> R, then R -> L, then blend
        
    // L -> R
    WDL_TypedBuf<BL_FLOAT> phasesL = *phases;
    ProcessPhasesHorizLToR(&phasesL, width, height);
    
    // R -> L
    WDL_TypedBuf<BL_FLOAT> phasesR = *phases;
    ProcessPhasesHorizRToL(&phasesR, width, height);

    CombinePhaseHorizBiggestMagn(phasesL, phasesR,
                                 magns, phases, width, height);
}

// Method 3
// Take the biggest magn, used to choose the phase "direction"
void
SimpleInpaintPolar3::
CombinePhaseHorizBiggestMagn(const WDL_TypedBuf<BL_FLOAT> &phasesL,
                             const WDL_TypedBuf<BL_FLOAT> &phasesR,
                             const WDL_TypedBuf<BL_FLOAT> &magns,
                             WDL_TypedBuf<BL_FLOAT> *phasesResult,
                             int width, int height)
{
    phasesResult->Resize(phasesL.GetSize());
                         
    // Combine
    for (int j = 0; j < height; j++)
    {
        BL_FLOAT m0 = magns.Get()[0 + j*width];
        BL_FLOAT m1 = magns.Get()[(width - 1) + j*width];
        
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT p0 = phasesL.Get()[i + j*width];
            BL_FLOAT p1 = phasesR.Get()[i + j*width];

            // Take the most significant magn to choose
            // which phase direction we take
            //
            // To test: use a pure sine, make a hole,
            // and inpaint with region over one edge of the hole
            BL_FLOAT p = (m0 >= m1) ? p0 : p1;
                
            phasesResult->Get()[i + j*width] = p;
        }
    }
}

void
SimpleInpaintPolar3::ProcessPhasesVertDToU(WDL_TypedBuf<BL_FLOAT> *phases,
                                           int width, int height)
{
    for (int i = 0; i < width; i++)
    {
        // First point
        BL_FLOAT m0 = phases->Get()[i + 0*width];

        // Compute phse derivative at point 0
        BL_FLOAT m01 = phases->Get()[i + 1*width];

        BL_FLOAT m01Close = m01;
        BLUtilsPhases::FindClosestPhase(&m01Close, m0);
        BL_FLOAT dphase = m01Close - m0;
        
        // Last point
        BL_FLOAT m1 = phases->Get()[i + (height - 1)*width];

        // Extrapolated point, from the left derivative
        BL_FLOAT extra = m0 + dphase*(height - 1);
        
        // Last point, wrapped to the extrapolated point
        BLUtilsPhases::FindClosestPhase(&m1, extra);
        
        for (int j = 1; j < height - 1; j++)
        {
            BL_FLOAT t = ((BL_FLOAT)j)/(height - 1);

            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            phases->Get()[i + j*width] = m;
        }
    }
}

void
SimpleInpaintPolar3::ProcessPhasesVertUToD(WDL_TypedBuf<BL_FLOAT> *phases,
                                           int width, int height)
{
    for (int i = 0; i < width; i++)
    {
        // First point
        BL_FLOAT m0 = phases->Get()[i + (height - 1)*width];

        // Compute phse derivative at point 0
        BL_FLOAT m01 = phases->Get()[i + (height - 2)*width];

        BL_FLOAT m01Close = m01;
        BLUtilsPhases::FindClosestPhase(&m01Close, m0);
        BL_FLOAT dphase = m01Close - m0;
        
        // Last point
        BL_FLOAT m1 = phases->Get()[i + 0*width];

        // Extrapolated point, from the left derivative
        BL_FLOAT extra = m0 + dphase*(height - 1);
        
        // Last point, wrapped to the extrapolated point
        BLUtilsPhases::FindClosestPhase(&m1, extra);
        
        for (int j = 1; j < height - 1; j++)
        {
            BL_FLOAT t = ((BL_FLOAT)j)/(height - 1);
            
            BL_FLOAT m = (1.0 - t)*m1 + t*m0;

            phases->Get()[i + j*width] = m;
        }
    }
}

// TEST: pure impulse, cut, try to inpaint over half of the hole
void
SimpleInpaintPolar3::
ProcessPhasesVertCombined(const WDL_TypedBuf<BL_FLOAT> &magns,
                          WDL_TypedBuf<BL_FLOAT> *phases,
                          int width, int height)
{
    // Process D -> U, then U -> D, then blend
    
    // D -> U
    WDL_TypedBuf<BL_FLOAT> phasesL = *phases;
    ProcessPhasesVertDToU(&phasesL, width, height);
    
    // U -> D
    WDL_TypedBuf<BL_FLOAT> phasesR = *phases;
    ProcessPhasesVertUToD(&phasesR, width, height);

    // Combine
    CombinePhaseVertBiggestMagn(phasesL, phasesR,
                                magns, phases, width, height);
    
}

// Take the biggest magn, used to choose the phase "direction"
void
SimpleInpaintPolar3::
CombinePhaseVertBiggestMagn(const WDL_TypedBuf<BL_FLOAT> &phasesD,
                            const WDL_TypedBuf<BL_FLOAT> &phasesU,
                            const WDL_TypedBuf<BL_FLOAT> &magns,
                            WDL_TypedBuf<BL_FLOAT> *phasesResult,
                            int width, int height)
{
    phasesResult->Resize(phasesD.GetSize());
                         
    // Combine
    for (int i = 0; i < width; i++)
    {
        BL_FLOAT m0 = magns.Get()[i + 0*width];
        BL_FLOAT m1 = magns.Get()[i + (height - 1)*width];
        
        for (int j = 1; j < height - 1; j++)
        {
            BL_FLOAT p0 = phasesD.Get()[i + j*width];
            BL_FLOAT p1 = phasesU.Get()[i + j*width];

            // Take the most significant magn to choose
            // which phase direction we take
            //
            // To test: use a pure sine, make a hole,
            // and inpaint with region over one edge of the hole
            BL_FLOAT p = (m0 >= m1) ? p0 : p1;
                
            phasesResult->Get()[i + j*width] = p;
        }
    }
}

void
SimpleInpaintPolar3::DBG_DumpPhaseCol(const char *fileName,
                                      int timeIndex,
                                      WDL_TypedBuf<BL_FLOAT> *phases,
                                      int width, int height)
{
    WDL_TypedBuf<BL_FLOAT> buf;
    buf.Resize(height);

    for (int j = 0; j < height; j++)
    {
        BL_FLOAT val = phases->Get()[timeIndex + j*width];
        buf.Get()[j] = val;
    }
    
    //UnwrapPhases2(&buf);
    //UnwrapPhases2(&buf, true);
        
    BLDebug::DumpData(fileName, buf);
}

void
SimpleInpaintPolar3::DBG_DumpPhaseLine(const char *fileName,
                                       int freqIndex,
                                       WDL_TypedBuf<BL_FLOAT> *phases,
                                       int width, int height)
{
    WDL_TypedBuf<BL_FLOAT> buf;
    buf.Resize(width);

    for (int i = 0; i < width; i++)
    {
        BL_FLOAT val = phases->Get()[i + freqIndex*width];
        buf.Get()[i] = val;
    }
        
    BLDebug::DumpData(fileName, buf);
}

void
SimpleInpaintPolar3::DBG_DumpSignal(const char *fileName,
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
    BLUtilsPhases::UnwrapPhases2(&phases0, false); // true
    
    char phasesFileName[512];
    sprintf(phasesFileName, "phases-%s", fileName);
    BLDebug::DumpData(phasesFileName, phases0);
    
    //
    WDL_TypedBuf<WDL_FFT_COMPLEX> comp0;
    BLUtilsComp::MagnPhaseToComplex(&comp0, magns0, phases0);

    WDL_TypedBuf<WDL_FFT_COMPLEX> comp1;
    BLUtilsFft::FillSecondFftHalf(comp0, &comp1);

    WDL_TypedBuf<BL_FLOAT> samples;
    FftProcessObj16::FftToSamples(comp1, &samples);

    BLDebug::DumpData(fileName, samples);
}
