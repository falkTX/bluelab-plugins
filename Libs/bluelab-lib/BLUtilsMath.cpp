#include <BLUtils.h>

#include "BLUtilsMath.h"

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::Round(FLOAT_TYPE val, int precision)
{
    val = val*std::pow((FLOAT_TYPE)10.0, precision);
    val = bl_round(val);
    //val *= std::pow(10, -precision);
    val *= std::pow((FLOAT_TYPE)10.0, -precision);
    
    return val;
}
template float BLUtilsMath::Round(float val, int precision);
template double BLUtilsMath::Round(double val, int precision);

template <typename FLOAT_TYPE>
void
BLUtilsMath::Round(FLOAT_TYPE *buf, int nFrames, int precision)
{
    for (int i = 0; i < nFrames; i++)
    {
        FLOAT_TYPE val = buf[i];
        FLOAT_TYPE res = BLUtilsMath::Round(val, precision);
        
        buf[i] = res;
    }
}
template void BLUtilsMath::Round(float *buf, int nFrames, int precision);
template void BLUtilsMath::Round(double *buf, int nFrames, int precision);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::DomainAtan2(FLOAT_TYPE x, FLOAT_TYPE y)
{
    FLOAT_TYPE signx;
    if (x > 0.) signx = 1.;
    else signx = -1.;
    
    if (x == 0.) return 0.;
    if (y == 0.) return signx * M_PI / 2.;
    
    return std::atan2(x, y);
}
template float BLUtilsMath::DomainAtan2(float x, float y);
template double BLUtilsMath::DomainAtan2(double x, double y);

int
BLUtilsMath::NextPowerOfTwo(int value)
{
    int result = 1;
    
    while(result < value)
        result *= 2;
    
    return result;
}

template <typename FLOAT_TYPE>
int
BLUtilsMath::SecondOrderEqSolve(FLOAT_TYPE a, FLOAT_TYPE b,
                                FLOAT_TYPE c, FLOAT_TYPE res[2])
{
    // See: http://math.lyceedebaudre.net/premiere-sti2d/second-degre/resoudre-une-equation-du-second-degre
    //
    FLOAT_TYPE delta = b*b - 4.0*a*c;
    
    if (delta > 0.0)
    {
        res[0] = (-b - std::sqrt(delta))/(2.0*a);
        res[1] = (-b + std::sqrt(delta))/(2.0*a);
        
        return 2;
    }
    
    if (std::fabs(delta) < BL_EPS)
    {
        res[0] = -b/(2.0*a);
        
        return 1;
    }
    
    return 0;
}
template int BLUtilsMath::SecondOrderEqSolve(float a, float b, float c,
                                             float res[2]);
template int BLUtilsMath::SecondOrderEqSolve(double a, double b, double c,
                                             double res[2]);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::FindNearestHarmonic(FLOAT_TYPE value, FLOAT_TYPE refValue)
{
    if (value < refValue)
        return refValue;
    
    FLOAT_TYPE coeff = value / refValue;
    FLOAT_TYPE rem = coeff - (int)coeff;
    
    FLOAT_TYPE result = value;
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
template float BLUtilsMath::FindNearestHarmonic(float value, float refValue);
template double BLUtilsMath::FindNearestHarmonic(double value, double refValue);

// Recursive function to return gcd of a and b
//
// See: https://www.geeksforgeeks.org/program-find-gcd-floating-point-numbers/
//
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::gcd(FLOAT_TYPE a, FLOAT_TYPE b)
{    
    if (a < b)
        return gcd(b, a);
    
    // base case
    if (std::fabs(b) < BL_EPS3)
        return a;
    
    else
        return (gcd(b, a - std::floor(a / b) * b));
}
template float BLUtilsMath::gcd(float a, float b);
template double BLUtilsMath::gcd(double a, double b);

// Function to find gcd of array of numbers
//
// See: https://www.geeksforgeeks.org/gcd-two-array-numbers/
//
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::gcd(const vector<FLOAT_TYPE> &arr)
{
    FLOAT_TYPE result = arr[0];
    for (int i = 1; i < arr.size(); i++)
        result = gcd(arr[i], result);
    
    return result;
}
template float BLUtilsMath::gcd(const vector<float> &arr);
template double BLUtilsMath::gcd(const vector<double> &arr);

// See: http://paulbourke.net/miscellaneous/correlate/
template <typename FLOAT_TYPE>
void
BLUtilsMath::CrossCorrelation2D(const vector<WDL_TypedBuf<FLOAT_TYPE> > &image,
                                const vector<WDL_TypedBuf<FLOAT_TYPE> > &mask,
                                vector<WDL_TypedBuf<FLOAT_TYPE> > *corr)
{
    if (image.empty())
        return;
    
    if (image.size() != mask.size())
        return;
    
    // Allocate
    int lineSize = image[0].GetSize();
    
    corr->resize(image.size());
    for (int i = 0; i < image.size(); i++)
    {
        (*corr)[i].Resize(lineSize);
    }
    
    //
    FLOAT_TYPE imageAvg = BLUtils::ComputeAvg(image);
    FLOAT_TYPE maskAvg = BLUtils::ComputeAvg(mask);
    
    // Compute
    for (int i = 0; i < image.size(); i++)
    {
        for (int j = 0; j < lineSize; j++)
        {
            FLOAT_TYPE rij = 0.0;
            
            // Sum
            for (int ii = -(int)image.size()/2; ii < (int)image.size()/2; ii++)
            {
                for (int jj = -lineSize/2; jj < lineSize/2; jj++)
                {
                    // Mask
                    if (ii + (int)image.size()/2 < 0)
                        continue;
                    if (ii + (int)image.size()/2 >= (int)mask.size())
                        continue;
                 
                    if (jj + lineSize/2 < 0)
                        continue;
                    if (jj + lineSize/2 >= mask[ii + (int)image.size()/2].GetSize())
                        continue;
                    
                    FLOAT_TYPE maskVal =
                        mask[ii + (int)image.size()/2].Get()[jj + lineSize/2];
                    
                    // Image
                    if (i + ii < 0)
                        continue;
                    if (i + ii >= (int)image.size())
                        continue;
                    
                    if (j + jj < 0)
                        continue;
                    if (j + jj >= image[i + ii].GetSize())
                        continue;
                    
                    FLOAT_TYPE imageVal = image[i + ii].Get()[j + jj];
                    
                    FLOAT_TYPE r = (maskVal - maskAvg)*(imageVal - imageAvg);
                    
                    rij += r;
                }
            }
            
            (*corr)[i].Get()[j] = rij;
        }
    }
}
template void
BLUtilsMath::CrossCorrelation2D(const vector<WDL_TypedBuf<float> > &image,
                                const vector<WDL_TypedBuf<float> > &mask,
                                vector<WDL_TypedBuf<float> > *corr);
template void
BLUtilsMath::CrossCorrelation2D(const vector<WDL_TypedBuf<double> > &image,
                                const vector<WDL_TypedBuf<double> > &mask,
                                vector<WDL_TypedBuf<double> > *corr);

template <typename FLOAT_TYPE>
void
BLUtilsMath::Transpose(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                       int width, int height)
{
    if (ioBuf->GetSize() != width*height)
        return;
    
    WDL_TypedBuf<FLOAT_TYPE> result = *ioBuf;
    
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            FLOAT_TYPE val = ioBuf->Get()[i + j*width];
            result.Get()[j + i*height] = val;
        }
    }
    
    *ioBuf = result;
}
template void BLUtilsMath::Transpose(WDL_TypedBuf<float> *ioBuf,
                                     int width, int height);
template void BLUtilsMath::Transpose(WDL_TypedBuf<double> *ioBuf,
                                     int width, int height);

template <typename FLOAT_TYPE>
void
BLUtilsMath::Reshape(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                     int inWidth, int inHeight,
                     int outWidth, int outHeight)
{
    if (ioBuf->GetSize() != inWidth*inHeight)
        return;
    
    if (ioBuf->GetSize() != outWidth*outHeight)
        return;
    
    WDL_TypedBuf<FLOAT_TYPE> result;
    result.Resize(ioBuf->GetSize());
    
#if 0
    int idx = 0;
    int idy = 0;
    for (int j = 0; j < inHeight; j++)
    {
        for (int i = 0; i < inWidth; i++)
        {
            result.Get()[i + j*inWidth] = ioBuf->Get()[idx + idy*outWidth];
            
            idx++;
            if (idx >= outWidth)
            {
                idx = 0;
                idy++;
            }
        }
    }
#endif
    
#if 1
    if (inWidth > outWidth)
    {
        int idx = 0;
        int idy = 0;
        int stride = 0;
        for (int j = 0; j < outHeight; j++)
        {
            for (int i = 0; i < outWidth; i++)
            {
                result.Get()[i + j*outWidth] =
                    ioBuf->Get()[(idx + stride) + idy*inWidth];
            
                idx++;
                if (idx >= outWidth)
                {
                    idx = 0;
                    idy++;
                }
            
                if (idy >= inHeight)
                {
                    idy = 0;
                    stride += outWidth;
                }
            }
        }
    }
    else // outWidth > inWidth
    {
        int idx = 0;
        int idy = 0;
        int stride = 0;
        for (int j = 0; j < inHeight; j++)
        {
            for (int i = 0; i < inWidth; i++)
            {
                result.Get()[(idx + stride) + idy*outWidth] =
                    ioBuf->Get()[i + j*inWidth];
                
                idx++;
                if (idx >= inWidth)
                {
                    idx = 0;
                    idy++;
                }
                
                if (idy >= outHeight)
                {
                    idy = 0;
                    stride += inWidth;
                }
            }
        }
    }
        
#endif
    
    *ioBuf = result;
}
template void BLUtilsMath::Reshape(WDL_TypedBuf<float> *ioBuf,
                                   int inWidth, int inHeight,
                                   int outWidth, int outHeight);
template void BLUtilsMath::Reshape(WDL_TypedBuf<double> *ioBuf,
                                   int inWidth, int inHeight,
                                   int outWidth, int outHeight);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::ComputeVariance(const WDL_TypedBuf<FLOAT_TYPE> &data)
{
    if (data.GetSize() == 0)
        return 0.0;
    
    FLOAT_TYPE avg = BLUtils::ComputeAvg(data);
    
    FLOAT_TYPE variance = 0.0;
    for (int i = 0; i < data.GetSize(); i++)
    {
        FLOAT_TYPE val = data.Get()[i];
            
        variance += (val - avg)*(val - avg);
    }
    
    variance /= data.GetSize();
    
    return variance;
}
template float BLUtilsMath::ComputeVariance(const WDL_TypedBuf<float> &data);
template double BLUtilsMath::ComputeVariance(const WDL_TypedBuf<double> &data);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::ComputeVariance(const vector<FLOAT_TYPE> &data)
{
    if (data.size() == 0)
        return 0.0;
    
    FLOAT_TYPE avg = BLUtils::ComputeAvg(data);
    
    FLOAT_TYPE variance = 0.0;
    for (int i = 0; i < data.size(); i++)
    {
        FLOAT_TYPE val = data[i];
        
        variance += (val - avg)*(val - avg);
    }
    
    variance /= data.size();
    
    return variance;
}
template float BLUtilsMath::ComputeVariance(const vector<float> &data);
template double BLUtilsMath::ComputeVariance(const vector<double> &data);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::ComputeVariance2(const vector<FLOAT_TYPE> &data)
{
    if (data.size() == 0)
        return 0.0;
    
    if (data.size() == 1)
        return data[0];
    
    FLOAT_TYPE avg = BLUtils::ComputeAvg(data);
    
    FLOAT_TYPE variance = 0.0;
    for (int i = 0; i < data.size(); i++)
    {
        FLOAT_TYPE val = data[i];
        
        // This formula is used in mask predictor comp3
        variance += val*val - avg*avg;
    }
    
    variance /= data.size();
    
    return variance;
}
template float BLUtilsMath::ComputeVariance2(const vector<float> &data);
template double BLUtilsMath::ComputeVariance2(const vector<double> &data);

// Checked! That computes Lagrange interp
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::LagrangeInterp4(FLOAT_TYPE x,
                             FLOAT_TYPE p0[2], FLOAT_TYPE p1[2],
                             FLOAT_TYPE p2[2], FLOAT_TYPE p3[2])
{
    FLOAT_TYPE pts[4][2] = { { p0[0], p0[1] },
                             { p1[0], p1[1] },
                             { p2[0], p2[1] },
                             { p3[0], p3[1] } };
    
    FLOAT_TYPE l[4] = { 1.0, 1.0, 1.0, 1.0 };
    for (int j = 0; j < 4; j++)
    {
        for (int m = 0; m < 4; m++)
        {
            if (m != j)
            {
                FLOAT_TYPE xxm = x - pts[m][0];
                FLOAT_TYPE xjxm = pts[j][0] - pts[m][0];
                
                //if (std::fabs(xjxm) > BL_EPS)
                //{
                l[j] *= xxm/xjxm;
                //}
            }
        }
    }
    
    FLOAT_TYPE y = 0.0;
    for (int j = 0; j < 4; j++)
    {
        y += pts[j][1]*l[j];
    }
    
    return y;
}
template float BLUtilsMath::LagrangeInterp4(float x,
                                            float p0[2], float p1[2],
                                            float p2[2], float p3[2]);
template double BLUtilsMath::LagrangeInterp4(double x,
                                             double p0[2], double p1[2],
                                             double p2[2], double p3[2]);

// See: https://stackoverflow.com/questions/8424170/1d-linear-convolution-in-ansi-c-code
// a is Signal
// b is Kernel
// result size is: SignalLen + KernelLen - 1
void
BLUtilsMath::Convolve(const WDL_TypedBuf<BL_FLOAT> &a,
                      const WDL_TypedBuf<BL_FLOAT> &b,
                      WDL_TypedBuf<BL_FLOAT> *result,
                      int convoMode)
{
    // Standard convolution
    if (a.GetSize() != b.GetSize())
        return;
    result->Resize(a.GetSize() + b.GetSize() - 1);
    BLUtils::FillAllZero(result);
    
    for (int i = 0; i < a.GetSize() + b.GetSize() - 1; i++)
    {
        int kmin, kmax, k;
        
        result->Get()[i] = 0.0;
        
        kmin = (i >= b.GetSize() - 1) ? i - (b.GetSize() - 1) : 0;
        kmax = (i < a.GetSize() - 1) ? i : a.GetSize() - 1;
        
        for (k = kmin; k <= kmax; k++)
        {
            result->Get()[i] += a.Get()[k] * b.Get()[i - k];
        }
    }
    
    // Manage the modes
    //
    if (convoMode == CONVO_MODE_FULL)
        // We are ok
        return;
    
    if (convoMode == CONVO_MODE_SAME)
    {
        int maxSize = (a.GetSize() > b.GetSize()) ? a.GetSize() : b.GetSize();
        
        int numCrop = result->GetSize() - maxSize;
        int numCrop0 = numCrop/2;
        int numCrop1 = numCrop0;
        
        // Adjust
        if (numCrop0 + numCrop1 < numCrop)
            numCrop0++;
        
        if (numCrop0 > 0)
        {
            BLUtils::ConsumeLeft(result, numCrop0);
            result->Resize(result->GetSize() - numCrop1);
        }
        
        return;
    }
    
    if (convoMode == CONVO_MODE_VALID)
    {
        int maxSize = (a.GetSize() > b.GetSize()) ? a.GetSize() : b.GetSize();
        int minSize = (a.GetSize() < b.GetSize()) ? a.GetSize() : b.GetSize();
        
        int numCrop2 = (result->GetSize() - (maxSize - minSize + 1))/2;
        
        if (numCrop2 > 0)
        {
            BLUtils::ConsumeLeft(result, numCrop2);
            result->Resize(result->GetSize() - numCrop2);
        }
        
        return;
    }
}

// See: https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
//
// Returns 1 if the lines intersect, otherwise 0. In addition, if the lines
// intersect the intersection point may be stored in the floats i_x and i_y.
static int
get_line_intersection(BL_FLOAT p0_x, BL_FLOAT p0_y, BL_FLOAT p1_x, BL_FLOAT p1_y,
                      BL_FLOAT p2_x, BL_FLOAT p2_y, BL_FLOAT p3_x, BL_FLOAT p3_y,
                      BL_FLOAT *i_x, BL_FLOAT *i_y)
{
    BL_FLOAT s1_x, s1_y, s2_x, s2_y;
    s1_x = p1_x - p0_x;
    s1_y = p1_y - p0_y;
    
    s2_x = p3_x - p2_x;
    s2_y = p3_y - p2_y;
    
    BL_FLOAT s, t;
    s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
    t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);
    
    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
        // Collision detected
        if (i_x != NULL)
            *i_x = p0_x + (t * s1_x);
        if (i_y != NULL)
            *i_y = p0_y + (t * s1_y);
        
        return 1;
    }
    
    return 0; // No collision
}

bool
BLUtilsMath::SegSegIntersect(BL_FLOAT seg0[2][2], BL_FLOAT seg1[2][2])
{
    int inter =
        get_line_intersection(seg0[0][0], seg0[0][1], seg0[1][0], seg0[1][1],
                              seg1[0][0], seg1[0][1], seg1[1][0], seg1[1][1],
                              NULL, NULL);
    
    return (bool)inter;
}

static bool
ccw(BL_FLOAT A[2], BL_FLOAT B[2], BL_FLOAT C[2])
{
    bool res = (C[1] - A[1])*(B[0] - A[0]) > (B[1] - A[1])*(C[0] - A[0]);

    return res;
}

bool
BLUtilsMath::SegSegIntersect2(BL_FLOAT seg0[2][2], BL_FLOAT seg1[2][2])
{
    //bool res = ccw(A,C,D) != ccw(B,C,D) and ccw(A,B,C) != ccw(A,B,D);
    bool res = ((ccw(seg0[0], seg1[0], seg1[1]) != ccw(seg0[1], seg1[0], seg1[1])) &&
                (ccw(seg0[0], seg0[1], seg1[0]) != ccw(seg0[0], seg0[1], seg1[1])));

    return res;
}

// Schlick sigmoid, see:
//
// https://dept-info.labri.u-bordeaux.fr/~schlick/DOC/gem2.ps.gz
//
// a included in [0, 1]
// a = 0.5 -> gives a line

#if 0 // Naive method (first page of the paper)
// bias
static float betaA(float t, float a)
{
    float bA = powf(t, -log2f(a));
    return bA;
}

static double betaA(double t, double a)
{
    double bA = pow(t, -log2(a));
    return bA;
}

// gain
template <typename FLOAT_TYPE>
FLOAT_TYPE BLUtilsMath::ApplySigmoid(FLOAT_TYPE t, FLOAT_TYPE a)
{
    if (t < 0.0)
        t = 0.0;
    if (t > 1.0)
        t = 1.0;
    
    // gain
    FLOAT_TYPE gammaA;
    if (t < (FLOAT_TYPE)0.5)
        gammaA = 0.5*betaA((FLOAT_TYPE)2.0*t, a);
    else
        gammaA = 1.0 - 0.5*betaA((FLOAT_TYPE)(2.0 - 2.0*t), a);

    return gammaA;
        
}
template float BLUtilsMath::ApplySigmoid(float t, float a);
template double BLUtilsMath::ApplySigmoid(double t, double a);
#endif

#if 1 //Schlick method (more efficient)
// gain
template <typename FLOAT_TYPE>
FLOAT_TYPE BLUtilsMath::ApplySigmoid(FLOAT_TYPE t, FLOAT_TYPE a)
{
    if (t < 0.0)
        t = 0.0;
    if (t > 1.0)
        t = 1.0;
    
    // gain
    FLOAT_TYPE gammaA;
#if 0 
    if (t < (FLOAT_TYPE)0.5)
        gammaA = t/((1.0/a - 2.0)*(1.0 - 2.0*t) + 1.0);
    else
        gammaA = ((1.0/a - 2.0)*(1.0 - 2.0*t) - t)/
            ((1.0/a - 2.0)*(1.0 - 2.0*t) - 1.0);
#endif
#if 1 // Optim
    FLOAT_TYPE fac0 = (1.0/a - 2.0)*(1.0 - 2.0*t);
    
    if (t < (FLOAT_TYPE)0.5)
        gammaA = t/(fac0 + 1.0);
    else
        gammaA = (fac0 - t)/(fac0 - 1.0);
#endif
            
    return gammaA;
}
template float BLUtilsMath::ApplySigmoid(float t, float a);
template double BLUtilsMath::ApplySigmoid(double t, double a);
#endif

template <typename FLOAT_TYPE>
void
BLUtilsMath::ApplySigmoid(WDL_TypedBuf<FLOAT_TYPE> *data, FLOAT_TYPE a)
{
    int dataSize = data->GetSize();
    FLOAT_TYPE *dataBuf = data->Get();

    FLOAT_TYPE fac0 = (1.0/a - 2.0);
    
    for (int i = 0; i < dataSize; i++)
    {
        FLOAT_TYPE t = dataBuf[i];

        FLOAT_TYPE fac1 = fac0*(1.0 - 2.0*t);

        FLOAT_TYPE gammaA;
        if (t < (FLOAT_TYPE)0.5)
            gammaA = t/(fac1 + 1.0);
        else
            gammaA = (fac1 - t)/(fac1 - 1.0);

        dataBuf[i] = gammaA;
    }
}
template void BLUtilsMath::ApplySigmoid(WDL_TypedBuf<float> *data, float a);
template void BLUtilsMath::ApplySigmoid(WDL_TypedBuf<double> *data, double a);

template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::ApplyGamma(FLOAT_TYPE t, FLOAT_TYPE a)
{
    FLOAT_TYPE bA = t/((1.0/a - 2.0)*(1.0 - t) + 1.0);
    
    return bA;
}
template float BLUtilsMath::ApplyGamma(float t, float a);
template double BLUtilsMath::ApplyGamma(double t, double a);
    
template <typename FLOAT_TYPE>
void
BLUtilsMath::ApplyGamma(WDL_TypedBuf<FLOAT_TYPE> *data, FLOAT_TYPE a)
{
    int dataSize = data->GetSize();
    FLOAT_TYPE *dataBuf = data->Get();

    FLOAT_TYPE fac0 = (1.0/a - 2.0);
    
    for (int i = 0; i < dataSize; i++)
    {
        FLOAT_TYPE t = dataBuf[i];

        FLOAT_TYPE bA = t/(fac0*(1.0 - t) + 1.0);

        dataBuf[i] = bA;
    }
}
template void BLUtilsMath::ApplyGamma(WDL_TypedBuf<float> *data, float a);
template void BLUtilsMath::ApplyGamma(WDL_TypedBuf<double> *data, double a);
    
template <typename FLOAT_TYPE>
void BLUtilsMath::LinearResample(const FLOAT_TYPE *srcBuf, int srcSize,
                                 FLOAT_TYPE *dstBuf, int dstSize)
{   
	FLOAT_TYPE scale = srcSize/(FLOAT_TYPE)dstSize;
    
	FLOAT_TYPE index = 0.0;
	for (int i = 0; i < dstSize; i++)
	{
		int indexI = (int)floor(index);
		FLOAT_TYPE t = index - indexI;
		FLOAT_TYPE v0 = srcBuf[indexI];
		FLOAT_TYPE v1 = srcBuf[indexI + 1];
        
        dstBuf[i] = (1.0 - t)*v0 + t*v1;
		index += scale;
	}
    
    // #bluelab
    // Ensure that the last value is the same on the resampled signal
    // (this was not the case before)
    dstBuf[dstSize - 1] = srcBuf[srcSize - 1];
}
template void BLUtilsMath::LinearResample(const float *srcBuf, int srcSize,
                                          float *dstBuf, int dstSize);
template void BLUtilsMath::LinearResample(const double *srcBuf, int srcSize,
                                          double *dstBuf, int dstSize);

// NOTE: tested, this is ok!
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::PolygonArea(const FLOAT_TYPE x[], const FLOAT_TYPE y[], int n)
{
    if (n < 3)
        return 0.0;
    
    FLOAT_TYPE area = 0.0;

    // Shoelace formula
    // See: https://rosettacode.org/wiki/Shoelace_formula_for_polygonal_area#C
    for (int i = 0; i < n; i++)
        area += x[i]*y[(i + 1) % n] - x[(i + 1) % n]*y[i];
    
    // Return absolute value
    return std::fabs(area*0.5);
}
template float BLUtilsMath::PolygonArea(const float x[], const float y[], int n);
template double BLUtilsMath::PolygonArea(const double x[], const double y[], int n);

// NOTE: should also work for obtuse trapezoids
template <typename FLOAT_TYPE>
FLOAT_TYPE
BLUtilsMath::TrapezoidArea(FLOAT_TYPE a, FLOAT_TYPE b, FLOAT_TYPE h)
{
    FLOAT_TYPE area = (a + b)*h*0.5;
    
    return area;
}
template float BLUtilsMath::TrapezoidArea(float a, float b, float h);
template double BLUtilsMath::TrapezoidArea(double a, double b, double h);

