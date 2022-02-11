#include <BLUtils.h>

#include <BLDebug.h>

// For debug functions
#include <FftProcessObj16.h>

#include "SimpleInpaintPolar2.h"

// TODO: in BLUtils, put TWO_PI in BLUtils.h
// and remove this
#define TWO_PI 6.28318530717959

SimpleInpaintPolar2::SimpleInpaintPolar2(bool processHoriz, bool processVert)
{
    mProcessHoriz = processHoriz;
    mProcessVert = processVert;
}

SimpleInpaintPolar2::~SimpleInpaintPolar2() {}

void
SimpleInpaintPolar2::Process(WDL_TypedBuf<BL_FLOAT> *magns,
                             WDL_TypedBuf<BL_FLOAT> *phases,
                             int width, int height)
{
    if ((width <= 2) || (height <= 2))
        return;
    
    if (mProcessHoriz && !mProcessVert)
    {
        ProcessMagnsHoriz(magns, width, height);
        //ProcessPhasesHorizontalLToR(phases, width, height); // ORIG
        //ProcessPhasesHorizontalRToL(phases, width, height);
        ProcessPhasesHorizCombined(*magns, phases, width, height);
        
        return;
    }

    if (!mProcessHoriz && mProcessVert)
    {
        ProcessMagnsVert(magns, width, height);
        //ProcessPhasesVertDToU(phases, width, height);
        //ProcessPhasesVertUToD(phases, width, height);
        ProcessPhasesVertCombined(*magns, phases, width, height);
        
        return;
    }

    
        
    // else...
    //ProcessMagnsBothDir(magns, width, height);   
    ProcessBothDir(magns, phases, width, height);   
}

void
SimpleInpaintPolar2::ProcessMagnsHoriz(WDL_TypedBuf<BL_FLOAT> *magns,
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
SimpleInpaintPolar2::ProcessMagnsVert(WDL_TypedBuf<BL_FLOAT> *magns,
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

// Still used ?
void
SimpleInpaintPolar2::ProcessMagnsBothDir(WDL_TypedBuf<BL_FLOAT> *magns,
                                         int width, int height)
{
    WDL_TypedBuf<BL_FLOAT> horiz = *magns;
    ProcessMagnsHoriz(&horiz, width, height);

    WDL_TypedBuf<BL_FLOAT> vert = *magns;
    ProcessMagnsHoriz(&vert, width, height);

    for (int i = 0; i < magns->GetSize(); i++)
    {
        BL_FLOAT h = horiz.Get()[i];
        BL_FLOAT v = vert.Get()[i];

        BL_FLOAT m = (h + v)*0.5;
        magns->Get()[i] = m;
    }
}

void
SimpleInpaintPolar2::ProcessBothDir(WDL_TypedBuf<BL_FLOAT> *magns,
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
    InterpComp(magns0, phases0, magns1, phases1, 0.5, magns, phases);
}

void
SimpleInpaintPolar2::ProcessPhasesHorizLToR(WDL_TypedBuf<BL_FLOAT> *phases,
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

            // TEST
            //#define SIGMO_A 0.25 //0.5 //0.95 //0.05
            //t = BLUtils::ApplySigmoid(t, SIGMO_A);
            
            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            phases->Get()[i + j*width] = m;
        }
    }
}

void
SimpleInpaintPolar2::ProcessPhasesHorizRToL(WDL_TypedBuf<BL_FLOAT> *phases,
                                            int width, int height)
{
    for (int j = 0; j < height; j++)
    {
        // First point
        //BL_FLOAT m0 = phases->Get()[0 + j*width];
        BL_FLOAT m0 = phases->Get()[(width - 1) + j*width];

        // Compute phse derivative at point 0
        //BL_FLOAT m01 = phases->Get()[1 + j*width];
        BL_FLOAT m01 = phases->Get()[(width - 2) + j*width];
#if 0 //1 // ORIGIN: works well (most of the time..)
        BL_FLOAT dphase = m01 - m0;
#endif
#if 1 // NEW
        BL_FLOAT m01Close = m01;
        FindClosestPhase(&m01Close, m0);
        BL_FLOAT dphase = m01Close - m0;
#endif
        
        // Last point
        //BL_FLOAT m1 = phases->Get()[(width - 1) + j*width];
        BL_FLOAT m1 = phases->Get()[0 + j*width];

        // Extrapolated point, from the left derivative
        BL_FLOAT extra = m0 + dphase*(width - 1);
        
        // Last point, wrapped to the extrapolated point
        FindClosestPhase(&m1, extra);
        
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(width - 1);
            
            //BL_FLOAT m = (1.0 - t)*m0 + t*m1;
            BL_FLOAT m = (1.0 - t)*m1 + t*m0;

            phases->Get()[i + j*width] = m;
        }
    }
}

// TEST: pure sine, cut, try to inpaint over half of the hole
void
SimpleInpaintPolar2::
ProcessPhasesHorizCombined(const WDL_TypedBuf<BL_FLOAT> &magns,
                           WDL_TypedBuf<BL_FLOAT> *phases,
                           int width, int height)
{
    // Process L -> R, then R -> L, then blend

    //#define DBG_INDEX 22
    //DBG_DumpPhaseLine("phases0.txt", DBG_INDEX, phases, width, height);
        
    // L -> R
    WDL_TypedBuf<BL_FLOAT> phasesL = *phases;
    ProcessPhasesHorizLToR(&phasesL, width, height);

    //DBG_DumpPhaseLine("phases1.txt", DBG_INDEX, &phasesL, width, height);
    
    // R -> L
    WDL_TypedBuf<BL_FLOAT> phasesR = *phases;
    ProcessPhasesHorizRToL(&phasesR, width, height);

    //DBG_DumpPhaseLine("phases2.txt", DBG_INDEX, &phasesR, width, height);

    //CombinePhaseHorizInterp(phasesL, phasesR, phases, width, height);
    //CombinePhaseHorizInterpComp(phasesL, phasesR, magns, phases, width, height);
    CombinePhaseHorizBiggestMagn(phasesL, phasesR,
                                 magns, phases, width, height);
    
    
    //DBG_DumpPhaseLine("phases3.txt", DBG_INDEX, phases, width, height);
}

// Method 1 (not good)
void
SimpleInpaintPolar2::CombinePhaseHorizInterp(const WDL_TypedBuf<BL_FLOAT> &phasesL,
                                             const WDL_TypedBuf<BL_FLOAT> &phasesR,
                                             WDL_TypedBuf<BL_FLOAT> *phasesResult,
                                             int width, int height)
{
    phasesResult->Resize(phasesL.GetSize());
                         
    // Combine
    for (int j = 0; j < height; j++)
    {
        BL_FLOAT offset = phasesL.Get()[j*width] - phasesR.Get()[j*width];
            
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT p0 = phasesL.Get()[i + j*width];
            BL_FLOAT p1 = phasesR.Get()[i + j*width];

            p1 += offset;
            
            BL_FLOAT t = ((BL_FLOAT)i)/(width - 1);

#define SIGMO_A 0.95 //0.05 //0.25 //0.5 //0.95 //0.05
            t = BLUtils::ApplySigmoid(t, SIGMO_A);
            
            BL_FLOAT p = (1.0 - t)*p0 + t*p1;

            phasesResult->Get()[i + j*width] = p;
        }
    }
}

// Method 2
void
SimpleInpaintPolar2::
CombinePhaseHorizInterpComp(const WDL_TypedBuf<BL_FLOAT> &phasesL,
                            const WDL_TypedBuf<BL_FLOAT> &phasesR,
                            const WDL_TypedBuf<BL_FLOAT> &magns,
                            WDL_TypedBuf<BL_FLOAT> *phasesResult,
                            int width, int height)
{
    phasesResult->Resize(phasesL.GetSize());
                         
    // Combine
    for (int j = 0; j < height; j++)
    {
        BL_FLOAT offset = phasesL.Get()[j*width] - phasesR.Get()[j*width];
            
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT m = magns.Get()[i + j*width];
            
            BL_FLOAT p0 = phasesL.Get()[i + j*width];
            BL_FLOAT p1 = phasesR.Get()[i + j*width];
            p1 += offset;
            
            BL_FLOAT t = ((BL_FLOAT)i)/(width - 1);

            //#define SIGMO_A 0.95 //0.05 //0.25 //0.5 //0.95 //0.05
            //t = BLUtils::ApplySigmoid(t, SIGMO_A);

            BL_FLOAT p;
            BL_FLOAT resMagn;
            InterpComp(m, p0, m, p1, t, &resMagn, &p);
            
            //BL_FLOAT p = (1.0 - t)*p0 + t*p1;

            phasesResult->Get()[i + j*width] = p;

            // Useful ?
            // NOTE: seems worse with it...
            //ioMagns->Get()[i + j*width] = resMagn;
        }
    }
}

// Method 3
// Take the biggest magn, used to choose the phase "direction"
void
SimpleInpaintPolar2::
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
SimpleInpaintPolar2::ProcessPhasesVertDToU(WDL_TypedBuf<BL_FLOAT> *phases,
                                           int width, int height)
{
    for (int i = 0; i < width; i++)
    {
        // First point
        BL_FLOAT m0 = phases->Get()[i + 0*width];

        // Compute phse derivative at point 0
        BL_FLOAT m01 = phases->Get()[i + 1*width];

        BL_FLOAT m01Close = m01;
        FindClosestPhase(&m01Close, m0);
        BL_FLOAT dphase = m01Close - m0;
        
        // Last point
        BL_FLOAT m1 = phases->Get()[i + (height - 1)*width];

        // Extrapolated point, from the left derivative
        BL_FLOAT extra = m0 + dphase*(height - 1);
        
        // Last point, wrapped to the extrapolated point
        FindClosestPhase(&m1, extra);
        
        for (int j = 1; j < height - 1; j++)
        {
            BL_FLOAT t = ((BL_FLOAT)j)/(height - 1);

            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            phases->Get()[i + j*width] = m;
        }
    }
}

void
SimpleInpaintPolar2::ProcessPhasesVertUToD(WDL_TypedBuf<BL_FLOAT> *phases,
                                           int width, int height)
{
    for (int i = 0; i < width; i++)
    {
        // First point
        BL_FLOAT m0 = phases->Get()[i + (height - 1)*width];

        // Compute phse derivative at point 0
        BL_FLOAT m01 = phases->Get()[i + (height - 2)*width];

        BL_FLOAT m01Close = m01;
        FindClosestPhase(&m01Close, m0);
        BL_FLOAT dphase = m01Close - m0;
        
        // Last point
        //BL_FLOAT m1 = phases->Get()[(width - 1) + j*width];
        BL_FLOAT m1 = phases->Get()[i + 0*width];

        // Extrapolated point, from the left derivative
        BL_FLOAT extra = m0 + dphase*(height - 1);
        
        // Last point, wrapped to the extrapolated point
        FindClosestPhase(&m1, extra);
        
        for (int j = 1; j < height - 1; j++)
        {
            BL_FLOAT t = ((BL_FLOAT)j)/(height - 1);
            
            //BL_FLOAT m = (1.0 - t)*m0 + t*m1;
            BL_FLOAT m = (1.0 - t)*m1 + t*m0;

            phases->Get()[i + j*width] = m;
        }
    }
}

// TEST: pure impulse, cut, try to inpaint over half of the hole
void
SimpleInpaintPolar2::
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
SimpleInpaintPolar2::
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
SimpleInpaintPolar2::InterpComp(BL_FLOAT magn0, BL_FLOAT phase0,
                                BL_FLOAT magn1, BL_FLOAT phase1,
                                BL_FLOAT t,
                                BL_FLOAT *resMagn, BL_FLOAT *resPhase)
{
    WDL_FFT_COMPLEX c0;
    MAGN_PHASE_COMP(magn0, phase0, c0);

    WDL_FFT_COMPLEX c1;
    MAGN_PHASE_COMP(magn1, phase1, c1);

    WDL_FFT_COMPLEX resC;
    resC.re = (1.0 - t)*c0.re + t*c1.re;
    resC.im = (1.0 - t)*c0.im + t*c1.im;

    *resMagn = COMP_MAGN(resC);
    *resPhase = COMP_PHASE(resC);
}

void
SimpleInpaintPolar2::InterpComp(const WDL_TypedBuf<BL_FLOAT> &magns0,
                                const WDL_TypedBuf<BL_FLOAT> &phases0,
                                const WDL_TypedBuf<BL_FLOAT> &magns1,
                                const WDL_TypedBuf<BL_FLOAT> &phases1,
                                BL_FLOAT t,
                                WDL_TypedBuf<BL_FLOAT> *resMagns,
                                WDL_TypedBuf<BL_FLOAT> *resPhases)
{
    // Check input size
    if (magns0.GetSize() != phases0.GetSize())
        return;
    if (magns1.GetSize() != phases1.GetSize())
        return;
    if (magns0.GetSize() != magns1.GetSize())
        return;

    // Resize result
    resMagns->Resize(magns0.GetSize());
    resPhases->Resize(phases0.GetSize());

    WDL_FFT_COMPLEX c0;
    WDL_FFT_COMPLEX c1;
    WDL_FFT_COMPLEX resC;
    for (int i = 0; i < magns0.GetSize(); i++)
    {
        // 0
        BL_FLOAT magn0 = magns0.Get()[i];
        BL_FLOAT phase0 = phases0.Get()[i];
        MAGN_PHASE_COMP(magn0, phase0, c0);

        // 1
        BL_FLOAT magn1 = magns1.Get()[i];
        BL_FLOAT phase1 = phases1.Get()[i];
        MAGN_PHASE_COMP(magn1, phase1, c1);

        // interp
        resC.re = (1.0 - t)*c0.re + t*c1.re;
        resC.im = (1.0 - t)*c0.im + t*c1.im;

        // res
        BL_FLOAT resMagn = COMP_MAGN(resC);
        BL_FLOAT resPhase = COMP_PHASE(resC);
        
        resMagns->Get()[i] = resMagn;
        resPhases->Get()[i] = resPhase;
    }
}

void
SimpleInpaintPolar2::DBG_DumpPhaseCol(const char *fileName,
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

    //BLDebug::DumpData("raw.txt", buf);
    
    //UnwrapPhases2(&buf);
    //UnwrapPhases2(&buf, true);
        
    BLDebug::DumpData(fileName, buf);
}

void
SimpleInpaintPolar2::DBG_DumpPhaseLine(const char *fileName,
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
