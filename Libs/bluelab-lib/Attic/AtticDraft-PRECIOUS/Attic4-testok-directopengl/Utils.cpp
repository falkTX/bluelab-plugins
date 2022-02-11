//
//  Utils.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#include <math.h>

#include "Utils.h"

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
Utils::ComputeAvg(const double *output, int nFrames)
{
    double avg = 0.0;
    
    for (int i = 0; i < nFrames; i++)
    {
        double value = output[i];
        avg += value;
    }
    
    avg = avg/nFrames;
    
    return avg;
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
Utils::NormalizedXTodB(double x, double mindB, double maxdB)
{
    x = x*(maxdB - mindB) + mindB;
    
    if (x > 0.0)
        x = AmpToDB(x);
        
    double lMin = AmpToDB(mindB);
    double lMax = AmpToDB(maxdB);
        
    x = (x - lMin)/(lMax - lMin);
    
    return x;
}

double
Utils::NormalizedYTodB(double y, double mindB, double maxdB)
{
#define EPS 1e-16
#define INF 1e16
    
    if (y < EPS)
        y = -INF;
    else
        y = AmpToDB(y);
    
    y = (y - mindB)/(maxdB - mindB);
    
    return y;
}

double
Utils::NormalizedYTodB2(double y, double mindB, double maxdB)
{
    y = AmpToDB(y);
    
    y = (y - mindB)/(maxdB - mindB);
    
    return y;
}

double
Utils::NormalizedYTodB3(double y, double mindB, double maxdB)
{
    y = y*(maxdB - mindB) + mindB;
    
    if (y > 0.0)
        y = AmpToDB(y);
    
    double lMin = AmpToDB(mindB);
    double lMax = AmpToDB(maxdB);
    
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
Utils::StereoToMono(WDL_TypedBuf<double> *monoResult,
                    const double *in0, const double *in1, int nFrames)
{
    if ((in0 == NULL) && (in1 == NULL))
        return;
    
    monoResult->Resize(nFrames);
    
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

// Compute the similarity between two noramlized curves
double
Utils::ComputeCurveMatchCoeff(const double *curve0, const double *curve1, int nFrames)
{
    double area = 0.0;
    for (int i = 0; i < nFrames; i++)
    {
        double val0 = curve0[i];
        double val1 = curve1[i];
        
        double a = fabs(val0 - val1);
        
        area += a;
    }
    
    double coeff = area / nFrames;
    
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
    
    // scin
    scIn[0] = ((numInScChannels > 0) && isInConnected[numInChannels - numInScChannels]) ?
                inputs[numInChannels - numInScChannels] : NULL;
    scIn[1] = ((numInScChannels > 1) && isInConnected[numInChannels - numInScChannels + 1]) ?
                inputs[numInChannels - numInScChannels + 1] : NULL;
    
    // out
    out[0] = plug->IsOutChannelConnected(0) ? outputs[0] : NULL;
    out[1] = plug->IsOutChannelConnected(1) ? outputs[1] : NULL;
}

double
Utils::FftBinToFreq(int binNum, int numBins, int sampleRate)
{
    if (binNum > numBins/2)
        // Second half => not relevant
        return -1.0;
        
    return ((double)(binNum * sampleRate))/numBins;
}

int
Utils::FreqToFftBin(double freq, int numBins, int sampleRate, double *t)
{
    double fftBin = (freq*(double)numBins)/(double)sampleRate;
    
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
Utils::MinMaxFftBinFreq(double *minFreq, double *maxFreq, int numBins, int sampleRate)
{
    *minFreq = ((double)sampleRate)/numBins;
    *maxFreq = ((double)(numBins/2 - 1)*sampleRate)/numBins;
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
