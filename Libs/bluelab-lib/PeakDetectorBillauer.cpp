#include <BLUtilsMath.h>
#include <BLDebug.h>

#include "PeakDetectorBillauer.h"

// NOTE: 0.5 is better than 0.2
//
// If too low, some harmonic parts will go to the noise envelope
//
// TODO: make a better strategy, to avoid partial barbs while keeping
// better left and right indices
#define PEAKS_WIDTH_RATIO 0.5 //0.2

// Keep 20 peaks and suppress only if more than 20 peaks
#define SUPPRESS_MIN_NUM_PEAKS 20.0

PeakDetectorBillauer::PeakDetectorBillauer(BL_FLOAT maxDelta)
{
    //mDelta = 0.25;
        
    mMaxDelta = maxDelta;
    
    mDelta = 0.25*mMaxDelta;

    mThreshold2 = 1.0;
}

PeakDetectorBillauer::~PeakDetectorBillauer() {}

void
PeakDetectorBillauer::SetThreshold(BL_FLOAT threshold)
{
    //mDelta = threshold;
    
    mDelta = threshold*mMaxDelta;
}

void
PeakDetectorBillauer::SetThreshold2(BL_FLOAT threshold2)
{
    mThreshold2 = threshold2;
}

void
PeakDetectorBillauer::
DetectPeaks(const WDL_TypedBuf<BL_FLOAT> &data, vector<Peak> *peaks,
            int minIndex, int maxIndex)
{
    // Arguments
    if (minIndex < 0)
        minIndex = 0;
    if (maxIndex < 0)
        maxIndex = data.GetSize() - 1;
    
    // First, fill the min and max arrays
    //
    vector<int> mintab;
    vector<int> maxtab;

    BL_FLOAT mn = BL_INF;
    BL_FLOAT mx = -BL_INF;
    int mnpos = -1;
    int mxpos = -1;

    // Look for max first
    bool lookformax = true;

    // Check if we start by a peak
    bool startedbypeak = false;
    if (maxIndex - minIndex >= 2)
    {
        BL_FLOAT val0 = data.Get()[minIndex];
        BL_FLOAT val1 = data.Get()[minIndex + 1];

        if (val0 > val1)
            // We are starting on a descending slope
        {
            // Start by looking for min
            maxtab.push_back(minIndex);
            mx = val0;
            mxpos = minIndex;
            lookformax = false;

            startedbypeak = true;
        }
    }
    
    for (int i = minIndex; i <= maxIndex; i++)
    {
        BL_FLOAT t = data.Get()[i];
        
        if (t > mx)
        {
            mx = t;
            mxpos = i;
        }
        
        if (t < mn)
        {
            mn = t;
            mnpos = i;
        }

        if (lookformax)
        {
            if (t < mx - mDelta)
            {
                maxtab.push_back(mxpos);
                mn = t;
                mnpos = i;
                lookformax = false;

                // See: https://github.com/xuphys/peakdetect/blob/master/peakdetect.c
                //i = mxpos - 1;
            }
        }
        else
        {
            if (t > mn + mDelta)
            {
                mintab.push_back(mnpos);
                mx = t;
                mxpos = i;
                lookformax = true;

                // See: https://github.com/xuphys/peakdetect/blob/master/peakdetect.c
                //i = mnpos - 1;
            }
        }
    }

    // We started by a peak
    bool keepfirstpeak = true;
    if (startedbypeak)
    {
        // Check if we must keep the first peak
        if ((maxtab.size() > 0) && (mintab.size() > 0))
        {
            if (!(data.Get()[maxtab[0]] >= data.Get()[mintab[0]] + mDelta))
                keepfirstpeak = false;
        }
    }
    
    // Secondly, fill the peaks
    //
    //peaks->resize(maxtab.size());
    peaks->clear();
    for (int i = 0; i < maxtab.size(); i++)
    {
        if ((i == 0) && !keepfirstpeak)
            continue;
        
        //Peak &p = (*peaks)[i];
        Peak p;
        p.mPeakIndex = maxtab[i];
        p.mLeftIndex = (i < mintab.size()) ? mintab[i] : minIndex;
        p.mRightIndex = (i + 1 < mintab.size()) ? mintab[i + 1] : maxIndex;

        peaks->push_back(p);
    }

    // Debug
#if 0
    if (peaks->size() > 1)
        DBG_DumpPeaks(data, *peaks);
#endif
    
#if 0
    bool peakOk = DBG_TestPeaks(data, *peaks);
    if (!peakOk)
        DBG_DumpPeaks(data, *peaks);
#endif

    // NOTE: need SuppressSmallPeaks() before AdjustPeaksWidth()
    // strangely... (it should be the contrary)
    
    
    // Post process
    SuppressSmallPeaks(data, peaks);

    AdjustPeaksWidth(data, peaks);
}

// Simple method, with hard coded value
void
PeakDetectorBillauer::SuppressSmallPeaksSimple(const WDL_TypedBuf<BL_FLOAT> &data,
                                               vector<Peak> *peaks)
{
    // Peaks must be +2dB over the noise floor
    // => this will avoid having many many flat partials at high frequencies
#define MIN_PARTIAL_HEIGHT_OVER_NF 0.0 //0.5 //2.0
    
    vector<Peak> newPeaks;
    for (int i = 0; i < peaks->size(); i++)
    {
        const Peak &p = (*peaks)[i];
        
        BL_FLOAT lm = data.Get()[p.mLeftIndex];
        BL_FLOAT rm = data.Get()[p.mRightIndex];

        BL_FLOAT base = (lm < rm) ? lm : rm;

        BL_FLOAT height = data.Get()[p.mPeakIndex] - base;

        if (height >= MIN_PARTIAL_HEIGHT_OVER_NF) // Hard coded
            newPeaks.push_back(p);
    }
    
    *peaks = newPeaks;
}

void
PeakDetectorBillauer::SuppressSmallPeaks(const WDL_TypedBuf<BL_FLOAT> &data,
                                         vector<Peak> *peaks)
{
    if (mThreshold2 >= 1.0)
        // Take all peaks
        return;

    if (peaks->size() < SUPPRESS_MIN_NUM_PEAKS)
        return;
    
    // Compute peaks prominence
    for (int i = 0; i < peaks->size(); i++)
    {
        Peak &p = (*peaks)[i];
        ComputePeakProminence(data, &p);
    }

    // Sort peaks by prominence
    sort(peaks->begin(), peaks->end(), Peak::ProminenceLess);

    // Order biggest peaks first
    reverse(peaks->begin(), peaks->end());

    // Keep only the biggest peaks
    int numToTakePeaks = peaks->size()*mThreshold2;

    if ((numToTakePeaks < SUPPRESS_MIN_NUM_PEAKS) &&
        (peaks->size() > SUPPRESS_MIN_NUM_PEAKS))
        numToTakePeaks = SUPPRESS_MIN_NUM_PEAKS;
    
    peaks->resize(numToTakePeaks);
}

// With Billauer, and only one peak, left and right peak indices will
// be near to the minimum and maximum
//
// => So recompute the peak bounds, so they really match the peak,
// and they not cover the whole range of freqeuncies
void
PeakDetectorBillauer::AdjustPeaksWidth(const WDL_TypedBuf<BL_FLOAT> &data,
                                       vector<Peak> *peaks)
{
#if 0 // DEBUG
    BLDebug::DumpData("data.txt", data);
    
    for (int i = 0; i < peaks.size(); i++)
    {
        int width = peaks[i].mRightIndex - peaks[i].mLeftIndex;
        fprintf(stderr, "peak[%d] width: %d\n", i, width);
    }
#endif
    
    for (int i = 0; i < peaks->size(); i++)
    {
        Peak &peak = (*peaks)[i];

        BL_FLOAT peakAmp = data.Get()[peak.mPeakIndex];
        BL_FLOAT thrs = peakAmp*PEAKS_WIDTH_RATIO;

        // Adjust left index
        for (int j = peak.mPeakIndex - 1; j >= 0; j--)
        {
            BL_FLOAT a = data.Get()[j];
            if (a < thrs)
            {
                peak.mLeftIndex = j;
                
                break;
            }
        }

        // Adjust right index
        for (int j = peak.mPeakIndex + 1; j < data.GetSize(); j++)
        {
            BL_FLOAT a = data.Get()[j];
            if (a < thrs)
            {
                peak.mRightIndex = j;
                
                break;
            }
        }
    }
}

// Prominence is computed from the highest border
//
// See: https://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.peak_prominences.html#scipy.signal.peak_prominences
void
PeakDetectorBillauer::ComputePeakProminence(const WDL_TypedBuf<BL_FLOAT> &data,
                                            Peak *p)
{
    BL_FLOAT lm = data.Get()[p->mLeftIndex];
    BL_FLOAT rm = data.Get()[p->mRightIndex];

    BL_FLOAT base = (lm > rm) ? lm : rm;

    p->mProminence = data.Get()[p->mPeakIndex] - base;
}

bool
PeakDetectorBillauer::DBG_TestPeaks(const WDL_TypedBuf<BL_FLOAT> &data,
                                    const vector<Peak> &peaks)
{
    for (int i = 0; i < peaks.size(); i++)
    {
        const Peak &peak = peaks[i];

        BL_FLOAT peakAmp = data.Get()[peak.mPeakIndex];

        BL_FLOAT peakAmp0 = peakAmp;
        if (i - 1 >= 0)
            peakAmp0 = data.Get()[peak.mPeakIndex - 1];

        BL_FLOAT peakAmp1 = peakAmp;
        if (i + 1 < peaks.size())
            peakAmp1 = data.Get()[peak.mPeakIndex + 1];

        if ((peakAmp0 > peakAmp) || (peakAmp1 > peakAmp))
            return false;
    }

    return true;
}

void
PeakDetectorBillauer::DBG_DumpPeaks(const WDL_TypedBuf<BL_FLOAT> &data,
                                    const vector<Peak> &peaks)
{
    BLDebug::DumpData("peaks-data.txt", data);
    
    for (int i = 0; i < peaks.size(); i++)
    {
        const Peak &p = peaks[i];
        fprintf(stderr, "indices: [%d %d %d]\n",
                p.mLeftIndex, p.mPeakIndex, p.mRightIndex);
    }   
}
