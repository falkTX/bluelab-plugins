//
//  PitchShiftFftObj3.h
//  BL-PitchShift
//
//  Created by Pan on 16/04/18.
//
//

#ifndef __BL_PitchShift__PitchShiftPrusaFftObj__
#define __BL_PitchShift__PitchShiftPrusaFftObj__

#include <FftProcessObj16.h>

class PitchShift;
class PitchShiftPrusaFftObj : public ProcessObj
{
public:
    PitchShiftPrusaFftObj(int bufferSize, int oversampling, int freqRes,
                          BL_FLOAT sampleRate);
    
    virtual ~PitchShiftPrusaFftObj();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;

    void ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer) override;
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    void Reset();
    
    void SetFactor(BL_FLOAT factor);
    
protected:
    void Convert(WDL_TypedBuf<BL_FLOAT> *magns,
                 WDL_TypedBuf<BL_FLOAT> *phases,
                 BL_FLOAT factor);
    
    void ResetPitchShift();
        
    //
    BL_FLOAT mFactor;

    //
    struct Frame
    {
        WDL_TypedBuf<BL_FLOAT> mMagns;
        WDL_TypedBuf<BL_FLOAT> mPhases;
        
        WDL_TypedBuf<BL_FLOAT> mDTPhases;
        WDL_TypedBuf<BL_FLOAT> mDFPhases;
        
        WDL_TypedBuf<BL_FLOAT> mEstimPhases;
    };
    Frame mPrevFrame;
    
    struct Tuple
    {
        BL_FLOAT mMagn;
        int mBinIdx;
        int mTimeIdx;

        // Optimization idea...
        /*float mMagn;
          short mBinIdx;
          short mTimeIdx;*/
        
        bool operator< (const Tuple &t0)
        { return (this->mMagn < t0.mMagn); } // Was tested successfully!

        // For sorting and sorted search
        static bool IndexSmaller(const Tuple &t0, const Tuple &t1)
        {
            if (t0.mTimeIdx < t1.mTimeIdx)
                return true;

            if (t0.mBinIdx < t1.mBinIdx)
                return true;

            return false;
        }
        
        static bool IndexEqual(const Tuple &t0, const Tuple &t1)
        {
            if (t0.mTimeIdx != t1.mTimeIdx)
                return false;
            
            if (t0.mBinIdx != t1.mBinIdx)
                return false;
            
            return true;
        }
    };

    bool Contains(const vector<Tuple> &hp, int binIdx, int timeIdx);
    int ContainsSorted(/*const*/ vector<Tuple> &hp, int binIdx, int timeIdx); // Optim

    void Remove(vector<Tuple> *hp, int binIdx, int timeIdx);
    void RemoveIdx(vector<Tuple> *hp, int idx);

    void AddHeap(vector<Tuple> *hp, int binIdx, int timeIdx, BL_FLOAT magn);
    
    void DBG_Dump(const char *fileName, const vector<Tuple> &hp);
    
private:
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
};

#endif /* defined(__BL_PitchShift__PitchShiftPrusaFftObj__) */
