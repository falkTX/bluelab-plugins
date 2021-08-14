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

PeakDetectorBillauer::PeakDetectorBillauer()
{
    mDelta = 0.25;
}

PeakDetectorBillauer::~PeakDetectorBillauer() {}

void
PeakDetectorBillauer::SetThreshold(BL_FLOAT threshold)
{
    mDelta = threshold;
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

    // Secondly, fill the peaks
    //
    peaks->resize(maxtab.size());
    for (int i = 0; i < maxtab.size(); i++)
    {
        Peak &p = (*peaks)[i];
        p.mPeakIndex = maxtab[i];
        p.mLeftIndex = (i < mintab.size()) ? mintab[i] : minIndex;
        p.mRightIndex = (i + 1 < mintab.size()) ? mintab[i + 1] : maxIndex;
    }

#if 0
    bool peakOk = DBG_TestPeaks(data, *peaks);
    if (!peakOk)
        DBG_DumpPeaks(data, *peaks);
#endif
    
    // Post process
    AdjustPeaksWidth(data, peaks);
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
