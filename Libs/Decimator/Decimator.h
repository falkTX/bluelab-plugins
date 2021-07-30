#ifndef DECIMATOR_H
#define DECIMATOR_H

#include <BLUtils.h>
#include <BLUtilsMath.h>

// Polyphase decimation filter.
//
// Convert an oversampled audio stream to non-oversampled.  Uses a
// windowed sinc FIR filter w/ Blackman window to control aliasing.
// Christian Floisand's 'blog explains it very well.
//
// This version has a very simple main processing loop (the decimate
// method) which vectorizes easily.
//
// Refs:
//   https://christianfloisand.wordpress.com/2012/12/05/audio-resampling-part-1/
//   https://christianfloisand.wordpress.com/2013/01/28/audio-resampling-part-2/
//   http://www.dspguide.com/ch16.htm
//   http://en.wikipedia.org/wiki/Window_function#Blackman_windows

#ifndef DECIMATOR_SAMPLE
#define DECIMATOR_SAMPLE double
#endif

class Decimator {

public:
           Decimator();
          ~Decimator();

    void   initialize(double   decimatedSampleRate,
                      double   passFrequency,
                      unsigned oversampleRatio);

    double oversampleRate()  const { return mOversampleRate; }
    int    oversampleRatio() const { return mRatio; }

    void   decimate(DECIMATOR_SAMPLE *in, DECIMATOR_SAMPLE *out, size_t outCount);
           // N.B., input must have (ratio * outCount) samples.

private:
    double mDecimatedSampleRate;
    double mOversampleRate;
    int    mRatio;              // oversample ratio
    DECIMATOR_SAMPLE *mKernel;
    size_t mKernelSize;
    DECIMATOR_SAMPLE *mShift;              // shift register
    size_t mCursor;

};

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

Decimator::Decimator()
: mKernel(NULL),
  mShift(NULL)
{}

Decimator::~Decimator()
{
    delete [] mKernel;
    delete [] mShift;
}

void Decimator::initialize(double   decimatedSampleRate,
                           double   passFrequency,
                           unsigned oversampleRatio)
{
    mDecimatedSampleRate = decimatedSampleRate;
    mRatio = oversampleRatio;
    mOversampleRate = decimatedSampleRate * oversampleRatio;
    double NyquistFreq = decimatedSampleRate / 2;
    assert(passFrequency < NyquistFreq);

    // See DSP Guide.
    double Fc = (NyquistFreq + passFrequency) / 2 / mOversampleRate;
    double BW = (NyquistFreq - passFrequency) / mOversampleRate;
    int M = ceil(4 / BW);
    if (M % 2) M++;
    size_t activeKernelSize = M + 1;
    size_t inactiveSize = mRatio - activeKernelSize % mRatio;
    mKernelSize = activeKernelSize + inactiveSize;

    // DSP Guide uses approx. values.  Got these from Wikipedia.
    double a0 = 7938. / 18608., a1 = 9240. / 18608., a2 = 1430. / 18608.;

    // Allocate and initialize the FIR filter kernel.
    delete [] mKernel;
    mKernel = new DECIMATOR_SAMPLE[mKernelSize];
    double gain = 0;
    for (size_t i = 0; i < inactiveSize; i++)
        mKernel[i] = 0;
    for (int i = 0; i < activeKernelSize; i++) {
        double y;
        if (i == M/2)
            y = 2 * M_PI * Fc;
        else
            y = (sin(2 * M_PI * Fc * (i - M / 2)) / (i - M / 2) *
                 (a0 - a1 * cos(2 * M_PI * i/ M) + a2 * cos(4 * M_PI / M)));
        gain += y;
        mKernel[inactiveSize + i] = y;
    }

    // Adjust the kernel for unity gain.
    DECIMATOR_SAMPLE inv_gain = 1 / gain;
    for (size_t i = inactiveSize; i < mKernelSize; i++)
        mKernel[i] *= inv_gain;

    // Allocate and clear the shift register.
    delete [] mShift;
    mShift = new DECIMATOR_SAMPLE[mKernelSize];
    for (size_t i = 0; i < mKernelSize; i++)
        mShift[i] = 0;
    mCursor = 0;
}

// The filter kernel is linear.  Coefficients for oldest samples
// are on the left; newest on the right.
//
// The shift register is circular.  Oldest samples are at cursor;
// newest are just left of cursor.
//
// We have to do the multiply-accumulate in two pieces.
//
//  Kernel
//  +------------+----------------+
//  | 0 .. n-c-1 |   n-c .. n-1   |
//  +------------+----------------+
//   ^            ^                ^
//   0            n-c              n
//
//  Shift Register
//  +----------------+------------+
//  |   n-c .. n-1   | 0 .. n-c-1 |
//  +----------------+------------+
//   ^                ^            ^
//   mShift           shiftp       n

void Decimator::decimate(DECIMATOR_SAMPLE *in, DECIMATOR_SAMPLE *out, size_t outCount)
{
    assert(!(mCursor % mRatio));
    assert(mCursor < mKernelSize);
    size_t cursor = mCursor;
    DECIMATOR_SAMPLE *inp = in;
    DECIMATOR_SAMPLE *shiftp = mShift + cursor;
    for (size_t i = 0; i < outCount; i++) {

        // Insert mRatio input samples at cursor.
        for (size_t j = 0; j < mRatio; j++)
            *shiftp++ = *inp++;
        if ((cursor += mRatio) == mKernelSize) {
            cursor = 0;
            shiftp = mShift;
        }

        // Calculate one output sample.
        double acc = 0;
        size_t size0 = mKernelSize - cursor;
        size_t size1 = cursor;
        const DECIMATOR_SAMPLE *kernel1 = mKernel + size0;
        for (size_t j = 0; j < size0; j++)
            acc += shiftp[j] * mKernel[j];
        for (size_t j = 0; j < size1; j++)
            acc += mShift[j] * kernel1[j];
        out[i] = acc;
    }
    mCursor = cursor;
}

#endif // DECIMATOR_H
