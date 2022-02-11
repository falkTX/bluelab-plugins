//
//  Utils.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#include <math.h>

#include <vector>
#include <algorithm>
using namespace std;

#ifdef WIN32
#include <Windows.h>
#endif

#if 0
// Mel / Mfcc
extern "C" {
#include <libmfcc.h>
}
#endif

// For AmpToDB
#include "../../WDL/IPlug/Containers.h"

#include "Utils.h"
#include "CMA2Smoother.h"
#include "Debug.h"

// TODO: must check if it is used
/* double
Utils::ampToDB(double amp, double minDB)
{
    if (amp <= 0.0)
        return minDB;
    
    double db = 20. * log10(amp);
    
    return db;
} */

#define FIND_VALUE_INDEX_EXPE 0

double
Utils::ComputeRMSAvg(const double *values, int nFrames)
{
    double avg = 0.0;
    
    for (int i = 0; i < nFrames; i++)
    {
        double value = values[i];
        avg += value*value;
    }
    
    avg = sqrt(avg/nFrames);
    
    //avg = sqrt(avg)/nFrames;
    
    return avg;
}

double
Utils::ComputeRMSAvg(const WDL_TypedBuf<double> &values)
{
    return ComputeRMSAvg(values.Get(), values.GetSize());
}

double
Utils::ComputeRMSAvg2(const double *values, int nFrames)
{
    double avg = 0.0;
    
    for (int i = 0; i < nFrames; i++)
    {
        double value = values[i];
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

void
Utils::ComputeSquare(WDL_TypedBuf<double> *buf)
{
    for (int i = 0; i < buf->GetSize(); i++)
    {
        double val = buf->Get()[i];

        val = val*val;
        
        buf->Get()[i] = val;
    }
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

void
Utils::ComputeMax(WDL_TypedBuf<double> *max, const WDL_TypedBuf<double> &buf)
{
    if (max->GetSize() != buf.GetSize())
        max->Resize(buf.GetSize());
                    
    for (int i = 0; i < max->GetSize(); i++)
    {
        double val = buf.Get()[i];
        double m = max->Get()[i];
        
        if (val > m)
            max->Get()[i] = val;
    }
}

void
Utils::ComputeMax(WDL_TypedBuf<double> *max, const double *buf)
{
    for (int i = 0; i < max->GetSize(); i++)
    {
        double val = buf[i];
        double m = max->Get()[i];
        
        if (val > m)
            max->Get()[i] = val;
    }
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

// Same as NormalizedYTodBInv
double
Utils::NormalizedXTodBInv(double x, double mindB, double maxdB)
{
    double lMin = ::AmpToDB(mindB);
    double lMax = ::AmpToDB(maxdB);
    
    double result = x*(lMax - lMin) + lMin;
    
    if (result > 0.0)
        result = ::DBToAmp(result);
    
    result = (result - mindB)/(maxdB - mindB);
    
    return result;
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
    
    result = ::DBToAmp(result);
    
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

void
Utils::ComputeAvg(WDL_TypedBuf<double> *avg,
                    const WDL_TypedBuf<double> &values0,
                    const WDL_TypedBuf<double> &values1)
{
    avg->Resize(values0.GetSize());
                
    for (int i = 0; i < values0.GetSize(); i++)
    {
        double val0 = values0.Get()[i];
        double val1 = values1.Get()[i];
        
        double res = (val0 + val1)/2.0;
        
        avg->Get()[i] = res;
    }
}

void
Utils::ComplexSum(WDL_TypedBuf<double> *ioMagns,
                  WDL_TypedBuf<double> *ioPhases,
                  const WDL_TypedBuf<double> &magns,
                  WDL_TypedBuf<double> &phases)
{
    for (int i = 0; i < ioMagns->GetSize(); i++)
    {
        double magn0 = ioMagns->Get()[i];
        double phase0 = ioPhases->Get()[i];
        
        WDL_FFT_COMPLEX comp0;
        Utils::MagnPhaseToComplex(&comp0, magn0, phase0);
        
        double magn1 = magns.Get()[i];
        double phase1 = phases.Get()[i];
        
        WDL_FFT_COMPLEX comp1;
        Utils::MagnPhaseToComplex(&comp1, magn1, phase1);
        
        comp0.re += comp1.re;
        comp0.im += comp1.im;
        
        Utils::ComplexToMagnPhase(comp0, &magn0, &phase0);
        
        ioMagns->Get()[i] = magn0;
        ioPhases->Get()[i] = phase0;
    }
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

bool
Utils::IsAllZero(const WDL_TypedBuf<WDL_FFT_COMPLEX> &buffer)
{
#define EPS 1e-16
    
    for (int i = 0; i < buffer.GetSize(); i++)
    {
        double re = fabs(buffer.Get()[i].re);
        if (re > EPS)
            return false;
        
        double im = fabs(buffer.Get()[i].im);
        if (im > EPS)
            return false;
    }
    
    return true;
}


bool
Utils::IsAllSmallerEps(const WDL_TypedBuf<double> &buffer, double eps)
{
    if (buffer.GetSize() == 0)
        return true;
    
    for (int i = 0; i < buffer.GetSize(); i++)
    {
        double val = fabs(buffer.Get()[i]);
        if (val > eps)
            return false;
    }
    
    return true;
}

// OPTIM PROF Infra
#if 0 // ORIGIN VERSION
void
Utils::FillAllZero(WDL_TypedBuf<double> *ioBuf)
{
    for (int i = 0; i < ioBuf->GetSize(); i++)
        ioBuf->Get()[i] = 0.0;
}
#else // OPTIMIZED
void
Utils::FillAllZero(WDL_TypedBuf<double> *ioBuf)
{
    memset(ioBuf->Get(), 0, ioBuf->GetSize()*sizeof(double));
}
#endif

void
Utils::FillAllZero(WDL_TypedBuf<int> *ioBuf)
{
    for (int i = 0; i < ioBuf->GetSize(); i++)
        ioBuf->Get()[i] = 0;
}

void
Utils::FillAllZero(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuf)
{
	for (int i = 0; i < ioBuf->GetSize(); i++)
	{
		ioBuf->Get()[i].re = 0.0;
		ioBuf->Get()[i].im = 0.0;
	}
}

void
Utils::FillAllZero(double *ioBuf, int size)
{
    memset(ioBuf, 0, size*sizeof(double));
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

void
Utils::StereoToMono(WDL_TypedBuf<double> *monoResult,
                    const vector< WDL_TypedBuf<double> > &in0)
{
    if (in0.empty())
        return;
    
    if (in0.size() == 1)
        *monoResult = in0[0];
    
    if (in0.size() == 2)
    {
        StereoToMono(monoResult, in0[0], in0[1]);
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
Utils::AntiClipping(WDL_TypedBuf<double> *values, double maxValue)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        
        if (val > maxValue)
            val = maxValue;
        
        values->Get()[i] = val;
    }
}

void
Utils::SamplesAntiClipping(WDL_TypedBuf<double> *samples, double maxValue)
{
    for (int i = 0; i < samples->GetSize(); i++)
    {
        double val = samples->Get()[i];
        
        if (val > maxValue)
            val = maxValue;
        
        if (val < -maxValue)
            val = -maxValue;
        
        samples->Get()[i] = val;
    }
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

void
Utils::GetPlugIOBuffers(IPlug *plug,
                        double **inputs, double **outputs, int nFrames,
                        vector<WDL_TypedBuf<double> > *inp,
                        vector<WDL_TypedBuf<double> > *scIn,
                        vector<WDL_TypedBuf<double> > *outp)
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
    int numInputs = numInChannels - numInScChannels;
    
    if ((numInputs > 0) && isInConnected[0])
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (inputs[0] != NULL)
        {
            WDL_TypedBuf<double> input;
            input.Resize(nFrames);
            input.Set(inputs[0], nFrames);
            inp->push_back(input);
        }
    }
    
    if ((numInputs > 1) && isInConnected[1])
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (inputs[1] != NULL)
        {
            WDL_TypedBuf<double> input;
            input.Resize(nFrames);
            input.Set(inputs[1], nFrames);
            inp->push_back(input);
        }
    }

    // When in application mode, must deactivate sidechains
    // BUG: otherwise it crashes if we try to get sidechains
#ifndef SA_API
    // scin
    if ((numInScChannels > 0) && isInConnected[numInChannels - numInScChannels])
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (inputs[numInChannels - numInScChannels] != NULL)
        {
            WDL_TypedBuf<double> sc;
            sc.Resize(nFrames);
            sc.Set(inputs[numInChannels - numInScChannels], nFrames);
            scIn->push_back(sc);
        }
    }
    
    if ((numInScChannels > 1) && isInConnected[numInChannels - numInScChannels + 1])
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (inputs[numInChannels - numInScChannels + 1] != NULL)
        {
            WDL_TypedBuf<double> sc;
            sc.Resize(nFrames);
            sc.Set(inputs[numInChannels - numInScChannels + 1], nFrames);
            scIn->push_back(sc);
        }
    }
#endif
    
    // out
    if (plug->IsOutChannelConnected(0))
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (outputs[0] != NULL)
        {
            WDL_TypedBuf<double> out;
            out.Resize(nFrames);
            out.Set(outputs[0], nFrames);
            outp->push_back(out);
        }
    }
    
    if (plug->IsOutChannelConnected(1))
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (outputs[1] != NULL)
        {
            WDL_TypedBuf<double> out;
            out.Resize(nFrames);
            out.Set(outputs[1], nFrames);
            outp->push_back(out);
        }
    }
    
    // Set inputs and outputs to NULL if necessary
    // (will avoid later crashes)
    if (inp->size() == 1)
        inputs[1] = NULL;
    
    if (outp->size() == 1)
        outputs[1] = NULL;
    
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
Utils::GetIOBuffers(int index,
                    vector<WDL_TypedBuf<double> > &in,
                    vector<WDL_TypedBuf<double> > &out,
                    double **inBuf, double **outBuf)
{
    *inBuf = NULL;
    *outBuf = NULL;
    
    if (out.size() > index)
        // We want to ouput
    {
        *outBuf = out[index].Get();
        
        *inBuf = NULL;
        if (in.size() > index)
            *inBuf = in[index].Get();
        else
            *inBuf = in[0].Get();
        
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

bool
Utils::PlugIOAllZero(const vector<WDL_TypedBuf<double> > &inputs,
                     const vector<WDL_TypedBuf<double> > &outputs)
{
    bool allZero0 = false;
    bool channelDefined0 = ((inputs.size() > 0) && (outputs.size() > 0));
    if (channelDefined0)
    {
        allZero0 = (Utils::IsAllZero(inputs[0].Get(), inputs[0].GetSize()) &&
                    Utils::IsAllZero(outputs[0].Get(), outputs[0].GetSize()));
    }
    
    bool allZero1 = false;
    bool channelDefined1 = ((inputs.size() > 1) && (outputs.size() > 1));
    if (channelDefined1)
    {
        allZero1 = (Utils::IsAllZero(inputs[1].Get(), inputs[1].GetSize()) &&
                    Utils::IsAllZero(outputs[1].Get(), outputs[1].GetSize()));
    }
    
    if (!channelDefined1 && allZero0)
        return true;
    
    if (channelDefined1 && allZero0 && allZero1)
        return true;
    
    return false;
}

void
Utils::PlugCopyOutputs(const vector<WDL_TypedBuf<double> > &outp,
                       double **outputs, int nFrames)
{
    for (int i = 0; i < outp.size(); i++)
    {
        if (outputs[i] == NULL)
            continue;
     
        const WDL_TypedBuf<double> &out = outp[i];
        
        if (out.GetSize() == nFrames)
            memcpy(outputs[i], out.Get(), nFrames*sizeof(double));
    }
}

int
Utils::PlugComputeBufferSize(int bufferSize, double sampleRate)
{
    double ratio = sampleRate/44100.0;
    ratio = round(ratio);
    
    // FIX: Logic Auval checks for 11025 sample rate
    // So ratio would be 0.
    if (ratio < 1.0)
        ratio = 1.0;
    
    int result = bufferSize*ratio;
    
    return result;
}

// Fails sometimes...
int
Utils::PlugComputeLatency(IPlug *plug,
                          int nativeBufferSize, int nativeLatency,
                          double sampleRate)
{
#define EPS 1e-8
    
#define NATIVE_SAMPLE_RATE 44100.0
    
    // How many blocks for filling BUFFER_SIZE ?
    int blockSize = plug->GetBlockSize();
    double coeff = sampleRate/NATIVE_SAMPLE_RATE;
    
    // FIX: for 48KHz and multiples
    coeff = round(coeff);
    
    double numBuffers = coeff*((double)nativeBufferSize)/blockSize;
    if (numBuffers > (int)numBuffers)
        numBuffers = (int)numBuffers + 1;
    
    // GOOD !
    // Compute remaining, in order to compensate for
    // remaining compensation in FftProcessObj15
    double remaining = numBuffers*blockSize/coeff - nativeBufferSize;
    
    double newLatency = numBuffers*blockSize - (int)remaining;
    
    return newLatency;
}

// Fails sometimes...
void
Utils::PlugUpdateLatency(IPlug *plug,
                         int nativeBufferSize, int nativeLatency,
                         double sampleRate)
{
    if (fabs(sampleRate - NATIVE_SAMPLE_RATE) < EPS)
        // We are in the native state, no need to tweek latency
    {
        plug->SetLatency(nativeLatency);
        
        return;
    }

    // Fails sometimes...
    int newLatency = Utils::PlugComputeLatency(plug,
                                               nativeBufferSize, nativeLatency,
                                               sampleRate);

    // Set latency dynamically
    // (not sure it works for all hosts)
    plug->SetLatency(newLatency);
}

double
Utils::GetBufferSizeCoeff(IPlug *plug, int nativeBufferSize)
{
    double sampleRate = plug->GetSampleRate();
    int bufferSize = Utils::PlugComputeBufferSize(nativeBufferSize, sampleRate);
    double bufferSizeCoeff = ((double)bufferSize)/nativeBufferSize;
    
    return bufferSizeCoeff;
}

bool
Utils::ChannelAllZero(const vector<WDL_TypedBuf<double> > &channel)
{
    for (int i = 0; i < channel.size(); i++)
    {
        bool allZero = Utils::IsAllZero(channel[i]);
        if (!allZero)
            return false;
    }
    
    return true;
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

// Fixed version
// In the case we want to fill a BUFFER_SIZE/2 array, as it should be
// (for stereo phase correction)
double
Utils::FftBinToFreq2(int binNum, int numBins, double sampleRate)
{
    if (binNum > numBins)
        // Second half => not relevant
        return -1.0;
    
    // Problem here ?
    return binNum*sampleRate/(numBins*2.0);
}


// This version may be false
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
Utils::FftFreqs(WDL_TypedBuf<double> *freqs, int numBins, double sampleRate)
{
    freqs->Resize(numBins);
    for (int i = 0; i < numBins; i++)
    {
        double freq = FftBinToFreq2(i, numBins, sampleRate);
        
        freqs->Get()[i] = freq;
    }
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
Utils::ComplexToPhase(WDL_TypedBuf<double> *result,
                      const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf)
{
    result->Resize(complexBuf.GetSize());
    
    for (int i = 0; i < complexBuf.GetSize(); i++)
    {
        double phase = COMP_PHASE(complexBuf.Get()[i]);
        result->Get()[i] = phase;
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
    //complexBuf->Resize(0);
    
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

void
Utils::NormalizeFftValues(WDL_TypedBuf<double> *magns)
{
    double sum = 0.0f;
    
    // Not test "/2"
    for (int i = 1; i < magns->GetSize()/*/2*/; i++)
    {
        double magn = magns->Get()[i];
        
        sum += magn;
    }
    
    sum /= magns->GetSize()/*/2*/ - 1;
    
    magns->Get()[0] = sum;
    magns->Get()[magns->GetSize() - 1] = sum;
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
Utils::AppendValues(WDL_TypedBuf<double> *ioBuffer, const WDL_TypedBuf<double> &values)
{
    if (ioBuffer->GetSize() == 0)
    {
        *ioBuffer = values;
        return;
    }
    
    ioBuffer->Add(values.Get(), values.GetSize());
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
Utils::ConsumeRight(WDL_TypedBuf<double> *ioBuffer, int numToConsume)
{
    int size = ioBuffer->GetSize();
    
    int newSize = size - numToConsume;
    if (newSize < 0)
        newSize = 0;
    
    ioBuffer->Resize(newSize);
}

void
Utils::ConsumeLeft(vector<WDL_TypedBuf<double> > *ioBuffer)
{
    if (ioBuffer->empty())
        return;
    
    vector<WDL_TypedBuf<double> > copy = *ioBuffer;
    
    ioBuffer->resize(0);
    for (int i = 1; i < copy.size(); i++)
    {
        WDL_TypedBuf<double> &buf = copy[i];
        ioBuffer->push_back(buf);
    }
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
Utils::TakeHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *res, const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf)
{
    int halfSize = buf.GetSize() / 2;
    
    res->Resize(0);
    res->Add(buf.Get(), halfSize);
}

// OPTIM PROF Infra
#if 0 // ORIGIN
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
#else // Optimized
void
Utils::ResizeFillZeros(WDL_TypedBuf<double> *buf, int newSize)
{
    int prevSize = buf->GetSize();
    buf->Resize(newSize);
    
    if (newSize <= prevSize)
        return;
    
    memset(&buf->Get()[prevSize], 0, (newSize - prevSize)*sizeof(double));
}
#endif

void
Utils::ResizeFillValue(WDL_TypedBuf<double> *buf, int newSize, double value)
{
    buf->Resize(newSize);
    FillAllValue(buf, value);
}

void
Utils::ResizeFillValue2(WDL_TypedBuf<double> *buf, int newSize, double value)
{
    int prevSize = buf->GetSize();
    buf->Resize(newSize);
    
    for (int i = prevSize; i < newSize; i++)
        buf->Get()[i] = value;
}

void
Utils::ResizeFillRandom(WDL_TypedBuf<double> *buf, int newSize, double coeff)
{
    buf->Resize(newSize);
    
    for (int i = 0; i < buf->GetSize(); i++)
    {
        double r = ((double)rand())/RAND_MAX;
        
        double newVal = r*coeff;
        
        buf->Get()[i] = newVal;
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
Utils::InsertZeros(WDL_TypedBuf<double> *buf, int index, int numZeros)
{
    WDL_TypedBuf<double> result;
    result.Resize(buf->GetSize() + numZeros);
    Utils::FillAllZero(&result);
    
    //
    if (buf->GetSize() < index)
    {
        *buf = result;
        
        return;
    }
        
    // Before zeros
    memcpy(result.Get(), buf->Get(), index*sizeof(double));
    
    // After zeros
    memcpy(&result.Get()[index + numZeros], &buf->Get()[index],
           (buf->GetSize() - index)*sizeof(double));
    
    *buf = result;
}

void
Utils::InsertValues(WDL_TypedBuf<double> *buf, int index,
                    int numValues, double value)
{
    for (int i = 0; i < numValues; i++)
        buf->Insert(value, index);
}

// BUGGY
void
Utils::RemoveValuesCyclic(WDL_TypedBuf<double> *buf, int index, int numValues)
{
    // Remove too many => empty the result
    if (numValues >= buf->GetSize())
    {
        buf->Resize(0);
        
        return;
    }
    
    // Prepare the result with the new size
    WDL_TypedBuf<double> result;
    result.Resize(buf->GetSize() - numValues);
    Utils::FillAllZero(&result);
    
    // Copy cyclicly
    for (int i = 0; i < result.GetSize(); i++)
    {
        int idx = index + i;
        idx = idx % buf->GetSize();
        
        double val = buf->Get()[idx];
        result.Get()[i] = val;
    }
    
    *buf = result;
}

// Remove before and until index
void
Utils::RemoveValuesCyclic2(WDL_TypedBuf<double> *buf, int index, int numValues)
{
    // Remove too many => empty the result
    if (numValues >= buf->GetSize())
    {
        buf->Resize(0);
        
        return;
    }
    
    // Manage negative index
    if (index < 0)
        index += buf->GetSize();
    
    // Prepare the result with the new size
    WDL_TypedBuf<double> result;
    result.Resize(buf->GetSize() - numValues);
    Utils::FillAllZero(&result); // Just in case
    
    // Copy cyclicly
    int bufPos = index + 1;
    int resultPos = index + 1 - numValues;
    if (resultPos < 0)
        resultPos += result.GetSize();
    
    for (int i = 0; i < result.GetSize(); i++)
    {
        bufPos = bufPos % buf->GetSize();
        resultPos = resultPos % result.GetSize();
        
        double val = buf->Get()[bufPos];
        result.Get()[resultPos] = val;
        
        bufPos++;
        resultPos++;
    }
    
    *buf = result;
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
Utils::AddValues(WDL_TypedBuf<double> *result,
                 const WDL_TypedBuf<double> &buf0,
                 const WDL_TypedBuf<double> &buf1)
{
    result->Resize(buf0.GetSize());
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        double a = buf0.Get()[i];
        double b = buf1.Get()[i];
        
        double sum = a + b;
        
        result->Get()[i] = sum;
    }
}

void
Utils::AddValues(WDL_TypedBuf<double> *ioBuf, const double *addBuf)
{
    for (int i = 0; i < ioBuf->GetSize(); i++)
    {
        ioBuf->Get()[i] += addBuf[i];
    }
}

void
Utils::MultValues(WDL_TypedBuf<double> *buf, double value)
{
    for (int i = 0; i < buf->GetSize(); i++)
    {
        double v = buf->Get()[i];
        v *= value;
        buf->Get()[i] = v;
    }
}

// OPTIM PROF Infra
#if 0 // ORIGIN
void
Utils::MultValuesRamp(WDL_TypedBuf<double> *buf, double value0, double value1)
{
    for (int i = 0; i < buf->GetSize(); i++)
    {
        double t = ((double)i)/(buf->GetSize() - 1);
        double value = (1.0 - t)*value0 + t*value1;
        
        double v = buf->Get()[i];
        v *= value;
        buf->Get()[i] = v;
    }
}
#else // OPTIMIZED
void
Utils::MultValuesRamp(WDL_TypedBuf<double> *buf, double value0, double value1)
{
    //double step = 0.0;
    //if (buf->GetSize() >= 2)
    //    step = 1.0/(buf->GetSize() - 1);
    //double t = 0.0;
    double value = value0;
    double step = 0.0;
    if (buf->GetSize() >= 2)
        step = (value1 - value0)/(buf->GetSize() - 1);
    for (int i = 0; i < buf->GetSize(); i++)
    {
        //t += step;
        //double value = (1.0 - t)*value0 + t*value1;
        
        double v = buf->Get()[i];
        v *= value;
        buf->Get()[i] = v;
        
        value += step;
    }
}
#endif

void
Utils::MultValues(double *buf, int size, double value)
{
    for (int i = 0; i < size; i++)
    {
        double v = buf[i];
        v *= value;
        buf[i] = v;
    }
}

void
Utils::ApplyPow(WDL_TypedBuf<double> *values, double exp)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];

        val = pow(val, exp);
        
        values->Get()[i] = val;
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
    if (padSize == 0)
        return;
    
    WDL_TypedBuf<double> newBuf;
    newBuf.Resize(buf->GetSize() + padSize);
    
    memset(newBuf.Get(), 0, padSize*sizeof(double));
    
    memcpy(&newBuf.Get()[padSize], buf->Get(), buf->GetSize()*sizeof(double));
    
    *buf = newBuf;
}

void
Utils::PadZerosRight(WDL_TypedBuf<double> *buf, int padSize)
{
    if (padSize == 0)
        return;
    
    long prevSize = buf->GetSize();
    buf->Resize(prevSize + padSize);
    
    memset(&buf->Get()[prevSize], 0, padSize*sizeof(double));
}

double
Utils::Interp(double val0, double val1, double t)
{
    double res = (1.0 - t)*val0 + t*val1;
    
    return res;
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

void
Utils::Fade(WDL_TypedBuf<double> *ioBuf0,
            const WDL_TypedBuf<double> &buf1,
            double fadeStart, double fadeEnd,
            bool fadeIn,
            double startPos, double endPos)
{
    // NEW: check for bounds !
    // Added for Ghost-X and FftProcessObj15 (latency fix)
    if (startPos < 0.0)
        startPos = 0.0;
    if (startPos > 1.0)
        startPos = 1.0;
    
    if (endPos < 0.0)
        endPos = 0.0;
    if (endPos > 1.0)
        endPos = 1.0;
    
    long startIdx = startPos*ioBuf0->GetSize();
    long endIdx = endPos*ioBuf0->GetSize();
 
    int bufSize = ioBuf0->GetSize() - 1;
    for (int i = startIdx; i < endIdx; i++)
    {
        double prevVal = ioBuf0->Get()[i];
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
        
        double result;
        if (fadeIn)
            result = (1.0 - t)*prevVal + t*newVal;
        else
            result = t*prevVal + (1.0 - t)*newVal;
        
        ioBuf0->Get()[i] = result;
    }
}

void
Utils::DoubleFade(WDL_TypedBuf<double> *ioBuf0,
                  const WDL_TypedBuf<double> &buf1,
                  double fadeStart, double fadeEnd)
{
    Fade(ioBuf0, buf1, fadeStart, fadeEnd, true, 0.0, 0.5);
    Fade(ioBuf0, buf1, 1.0 - fadeEnd, 1.0 - fadeStart, false, 0.5, 1.0);
}

void
Utils::DoubleFade(double *ioBuf0Data,
                  const double *buf1Data,
                  int bufSize,
                  double fadeStart, double fadeEnd)
{
    WDL_TypedBuf<double> buf0;
    buf0.Resize(bufSize);
    memcpy(buf0.Get(), ioBuf0Data, bufSize*sizeof(double));
    
    WDL_TypedBuf<double> buf1;
    buf1.Resize(bufSize);
    memcpy(buf1.Get(), buf1Data, bufSize*sizeof(double));
    
    Fade(&buf0, buf1, fadeStart, fadeEnd, true, 0.0, 0.5);
    Fade(&buf0, buf1, 1.0 - fadeEnd, 1.0 - fadeStart, false, 0.5, 1.0);
    
    memcpy(ioBuf0Data, buf0.Get(), bufSize*sizeof(double));
}

double
Utils::AmpToDB(double sampleVal, double eps, double minDB)
{
    double result = minDB;
    double absSample = fabs(sampleVal);
    if (absSample > eps/*EPS*/)
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

void
Utils::AmpToDB(WDL_TypedBuf<double> *dBBuf,
               const WDL_TypedBuf<double> &ampBuf)
{
    *dBBuf = ampBuf;
    
    for (int i = 0; i < dBBuf->GetSize(); i++)
    {
        double amp = dBBuf->Get()[i];
        double dbAmp = ::AmpToDB(amp);
        
        dBBuf->Get()[i] = dbAmp;
    }
}

// OPTIM PROF Infra
// (compute in place)
void
Utils::AmpToDB(WDL_TypedBuf<double> *ioBuf,
               double eps, double minDB)
{
    for (int i = 0; i < ioBuf->GetSize(); i++)
    {
        double amp = ioBuf->Get()[i];
        double dbAmp = AmpToDB(amp, eps, minDB);
        
        ioBuf->Get()[i] = dbAmp;
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

double
Utils::AmpToDBNorm(double sampleVal, double eps, double minDB)
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
    
    result += -minDB;
    result /= -minDB;
    
    return result;
}

void
Utils::AmpToDBNorm(WDL_TypedBuf<double> *dBBufNorm,
                   const WDL_TypedBuf<double> &ampBuf,
                   double eps, double minDB)
{
    dBBufNorm->Resize(ampBuf.GetSize());
    
    for (int i = 0; i < ampBuf.GetSize(); i++)
    {
        double amp = ampBuf.Get()[i];
        double dbAmpNorm = AmpToDBNorm(amp, eps, minDB);
        
        dBBufNorm->Get()[i] = dbAmpNorm;
    }
}

void
Utils::DBToAmp(WDL_TypedBuf<double> *ioBuf)
{
    for (int i = 0; i < ioBuf->GetSize(); i++)
    {
        double db = ioBuf->Get()[i];
        
        double amp = ::DBToAmp(db);
        
        ioBuf->Get()[i] = amp;
    }
}

double
Utils::DBToAmpNorm(double sampleVal, double eps, double minDB)
{
    double db = sampleVal;
    if (db < minDB)
        db = minDB;
    
    db *= -minDB;
    db += minDB;
    
    double result = ::DBToAmp(db);
    
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
        if (i > addBuf.GetSize() - 1)
            break;
        
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
Utils::Permute(WDL_TypedBuf<double> *values,
               const WDL_TypedBuf<int> &indices,
               bool forward)
{
    if (values->GetSize() != indices.GetSize())
        return;
    
    WDL_TypedBuf<double> origValues = *values;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        int idx = indices.Get()[i];
        if (idx >= values->GetSize())
            // Error
            return;
        
        if (forward)
            values->Get()[idx] = origValues.Get()[i];
        else
            values->Get()[i] = origValues.Get()[idx];
    }
}

void
Utils::Permute(vector< vector< int > > *values,
               const WDL_TypedBuf<int> &indices,
               bool forward)
{
    if (values->size() != indices.GetSize())
    return;
    
    vector<vector<int> > origValues = *values;
    
    for (int i = 0; i < values->size(); i++)
    {
        int idx = indices.Get()[i];
        if (idx >= values->size())
            // Error
            return;
        
        if (forward)
            (*values)[idx] = origValues[i];
        else
            (*values)[i] = origValues[idx];
    }
}

void
Utils::ClipMin(WDL_TypedBuf<double> *values, double min)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        if (val < min)
            val = min;
        
        values->Get()[i] = val;
    }
}

void
Utils::ClipMax(WDL_TypedBuf<double> *values, double max)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        if (val > max)
            val = max;
        
        values->Get()[i] = val;
    }
}

void
Utils::ClipMinMax(double *val, double min, double max)
{
    if (*val < min)
        *val = min;
    if (*val > max)
        *val = max;
}


void
Utils::ClipMinMax(WDL_TypedBuf<double> *values, double min, double max)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        if (val < min)
            val = min;
        if (val > max)
            val = max;
        
        values->Get()[i] = val;
    }
}

void
Utils::Diff(WDL_TypedBuf<double> *diff,
            const WDL_TypedBuf<double> &prevValues,
            const WDL_TypedBuf<double> &values)
{
    diff->Resize(values.GetSize());
                 
    for (int i = 0; i < values.GetSize(); i++)
    {
        double val = values.Get()[i];
        double prevVal = prevValues.Get()[i];
        
        double d = val - prevVal;
        
        diff->Get()[i] = d;
    }
}

void
Utils::ApplyDiff(WDL_TypedBuf<double> *values,
                 const WDL_TypedBuf<double> &diff)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        
        double d = diff.Get()[i];
        
        val += d;
        
        values->Get()[i] = val;
    }
}

bool
Utils::IsEqual(const WDL_TypedBuf<double> &values0,
               const WDL_TypedBuf<double> &values1)
{
#define EPS 1e-15
    
    if (values0.GetSize() != values1.GetSize())
        return false;
    
    for (int i = 0; i < values0.GetSize(); i++)
    {
        double val0 = values0.Get()[i];
        double val1 = values1.Get()[i];
        
        if (fabs(val0 - val1) > EPS)
            return false;
    }
    
    return true;
}

void
Utils::ReplaceValue(WDL_TypedBuf<double> *values, double srcValue, double dstValue)
{
#define EPS 1e-15
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        
        if (fabs(val - srcValue) < EPS)
            val = dstValue;
        
        values->Get()[i] = val;
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
Utils::Reverse(WDL_TypedBuf<int> *values)
{
    WDL_TypedBuf<int> origValues = *values;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        int val = origValues.Get()[i];
        
        int idx = values->GetSize() - i - 1;
        
        values->Get()[idx] = val;
    }
}

void
Utils::Reverse(WDL_TypedBuf<double> *values)
{
    WDL_TypedBuf<double> origValues = *values;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = origValues.Get()[i];
        
        int idx = values->GetSize() - i - 1;
        
        values->Get()[idx] = val;
    }
}

void
Utils::ApplySqrt(WDL_TypedBuf<double> *values)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        
        if (val > 0.0)
            val = sqrt(val);
        
        values->Get()[i] = val;
    }
}

void
Utils::DecimateValues(WDL_TypedBuf<double> *result,
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
Utils::DecimateValues(WDL_TypedBuf<double> *ioValues,
                      double decFactor)
{
    WDL_TypedBuf<double> origSamples = *ioValues;
    DecimateValues(ioValues, origSamples, decFactor);
}

void
Utils::DecimateValuesDb(WDL_TypedBuf<double> *result,
                        const WDL_TypedBuf<double> &buf,
                        double decFactor, double minValueDb)
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
    double maxSample = minValueDb;
    for (int i = 0; i < buf.GetSize(); i++)
    {
        double samp = buf.Get()[i];
        //if (fabs(samp) > fabs(maxSample))
        if (samp > maxSample)
            maxSample = samp;
        
        // Fix for spectrograms
        //count += 1.0/decFactor;
        count += decFactor;
        
        if (count >= 1.0)
        {
            result->Get()[resultIdx++] = maxSample;
            
            maxSample = minValueDb;
            
            count -= 1.0;
        }
        
        if (resultIdx >=  result->GetSize())
            break;
    }
}

// FIX: to avoid long series of positive values not looking like waveforms
// FIX2: improved initial fix: really avoid loosing interesting min and max
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
    
    // When set to the first value instead of 0,
    // it avoid a first line from 0 to the first value at the beginning
    //double minSample = 0.0;
    //double maxSample = 0.0;
    //double prevSample = 0.0;
    
    double minSample = buf.Get()[0];
    double maxSample = buf.Get()[0];
    double prevSample = buf.Get()[0];
    
    // When set to true, avoid flat beginning when the first values are negative
    //bool zeroCrossed = false;
    bool zeroCrossed = true;
    
    double prevSampleUsed = 0.0;
    
    for (int i = 0; i < buf.GetSize(); i++)
    {
        double samp = buf.Get()[i];
        //if (fabs(samp) > fabs(maxSample))
        //    maxSample = samp;
        if (samp > maxSample)
            maxSample = samp;
        
        if (samp < minSample)
            minSample = samp;
        
        // Optimize by removing the multiplication
        // (sometimes we run through millions of samples,
        // so it could be worth it to optimize this)
        
        //if (samp*prevSample < 0.0)
        if ((samp > 0.0 && prevSample < 0.0) ||
            (samp < 0.0 && prevSample > 0.0))
            zeroCrossed = true;
        
        prevSample = samp;
        
        // Fix for spectrograms
        //count += 1.0/decFactor;
        count += decFactor;
        
        if (count >= 1.0)
        {
            // Take care, if we crossed zero,
            // we take alternately positive and negative samples
            // (otherwise, we could have very long series of positive samples
            // for example. And this won't look like a waveform anymore...
            double sampleToUse;
            if (!zeroCrossed)
            {
                // Prefer reseting only min or max, not both, to avoid loosing
                // interesting values
                if (prevSampleUsed >= 0.0)
                {
                    sampleToUse = maxSample;
                    
                    // FIX: avoid segments stuck at 0 during several samples
                    maxSample = samp; //0.0;
                }
                else
                {
                    sampleToUse = minSample;
                    minSample = samp; //0.0;
                }
            } else
            {
                if (prevSampleUsed >= 0.0)
                {
                    sampleToUse = minSample;
                    minSample = samp; //0.0;
                }
                else
                {
                    sampleToUse = maxSample;
                    maxSample = samp; //0.0;
                }
            }
            
            result->Get()[resultIdx++] = sampleToUse;
            
            // Prefer reseting only min or max, not both, to avoid loosing
            // interesting values
            
            //minSample = 0.0;
            //maxSample = 0.0;
            
            count -= 1.0;
            
            prevSampleUsed = sampleToUse;
            zeroCrossed = false;
        }
        
        if (resultIdx >=  result->GetSize())
            break;
    }
}

// DOESN'T WORK...
// Incremental version
// Try to fix long sections of 0 values
void
Utils::DecimateSamples2(WDL_TypedBuf<double> *result,
                        const WDL_TypedBuf<double> &buf,
                        double decFactor)
{
    double factor = 0.5;
    
    // Decimate progressively
    WDL_TypedBuf<double> tmp = buf;
    while(tmp.GetSize() > buf.GetSize()*decFactor*2.0)
    {
        DecimateSamples(&tmp, factor);
    }
    
    // Last step
    DecimateSamples(&tmp, decFactor);
    
    *result = tmp;
}

void
Utils::DecimateSamples(WDL_TypedBuf<double> *ioSamples,
                       double decFactor)
{
    WDL_TypedBuf<double> origSamples = *ioSamples;
    DecimateSamples(ioSamples, origSamples, decFactor);
}

// Simply take some samples and throw out the others
// ("sparkling" when zooming in Ghost)
void
Utils::DecimateSamplesFast(WDL_TypedBuf<double> *result,
                           const WDL_TypedBuf<double> &buf,
                           double decFactor)
{
    Utils::ResizeFillZeros(result, buf.GetSize()*decFactor);
    
    int step = (decFactor > 0) ? 1.0/decFactor : buf.GetSize();
    int resId = 0;
    for (int i = 0; i < buf.GetSize(); i+= step)
    {
        double val = buf.Get()[i];
        
        if (resId >= result->GetSize())
            break;
        
        result->Get()[resId++] = val;
    }
}

int
Utils::SecondOrderEqSolve(double a, double b, double c, double res[2])
{
    // See: http://math.lyceedebaudre.net/premiere-sti2d/second-degre/resoudre-une-equation-du-second-degre
    //
    double delta = b*b - 4.0*a*c;
    
    if (delta > 0.0)
    {
        res[0] = (-b - sqrt(delta))/(2.0*a);
        res[1] = (-b + sqrt(delta))/(2.0*a);
        
        return 2;
    }
    
#define EPS 1e-15
    if (fabs(delta) < EPS)
    {
        res[0] = -b/(2.0*a);
        
        return 1;
    }
    
    return 0;
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
        
#if 1 // FIX: fill the value at the middle
      // (which was not filled, and could be undefined if not filled outside the function)
      //
      // NOTE: added for Rebalance, to fix a bug:
      // - waveform values like 1e+250
      //
      // NOTE: quick fix, better solution could be found, by
      // comparing with WDL fft
      //
      // NOTE: could fix many plugins, like for example StereoViz
      //
      ioBuffer->Get()[ioBuffer->GetSize()/2].re = 0.0;
      ioBuffer->Get()[ioBuffer->GetSize()/2].im = 0.0;
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
Utils::FillSecondFftHalf(WDL_TypedBuf<double> *ioMagns)
{
    if (ioMagns->GetSize() < 2)
        return;

    for (int i = 1; i < ioMagns->GetSize()/2; i++)
    {
        int id0 = i + ioMagns->GetSize()/2;
        
        // WARNING: doesn't fill exactly the symetry (the last value differs)
        // but WDL generates ffts like that
        int id1 = ioMagns->GetSize()/2 - i;
        
        // FIX: fill the value at the middle
        ioMagns->Get()[ioMagns->GetSize()/2] = 0.0;
        
        ioMagns->Get()[id0] = ioMagns->Get()[id1];
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

void
Utils::CopyBuf(WDL_TypedBuf<double> *toBuf, const double *fromData, int fromSize)
{
    toBuf->Resize(fromSize);
    memcpy(toBuf->Get(), fromData, fromSize*sizeof(double));
}

void
Utils::CopyBuf(double *toData, const WDL_TypedBuf<double> &fromBuf)
{
    memcpy(toData, fromBuf.Get(), fromBuf.GetSize()*sizeof(double));
}

void
Utils::Replace(WDL_TypedBuf<double> *dst, int startIdx, const WDL_TypedBuf<double> &src)
{
    for (int i = 0; i < src.GetSize(); i++)
    {
        if (startIdx + i >= dst->GetSize())
            break;
        
        double val = src.Get()[i];
        
        dst->Get()[startIdx + i] = val;
    }
}

#if !FIND_VALUE_INDEX_EXPE
// Current version
int
Utils::FindValueIndex(double val, const WDL_TypedBuf<double> &values, double *outT)
{
    if (outT != NULL)
        *outT = 0.0;
    
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
                
                if (fabs(val1 - val0) > EPS)
                    *outT = (val - val0)/(val1 - val0);
            }
            
            return idx0;
        }
    }
    
    return -1;
}
#else
// New version
// NOT very well tested yet (but should be better - or same -)
int
Utils::FindValueIndex(double val, const WDL_TypedBuf<double> &values, double *outT)
{
    *outT = 0.0;
    
    for (int i = 0; i < values.GetSize(); i++)
    {
        double v = values.Get()[i];
        
        if (v == val)
            // Same value
        {
            *outT = 0.0;
            
            return i;
        }
        
        if (v > val)
        {
            int idx0 = i - 1;
            if (idx0 < 0)
                // We have not found the value in the array
                // because the first value of the array is already grater than
                // the value we are testing
            {
                idx0 = 0;
                *outT = 0.0;
                
                return idx0;
            }
            
            // We are between values
            if (outT != NULL)
            {
                int idx1 = idx0 + 1;
                
                double val0 = values.Get()[idx0];
                double val1 = values.Get()[idx1];
                
                if (fabs(val1 - val0) > EPS)
                    *outT = (val - val0)/(val1 - val0);
            }
            
            return idx0;
        }
    }
    
    return -1;
}
#endif

// Find the matching index from srcVal and the src list,
// and return the corresponding value from the dstValues
//
// The src values do not need to be sorted !
// (as opposite to FindValueIndex())
//
// Used to sort without loosing the consistency
class Value
{
public:
    static bool IsGreater(const Value& v0, const Value& v1)
        { return v0.mSrcValue < v1.mSrcValue; }
    
    double mSrcValue;
    double mDstValue;
};

double
Utils::FindMatchingValue(double srcVal,
                         const WDL_TypedBuf<double> &srcValues,
                         const WDL_TypedBuf<double> &dstValues)
{
    // Fill the vector
    vector<Value> values;
    values.resize(srcValues.GetSize());
    for (int i = 0; i < srcValues.GetSize(); i++)
    {
        double srcValue = srcValues.Get()[i];
        double dstValue = dstValues.Get()[i];
        
        Value val;
        val.mSrcValue = srcValue;
        val.mDstValue = dstValue;
        
        values[i] = val;
    }
    
    // Sort the vector
    sort(values.begin(), values.end(), Value::IsGreater);
    
    // Re-create the WDL list
    WDL_TypedBuf<double> sortedSrcValues;
    sortedSrcValues.Resize(values.size());
    for (int i = 0; i < values.size(); i++)
    {
        double val = values[i].mSrcValue;
        sortedSrcValues.Get()[i] = val;
    }
    
    // Find the index and the t parameter
    double t;
    int idx = FindValueIndex(srcVal, sortedSrcValues, &t);
    if (idx < 0)
        idx = 0;
    
    // Find the new matching value
    double dstVal0 = dstValues.Get()[idx];
    if (idx + 1 >= dstValues.GetSize())
        // LAst value
        return dstVal0;
    
    double dstVal1 = dstValues.Get()[idx + 1];
    
    double res = Interp(dstVal0, dstVal1, t);
    
    return res;
}

void
Utils::PrepareMatchingValueSorted(WDL_TypedBuf<double> *srcValues,
                                  WDL_TypedBuf<double> *dstValues)
{
    // Fill the vector
    vector<Value> values;
    values.resize(srcValues->GetSize());
    for (int i = 0; i < srcValues->GetSize(); i++)
    {
        double srcValue = srcValues->Get()[i];

        double dstValue = 0.0;
        if (dstValues != NULL)
            dstValue = dstValues->Get()[i];
        
        Value val;
        val.mSrcValue = srcValue;
        val.mDstValue = dstValue;
        
        values[i] = val;
    }
    
    // Sort the vector
    sort(values.begin(), values.end(), Value::IsGreater);
    
    // Re-create the WDL lists
    
    // Src values
    for (int i = 0; i < values.size(); i++)
    {
        double val = values[i].mSrcValue;
        srcValues->Get()[i] = val;
    }
    
    // Dst values
    if (dstValues != NULL)
    {
        WDL_TypedBuf<double> sortedDstValues;
        sortedDstValues.Resize(values.size());
        for (int i = 0; i < values.size(); i++)
        {
            double val = values[i].mDstValue;
            dstValues->Get()[i] = val;
        }
    }
}

double
Utils::FindMatchingValueSorted(double srcVal,
                               const WDL_TypedBuf<double> &sortedSrcValues,
                               const WDL_TypedBuf<double> &sortedDstValues)
{
    // Find the index and the t parameter
    double t;
    int idx = FindValueIndex(srcVal, sortedSrcValues, &t);
    if (idx < 0)
        idx = 0;
        
    // Find the new matching value
    double dstVal0 = sortedDstValues.Get()[idx];
    if (idx + 1 >= sortedDstValues.GetSize())
        // Last value
        return dstVal0;
    
    double dstVal1 = sortedDstValues.Get()[idx + 1];
    
    double res = Interp(dstVal0, dstVal1, t);
    
    return res;
}

double
Utils::FactorToDivFactor(double val, double coeff)
{
    double res = pow(coeff, val);
    
    return res;
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
    
    // Should be defined to 1 !
#if 0 // TODO: check it and validate it (code factoring :) ) !
    *envelope = maxValues;
    
    FillMissingValues(envelope, extendBoundsValues);
    
#else
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
#endif
}

// GOOD: makes good linerp !
// And fixed NaN
void
Utils::FillMissingValues(WDL_TypedBuf<double> *values,
                         bool extendBounds, double undefinedValue)
{
    if (extendBounds)
    // Extend the last value to the end
    {
        // Find the last max
        int lastIndex = values->GetSize() - 1;
        double lastValue = 0.0;
        for (int i = values->GetSize() - 1; i > 0; i--)
        {
            double val = values->Get()[i];
            if (val > undefinedValue)
            {
                lastValue = val;
                lastIndex = i;
                
                break;
            }
        }
        
        // Fill the last values with last max
        for (int i = values->GetSize() - 1; i > lastIndex; i--)
        {
            values->Get()[i] = lastValue;
        }
    }
    
    // Fill the holes by linear interpolation
    double startVal = 0.0;
    
    // First, find start val
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        if (val > undefinedValue)
        {
            startVal = val;
        }
    }
    
    int loopIdx = 0;
    int startIndex = 0;
    //double lastValidVal = 0.0;
    
    // Then start the main loop
    while(loopIdx < values->GetSize())
    {
        double val = values->Get()[loopIdx];
        
        if (val > undefinedValue)
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
            if (!extendBounds &&
                (loopIdx == 0))
                startVal = 0.0;
            
            // Find how many missing values we have
            int endIndex = startIndex + 1;
            double endVal = 0.0;
            bool defined = false;
            
            while(endIndex < values->GetSize())
            {
                if (endIndex < values->GetSize())
                    endVal = values->Get()[endIndex];
                
                defined = (endVal > undefinedValue);
                if (defined)
                    break;
                
                endIndex++;
            }
            
#if 0 // Make problems with series ending with zeros
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
                // FIX "+1": avoid NaN, and better linerp !
                double t = ((double)(i - startIndex))/(endIndex - startIndex /*+ 1*/);
                
                double newVal = (1.0 - t)*startVal + t*endVal;
                    
                values->Get()[i] = newVal;
            }
            
            startIndex = endIndex;
            loopIdx = endIndex;
        }
    }
}

void
Utils::FillMissingValues2(WDL_TypedBuf<double> *values,
                          bool extendBounds, double undefinedValue)
{
    if (extendBounds)
        // Extend the last value to the end
    {
        // Find the last max
        int lastIndex = values->GetSize() - 1;
        double lastValue = undefinedValue;
        for (int i = values->GetSize() - 1; i > 0; i--)
        {
            double val = values->Get()[i];
            if (val > undefinedValue)
            {
                lastValue = val;
                lastIndex = i;
                
                break;
            }
        }
        
        // Fill the last values with last max
        for (int i = values->GetSize() - 1; i > lastIndex; i--)
        {
            values->Get()[i] = lastValue;
        }
    }
    
    // Fill the holes by linear interpolation
    double startVal = undefinedValue;
    
    // First, find start val
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        if (val > undefinedValue)
        {
            startVal = val;
        }
    }
    
    int loopIdx = 0;
    int startIndex = 0;
    //double lastValidVal = 0.0;
    
    // Then start the main loop
    while(loopIdx < values->GetSize())
    {
        double val = values->Get()[loopIdx];
        
        if (val > undefinedValue)
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
            if (!extendBounds &&
                (loopIdx == 0))
                startVal = undefinedValue;
            
            // Find how many missing values we have
            int endIndex = startIndex + 1;
            double endVal = undefinedValue;
            bool defined = false;
            
            while(endIndex < values->GetSize())
            {
                if (endIndex < values->GetSize())
                    endVal = values->Get()[endIndex];
                
                defined = (endVal > undefinedValue);
                if (defined)
                    break;
                
                endIndex++;
            }
            
#if 0 // Make problems with series ending with zeros
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
                // FIX "+1": avoid NaN, and better linerp !
                double t = ((double)(i - startIndex))/(endIndex - startIndex /*+ 1*/);
                
                double newVal = (1.0 - t)*startVal + t*endVal;
                
                values->Get()[i] = newVal;
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

double
Utils::FindMaxValue(const WDL_TypedBuf<double> &values)
{
    double maxValue = -1e15;
    for (int i = 0; i < values.GetSize(); i++)
    {
        double value = values.Get()[i];
        
        if (value > maxValue)
        {
            maxValue = value;
        }
    }
    
    return maxValue;
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

double
Utils::LogScale(double value, double factor)
{
    double result = factor*value;
    
    result = (exp(result) - 1.0)/(exp(factor) - 1.0);
    
    return result;
    
#if 0 // inverse process
    double result = value;
    
    result *= exp(factor) - 1.0;
    result += 1.0;
    result = log(result);
    
    return result;
#endif
}

double
Utils::LogScaleNorm(double value, double maxValue, double factor)
{
    double result = value/maxValue;
    
    result = LogScale(result, factor);
    
    result *= maxValue;
    
    return result;
}

double
Utils::LogScaleNormInv(double value, double maxValue, double factor)
{
    double result = value/maxValue;
    
    //result = LogScale(result, factor);
    result *= exp(factor) - 1.0;
    result += 1.0;
    result = log(result);
    result /= factor;
    
    result *= maxValue;
    
    return result;
}

void
Utils::FreqsToLogNorm(WDL_TypedBuf<double> *resultMagns,
                      const WDL_TypedBuf<double> &magns,
                      double hzPerBin)
{
#define EPS 1e-15
    
    Utils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    double maxFreq = hzPerBin*(magns.GetSize() - 1);
    double maxLog = log10(maxFreq);
    
    for (int i = 0; i < resultMagns->GetSize(); i++)
    {
        double logVal = i*maxLog/resultMagns->GetSize();
        double freq = pow(10.0, logVal);
        
        if (maxFreq < EPS)
            return;
        
        double id0 = (freq/maxFreq) * resultMagns->GetSize();
        double t = id0 - (int)(id0);
        
        if ((int)id0 >= magns.GetSize())
            continue;
        
        int id1 = id0 + 1;
        if (id1 >= magns.GetSize())
            continue;
        
        double magn0 = magns.Get()[(int)id0];
        double magn1 = magns.Get()[id1];
        
        double magn = (1.0 - t)*magn0 + t*magn1;
        
        resultMagns->Get()[i] = magn;
    }
}

void
Utils::LogToFreqsNorm(WDL_TypedBuf<double> *resultMagns,
                      const WDL_TypedBuf<double> &magns,
                      double hzPerBin)
{
    Utils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    double maxFreq = hzPerBin*(magns.GetSize() - 1);
    double maxLog = log10(maxFreq);
    
    for (int i = 0; i < resultMagns->GetSize(); i++)
    {
        double freq = hzPerBin*i;
        double logVal = 0.0;
        // Check for log(0) => -inf
        if (freq > 0)
            logVal = log10(freq);
        
        double id0 = (logVal/maxLog) * resultMagns->GetSize();
        
        if ((int)id0 >= magns.GetSize())
            continue;
        
        // Linear
        double t = id0 - (int)(id0);
        
        int id1 = id0 + 1;
        if (id1 >= magns.GetSize())
            continue;
        
        double magn0 = magns.Get()[(int)id0];
        double magn1 = magns.Get()[id1];
        
        double magn = (1.0 - t)*magn0 + t*magn1;
        
        // Nearest
        //double magn = magns.Get()[(int)id0];
        
        resultMagns->Get()[i] = magn;
    }
}

void
Utils::FreqsToDbNorm(WDL_TypedBuf<double> *resultMagns,
                     const WDL_TypedBuf<double> &magns,
                     double hzPerBin,
                     double minValue, double maxValue)
{
#define EPS 1e-15
    
    Utils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    //double maxFreq = hzPerBin*(magns.GetSize() - 1);
    //double maxDb = Utils::NormalizedXTodB(1.0, minValue, maxValue);
    
    // We work in normalized coordinates
    double maxFreq = 1.0;
    
    for (int i = 0; i < resultMagns->GetSize(); i++)
    {
        double dbVal = ((double)i)/resultMagns->GetSize();
        double freq = Utils::NormalizedXTodBInv(dbVal, minValue, maxValue);
        
        if (maxFreq < EPS)
            return;
        
        double id0 = (freq/maxFreq) * resultMagns->GetSize();
        double t = id0 - (int)(id0);
        
        if ((int)id0 >= magns.GetSize())
            continue;
        
        int id1 = id0 + 1;
        if (id1 >= magns.GetSize())
            continue;
        
        double magn0 = magns.Get()[(int)id0];
        double magn1 = magns.Get()[id1];
        
        double magn = (1.0 - t)*magn0 + t*magn1;
        
        resultMagns->Get()[i] = magn;
    }
}

// TODO: need to check this
double
Utils::LogNormToFreq(int idx, double hzPerBin, int bufferSize)
{
    double maxFreq = hzPerBin*(bufferSize - 1);
    double maxLog = log10(maxFreq);
    
    double freq = hzPerBin*idx;
    double logVal = log10(freq);
    
    double id0 = (logVal/maxLog) * bufferSize;
    
    double result = id0*hzPerBin;
    
    return result;
}

// GOOD !
int
Utils::FreqIdToLogNormId(int idx, double hzPerBin, int bufferSize)
{
    double maxFreq = hzPerBin*(bufferSize/2);
    double maxLog = log10(maxFreq);
    
    double freq = hzPerBin*idx;
    double logVal = log10(freq);
        
    double resultId = (logVal/maxLog)*(bufferSize/2);
    
    resultId = round(resultId);
    if (resultId < 0)
        resultId = 0;
    if (resultId > bufferSize/2 - 1)
        resultId = bufferSize/2;
    
    return resultId;
}

void
Utils::ApplyWindow(WDL_TypedBuf<double> *values,
                   const WDL_TypedBuf<double> &window)
{
    if (values->GetSize() != window.GetSize())
        return;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        double w = window.Get()[i];
        
        val *= w;
        
        values->Get()[i] = val;
    }
}

void
Utils::ApplyWindowRescale(WDL_TypedBuf<double> *values,
                          const WDL_TypedBuf<double> &window)
{
    double coeff = ((double)window.GetSize())/values->GetSize();
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        
        int winIdx = i*coeff;
        if (winIdx >= window.GetSize())
            continue;
        
        double w = window.Get()[winIdx];
        
        val *= w;
        
        values->Get()[i] = val;
    }
}

void
Utils::ApplyWindowFft(WDL_TypedBuf<double> *ioMagns,
                      const WDL_TypedBuf<double> &phases,
                      const WDL_TypedBuf<double> &window)
{
    WDL_TypedBuf<int> samplesIds;
    Utils::FftIdsToSamplesIds(phases, &samplesIds);
    
    WDL_TypedBuf<double> sampleMagns = *ioMagns;
    
    Utils::Permute(&sampleMagns, samplesIds, true);
    
    Utils::ApplyWindow(&sampleMagns, window);
    
    Utils::Permute(&sampleMagns, samplesIds, false);
    
#if 1
    // Must multiply by 2... why ?
    //
    // Maybe due to hanning window normalization of windows
    //
    Utils::MultValues(&sampleMagns, 2.0);
#endif
    
    *ioMagns = sampleMagns;
}

// boundSize is used to not divide by the extremities, which are often zero
void
Utils::UnapplyWindow(WDL_TypedBuf<double> *values, const WDL_TypedBuf<double> &window,
                     int boundSize)
{
#define EPS 1e-6
    
    if (values->GetSize() != window.GetSize())
        return;
    
    double max0 = Utils::ComputeMaxAbs(*values);
    
    // First, apply be the invers window
    int start = boundSize;
    for (int i = boundSize; i < values->GetSize() - boundSize; i++)
    {
        double val = values->Get()[i];
        double w = window.Get()[i];
        
        if (w > EPS)
        {
            val /= w;
            values->Get()[i] = val;
        }
        else
        {
            int newStart = i;
            if (newStart > values->GetSize()/2)
                newStart = values->GetSize() - i - 1;
            
            if (newStart > start)
                start = newStart;
        }
    }
    
    if (start >= values->GetSize()/2)
        // Error
        return;
    
    // Then fill the missing values
    
    // Start
    double startVal = values->Get()[start];
    for (int i = 0; i < start; i++)
    {
        values->Get()[i] = startVal;
    }
    
    // End
    int end = values->GetSize() - start - 1;
    double endVal = values->Get()[end];
    for (int i = values->GetSize() - 1; i > end; i--)
    {
        values->Get()[i] = endVal;
    }
    
    double max1 = Utils::ComputeMaxAbs(*values);
    
    // Normalize
    if (max1 > 0.0)
    {
        double coeff = max0/max1;
        
        Utils::MultValues(values, coeff);
    }
}

void
Utils::UnapplyWindowFft(WDL_TypedBuf<double> *ioMagns,
                        const WDL_TypedBuf<double> &phases,
                        const WDL_TypedBuf<double> &window,
                        int boundSize)
{
    WDL_TypedBuf<int> samplesIds;
    Utils::FftIdsToSamplesIds(phases, &samplesIds);
    
    WDL_TypedBuf<double> sampleMagns = *ioMagns;
    
    Utils::Permute(&sampleMagns, samplesIds, true);
    
    Utils::UnapplyWindow(&sampleMagns, window, boundSize);
    
    Utils::Permute(&sampleMagns, samplesIds, false);
    
#if 1
    // Must multiply by 8... why ?
    //
    // Maybe due to hanning window normalization of windows
    //
    Utils::MultValues(&sampleMagns, 8.0);
#endif
    
    *ioMagns = sampleMagns;
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
        
        // TODO: optimize this !
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        double samplePos = ((double)bufSize)*phaseDiff/(2.0*M_PI);
        
        samplesIds->Get()[i] = (int)samplePos;
    }
    
    // NOT SURE AT ALL !
    // Just like that, seems inverted
    // So we reverse back !
    //Utils::Reverse(samplesIds);
}

// See: http://werner.yellowcouch.org/Papers/transients12/index.html
void
Utils::FftIdsToSamplesIdsFloat(const WDL_TypedBuf<double> &phases,
                               WDL_TypedBuf<double> *samplesIds)
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
        
        // TODO: optimize this !
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        double samplePos = ((double)bufSize)*phaseDiff/(2.0*M_PI);
        
        samplesIds->Get()[i] = samplePos;
    }
    
    // NOT SURE AT ALL !
    // Just like that, seems inverted
    // So we reverse back !
    //Utils::Reverse(samplesIds);
}

void
Utils::FftIdsToSamplesIdsSym(const WDL_TypedBuf<double> &phases,
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
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        double samplePos = ((double)bufSize)*phaseDiff/(2.0*M_PI);
        
        // For sym...
        samplePos *= 2.0;
        
        samplePos = fmod(samplePos, samplesIds->GetSize());
        
        samplesIds->Get()[i] = (int)samplePos;
    }
}

void
Utils::SamplesIdsToFftIds(const WDL_TypedBuf<double> &phases,
                          WDL_TypedBuf<int> *fftIds)
{
    fftIds->Resize(phases.GetSize());
    Utils::FillAllZero(fftIds);
    
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
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        double samplePos = ((double)bufSize)*phaseDiff/(2.0*M_PI);
        
        int samplePosI = (int)samplePos;
        
        if ((samplePosI > 0) && (samplePosI < fftIds->GetSize()))
            fftIds->Get()[samplePosI] = i;
    }
}

void
Utils::FindNextPhase(double *phase, double refPhase)
{
    while(*phase < refPhase)
        *phase += 2.0*M_PI;
}

void
Utils::ComputeTimeDelays(WDL_TypedBuf<double> *timeDelays,
                         const WDL_TypedBuf<double> &phasesL,
                         const WDL_TypedBuf<double> &phasesR,
                         double sampleRate)
{
    if (phasesL.GetSize() != phasesR.GetSize())
        // R can be empty if we are in mono
        return;
    
    timeDelays->Resize(phasesL.GetSize());
    
    WDL_TypedBuf<double> samplesIdsL;
    Utils::FftIdsToSamplesIdsFloat(phasesL, &samplesIdsL);
    
    WDL_TypedBuf<double> samplesIdsR;
    Utils::FftIdsToSamplesIdsFloat(phasesR, &samplesIdsR);
    
    for (int i = 0; i < timeDelays->GetSize(); i++)
    {
        double sampIdL = samplesIdsL.Get()[i];
        double sampIdR = samplesIdsR.Get()[i];
        
        double diff = sampIdR - sampIdL;
        
        double delay = diff/sampleRate;
        
        timeDelays->Get()[i] = delay;
    }
}

#if 0 // Quite costly version !
void
Utils::UnwrapPhases(WDL_TypedBuf<double> *phases)
{
    double prevPhase = phases->Get()[0];
    
    FindNextPhase(&prevPhase, 0.0);
    
    for (int i = 0; i < phases->GetSize(); i++)
    {
        double phase = phases->Get()[i];
        
        FindNextPhase(&phase, prevPhase);
        
        phases->Get()[i] = phase;
        
        prevPhase = phase;
    }
}
#endif

// Optimized version
void
Utils::UnwrapPhases(WDL_TypedBuf<double> *phases)
{
    if (phases->GetSize() == 0)
        // Empty phases
        return;
    
    double prevPhase = phases->Get()[0];
    FindNextPhase(&prevPhase, 0.0);
    
    double sum = 0.0;
    for (int i = 0; i < phases->GetSize(); i++)
    {
        double phase = phases->Get()[i];
        phase += sum;
        
        while(phase < prevPhase)
        {
            phase += 2.0*M_PI;
            
            sum += 2.0*M_PI;
        }
        
        phases->Get()[i] = phase;
        
        prevPhase = phase;
    }
}

double
Utils::MapToPi(double val)
{
    /* Map delta phase into +/- Pi interval */
    val =  fmod(val, 2.0*M_PI);
    if (val <= -M_PI)
        val += 2.0*M_PI;
    if (val > M_PI)
        val -= 2.0*M_PI;
    
    return val;
}

void
Utils::MapToPi(WDL_TypedBuf<double> *values)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        
        val = MapToPi(val);
        
        values->Get()[i] = val;
    }
}

void
Utils::PolarToCartesian(const WDL_TypedBuf<double> &Rs,
                        const WDL_TypedBuf<double> &thetas,
                        WDL_TypedBuf<double> *xValues,
                        WDL_TypedBuf<double> *yValues)
{
    xValues->Resize(thetas.GetSize());
    yValues->Resize(thetas.GetSize());
    
    for (int i = 0; i < thetas.GetSize(); i++)
    {
        double theta = thetas.Get()[i];
        
        double r = Rs.Get()[i];
        
        double x = r*cos(theta);
        double y = r*sin(theta);
        
        xValues->Get()[i] = x;
        yValues->Get()[i] = y;
    }
}

void
Utils::PhasesPolarToCartesian(const WDL_TypedBuf<double> &phasesDiff,
                              const WDL_TypedBuf<double> *magns,
                              WDL_TypedBuf<double> *xValues,
                              WDL_TypedBuf<double> *yValues)
{
    xValues->Resize(phasesDiff.GetSize());
    yValues->Resize(phasesDiff.GetSize());
    
    for (int i = 0; i < phasesDiff.GetSize(); i++)
    {
        double phaseDiff = phasesDiff.Get()[i];
        
        // TODO: check this
        phaseDiff = MapToPi(phaseDiff);
        
        double magn = 1.0;
        if ((magns != NULL) && (magns->GetSize() > 0))
            magn = magns->Get()[i];
        
        double x = magn*cos(phaseDiff);
        double y = magn*sin(phaseDiff);
        
        xValues->Get()[i] = x;
        yValues->Get()[i] = y;
    }
}

// From (angle, distance) to (normalized angle on x, hight distance on y)
void
Utils::CartesianToPolarFlat(WDL_TypedBuf<double> *xVector, WDL_TypedBuf<double> *yVector)
{
    for (int i = 0; i < xVector->GetSize(); i++)
    {
        double x0 = xVector->Get()[i];
        double y0 = yVector->Get()[i];
        
        double angle = atan2(y0, x0);
        
        // Normalize x
        double x = (angle/M_PI - 0.5)*2.0;
        
        // Keep y as it is ?
        //double y = y0;
        
        // or change it ?
        double y = sqrt(x0*x0 + y0*y0);
        
        xVector->Get()[i] = x;
        yVector->Get()[i] = y;
    }
}

void
Utils::PolarToCartesianFlat(WDL_TypedBuf<double> *xVector, WDL_TypedBuf<double> *yVector)
{
    for (int i = 0; i < xVector->GetSize(); i++)
    {
        double theta = xVector->Get()[i];
        double r = yVector->Get()[i];
        
        theta = ((theta + 1.0)*0.5)*M_PI;
        
        double x = r*cos(theta);
        double y = r*sin(theta);
        
        xVector->Get()[i] = x;
        yVector->Get()[i] = y;
    }
}

void
Utils::ApplyInverseWindow(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                          const WDL_TypedBuf<double> &window,
                          const WDL_TypedBuf<double> *originEnvelope)
{
    WDL_TypedBuf<double> magns;
    WDL_TypedBuf<double> phases;
    Utils::ComplexToMagnPhase(&magns, &phases, *fftSamples);
    
    ApplyInverseWindow(&magns, phases, window, originEnvelope);
    
    Utils::MagnPhaseToComplex(fftSamples, magns, phases);
}

void
Utils::ApplyInverseWindow(WDL_TypedBuf<double> *magns,
                          const WDL_TypedBuf<double> &phases,
                          const WDL_TypedBuf<double> &window,
                          const WDL_TypedBuf<double> *originEnvelope)
{
#define WIN_EPS 1e-3
    
    // Suppresses the amplification of noise at the border of the wndow
#define MAGN_EPS 1e-6
    
    WDL_TypedBuf<int> samplesIds;
    Utils::FftIdsToSamplesIds(phases, &samplesIds);
    
    const WDL_TypedBuf<double> origMagns = *magns;
    
    for (int i = 0; i < magns->GetSize(); i++)
        //for (int i = 1; i < magns->GetSize() - 1; i++)
    {
        int sampleIdx = samplesIds.Get()[i];
        
        double magn = origMagns.Get()[i];
        double win1 = window.Get()[sampleIdx];
        
        double coeff = 0.0;
        
        if (win1 > WIN_EPS)
            coeff = 1.0/win1;
        
        //coeff = win1;
        
        // Better with
        if (originEnvelope != NULL)
        {
            double originSample = originEnvelope->Get()[sampleIdx];
            coeff *= fabs(originSample);
        }
        
        if (magn > MAGN_EPS) // TEST
            magn *= coeff;
        
#if 0
        // Just in case
        if (magn > 1.0)
            magn = 1.0;
        if (magn < -1.0)
            magn = -1.0;
#endif
        
        magns->Get()[i] = magn;
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

double
Utils::ApplyParamShape(double normVal, double shape)
{
    return pow(normVal, 1.0/shape);
}

double
Utils::ComputeShapeForCenter0(double minKnobValue, double maxKnobValue)
{
    // Normalized position of the zero
    double normZero = -minKnobValue/(maxKnobValue - minKnobValue);
    
    double shape = log(0.5)/log(normZero);
    shape = 1.0/shape;
    
    return shape;
}

char *
Utils::GetFileExtension(const char *fileName)
{
	char *ext = (char *)strrchr(fileName, '.');
    
    // Here, we have for example ".wav"
    if (ext != NULL)
    {
        if (strlen(ext) > 0)
            // Skip the dot
            ext = &ext[1];
    }
        
    return ext;
}

char *
Utils::GetFileName(const char *path)
{
	char *fileName = (char *)strrchr(path, '/');

	// Here, we have for example "/file.wav"
	if (fileName != NULL)
	{
		if (strlen(fileName) > 0)
			// Skip the dot
			fileName = &fileName[1];
	}
	else
	{
		// There were no "/" in the path,
		// we already had the correct file name
		return (char *)path;
	}

	return fileName;
}

void
Utils::AppendValuesFile(const char *fileName, const WDL_TypedBuf<double> &values, char delim)
{
    // Compute the file size
    FILE *fileSz = fopen(fileName, "a+");
    if (fileSz == NULL)
        return;
    
    fseek(fileSz, 0L, SEEK_END);
    long size = ftell(fileSz);
    
    fseek(fileSz, 0L, SEEK_SET);
    fclose(fileSz);
    
    // Write
    FILE *file = fopen(fileName, "a+");
    
    for (int i = 0; i < values.GetSize(); i++)
    {
        if ((i == 0) && (size == 0))
            fprintf(file, "%g", values.Get()[i]);
        else
            fprintf(file, "%c%g", delim, values.Get()[i]);
    }
    
    //fprintf(file, "\n");
           
    fclose(file);
}

void
Utils::AppendValuesFile(const char *fileName, const WDL_TypedBuf<float> &values, char delim)
{
    // Compute the file size
    FILE *fileSz = fopen(fileName, "a+");
    if (fileSz == NULL)
        return;
    
    fseek(fileSz, 0L, SEEK_END);
    long size = ftell(fileSz);
    
    fseek(fileSz, 0L, SEEK_SET);
    fclose(fileSz);
    
    // Write
    FILE *file = fopen(fileName, "a+");
    
    for (int i = 0; i < values.GetSize(); i++)
    {
        if ((i == 0) && (size == 0))
            fprintf(file, "%g", values.Get()[i]);
        else
            fprintf(file, "%c%g", delim, values.Get()[i]);
    }
    
    fclose(file);
}

void
Utils::AppendValuesFileBin(const char *fileName, const WDL_TypedBuf<double> &values)
{
    // Write
    FILE *file = fopen(fileName, "ab+");
    
    fwrite(values.Get(), sizeof(double), values.GetSize(), file);
    
    fclose(file);
}

void
Utils::AppendValuesFileBin(const char *fileName, const WDL_TypedBuf<float> &values)
{
    // Write
    FILE *file = fopen(fileName, "ab+");
    
    fwrite(values.Get(), sizeof(float), values.GetSize(), file);
    
    fclose(file);
}

void
Utils::AppendValuesFileBinFloat(const char *fileName, const WDL_TypedBuf<double> &values)
{
    WDL_TypedBuf<float> floatBuf;
    floatBuf.Resize(values.GetSize());
    for (int i = 0; i < floatBuf.GetSize(); i++)
    {
        double val = values.Get()[i];
        floatBuf.Get()[i] = val;
    }
    
    // Write
    FILE *file = fopen(fileName, "ab+");
    
    fwrite(floatBuf.Get(), sizeof(float), floatBuf.GetSize(), file);
    
    fclose(file);
}

void
Utils::ResizeLinear(WDL_TypedBuf<double> *ioBuffer,
                    int newSize)
{
    double *data = ioBuffer->Get();
    int size = ioBuffer->GetSize();
    
    WDL_TypedBuf<double> result;
    Utils::ResizeFillZeros(&result, newSize);
    
    double ratio = ((double)(size - 1))/newSize;
    
    double pos = 0.0;
    for (int i = 0; i < newSize; i++)
    {
        // Optim
        //int x = (int)(ratio * i);
        //double diff = (ratio * i) - x;
        
        int x = (int)pos;
        double diff = pos - x;
        pos += ratio;
        
        if (x >= size - 1)
            continue;
        
        double a = data[x];
        double b = data[x + 1];
        
        double val = a*(1.0 - diff) + b*diff;
        
        result.Get()[i] = val;
    }
    
    *ioBuffer = result;
}

void
Utils::ResizeLinear(WDL_TypedBuf<double> *rescaledBuf,
                    const WDL_TypedBuf<double> &buf,
                    int newSize)
{
    double *data = buf.Get();
    int size = buf.GetSize();
    
    Utils::ResizeFillZeros(rescaledBuf, newSize);
    
    double ratio = ((double)(size - 1))/newSize;
    
    double pos = 0.0;
    for (int i = 0; i < newSize; i++)
    {
        // Optim
        //int x = (int)(ratio * i);
        //double diff = (ratio * i) - x;
        
        int x = (int)pos;
        double diff = pos - x;
        pos += ratio;
        
        if (x >= size - 1)
            continue;
        
        double a = data[x];
        double b = data[x + 1];
        
        double val = a*(1.0 - diff) + b*diff;
        
        rescaledBuf->Get()[i] = val;
    }
}

void
Utils::ResizeLinear2(WDL_TypedBuf<double> *ioBuffer,
                    int newSize)
{
    if (ioBuffer->GetSize() == 0)
        return;
    
    if (newSize == 0)
        return;
    
    // Fix last value
    double lastValue = ioBuffer->Get()[ioBuffer->GetSize() - 1];
    
    double *data = ioBuffer->Get();
    int size = ioBuffer->GetSize();
    
    WDL_TypedBuf<double> result;
    Utils::ResizeFillZeros(&result, newSize);
    
    double ratio = ((double)(size - 1))/newSize;
    
    double pos = 0.0;
    for (int i = 0; i < newSize; i++)
    {
        // Optim
        //int x = (int)(ratio * i);
        //double diff = (ratio * i) - x;
        
        int x = (int)pos;
        double diff = pos - x;
        pos += ratio;
        
        if (x >= size - 1)
            continue;
            
        double a = data[x];
        double b = data[x + 1];
        
        double val = a*(1.0 - diff) + b*diff;
        
        result.Get()[i] = val;
    }
    
    *ioBuffer = result;
    
    // Fix last value
    ioBuffer->Get()[ioBuffer->GetSize() - 1] = lastValue;
}

#if 0 // Disable, to avoid including libmfcc in each plugins
// Use https://github.com/jsawruk/libmfcc
//
// NOTE: Not tested !
void
Utils::FreqsToMfcc(WDL_TypedBuf<double> *result,
                   const WDL_TypedBuf<double> freqs,
                   double sampleRate)
{
    int numFreqs = freqs.GetSize();
    double *spectrum = freqs.Get();
    
    int numFilters = 48;
    int numCoeffs = 13;
    
    result->Resize(numCoeffs);
    
    for (int coeff = 0; coeff < numCoeffs; coeff++)
	{
		double res = GetCoefficient(spectrum, (int)sampleRate, numFilters, 128, coeff);
        
        result->Get()[coeff] = res;
	}
}
#endif

// See: https://haythamfayek.com/2016/04/21/speech-processing-for-machine-learning.html
double
Utils::FreqToMel(double freq)
{
    double res = 2595.0*log10(1.0 + freq/700.0);
    
    return res;
}

double
Utils::MelToFreq(double mel)
{
    double res = 700.0*(pow(10.0, mel/2595.0) - 1.0);
    
    return res;
}

// VERY GOOD !

// OPTIM PROF Infra
#if 0 // ORIGIN
void
Utils::FreqsToMelNorm(WDL_TypedBuf<double> *resultMagns,
                      const WDL_TypedBuf<double> &magns,
                      double hzPerBin,
                      double zeroValue)
{
#define EPS 1e-15
    
    //Utils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    // For dB
    resultMagns->Resize(magns.GetSize());
    Utils::FillAllValue(resultMagns, zeroValue);
    
    double maxFreq = hzPerBin*(magns.GetSize() - 1);
    double maxMel = FreqToMel(maxFreq);
    
    // Optim
    double melCoeff = maxMel/resultMagns->GetSize();
    double idCoeff = (1.0/maxFreq)*resultMagns->GetSize();
    
    for (int i = 0; i < resultMagns->GetSize(); i++)
    {
        // Optim
        //double mel = i*maxMel/resultMagns->GetSize();
        double mel = i*melCoeff;
        
        double freq = MelToFreq(mel);
        
        if (maxFreq < EPS)
            return;
        
        // Optim
        //double id0 = (freq/maxFreq) * resultMagns->GetSize();
        double id0 = freq*idCoeff;
        
        double t = id0 - (int)(id0);
        
        if ((int)id0 >= magns.GetSize())
            continue;
        
        int id1 = id0 + 1;
        if (id1 >= magns.GetSize())
            continue;
        
        double magn0 = magns.Get()[(int)id0];
        double magn1 = magns.Get()[id1];
        
        double magn = (1.0 - t)*magn0 + t*magn1;
        
        resultMagns->Get()[i] = magn;
    }
}
#else // OPTIMIZED
void
Utils::FreqsToMelNorm(WDL_TypedBuf<double> *resultMagns,
                      const WDL_TypedBuf<double> &magns,
                      double hzPerBin,
                      double zeroValue)
{
#define EPS 1e-15
    
    //Utils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    // For dB
    resultMagns->Resize(magns.GetSize());
    Utils::FillAllValue(resultMagns, zeroValue);
    
    double maxFreq = hzPerBin*(magns.GetSize() - 1);
    double maxMel = FreqToMel(maxFreq);
    
    // Optim
    double melCoeff = maxMel/resultMagns->GetSize();
    double idCoeff = (1.0/maxFreq)*resultMagns->GetSize();
    
    for (int i = 0; i < resultMagns->GetSize(); i++)
    {
        double mel = i*melCoeff;
        
        double freq = MelToFreq(mel);
        
        if (maxFreq < EPS)
            return;
        
        double id0 = freq*idCoeff;
        
        // Optim
        // (cast from double to int is costly)
        int id0i = (int)id0;
        
        double t = id0 - id0i;
        
        if (id0i >= magns.GetSize())
            continue;
        
        // NOTE: this optim doesn't compute exactly the same thing than the original version
        int id1 = id0i + 1;
        if (id1 >= magns.GetSize())
            continue;
        
        double magn0 = magns.Get()[id0i];
        double magn1 = magns.Get()[id1];
        
        double magn = (1.0 - t)*magn0 + t*magn1;
        
        resultMagns->Get()[i] = magn;
    }
}
#endif

void
Utils::MelToFreqsNorm(WDL_TypedBuf<double> *resultMagns,
                      const WDL_TypedBuf<double> &magns,
                      double hzPerBin, double zeroValue)
{
    //Utils::ResizeFillZeros(resultMagns, magns.GetSize());
    
    // For dB
    resultMagns->Resize(magns.GetSize());
    Utils::FillAllValue(resultMagns, zeroValue);
    
    double maxFreq = hzPerBin*(magns.GetSize() - 1);
    double maxMel = FreqToMel(maxFreq);
    
    for (int i = 0; i < resultMagns->GetSize(); i++)
    {
        double freq = hzPerBin*i;
        double mel = FreqToMel(freq);
        
        double id0 = (mel/maxMel) * resultMagns->GetSize();

        if ((int)id0 >= magns.GetSize())
            continue;
            
        // Linear
        double t = id0 - (int)(id0);
        
        int id1 = id0 + 1;
        if (id1 >= magns.GetSize())
            continue;
        
        double magn0 = magns.Get()[(int)id0];
        double magn1 = magns.Get()[id1];
        
        double magn = (1.0 - t)*magn0 + t*magn1;
        
        // Nearest
        //double magn = magns.Get()[(int)id0];
        
        resultMagns->Get()[i] = magn;
    }
}

// Ok
double
Utils::MelNormIdToFreq(double idx, double hzPerBin, int bufferSize)
{
    //double maxFreq = hzPerBin*(bufferSize - 1);
    double maxFreq = hzPerBin*(bufferSize/2);
    
    double maxMel = FreqToMel(maxFreq);
    
    double mel = (idx/(bufferSize/2))*maxMel;
    
    double result = MelToFreq(mel);
    
    return result;
}

// GOOD !
int
Utils::FreqIdToMelNormId(int idx, double hzPerBin, int bufferSize)
{
    double maxFreq = hzPerBin*(bufferSize/2);
    double maxMel = FreqToMel(maxFreq);
    
    double freq = hzPerBin*idx;
    double mel = FreqToMel(freq);
        
    double resultId = (mel/maxMel)*(bufferSize/2);
    
    resultId = round(resultId);
    if (resultId < 0)
        resultId = 0;
    if (resultId > bufferSize/2 - 1)
        resultId = bufferSize/2;
    
    return resultId;
}

double
Utils::FreqIdToMelNormIdF(double idx, double hzPerBin, int bufferSize)
{
    double maxFreq = hzPerBin*(bufferSize/2);
    double maxMel = FreqToMel(maxFreq);
    
    double freq = hzPerBin*idx;
    double mel = FreqToMel(freq);
    
    double resultId = (mel/maxMel)*(bufferSize/2);
    
    if (resultId < 0)
        resultId = 0;
    if (resultId > bufferSize/2 - 1)
        resultId = bufferSize/2;
    
    return resultId;
}

double
Utils::FreqToMelNormId(double freq, double hzPerBin, int bufferSize)
{
    double maxFreq = hzPerBin*(bufferSize/2);
    double maxMel = FreqToMel(maxFreq);
    
    double mel = FreqToMel(freq);
    
    double resultId = (mel/maxMel)*(bufferSize/2);
    
    resultId = round(resultId);
    if (resultId < 0)
        resultId = 0;
    if (resultId > bufferSize/2 - 1)
        resultId = bufferSize/2;
    
    return resultId;
}

double
Utils::FreqToMelNorm(double freq, double hzPerBin, int bufferSize)
{
    double maxFreq = hzPerBin*(bufferSize/2);
    double maxMel = FreqToMel(maxFreq);
    
    double mel = FreqToMel(freq);
    
    double result = mel/maxMel;
    
    return result;
}


// Touch plug param
// When param is modified out of GUI or indirectly,
// touch the host
// => Then automations can be read
//
// NOTE: take care with Waveform Tracktion
void
Utils::TouchPlugParam(IPlug *plug, int paramIdx)
{
    // Force host update param, for automation
    plug->BeginInformHostOfParamChange(paramIdx);
    plug->GetGUI()->SetParameterFromPlug(paramIdx, plug->GetParam(paramIdx)->GetNormalized(), true);
    plug->InformHostOfParamChange(paramIdx, plug->GetParam(paramIdx)->GetNormalized());
    plug->EndInformHostOfParamChange(paramIdx);
}

double
Utils::FindNearestHarmonic(double value, double refValue)
{
    if (value < refValue)
        return refValue;
    
    double coeff = value / refValue;
    double rem = coeff - (int)coeff;
    
    double result = value;
    if (rem < 0.5)
    {
        result = refValue*((int)coeff);
    }
    else
    {
        result = refValue*((int)coeff + 1);
    }
    
    return result;
}

// Recursive function to return gcd of a and b
//
// See: https://www.geeksforgeeks.org/program-find-gcd-floating-point-numbers/
//
double
Utils::gcd(double a, double b)
{
#define EPS 0.001
    
    if (a < b)
        return gcd(b, a);
    
    // base case
    if (fabs(b) < EPS)
        return a;
    
    else
        return (gcd(b, a - floor(a / b) * b));
}

// Function to find gcd of array of numbers
//
// See: https://www.geeksforgeeks.org/gcd-two-array-numbers/
//
double
Utils::gcd(const vector<double> &arr)
{
    double result = arr[0];
    for (int i = 1; i < arr.size(); i++)
        result = gcd(arr[i], result);
    
    return result;
}

void
Utils::MixParamToCoeffs(double mix, double *coeff0, double *coeff1)
{
    //mix = (mix - 0.5)*2.0;
    
    if (mix <= 0.0)
    {
        *coeff0 = 1.0;
        *coeff1 = 1.0 + mix;
    }
    else if (mix > 0.0)
    {
        *coeff0 = 1.0 - mix;
        *coeff1 = 1.0;
    }
}

void
Utils::SmoothDataWin(WDL_TypedBuf<double> *result,
                     const WDL_TypedBuf<double> &data,
                     const WDL_TypedBuf<double> &win)
{
    result->Resize(data.GetSize());
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        double sumVal = 0.0;
        double sumCoeff = 0.0;
        for (int j = 0; j < win.GetSize(); j++)
        {
            int idx = i + j - win.GetSize()/2;
            
            if ((idx < 0) || (idx > result->GetSize() - 1))
                continue;
            
            double val = data.Get()[idx];
            double coeff = win.Get()[j];
            
            sumVal += val*coeff;
            sumCoeff += coeff;
        }
        
        if (sumCoeff > EPS)
        {
            sumVal /= sumCoeff;
        }
        
        result->Get()[i] = sumVal;
    }
}

void
Utils::SmoothDataPyramid(WDL_TypedBuf<double> *result,
                         const WDL_TypedBuf<double> &data,
                         int maxLevel)
{
    WDL_TypedBuf<double> xCoordinates;
    xCoordinates.Resize(data.GetSize());
    for (int i = 0; i < data.GetSize(); i++)
    {
        xCoordinates.Get()[i] = i;
    }
    
    WDL_TypedBuf<double> yCoordinates = data;
    
    int level = 1;
    while(level < maxLevel)
    {
        WDL_TypedBuf<double> newXCoordinates;
        WDL_TypedBuf<double> newYCoordinates;
        
        // Comppute new size
        double newSize = xCoordinates.GetSize()/2.0;
        newSize = ceil(newSize);
        
        // Prepare
        newXCoordinates.Resize((int)newSize);
        newYCoordinates.Resize((int)newSize);
        
        // Copy the last value (in case of odd size)
        newXCoordinates.Get()[(int)newSize - 1] =
                xCoordinates.Get()[xCoordinates.GetSize() - 1];
        newYCoordinates.Get()[(int)newSize - 1] =
            yCoordinates.Get()[yCoordinates.GetSize() - 1];
        
        // Divide by 2
        for (int i = 0; i < xCoordinates.GetSize(); i += 2)
        {
            // x
            double x0 = xCoordinates.Get()[i];
            double x1 = x0;
            if (i + 1 < xCoordinates.GetSize())
                x1 = xCoordinates.Get()[i + 1];
            
            // y
            double y0 = yCoordinates.Get()[i];
            
            double y1 = y0;
            if (i + 1 < yCoordinates.GetSize())
                y1 = yCoordinates.Get()[i + 1];
            
            // new
            double newX = (x0 + x1)/2.0;
            double newY = (y0 + y1)/2.0;
            
            // add
            newXCoordinates.Get()[i/2] = newX;
            newYCoordinates.Get()[i/2] = newY;
        }
        
        xCoordinates = newXCoordinates;
        yCoordinates = newYCoordinates;
        
        level++;
    }
    
    // Prepare the result
    result->Resize(data.GetSize());
    Utils::FillAllZero(result);
    
    // Put the low LOD values into the result
    for (int i = 0; i < xCoordinates.GetSize(); i++)
    {
        double x = xCoordinates.Get()[i];
        double y = yCoordinates.Get()[i];
        
        x = round(x);
        
        // check bounds
        if (x < 0.0)
            x = 0.0;
        if (x > result->GetSize() - 1)
            x = result->GetSize() - 1;
        
        result->Get()[(int)x] = y;
    }
    
    // And complete the values that remained zero
    Utils::FillMissingValues(result, false, 0.0);
}

void
Utils::ApplyWindowMin(WDL_TypedBuf<double> *values, int winSize)
{
    WDL_TypedBuf<double> result;
    result.Resize(values->GetSize());
    
    for (int i = 0; i < result.GetSize(); i++)
    {
        double minVal = INF;
        for (int j = 0; j < winSize; j++)
        {
            int idx = i + j - winSize/2;
            
            if ((idx < 0) || (idx > result.GetSize() - 1))
                continue;
            
            double val = values->Get()[idx];
            if (val < minVal)
                minVal = val;
        }
        
        result.Get()[i] = minVal;
    }
    
    *values = result;
}

void
Utils::ApplyWindowMax(WDL_TypedBuf<double> *values, int winSize)
{
    WDL_TypedBuf<double> result;
    result.Resize(values->GetSize());
    
    for (int i = 0; i < result.GetSize(); i++)
    {
        double maxVal = -INF;
        for (int j = 0; j < winSize; j++)
        {
            int idx = i + j - winSize/2;
            
            if ((idx < 0) || (idx > result.GetSize() - 1))
                continue;
            
            double val = values->Get()[idx];
            if (val > maxVal)
                maxVal = val;
        }
        
        result.Get()[i] = maxVal;
    }
    
    *values = result;
}

double
Utils::ComputeMean(const WDL_TypedBuf<double> &values)
{
    if (values.GetSize() == 0)
        return 0.0;
    
    double sum = 0.0;
    for (int i = 0; i < values.GetSize(); i++)
    {
        double val = values.Get()[i];
        
        sum += val;
    }
    
    double result = sum/values.GetSize();
    
    return result;
}

double
Utils::ComputeSigma(const WDL_TypedBuf<double> &values)
{
    if (values.GetSize() == 0)
        return 0.0;
    
    double mean = ComputeMean(values);
    
    double sum = 0.0;
    for (int i = 0; i < values.GetSize(); i++)
    {
        double val = values.Get()[i];
        
        double diff = fabs(val - mean);
        
        sum += diff;
    }
    
    double result = sum/values.GetSize();
    
    return result;
}

double
Utils::ComputeMean(const vector<double> &values)
{
    if (values.empty())
        return 0.0;
    
    double sum = 0.0;
    for (int i = 0; i < values.size(); i++)
    {
        double val = values[i];
        
        sum += val;
    }
    
    double result = sum/values.size();
    
    return result;
}

double
Utils::ComputeSigma(const vector<double> &values)
{
    if (values.size())
        return 0.0;
    
    double mean = ComputeMean(values);
    
    double sum = 0.0;
    for (int i = 0; i < values.size(); i++)
    {
        double val = values[i];
        
        double diff = fabs(val - mean);
        
        sum += diff;
    }
    
    double result = sum/values.size();
    
    return result;
}

void
Utils::GetMinMaxFreqAxisValues(double *minHzValue, double *maxHzValue,
                               int bufferSize, double sampleRate)
{
    double hzPerBin = sampleRate/bufferSize;
    
    *minHzValue = 1*hzPerBin;
    *maxHzValue = (bufferSize/2)*hzPerBin;
}

void
Utils::GenNoise(WDL_TypedBuf<double> *ioBuf)
{
    for (int i = 0; i < ioBuf->GetSize(); i++)
    {
        double noise = ((double)rand())/RAND_MAX;
        ioBuf->Get()[i] = noise;
    }
}

double
Utils::ComputeDist(double p0[2], double p1[2])
{
    double a = p0[0] - p1[0];
    double b = p0[1] - p1[1];
    
    double d2 = a*a + b*b;
    double dist = sqrtf(d2);
    
    return dist;
}
