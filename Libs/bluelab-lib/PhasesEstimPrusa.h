//
//  PhasesEstimPrusa.h
//  BL-PitchShift
//
//  Created by Pan on 16/04/18.
//
//

#ifndef __BL_PitchShift__PhasesEstimPrusa__
#define __BL_PitchShift__PhasesEstimPrusa__

#include <FftProcessObj16.h>

class PhasesEstimPrusa
{
public:
    PhasesEstimPrusa(int bufferSize, int oversampling, int freqRes,
                     BL_FLOAT sampleRate);
    
    virtual ~PhasesEstimPrusa();

    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate);
    
    void Reset();

    void SetPitchFactor(BL_FLOAT factor);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &magns,
                 WDL_TypedBuf<BL_FLOAT> *phases);
    
protected:
    void Convert(WDL_TypedBuf<BL_FLOAT> *magns,
                 WDL_TypedBuf<BL_FLOAT> *phases,
                 BL_FLOAT factor);
    
    void ResetPitchShift();
        
    //
    int mBufferSize;
    int mOverlapping;
    int mFreqRes;
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mPitchFactor;

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

    // Was: PropagatePhasesPrusaOptim
    void PropagatePhasesPrusa(const Frame &frame0, Frame *frame1);
    
    struct Tuple
    {
        // We use magns only for comparing them
        float mMagn;

        // With this we can go until 32767 which is far enough for bin num
        short mBinIdx;

        // Time index is only 0 for (n-1), or 1 for n
        //short mTimeIdx;
        unsigned char mTimeIdx;

        unsigned char mIsValid;
        
        bool operator< (const Tuple &t0)
        { return (this->mMagn < t0.mMagn); } // Was tested successfully!
        
        // For optimization
        static bool MagnGreater(const Tuple &t0, const Tuple &t1)
        { return (t0.mMagn > t1.mMagn); }
        
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
    void Remove(vector<Tuple> *hp, int binIdx, int timeIdx);

    // Optimized for Tuple and fixed size
    // Vector must be sorted at the beginning
    class Heap
    {
    public:
        Heap(vector<Tuple> *vector)
        {
            mVector = vector;
            
            mFirstValidIndex = 0;

            // Find first valid index
            for (int i = 0; i < mVector->size(); i++)
            {
                if ((*mVector)[i].mIsValid == (unsigned char)1)
                    break;
                
                mFirstValidIndex++;
            }

            mBinToVecIdx.resize(mVector->size());
            for (int i = 0; i < mVector->size(); i++)
                mBinToVecIdx[(*mVector)[i].mBinIdx] = i;
        }

        virtual ~Heap() {}
        
        bool IsEmpty()
        {
            bool empty = (mFirstValidIndex >= mVector->size());
            
            return empty;
        }
        
        void Clear()
        {
            for (int i = 0; i < mVector->size(); i++)
            {
                Tuple &t = (*mVector)[i];
                t.mIsValid = (unsigned char)0;
            }

            mFirstValidIndex = mVector->size();
        }

        void Insert(int binIdx, unsigned char timeIndex) //, BL_FLOAT magn)
        {
            int vecIdx = mBinToVecIdx[binIdx];            

            //(*mVector)[vecIdx].mMagn = magn;
            //assert((*mVector)[vecIdx].mBinIdx == binIdx); // DEBUG
            (*mVector)[vecIdx].mTimeIdx = timeIndex;
            (*mVector)[vecIdx].mIsValid = (unsigned char)1;
            
            if (vecIdx < mFirstValidIndex)
                mFirstValidIndex = vecIdx;
        }
        
        const Tuple &GetMax()
        {
            Tuple &t = (*mVector)[mFirstValidIndex];
            
            return t;
        }

        void RemoveMax()
        {
            if (mFirstValidIndex >= mVector->size())
                return;
            
            Tuple &t0 = (*mVector)[mFirstValidIndex];
            t0.mIsValid = (unsigned char)0;

            vector<Tuple>::iterator it = mVector->begin() + mFirstValidIndex;
            while(it != mVector->end())
            {
                mFirstValidIndex++;

                it++;
                
                if (mFirstValidIndex >= mVector->size())
                    break;
                
                const Tuple &t1 = *it;
                if (t1.mIsValid == (unsigned char)1)
                    break;
            }
        }
        
        static const Tuple &Pop(Heap *heap0, Heap *heap1)
        {
            // First empty ?
            if (heap0->IsEmpty())
            {
                const Tuple &t1 = heap1->GetMax();
                heap1->RemoveMax();
                
                return t1;
            }

            // Second empty ?
            if (heap1->IsEmpty())
            {
                const Tuple &t0 = heap0->GetMax();
                heap0->RemoveMax();
                
                return t0;
            }

            // Both not empty, compare them
            const Tuple &t0 = heap0->GetMax();
            const Tuple &t1 = heap1->GetMax();

            if (t0.mMagn >= t1.mMagn)
            {
                heap0->RemoveMax();
                
                return t0;
            }
            else
            {
                heap1->RemoveMax();
                
                return t1;
            }
        }

    protected:
        vector<Tuple> *mVector;
        int mFirstValidIndex;

        vector<int> mBinToVecIdx;
    };

    class SortedVec
    {
    public:
        SortedVec(vector<Tuple> *vector)
        {
            mVector = vector;

            mBinToVecIdx.resize(vector->size());
            for (int i = 0; i < vector->size(); i++)
                mBinToVecIdx[(*vector)[i].mBinIdx] = i;

            // Compute num valid values
            mNumValidValues = 0;
            for (int i = 0; i < mVector->size(); i++)
            {
                if ((*mVector)[i].mIsValid == (unsigned char)1)
                    mNumValidValues++;
            }
        }

        virtual ~SortedVec() {}

        int Contains(int binIdx)
        {
            if ((binIdx < 0) || (binIdx >= mBinToVecIdx.size()))
                return -1;
            
            int vecIdx = mBinToVecIdx[binIdx];
            
            if ((*mVector)[vecIdx].mIsValid == (unsigned char)1)
                return vecIdx;
            
            return -1;
        }

        void Remove(int idx)
        {
            if ((idx < 0) || (idx >= mVector->size()))
                return;
            
            if ((*mVector)[idx].mIsValid == (unsigned char)0)
                return;
            
            (*mVector)[idx].mIsValid = (unsigned char)0;

            mNumValidValues--;
        }

        bool IsEmpty()
        {
            bool empty = (mNumValidValues <= 0);
            
            return empty;
        }
        
    protected:
        vector<Tuple> *mVector;

        vector<int> mBinToVecIdx;

        int mNumValidValues;
    };
    
private:
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    vector<Tuple> mTmpBuf1;
    vector<Tuple> mTmpBuf2;
    vector<Tuple> mTmpBuf3;
    Frame mTmpBuf4;
};

#endif /* defined(__BL_PitchShift__PhasesEstimPrusa__) */
