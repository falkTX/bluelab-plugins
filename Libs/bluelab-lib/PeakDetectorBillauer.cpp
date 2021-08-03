#include <BLUtilsMath.h>

#include "PeakDetectorBillauer.h"

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
}
