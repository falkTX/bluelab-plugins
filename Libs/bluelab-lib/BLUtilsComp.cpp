#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "BLUtilsComp.h"

#define USE_SIMD_OPTIM 1

void
BLUtilsComp::ComputeSquareConjugate(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf)
{
    int bufSize = buf->GetSize();
    WDL_FFT_COMPLEX *bufData = buf->Get();

    WDL_FFT_COMPLEX conj;
    WDL_FFT_COMPLEX tmp;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX &c = bufData[i];
        
        conj.re = c.re;
        conj.im = -c.im;

        tmp = c;
        
        COMP_MULT(tmp, conj, c);
    }
}

template <typename FLOAT_TYPE>
void
BLUtilsComp::ComplexSum(WDL_TypedBuf<FLOAT_TYPE> *ioMagns,
                        WDL_TypedBuf<FLOAT_TYPE> *ioPhases,
                        const WDL_TypedBuf<FLOAT_TYPE> &magns,
                        WDL_TypedBuf<FLOAT_TYPE> &phases)
{
    FLOAT_TYPE ioMagnsSize = ioMagns->GetSize();
    FLOAT_TYPE *ioMagnsData = ioMagns->Get();
    FLOAT_TYPE *ioPhasesData = ioPhases->Get();
    FLOAT_TYPE *magnsData = magns.Get();
    FLOAT_TYPE *phasesData = phases.Get();
    
    for (int i = 0; i < ioMagnsSize; i++)
    {
        FLOAT_TYPE magn0 = ioMagnsData[i];
        FLOAT_TYPE phase0 = ioPhasesData[i];
        
        WDL_FFT_COMPLEX comp0;
        BLUtilsComp::MagnPhaseToComplex(&comp0, magn0, phase0);
        
        FLOAT_TYPE magn1 = magnsData[i];
        FLOAT_TYPE phase1 = phasesData[i];
        
        WDL_FFT_COMPLEX comp1;
        BLUtilsComp::MagnPhaseToComplex(&comp1, magn1, phase1);
        
        comp0.re += comp1.re;
        comp0.im += comp1.im;
        
        BLUtilsComp::ComplexToMagnPhase(comp0, &magn0, &phase0);
        
        ioMagnsData[i] = magn0;
        ioPhasesData[i] = phase0;
    }
}
template void BLUtilsComp::ComplexSum(WDL_TypedBuf<float> *ioMagns,
                                      WDL_TypedBuf<float> *ioPhases,
                                      const WDL_TypedBuf<float> &magns,
                                      WDL_TypedBuf<float> &phases);
template void BLUtilsComp::ComplexSum(WDL_TypedBuf<double> *ioMagns,
                                      WDL_TypedBuf<double> *ioPhases,
                                      const WDL_TypedBuf<double> &magns,
                                      WDL_TypedBuf<double> &phases);
bool
BLUtilsComp::IsAllZeroComp(const WDL_TypedBuf<WDL_FFT_COMPLEX> &buffer)
{
#if !USE_SIMD_OPTIM
    int bufferSize = buffer.GetSize();
    WDL_FFT_COMPLEX *bufferData = buffer.Get();
    
    for (int i = 0; i < bufferSize; i++)
    {
        if (std::fabs(bufferData[i].re) > BL_EPS)
            return false;
        
        if (std::fabs(bufferData[i].im) > BL_EPS)
            return false;
    }
    
    return true;
#else
    int bufSize = buffer.GetSize()/**2*/;
    bool res = IsAllZeroComp(buffer.Get(), bufSize);
    
    return res;
#endif
}

bool
BLUtilsComp::IsAllZeroComp(const WDL_FFT_COMPLEX *buffer, int bufLen)
{
    for (int i = 0; i < bufLen; i++)
    {
        if (std::fabs(buffer[i].re) > BL_EPS)
            return false;
        if (std::fabs(buffer[i].im) > BL_EPS)
            return false;
    }
    
    return true;
}

template <typename FLOAT_TYPE>
void
BLUtilsComp::ComplexToMagnPhase(WDL_FFT_COMPLEX comp, FLOAT_TYPE *outMagn, FLOAT_TYPE *outPhase)
{
    *outMagn = COMP_MAGN(comp);
    
    *outPhase = std::atan2(comp.im, comp.re);
}
template void BLUtilsComp::ComplexToMagnPhase(WDL_FFT_COMPLEX comp, float *outMagn, float *outPhase);
template void BLUtilsComp::ComplexToMagnPhase(WDL_FFT_COMPLEX comp, double *outMagn, double *outPhase);

template <typename FLOAT_TYPE>
void
BLUtilsComp::MagnPhaseToComplex(WDL_FFT_COMPLEX *outComp, FLOAT_TYPE magn, FLOAT_TYPE phase)
{
    WDL_FFT_COMPLEX comp;
    comp.re = magn*std::cos(phase);
    comp.im = magn*std::sin(phase);
    
    *outComp = comp;
}
template void BLUtilsComp::MagnPhaseToComplex(WDL_FFT_COMPLEX *outComp, float magn, float phase);
template void BLUtilsComp::MagnPhaseToComplex(WDL_FFT_COMPLEX *outComp, double magn, double phase);

template <typename FLOAT_TYPE>
void
BLUtilsComp::ComplexToMagn(WDL_TypedBuf<FLOAT_TYPE> *result, const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf)
{
    result->Resize(complexBuf.GetSize());
    
    int complexBufSize = complexBuf.GetSize();
    WDL_FFT_COMPLEX *complexBufData = complexBuf.Get();
    FLOAT_TYPE *resultData = result->Get();
    
    for (int i = 0; i < complexBufSize; i++)
    {
        FLOAT_TYPE magn = COMP_MAGN(complexBufData[i]);
        resultData[i] = magn;
    }
}
template void BLUtilsComp::ComplexToMagn(WDL_TypedBuf<float> *result, const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
template void BLUtilsComp::ComplexToMagn(WDL_TypedBuf<double> *result, const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);

template <typename FLOAT_TYPE>
void
BLUtilsComp::ComplexToPhase(WDL_TypedBuf<FLOAT_TYPE> *result,
                            const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf)
{
    result->Resize(complexBuf.GetSize());
    
    int complexBufSize = complexBuf.GetSize();
    WDL_FFT_COMPLEX *complexBufData = complexBuf.Get();
    FLOAT_TYPE *resultData = result->Get();
    
    for (int i = 0; i < complexBufSize; i++)
    {
        FLOAT_TYPE phase = COMP_PHASE(complexBufData[i]);
        resultData[i] = phase;
    }
}
template void BLUtilsComp::ComplexToPhase(WDL_TypedBuf<float> *result,
                                          const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
template void BLUtilsComp::ComplexToPhase(WDL_TypedBuf<double> *result,
                                          const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);

template <typename FLOAT_TYPE>
void
BLUtilsComp::ComplexToMagnPhase(WDL_TypedBuf<FLOAT_TYPE> *resultMagn,
                                WDL_TypedBuf<FLOAT_TYPE> *resultPhase,
                                const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf)
{
    resultMagn->Resize(complexBuf.GetSize());
    resultPhase->Resize(complexBuf.GetSize());
    
    int complexBufSize = complexBuf.GetSize();
    WDL_FFT_COMPLEX *complexBufData = complexBuf.Get();
    FLOAT_TYPE *resultMagnData = resultMagn->Get();
    FLOAT_TYPE *resultPhaseData = resultPhase->Get();
    
    for (int i = 0; i < complexBufSize; i++)
    {
        FLOAT_TYPE magn = COMP_MAGN(complexBufData[i]);
        resultMagnData[i] = magn;
        
#if 1
        FLOAT_TYPE phase = std::atan2(complexBufData[i].im, complexBufData[i].re);
#endif
        
#if 0 // Make some leaks with diracs
        FLOAT_TYPE phase = DomainAtan2(complexBufData[i].im, complexBufData[i].re);
#endif
        resultPhaseData[i] = phase;
    }
}
template void BLUtilsComp::ComplexToMagnPhase(WDL_TypedBuf<float> *resultMagn,
                                              WDL_TypedBuf<float> *resultPhase,
                                              const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
template void BLUtilsComp::ComplexToMagnPhase(WDL_TypedBuf<double> *resultMagn,
                                              WDL_TypedBuf<double> *resultPhase,
                                              const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);


template <typename FLOAT_TYPE>
void
BLUtilsComp::MagnPhaseToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                                const WDL_TypedBuf<FLOAT_TYPE> &magns,
                                const WDL_TypedBuf<FLOAT_TYPE> &phases)
{
    //complexBuf->Resize(0);
    
    if (magns.GetSize() != phases.GetSize())
        // Error
        return;
    
    complexBuf->Resize(magns.GetSize());
    
    int magnsSize = magns.GetSize();
    FLOAT_TYPE *magnsData = magns.Get();
    FLOAT_TYPE *phasesData = phases.Get();
    WDL_FFT_COMPLEX *complexBufData = complexBuf->Get();
    
    for (int i = 0; i < magnsSize; i++)
    {
        FLOAT_TYPE magn = magnsData[i];
        FLOAT_TYPE phase = phasesData[i];
        
        WDL_FFT_COMPLEX res;
        res.re = magn*std::cos(phase);
        res.im = magn*std::sin(phase);
        
        complexBufData[i] = res;
    }
}
template void BLUtilsComp::MagnPhaseToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                                              const WDL_TypedBuf<float> &magns,
                                              const WDL_TypedBuf<float> &phases);
template void BLUtilsComp::MagnPhaseToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                                              const WDL_TypedBuf<double> &magns,
                                              const WDL_TypedBuf<double> &phases);


template <typename FLOAT_TYPE>
void BLUtilsComp::ComplexToReIm(WDL_TypedBuf<FLOAT_TYPE> *resultRe,
                                WDL_TypedBuf<FLOAT_TYPE> *resultIm,
                                const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf)
{
    resultRe->Resize(complexBuf.GetSize());
    resultIm->Resize(complexBuf.GetSize());
    
    for (int i = 0; i < complexBuf.GetSize(); i++)
    {
        const WDL_FFT_COMPLEX &c = complexBuf.Get()[i];
        resultRe->Get()[i] = c.re;
        resultIm->Get()[i] = c.im;
    }
}
template void BLUtilsComp::ComplexToReIm(WDL_TypedBuf<float> *resultRe,
                                         WDL_TypedBuf<float> *resultIm,
                                         const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
template void BLUtilsComp::ComplexToReIm(WDL_TypedBuf<double> *resultRe,
                                         WDL_TypedBuf<double> *resultIm,
                                         const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);

template <typename FLOAT_TYPE>
void BLUtilsComp::ReImToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                                const WDL_TypedBuf<FLOAT_TYPE> &reBuf,
                                const WDL_TypedBuf<FLOAT_TYPE> &imBuf)
{
    complexBuf->Resize(reBuf.GetSize());
    
    for (int i = 0; i < reBuf.GetSize(); i++)
    {
        WDL_FFT_COMPLEX c;
        c.re = reBuf.Get()[i];
        c.im = imBuf.Get()[i];
    
        complexBuf->Get()[i] = c;
    }
}
template void BLUtilsComp::ReImToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                                         const WDL_TypedBuf<float> &reBuf,
                                         const WDL_TypedBuf<float> &imBuf);
template void BLUtilsComp::ReImToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                                         const WDL_TypedBuf<double> &reBuf,
                                         const WDL_TypedBuf<double> &imBuf);

template <typename FLOAT_TYPE>
void
BLUtilsComp::InterpComp(FLOAT_TYPE magn0, FLOAT_TYPE phase0,
                        FLOAT_TYPE magn1, FLOAT_TYPE phase1,
                        FLOAT_TYPE t,
                        FLOAT_TYPE *resMagn, FLOAT_TYPE *resPhase)
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
template void BLUtilsComp::InterpComp(float magn0, float phase0,
                                      float magn1, float phase1,
                                      float t,
                                      float *resMagn, float *resPhase);
template void BLUtilsComp::InterpComp(double magn0, double phase0,
                                      double magn1, double phase1,
                                      double t,
                                      double *resMagn, double *resPhase);

template <typename FLOAT_TYPE>
void
BLUtilsComp::InterpComp(const WDL_TypedBuf<FLOAT_TYPE> &magns0,
                        const WDL_TypedBuf<FLOAT_TYPE> &phases0,
                        const WDL_TypedBuf<FLOAT_TYPE> &magns1,
                        const WDL_TypedBuf<FLOAT_TYPE> &phases1,
                        FLOAT_TYPE t,
                        WDL_TypedBuf<FLOAT_TYPE> *resMagns,
                        WDL_TypedBuf<FLOAT_TYPE> *resPhases)
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
        FLOAT_TYPE magn0 = magns0.Get()[i];
        FLOAT_TYPE phase0 = phases0.Get()[i];
        MAGN_PHASE_COMP(magn0, phase0, c0);

        // 1
        FLOAT_TYPE magn1 = magns1.Get()[i];
        FLOAT_TYPE phase1 = phases1.Get()[i];
        MAGN_PHASE_COMP(magn1, phase1, c1);

        // interp
        resC.re = (1.0 - t)*c0.re + t*c1.re;
        resC.im = (1.0 - t)*c0.im + t*c1.im;

        // res
        FLOAT_TYPE resMagn = COMP_MAGN(resC);
        FLOAT_TYPE resPhase = COMP_PHASE(resC);
        
        resMagns->Get()[i] = resMagn;
        resPhases->Get()[i] = resPhase;
    }
}
template void
BLUtilsComp::InterpComp(const WDL_TypedBuf<float> &magns0,
                        const WDL_TypedBuf<float> &phases0,
                        const WDL_TypedBuf<float> &magns1,
                        const WDL_TypedBuf<float> &phases1,
                        float t,
                        WDL_TypedBuf<float> *resMagns,
                        WDL_TypedBuf<float> *resPhases);

template void
BLUtilsComp::InterpComp(const WDL_TypedBuf<double> &magns0,
                        const WDL_TypedBuf<double> &phases0,
                        const WDL_TypedBuf<double> &magns1,
                        const WDL_TypedBuf<double> &phases1,
                        double t,
                        WDL_TypedBuf<double> *resMagns,
                        WDL_TypedBuf<double> *resPhases);
