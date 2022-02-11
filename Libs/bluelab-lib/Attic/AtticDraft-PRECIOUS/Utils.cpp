//
//  Utils.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#include <math.h>


// For AmpToDB
#include "../../WDL/IPlug/Containers.h"

#include "Utils.h"
#include "CMA2Smoother.h"
#include "Debug.h"

double Utils::ampToDB(double amp, double minDB)
{
    if (amp <= 0.0)
        return minDB;
    
    double db = 20. * log10(amp);
    
    return db;
}

double
Utils::ComputeRMSAvg(const double *output, int nFrames)
{
    double avg = 0.0;
    
    for (int i = 0; i < nFrames; i++)
    {
        double value = output[i];
        avg += value*value;
    }
    
    avg = sqrt(avg/nFrames);
    
    //avg = sqrt(avg)/nFrames;
    
    return avg;
}

double
Utils::ComputeRMSAvg2(const double *output, int nFrames)
{
    double avg = 0.0;
    
    for (int i = 0; i < nFrames; i++)
    {
        double value = output[i];
        avg += value*value;
    }
    
    avg = sqrt(avg)/nFrames;
    
    return avg;
}

double
Utils::ComputeRMSAvg2(const WDL_TypedBuf<double> &buf)
{
    return ComputeRMSAvg2(buf.Get(), buf.GetSize());
}

double
Utils::ComputeAvg(const double *buf, int nFrames)
{
    double avg = 0.0;
    
    for (int i = 0; i < nFrames; i++)
    {
        double value = buf[i];
        avg += value;
    }
    
    if (nFrames > 0)
        avg = avg/nFrames;
    
    return avg;
}

double
Utils::ComputeAvg(const WDL_TypedBuf<double> &buf)
{
    return ComputeAvg(buf.Get(), buf.GetSize());
}

double
Utils::ComputeAvg(const WDL_TypedBuf<double> &buf, int startIndex, int endIndex)
{
    double sum = 0.0;
    int numValues = 0;
    
    for (int i = startIndex; i <= endIndex ; i++)
    {
        if (i < 0)
            continue;
        if (i >= buf.GetSize())
            break;
        
        double value = buf.Get()[i];
        sum += value;
        numValues++;
    }
    
    double avg = 0.0;
    if (numValues > 0)
        avg = sum/numValues;
    
    return avg;
}

double
Utils::ComputeAvgSquare(const double *buf, int nFrames)
{
    double avg = 0.0;
    
    for (int i = 0; i < nFrames; i++)
    {
        double value = buf[i];
        avg += value*value;
    }
    
    if (nFrames > 0)
        avg = sqrt(avg)/nFrames;
    
    return avg;
}

double
Utils::ComputeAvgSquare(const WDL_TypedBuf<double> &buf)
{
    return ComputeAvgSquare(buf.Get(), buf.GetSize());
}

double
Utils::ComputeAbsAvg(const double *output, int nFrames)
{
    double avg = 0.0;
    
    for (int i = 0; i < nFrames; i++)
    {
        double value = output[i];
        avg += fabs(value);
    }
    
    avg = avg/nFrames;
    
    return avg;
}

double
Utils::ComputeAbsAvg(const WDL_TypedBuf<double> &buf)
{
    return ComputeAbsAvg(buf.Get(), buf.GetSize());
}

double
Utils::ComputeAbsAvg(const WDL_TypedBuf<double> &buf, int startIndex, int endIndex)
{
    double sum = 0.0;
    int numValues = 0;
    
    for (int i = startIndex; i <= endIndex ; i++)
    {
        if (i < 0)
            continue;
        if (i >= buf.GetSize())
            break;
        
        double value = buf.Get()[i];
        value = fabs(value);
        
        sum += value;
        numValues++;
    }
    
    double avg = 0.0;
    if (numValues > 0)
        avg = sum/numValues;
    
    return avg;
}

double
Utils::ComputeMax(const WDL_TypedBuf<double> &buf)
{
    return ComputeMax(buf.Get(), buf.GetSize());
}

double
Utils::ComputeMax(const double *output, int nFrames)
{
    double max = -1e16;
    
    for (int i = 0; i < nFrames; i++)
    {
        double value = output[i];
        
        if (value > max)
            max = value;
    }
    
    return max;
}

double
Utils::ComputeMaxAbs(const WDL_TypedBuf<double> &buf)
{
    double max = 0.0;
    for (int i = 0; i < buf.GetSize(); i++)
    {
        double value = buf.Get()[i];
        
        value = fabs(value);
        
        if (value > max)
            max = value;
    }
    
    return max;
}


double
Utils::ComputeSum(const WDL_TypedBuf<double> &buf)
{
    double result = 0.0;
    
    for (int i = 0; i < buf.GetSize(); i++)
    {
        double val = buf.Get()[i];
        
        result += val;
    }
    
    return result;
}

double
Utils::ComputeAbsSum(const WDL_TypedBuf<double> &buf)
{
    double result = 0.0;
    
    for (int i = 0; i < buf.GetSize(); i++)
    {
        double val = buf.Get()[i];
        
        result += fabs(val);
    }
    
    return result;
}

double
Utils::ComputeClipSum(const WDL_TypedBuf<double> &buf)
{
    double result = 0.0;
    
    for (int i = 0; i < buf.GetSize(); i++)
    {
        double val = buf.Get()[i];
        
        // Clip if necessary
        if (val < 0.0)
            val = 0.0;
        
        result += val;
    }
    
    return result;
}

void
Utils::ComputeSum(const WDL_TypedBuf<double> &buf0,
                  const WDL_TypedBuf<double> &buf1,
                  WDL_TypedBuf<double> *result)
{
    if (buf0.GetSize() != buf1.GetSize())
        return;
    
    result->Resize(buf0.GetSize());
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        double val0 = buf0.Get()[i];
        double val1 = buf1.Get()[i];
        
        double sum = val0 + val1;
        
        result->Get()[i] = sum;
    }
}

void
Utils::ComputeProduct(const WDL_TypedBuf<double> &buf0,
                      const WDL_TypedBuf<double> &buf1,
                      WDL_TypedBuf<double> *result)
{
    if (buf0.GetSize() != buf1.GetSize())
        return;
    
    result->Resize(buf0.GetSize());
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        double val0 = buf0.Get()[i];
        double val1 = buf1.Get()[i];
        
        double prod = val0 * val1;
        
        result->Get()[i] = prod;
    }
}

double
Utils::NormalizedXTodB(double x, double mindB, double maxdB)
{
    x = x*(maxdB - mindB) + mindB;
    
    if (x > 0.0)
        x = ::AmpToDB(x);
        
    double lMin = ::AmpToDB(mindB);
    double lMax = ::AmpToDB(maxdB);
        
    x = (x - lMin)/(lMax - lMin);
    
    return x;
}

double
Utils::NormalizedYTodB(double y, double mindB, double maxdB)
{
#define EPS 1e-16
#define INF 1e16
    
    //if (y < EPS)
    //y = -INF;
    
    if (fabs(y) < EPS)
        y = mindB;
    else
        y = ::AmpToDB(y);
    
    y = (y - mindB)/(maxdB - mindB);
    
    return y;
}

double
Utils::NormalizedYTodB2(double y, double mindB, double maxdB)
{
    y = ::AmpToDB(y);
    
    y = (y - mindB)/(maxdB - mindB);
    
    return y;
}

double
Utils::NormalizedYTodB3(double y, double mindB, double maxdB)
{
    y = y*(maxdB - mindB) + mindB;
    
    if (y > 0.0)
        y = ::AmpToDB(y);
    
    double lMin = ::AmpToDB(mindB);
    double lMax = ::AmpToDB(maxdB);
    
    y = (y - lMin)/(lMax - lMin);
    
    return y;
}

double
Utils::NormalizedYTodBInv(double y, double mindB, double maxdB)
{
    double result = y*(maxdB - mindB) + mindB;
    
    result = DBToAmp(result);
    
    return result;
}

double
Utils::AverageYDB(double y0, double y1, double mindB, double maxdB)
{
    double y0Norm = NormalizedYTodB(y0, mindB, maxdB);
    double y1Norm = NormalizedYTodB(y1, mindB, maxdB);
    
    double avg = (y0Norm + y1Norm)/2.0;
    
    double result = NormalizedYTodBInv(avg, mindB, maxdB);
    
    return result;
}

bool
Utils::IsAllZero(const WDL_TypedBuf<double> &buffer)
{
    return IsAllZero(buffer.Get(), buffer.GetSize());
}

bool
Utils::IsAllZero(const double *buffer, int nFrames)
{
    if (buffer == NULL)
        return true;
    
#define EPS 1e-16
    
    for (int i = 0; i < nFrames; i++)
    {
        double val = fabs(buffer[i]);
        if (val > EPS)
            return false;
    }
            
    return true;
}

void
Utils::FillAllZero(WDL_TypedBuf<double> *ioBuf)
{
    for (int i = 0; i < ioBuf->GetSize(); i++)
        ioBuf->Get()[i] = 0.0;
}

void
Utils::FillAllZero(WDL_TypedBuf<int> *ioBuf)
{
    for (int i = 0; i < ioBuf->GetSize(); i++)
        ioBuf->Get()[i] = 0;
}

void
Utils::FillAllValue(WDL_TypedBuf<double> *ioBuf, double val)
{
    for (int i = 0; i < ioBuf->GetSize(); i++)
        ioBuf->Get()[i] = val;
}

void
Utils::AddZeros(WDL_TypedBuf<double> *ioBuf, int size)
{
    int prevSize = ioBuf->GetSize();
    int newSize = prevSize + size;
    
    ioBuf->Resize(newSize);
    
    for (int i = prevSize; i < newSize; i++)
        ioBuf->Get()[i] = 0.0;
}

void
Utils::StereoToMono(WDL_TypedBuf<double> *monoResult,
                    const double *in0, const double *in1, int nFrames)
{
    if ((in0 == NULL) && (in1 == NULL))
        return;
    
    monoResult->Resize(nFrames);
    
    // Crashes with App mode, when using sidechain
    if ((in0 != NULL) && (in1 != NULL))
    {
        for (int i = 0; i < nFrames; i++)
            monoResult->Get()[i] = (in0[i] + in1[i])/2.0;
    }
    else
    {
        for (int i = 0; i < nFrames; i++)
            monoResult->Get()[i] = (in0 != NULL) ? in0[i] : in1[i];
    }
}

void
Utils::StereoToMono(WDL_TypedBuf<double> *monoResult,
                    const WDL_TypedBuf<double> &in0,
                    const WDL_TypedBuf<double> &in1)
{
    if ((in0.GetSize() == 0) && (in1.GetSize() == 0))
        return;
    
    monoResult->Resize(in0.GetSize());
    
    // Crashes with App mode, when using sidechain
    if ((in0.GetSize() > 0) && (in1.GetSize() > 0))
    {
        for (int i = 0; i < in0.GetSize(); i++)
            monoResult->Get()[i] = (in0.Get()[i] + in1.Get()[i])/2.0;
    }
    else
    {
        for (int i = 0; i < in0.GetSize(); i++)
            monoResult->Get()[i] = (in0.GetSize() > 0) ? in0.Get()[i] : in1.Get()[i];
    }
}

// Compute the similarity between two normalized curves
double
Utils::ComputeCurveMatchCoeff(const double *curve0, const double *curve1, int nFrames)
{
    // Use POW_COEFF, to make more difference between matching and out of matching
    // (added for EQHack)
#define POW_COEFF 4.0
    
    double area = 0.0;
    for (int i = 0; i < nFrames; i++)
    {
        double val0 = curve0[i];
        double val1 = curve1[i];
        
        double a = fabs(val0 - val1);
        
        a = pow(a, POW_COEFF);
        
        area += a;
    }
    
    double coeff = area / nFrames;
    
    //
    coeff = pow(coeff, 1.0/POW_COEFF);
    
    // Should be Normalized
    
    // Clip, just in case.
    if (coeff < 0.0)
        coeff = 0.0;

    if (coeff > 1.0)
        coeff = 1.0;
    
    // If coeff is 1, this is a perfect match
    // so reverse
    coeff = 1.0 - coeff;
    
    return coeff;
}

void
Utils::BypassPlug(double **inputs, double **outputs, int nFrames)
{
    if ((inputs[0] != NULL) && (outputs[0] != NULL))
        memcpy(outputs[0], inputs[0], nFrames*sizeof(double));

    if ((inputs[1] != NULL) && (outputs[1] != NULL))
        memcpy(outputs[1], inputs[1], nFrames*sizeof(double));
}

void
Utils::GetPlugIOBuffers(IPlug *plug, double **inputs, double **outputs,
                        double *in[2], double *scIn[2], double *out[2])
{
    int numInChannels = plug->NInChannels();
    int numInScChannels = plug->NInScChannels();
    
#ifdef AAX_API
    // Protools only takes one channel for side chain
    
    // force it to 1, just in case
    if (numInScChannels > 1)
        numInScChannels = 1;
#endif
    
    bool isInConnected[4] = { false, false, false, false };
    
    for (int i = 0; i < numInChannels; i++)
    {
        isInConnected[i] = plug->IsInChannelConnected(i);
    }
    
    // in
    in[0] = ((numInChannels - numInScChannels > 0) &&
	     isInConnected[0]) ? inputs[0] : NULL;
    in[1] = ((numInChannels - numInScChannels >  1) &&
	     isInConnected[1]) ? inputs[1] : NULL;
    
#ifndef SA_API
    // scin
    scIn[0] = ((numInScChannels > 0) && isInConnected[numInChannels - numInScChannels]) ?
                inputs[numInChannels - numInScChannels] : NULL;
    scIn[1] = ((numInScChannels > 1) && isInConnected[numInChannels - numInScChannels + 1]) ?
                inputs[numInChannels - numInScChannels + 1] : NULL;
#else
    // When in application mode, must deactivate sidechains
    // BUG: otherwise it crashes if we try to get sidechains
    scIn[0] = NULL;
    scIn[1] = NULL;
#endif
    
    // out
    out[0] = plug->IsOutChannelConnected(0) ? outputs[0] : NULL;
    out[1] = plug->IsOutChannelConnected(1) ? outputs[1] : NULL;
}

bool
Utils::GetIOBuffers(int index, double *in[2], double *out[2],
                    double **inBuf, double **outBuf)
{
    *inBuf = NULL;
    *outBuf = NULL;
    
    if (out[index] != NULL)
    // We want to ouput
    {
        *outBuf = out[index];
        
        *inBuf = in[index];
        if (*inBuf == NULL)
        {
            // We have only one input
            // So take it, this is the first one
            *inBuf = in[0];
        }
        
        if (*inBuf != NULL)
            // We have both buffers
            return true;
    }
    
    // We have either no buffer, or only one out of two
    return false;
}

bool
Utils::PlugIOAllZero(double *inputs[2], double *outputs[2], int nFrames)
{
    bool allZero0 = false;
    bool channelDefined0 = ((inputs[0] != NULL) && (outputs[0] != NULL));
    if (channelDefined0)
    {
        allZero0 = (Utils::IsAllZero(inputs[0], nFrames) &&
                    Utils::IsAllZero(outputs[0], nFrames));
    }
    
    bool allZero1 = false;
    bool channelDefined1 = ((inputs[1] != NULL) && (outputs[1] != NULL));
    if (channelDefined1)
    {
        allZero1 = (Utils::IsAllZero(inputs[1], nFrames) &&
                    Utils::IsAllZero(outputs[1], nFrames));
    }
    
    if (!channelDefined1 && allZero0)
        return true;
    
    if (channelDefined1 && allZero0 && allZero1)
        return true;
    
    return false;
}

double
Utils::FftBinToFreq(int binNum, int numBins, double sampleRate)
{
    if (binNum > numBins/2)
        // Second half => not relevant
        return -1.0;
    
    // Problem here ?
    return binNum*sampleRate/(numBins /*2.0*/); // Modif for Zarlino
}

int
Utils::FreqToFftBin(double freq, int numBins, double sampleRate, double *t)
{
    double fftBin = (freq*numBins/*/2.0*/)/sampleRate; // Modif for Zarlino
    
    // Round with 1e-10 precision
    // This is necessary otherwise we will take the wrong
    // bin when there are rounding errors like "80.999999999"
    fftBin = Utils::Round(fftBin, 10);
    
    if (t != NULL)
    {
        double freq0 = FftBinToFreq(fftBin, numBins, sampleRate);
        double freq1 = FftBinToFreq(fftBin + 1, numBins, sampleRate);
        
        *t = (freq - freq0)/(freq1 - freq0);
    }
    
    return fftBin;
}

void
Utils::MinMaxFftBinFreq(double *minFreq, double *maxFreq, int numBins, double sampleRate)
{
    *minFreq = sampleRate/(numBins/2.0);
    *maxFreq = ((double)(numBins/2.0 - 1.0)*sampleRate)/numBins;
}

void
Utils::ComplexToMagnPhase(WDL_FFT_COMPLEX comp, double *outMagn, double *outPhase)
{
    *outMagn = COMP_MAGN(comp);
    
    *outPhase = atan2(comp.im, comp.re);
}

void
Utils::MagnPhaseToComplex(WDL_FFT_COMPLEX *outComp, double magn, double phase)
{
    WDL_FFT_COMPLEX comp;
    comp.re = magn*cos(phase);
    comp.im = magn*sin(phase);
    
    *outComp = comp;
}

void
Utils::ComplexToMagn(WDL_TypedBuf<double> *result, const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf)
{
    result->Resize(complexBuf.GetSize());
    
    for (int i = 0; i < complexBuf.GetSize(); i++)
    {
        double magn = COMP_MAGN(complexBuf.Get()[i]);
        result->Get()[i] = magn;
    }
}

void
Utils::ComplexToMagnPhase(WDL_TypedBuf<double> *resultMagn,
                          WDL_TypedBuf<double> *resultPhase,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf)
{
    resultMagn->Resize(complexBuf.GetSize());
    resultPhase->Resize(complexBuf.GetSize());
    
    for (int i = 0; i < complexBuf.GetSize(); i++)
    {
        double magn = COMP_MAGN(complexBuf.Get()[i]);
        resultMagn->Get()[i] = magn;
        
#if 1
        double phase = atan2(complexBuf.Get()[i].im, complexBuf.Get()[i].re);
#endif
        
#if 0 // Make some leaks with diracs
        double phase = DomainAtan2(complexBuf.Get()[i].im, complexBuf.Get()[i].re);
#endif
        resultPhase->Get()[i] = phase;
    }
}

void
Utils::MagnPhaseToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                          const WDL_TypedBuf<double> &magns,
                          const WDL_TypedBuf<double> &phases)
{
    complexBuf->Resize(0);
    
    if (magns.GetSize() != phases.GetSize())
        // Error
        return;
    
    complexBuf->Resize(magns.GetSize());
    for (int i = 0; i < magns.GetSize(); i++)
    {
        double magn = magns.Get()[i];
        double phase = phases.Get()[i];
        
        WDL_FFT_COMPLEX res;
        res.re = magn*cos(phase);
        res.im = magn*sin(phase);
        
        complexBuf->Get()[i] = res;
    }
}

double
Utils::Round(double val, int precision)
{
    val = val*pow(10.0, precision);
    val = round(val);
    val *= pow(10, -precision);
    
    return val;
}

void
Utils::Round(double *buf, int nFrames, int precision)
{
    for (int i = 0; i < nFrames; i++)
    {
        double val = buf[i];
        double res = Utils::Round(val, precision);
        
        buf[i] = res;
    }
}

double
Utils::DomainAtan2(double x, double y)
{
    double signx;
    if (x > 0.) signx = 1.;
    else signx = -1.;
    
    if (x == 0.) return 0.;
    if (y == 0.) return signx * M_PI / 2.;
    
    return atan2(x, y);
}

void
Utils::ConsumeLeft(WDL_TypedBuf<double> *ioBuffer, int numToConsume)
{
    int newSize = ioBuffer->GetSize() - numToConsume;
    if (newSize <= 0)
    {
        ioBuffer->Resize(0);
        
        return;
    }
    
    // Resize down, skipping left
    WDL_TypedBuf<double> tmpChunk;
    tmpChunk.Add(&ioBuffer->Get()[numToConsume], newSize);
    *ioBuffer = tmpChunk;
}

void
Utils::TakeHalf(WDL_TypedBuf<double> *buf)
{
    int halfSize = buf->GetSize() / 2;
    
    buf->Resize(halfSize);
}

void
Utils::TakeHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf)
{
    int halfSize = buf->GetSize() / 2;
    
    buf->Resize(halfSize);
}

void
Utils::ResizeFillZeros(WDL_TypedBuf<double> *buf, int newSize)
{
    int prevSize = buf->GetSize();
    buf->Resize(newSize);
    
    if (newSize <= prevSize)
        return;
    
    for (int i = prevSize; i < newSize; i++)
    {
        if (i >= buf->GetSize())
            break;
        
        buf->Get()[i] = 0.0;
    }
}

void
Utils::ResizeFillZeros(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf, int newSize)
{
    int prevSize = buf->GetSize();
    buf->Resize(newSize);
    
    if (newSize <= prevSize)
        return;
    
    for (int i = prevSize; i < newSize; i++)
    {
        buf->Get()[i].re = 0.0;
        buf->Get()[i].im = 0.0;
    }
}


void
Utils::GrowFillZeros(WDL_TypedBuf<double> *buf, int numGrow)
{
    int newSize = buf->GetSize() + numGrow;
    
    ResizeFillZeros(buf, newSize);
}

void
Utils::AddValues(WDL_TypedBuf<double> *buf, double value)
{
    for (int i = 0; i < buf->GetSize(); i++)
    {
        buf->Get()[i] += value;
    }
}

void
Utils::MultValues(WDL_TypedBuf<double> *buf, double value)
{
    for (int i = 0; i < buf->GetSize(); i++)
    {
        buf->Get()[i] *= value;
    }
}

void
Utils::MultValues(WDL_TypedBuf<double> *buf, const WDL_TypedBuf<double> &values)
{
    for (int i = 0; i < buf->GetSize(); i++)
    {
        double val = values.Get()[i];
        buf->Get()[i] *= val;
    }
}

void
Utils::PadZerosLeft(WDL_TypedBuf<double> *buf, int padSize)
{
    WDL_TypedBuf<double> newBuf;
    newBuf.Resize(buf->GetSize() + padSize);
    
    memset(newBuf.Get(), 0, padSize*sizeof(double));
    
    memcpy(&newBuf.Get()[padSize], buf->Get(), buf->GetSize()*sizeof(double));
    
    *buf = newBuf;
}

void
Utils::Interp(WDL_TypedBuf<double> *result,
              const WDL_TypedBuf<double> *buf0,
              const WDL_TypedBuf<double> *buf1,
              double t)
{
    result->Resize(buf0->GetSize());
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        double val0 = buf0->Get()[i];
        double val1 = buf1->Get()[i];
        
        double res = (1.0 - t)*val0 + t*val1;
        
        result->Get()[i] = res;
    }
}

void
Utils::Interp2D(WDL_TypedBuf<double> *result,
                const WDL_TypedBuf<double> bufs[2][2], double u, double v)
{
    WDL_TypedBuf<double> bufv0;
    Interp(&bufv0, &bufs[0][0], &bufs[1][0], u);
    
    WDL_TypedBuf<double> bufv1;
    Interp(&bufv1, &bufs[0][1], &bufs[1][1], u);
    
    Interp(result, &bufv0, &bufv1, v);
}

void
Utils::ComputeAvg(WDL_TypedBuf<double> *result, const vector<WDL_TypedBuf<double> > &bufs)
{
    if (bufs.empty())
        return;
    
    result->Resize(bufs[0].GetSize());

    for (int i = 0; i < result->GetSize(); i++)
    {
        double val = 0.0;
        for (int j = 0; j < bufs.size(); j++)
        {
            val += bufs[j].Get()[i];
        }
        
        val /= bufs.size();
        
        result->Get()[i] = val;
    }
}

void
Utils::Mix(double *output, double *buf0, double *buf1, int nFrames, double mix)
{
    for (int i = 0; i < nFrames; i++)
    {
        double val = (1.0 - mix)*buf0[i] + mix*buf1[i];
        
        output[i] = val;
    }
}

void
Utils::Fade(const WDL_TypedBuf<double> &buf0,
            const WDL_TypedBuf<double> &buf1,
            double *resultBuf, double fadeStart, double fadeEnd)
{
    int bufSize = buf0.GetSize() - 1;
    
    for (int i = 0; i < buf0.GetSize(); i++)
    {
        double prevVal = buf0.Get()[i];
        double newVal = buf1.Get()[i];
        
        // Fades only on the part of the frame
        double t = 0.0;
        if ((i >= bufSize*fadeStart) &&
            (i < bufSize*fadeEnd))
        {
            t = (i - bufSize*fadeStart)/(bufSize*(fadeEnd - fadeStart));
        }
        
        if (i >= bufSize*fadeEnd)
            t = 1.0;
        
        double result = (1.0 - t)*prevVal + t*newVal;
        
        resultBuf[i] = result;
    }
}

void
Utils::Fade(WDL_TypedBuf<double> *buf, double fadeStart, double fadeEnd, bool fadeIn)
{
    Fade(buf->Get(), buf->GetSize(), fadeStart, fadeEnd, fadeIn);
}

void
Utils::Fade(double *buf, int origBufSize, double fadeStart, double fadeEnd, bool fadeIn)
{
    int bufSize = origBufSize - 1;
    
    for (int i = 0; i < origBufSize; i++)
    {
        double val = buf[i];
        
        // Fades only on the part of the frame
        double t = 0.0;
        if ((i >= bufSize*fadeStart) &&
            (i < bufSize*fadeEnd))
        {
            t = (i - bufSize*fadeStart)/(bufSize*(fadeEnd - fadeStart));
        }
        
        if (i >= bufSize*fadeEnd)
            t = 1.0;
        
        double result;
        
        if (fadeIn)
            result = t*val;
        else
            result = (1.0 - t)*val;
        
        buf[i] = result;
    }
}

void
Utils::Fade(const WDL_TypedBuf<double> &buf0,
            const WDL_TypedBuf<double> &buf1,
            double *resultBuf,
            double fadeStart, double fadeEnd,
            double startT, double endT)
{
    int bufSize = buf0.GetSize() - 1;
    
    for (int i = 0; i < buf0.GetSize(); i++)
    {
        double prevVal = buf0.Get()[i];
        double newVal = buf1.Get()[i];
        
        // Fades only on the part of the frame
        double u = 0.0;
        if ((i >= bufSize*fadeStart) &&
            (i < bufSize*fadeEnd))
        {
            u = (i - bufSize*fadeStart)/(bufSize*(fadeEnd - fadeStart));
        }
        
        if (i >= bufSize*fadeEnd)
            u = 1.0;
        
        double t = startT + u*(endT - startT);
        
        double result = (1.0 - t)*prevVal + t*newVal;
        
        resultBuf[i] = result;
    }
}

double
Utils::AmpToDB(double sampleVal, double eps, double minDB)
{
    double result = minDB;
    double absSample = fabs(sampleVal);
    if (absSample > EPS)
    {
        result = ::AmpToDB(absSample);
    }
    
    return result;
}

void
Utils::AmpToDB(WDL_TypedBuf<double> *dBBuf,
               const WDL_TypedBuf<double> &ampBuf,
               double eps, double minDB)
{
    dBBuf->Resize(ampBuf.GetSize());
    
    for (int i = 0; i < ampBuf.GetSize(); i++)
    {
        double amp = ampBuf.Get()[i];
        double dbAmp = AmpToDB(amp, eps, minDB);
        
        dBBuf->Get()[i] = dbAmp;
    }
}

double
Utils::AmpToDBClip(double sampleVal, double eps, double minDB)
{
    double result = minDB;
    double absSample = fabs(sampleVal);
    if (absSample > EPS)
    {
        result = ::AmpToDB(absSample);
    }
 
    // Avoid very low negative dB values, which are not significant
    if (result < minDB)
        result = minDB;
    
    return result;
}
   
    
int
Utils::NextPowerOfTwo(int value)
{
    int result = 1;
    
    while(result < value)
        result *= 2;
    
    return result;
}

void
Utils::AddValues(WDL_TypedBuf<double> *ioBuf, const WDL_TypedBuf<double> &addBuf)
{
    for (int i = 0; i < ioBuf->GetSize(); i++)
    {
        double val = ioBuf->Get()[i];
        double add = addBuf.Get()[i];
        
        val += add;
        
        ioBuf->Get()[i] = val;
    }
}

void
Utils::SubstractValues(WDL_TypedBuf<double> *ioBuf, const WDL_TypedBuf<double> &subBuf)
{
    for (int i = 0; i < ioBuf->GetSize(); i++)
    {
        double val = ioBuf->Get()[i];
        double sub = subBuf.Get()[i];
        
        val -= sub;
        
        ioBuf->Get()[i] = val;
    }
}

void
Utils::ComputeDiff(WDL_TypedBuf<double> *resultDiff,
                   const WDL_TypedBuf<double> &buf0,
                   const WDL_TypedBuf<double> &buf1)
{
    resultDiff->Resize(buf0.GetSize());
                   
    for (int i = 0; i < resultDiff->GetSize(); i++)
    {
        double val0 = buf0.Get()[i];
        double val1 = buf1.Get()[i];
        
        double diff = val1 - val0;
        
        resultDiff->Get()[i] = diff;
    }
}

void
Utils::MakeSymmetry(WDL_TypedBuf<double> *symBuf, const WDL_TypedBuf<double> &buf)
{
    symBuf->Resize(buf.GetSize()*2);
    
    for (int i = 0; i < buf.GetSize(); i++)
    {
        symBuf->Get()[i] = buf.Get()[i];
        symBuf->Get()[buf.GetSize()*2 - i - 1] = buf.Get()[i];
    }
}

void
Utils::DecimateSamples(WDL_TypedBuf<double> *result,
                       const WDL_TypedBuf<double> &buf,
                       double decFactor)
{
    if (buf.GetSize() == 0)
        return;
    
    if (decFactor >= 1.0)
    {
        *result = buf;
        
        return;
    }
    
    Utils::ResizeFillZeros(result, buf.GetSize()*decFactor);
    
    int resultIdx = 0;
    
    // Keep the maxima when decimating
    double count = 0.0;
    double maxSample = 0.0;
    for (int i = 0; i < buf.GetSize(); i++)
    {
        double samp = buf.Get()[i];
        if (fabs(samp) > fabs(maxSample))
            maxSample = samp;
        
        // Fix for spectrograms
        //count += 1.0/decFactor;
        count += decFactor;
        
        if (count >= 1.0)
        {
            result->Get()[resultIdx++] = maxSample;
            
            maxSample = 0.0;
            
            count -= 1.0;
        }
        
        if (resultIdx >=  result->GetSize())
            break;
    }
}

void
Utils::DecimateSamples(WDL_TypedBuf<double> *ioSamples,
                       double decFactor)
{
    WDL_TypedBuf<double> origSamples = *ioSamples;
    DecimateSamples(ioSamples, origSamples, decFactor);
}

void
Utils::FillSecondFftHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    if (ioBuffer->GetSize() < 2)
        return;
    
    // It is important that the "middle value" (ie index 1023) is duplicated
    // to index 1024. So we have twice the center value
    for (int i = 1; i < ioBuffer->GetSize()/2; i++)
    {
        int id0 = i + ioBuffer->GetSize()/2;
        
#if 1 // ORIG
        // Orig, bug...
        // doesn't fill exactly the symetry (the last value differs)
        // but WDL generates ffts like that
        int id1 = ioBuffer->GetSize()/2 - i;
#endif
        
#if 0 // Bug fix (but strange WDL behaviour)
        // Really symetric version
        // with correct last value
        // But if we apply to just generate WDL fft, the behaviour becomes different
        int id1 = ioBuffer->GetSize()/2 - i - 1;
#endif
        
        ioBuffer->Get()[id0].re = ioBuffer->Get()[id1].re;
        
        // Complex conjugate
        ioBuffer->Get()[id0].im = -ioBuffer->Get()[id1].im;
    }
}

void
Utils::CopyBuf(WDL_TypedBuf<double> *toBuf, const WDL_TypedBuf<double> &fromBuf)
{
    for (int i = 0; i < toBuf->GetSize(); i++)
    {
        if (i >= fromBuf.GetSize())
            break;
    
        double val = fromBuf.Get()[i];
        toBuf->Get()[i] = val;
    }
}

int
Utils::FindValueIndex(double val, const WDL_TypedBuf<double> &values, double *outT)
{
    for (int i = 0; i < values.GetSize(); i++)
    {
        double v = values.Get()[i];
        
        if (v > val)
        {
            int idx0 = i - 1;
            if (idx0 < 0)
                idx0 = 0;
            
            if (outT != NULL)
            {
                int idx1 = i;
            
                double val0 = values.Get()[idx0];
                double val1 = values.Get()[idx1];
                
                *outT = (val - val0)/(val1 - val0);
            }
            
            return idx0;
        }
    }
    
    return -1;
}

void
Utils::ShiftSamples(const WDL_TypedBuf<double> *ioSamples, int shiftSize)
{
    if (shiftSize < 0)
        shiftSize += ioSamples->GetSize();
    
    WDL_TypedBuf<double> copySamples = *ioSamples;
    
    for (int i = 0; i < ioSamples->GetSize(); i++)
    {
        int srcIndex = i;
        int dstIndex = (srcIndex + shiftSize) % ioSamples->GetSize();
        
        ioSamples->Get()[dstIndex] = copySamples.Get()[srcIndex];
    }
}

void
Utils::ComputeEnvelope(const WDL_TypedBuf<double> &samples,
                       WDL_TypedBuf<double> *envelope,
                       bool extendBoundsValues)
{
    WDL_TypedBuf<double> maxValues;
    maxValues.Resize(samples.GetSize());
    Utils::FillAllZero(&maxValues);
    
    // First step: put the maxima in the array
    double prevSamples[3] = { 0.0, 0.0, 0.0 };
    bool zeroWasCrossed = false;
    double prevValue = 0.0;
    for (int i = 0; i < samples.GetSize(); i++)
    {
        double sample = samples.Get()[i];
        
        // Wait for crossing the zero line a first time
        if (i == 0)
        {
            prevValue = sample;
            
            continue;
        }
        
        if (!zeroWasCrossed)
        {
            if (prevValue*sample < 0.0)
            {
                zeroWasCrossed = true;
            }
            
            prevValue = sample;
            
            // Before first zero cross, we don't take the maximum
            continue;
        }
        
        sample = fabs(sample);
        
        prevSamples[0] = prevSamples[1];
        prevSamples[1] = prevSamples[2];
        prevSamples[2] = sample;
        
        if ((prevSamples[1] >= prevSamples[0]) &&
           (prevSamples[1] >= prevSamples[2]))
           // Local maximum
        {
            int idx = i - 1;
            if (idx < 0)
                idx = 0;
            maxValues.Get()[idx] = prevSamples[1];
        }
    }
    
    // Suppress the last maximum until zero is crossed
    // (avoids finding maxima from edges of truncated periods)
    double prevValue2 = 0.0;
    for (int i = samples.GetSize() - 1; i > 0; i--)
    {
        double sample = samples.Get()[i];
        if (prevValue2*sample < 0.0)
            // Zero is crossed !
        {
            break;
        }
        
        prevValue2 = sample;
        
        // Suppress potential false maxima
        maxValues.Get()[i] = 0.0;
    }
    
    if (extendBoundsValues)
        // Extend the last maximum to the end
    {
        // Find the last max
        int lastMaxIndex = samples.GetSize() - 1;
        double lastMax = 0.0;
        for (int i = samples.GetSize() - 1; i > 0; i--)
        {
            double val = maxValues.Get()[i];
            if (val > 0.0)
            {
                lastMax = val;
                lastMaxIndex = i;
            
                break;
            }
        }
    
        // Fill the last values with last max
        for (int i = samples.GetSize() - 1; i > lastMaxIndex; i--)
        {
            maxValues.Get()[i] = lastMax;
        }
    }
    
    // Second step: fill the holes by linear interpolation
    //envelope->Resize(samples.GetSize());
    //Utils::FillAllZero(envelope);
    *envelope = maxValues;
    
    double startVal = 0.0;
    
    // First, find start val
    for (int i = 0; i < maxValues.GetSize(); i++)
    {
        double val = maxValues.Get()[i];
        if (val > 0.0)
        {
            startVal = val;
        }
    }
    
    int loopIdx = 0;
    int startIndex = 0;
    //double lastValidVal = 0.0;
    
    // Then start the main loop
    while(loopIdx < envelope->GetSize())
    {
        double val = maxValues.Get()[loopIdx];
        
        if (val > 0.0)
            // Defined
        {
            startVal = val;
            startIndex = loopIdx;
            
            loopIdx++;
        }
        else
            // Undefined
        {
            // Start at 0
            if (!extendBoundsValues &&
                (loopIdx == 0))
                startVal = 0.0;
            
            // Find how many missing values we have
            int endIndex = startIndex + 1;
            double endVal = 0.0;
            bool defined = false;
            
            while(endIndex < maxValues.GetSize())
            {
                if (endIndex < maxValues.GetSize())
                    endVal = maxValues.Get()[endIndex];
                
                defined = (endVal > 0.0);
                if (defined)
                    break;
                
                endIndex++;
            }
    
#if 0 // Make problems with envelopes ending with zeros
            if (defined)
            {
                lastValidVal = endVal;
            }
            else
                // Not found at the end
            {
                endVal = lastValidVal;
            }
#endif
            
            // Fill the missing values with lerp
            for (int i = startIndex; i < endIndex; i++)
            {
                double t = ((double)(i - startIndex))/(endIndex - startIndex - 1);
                
                double newVal = (1.0 - t)*startVal + t*endVal;
                envelope->Get()[i] = newVal;
            }
            
            startIndex = endIndex;
            loopIdx = endIndex;
        }
    }
}

// Smooth, then compute envelope
void
Utils::ComputeEnvelopeSmooth(const WDL_TypedBuf<double> &samples,
                             WDL_TypedBuf<double> *envelope,
                             double smoothCoeff,
                             bool extendBoundsValues)
{
    WDL_TypedBuf<double> smoothedSamples;
    smoothedSamples.Resize(samples.GetSize());
                           
    double cmaCoeff = smoothCoeff*samples.GetSize();
    
    WDL_TypedBuf<double> samplesAbs = samples;
    Utils::ComputeAbs(&samplesAbs);
    
    CMA2Smoother::ProcessOne(samplesAbs.Get(), smoothedSamples.Get(),
                             samplesAbs.GetSize(), cmaCoeff);
    
    
    // Restore the sign, for envelope computation
    for (int i = 0; i < samples.GetSize(); i++)
    {
        double sample = samples.Get()[i];
        
        if (sample < 0.0)
            smoothedSamples.Get()[i] *= -1.0;
    }
    
    ComputeEnvelope(smoothedSamples, envelope, extendBoundsValues);
}

// Compute an envelope by only smoothing
void
Utils::ComputeEnvelopeSmooth2(const WDL_TypedBuf<double> &samples,
                              WDL_TypedBuf<double> *envelope,
                              double smoothCoeff)
{
    envelope->Resize(samples.GetSize());
    
    double cmaCoeff = smoothCoeff*samples.GetSize();
    
    WDL_TypedBuf<double> samplesAbs = samples;
    Utils::ComputeAbs(&samplesAbs);
    
    CMA2Smoother::ProcessOne(samplesAbs.Get(), envelope->Get(),
                             samplesAbs.GetSize(), cmaCoeff);
    
    // Normalize
    // Because CMA2Smoother reduce the values
    
    double maxSamples = Utils::ComputeMax(samples.Get(), samples.GetSize());
    double maxEnvelope = Utils::ComputeMax(envelope->Get(), envelope->GetSize());
    
#define EPS 1e-15
    if (maxEnvelope > EPS)
    {
        double coeff = maxSamples/maxEnvelope;
        Utils::MultValues(envelope, coeff);
    }
}

void
Utils::ZeroBoundEnvelope(WDL_TypedBuf<double> *envelope)
{
    if (envelope->GetSize() == 0)
        return;
    
    envelope->Get()[0] = 0.0;
    envelope->Get()[envelope->GetSize() - 1] = 0.0;
}


void
Utils::ScaleNearest(WDL_TypedBuf<double> *values, int factor)
{
    WDL_TypedBuf<double> newValues;
    newValues.Resize(values->GetSize()*factor);
    
    for (int i = 0; i < newValues.GetSize(); i++)
    {
        double val = values->Get()[i/factor];
        newValues.Get()[i] = val;
    }
    
    *values = newValues;
}

int
Utils::FindMaxIndex(const WDL_TypedBuf<double> &values)
{
    int maxIndex = -1;
    double maxValue = -1e15;
    
    for (int i = 0; i < values.GetSize(); i++)
    {
        double value = values.Get()[i];
        
        if (value > maxValue)
        {
            maxValue = value;
            maxIndex = i;
        }
    }
    
    return maxIndex;
}

void
Utils::ComputeAbs(WDL_TypedBuf<double> *values)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        double value = values->Get()[i];
        
        value = fabs(value);
        
        values->Get()[i] = value;
    }
}

void
Utils::LogScaleX(WDL_TypedBuf<double> *values, double factor)
{
    WDL_TypedBuf<double> origValues = *values;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        double t0 = factor*((double)i)/values->GetSize();
        
        double t = (exp(t0) - 1.0)/(exp(factor) - 1.0);
        int dstIdx = (int)(t*values->GetSize());
        
        if (dstIdx < 0)
            // Should not happen
            dstIdx = 0;
        
        if (dstIdx > values->GetSize() - 1)
            // We never know...
            dstIdx = values->GetSize() - 1;
        
        double dstVal = origValues.Get()[dstIdx];
        values->Get()[i] = dstVal;
    }
}

// See: http://werner.yellowcouch.org/Papers/transients12/index.html
void
Utils::FftIdsToSamplesIds(const WDL_TypedBuf<double> &phases,
                          WDL_TypedBuf<int> *samplesIds)
{
    samplesIds->Resize(phases.GetSize());
    Utils::FillAllZero(samplesIds);
    
    int bufSize = phases.GetSize();
    
    double prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        double phase = phases.Get()[i];
        
        double phaseDiff = phase - prev;
        prev = phase;
        
        // Niko: Avoid having a big phase diff due to prev == 0
        if (i == 0)
            continue;
        
        // Niko: Works well with a dirac !
        phaseDiff = phaseDiff - M_PI/2.0;
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        double samplePos = ((double)bufSize)*phaseDiff/(2.0*M_PI);
        
        samplesIds->Get()[i] = (int)samplePos;
    }
}

void
Utils::CorrectEnvelope(WDL_TypedBuf<double> *samples,
                       const WDL_TypedBuf<double> &envelope0,
                       const WDL_TypedBuf<double> &envelope1)
{
#define EPS 1e-15
    
    for (int i = 0; i < samples->GetSize(); i++)
    {
        double sample = samples->Get()[i];
        
        double env0 = envelope0.Get()[i];
        double env1 = envelope1.Get()[i];
        
        double coeff = 0.0; //
        
        if ((env0 > EPS) && (env1 > EPS))
            coeff = env0/env1;
        
        sample *= coeff;
        
        // Just in case
        if (sample > 1.0)
            sample = 1.0;
        if (sample < -1.0)
            sample = -1.0;
        
        samples->Get()[i] = sample;
    }
}

int
Utils::GetEnvelopeShift(const WDL_TypedBuf<double> &envelope0,
                        const WDL_TypedBuf<double> &envelope1,
                        int precision)
{
    int max0 = Utils::FindMaxIndex(envelope0);
    int max1 = Utils::FindMaxIndex(envelope1);
    
    int shift = max1 - max0;
    
    if (precision > 1)
    {
        double newShift = ((double)shift)/precision;
        newShift = round(newShift);
        
        newShift *= precision;
        
        shift = newShift;
    }
    
    return shift;
}


