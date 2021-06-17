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

    void PropagatePhasesSimple(const Frame &frame0, Frame *frame1,
                               WDL_TypedBuf<BL_FLOAT> *magns);
    void PropagatePhasesPrusa(const Frame &frame0, Frame *frame1);
    void PropagatePhasesPrusaOptim(const Frame &frame0, Frame *frame1);
    
    struct Tuple
    {
#if 0 // ORIG
        BL_FLOAT mMagn;
        int mBinIdx;
        int mTimeIdx;
#endif

#if 1 // Optim size
        // We use magns only for comparing them
        float mMagn;

        // With this we can go until 32767 which is far enough for bin num
        short mBinIdx;

        // Time index is only 0 for (n-1), or 1 for n
        //short mTimeIdx;
        unsigned char mTimeIdx;

        unsigned char mIsValid;
#endif
        
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
    int ContainsSorted(/*const*/ vector<Tuple> &hp, int binIdx, int timeIdx); // Optim

    void Remove(vector<Tuple> *hp, int binIdx, int timeIdx);
    void RemoveIdx(vector<Tuple> *hp, int idx);

    void AddHeap(vector<Tuple> *hp, int binIdx, int timeIdx, BL_FLOAT magn);
    
    void DBG_Dump(const char *fileName, const vector<Tuple> &hp);

    // Optimized for Tuple and fixed size
    // Vector must be sorted at the beginning
    class Heap
    {
    public:
        Heap(vector<Tuple> *vector/*, int maxNumBins*/)
        {
            mVector = vector;
            
            mFirstValidIndex = 0;

            // Find first valid index
            for (int i = 0; i < mVector->size(); i++)
            {
                if ((*mVector)[i].mIsValid == 1)
                    break;
                
                mFirstValidIndex++;
            }
            
            //mBinToVecIdx.resize(maxNumBins);
            //for (int i = 0; i < mBinToVecIdx.size(); i++)
            //    mBinToVecIdx[i] = -1;

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
                t.mIsValid = 0;
            }

            mFirstValidIndex = mVector->size();
        }

        void Insert(int binIdx, int timeIndex, BL_FLOAT magn)
        {
            int vecIdx = mBinToVecIdx[binIdx];

            (*mVector)[vecIdx].mIsValid = 1;
            (*mVector)[vecIdx].mTimeIdx = timeIndex;
            (*mVector)[vecIdx].mMagn = magn;
            
            if (vecIdx < mFirstValidIndex)
                mFirstValidIndex = vecIdx;
        }
        
        const Tuple &GetMax()
        {
            Tuple &t = (*mVector)[mFirstValidIndex];
            
            return t;
        }

        // Return the index of the removed element
        //void/*int*/ RemoveMax()
        //{
        //    int idx = mFirstValidIndex;
        //
        //    RemoveIndex(idx);
        //
        //return idx;
        //}

        //void RemoveIndex(int idx)
        void RemoveMax()
        {
            if (mFirstValidIndex >= mVector->size())
                return;
            
            Tuple &t0 = (*mVector)[mFirstValidIndex];
            t0.mIsValid = 0;

            //if (idx < mFirstValidIndex)
            //    mFirstValidIndex = idx;
            
            while(mFirstValidIndex < mVector->size())
            {
                mFirstValidIndex++;

                if (mFirstValidIndex >= mVector->size())
                    break;
                
                Tuple &t1 = (*mVector)[mFirstValidIndex];
                if (t1.mIsValid == 1)
                    break;
            }
        }

        /*static bool AllEmpty(Heap *heap0, Heap *heap1)
          {
          if (heap0->IsEmpty() && heap1->IsEmpty())
          return true;
          
          return false;
          }*/
        
        static const Tuple &Pop(Heap *heap0, Heap *heap1)
        {
            // Both empty
            //if (heap0->IsEmpty() && heap1->IsEmpty())
            //    return;
            
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
                /*int idx = */heap0->RemoveMax();

                //if (t0.mTimeIdx == t1.mTimeIdx)
                //    // Same
                //    heap1->RemoveIndex(idx);
                
                return t0;
            }
            else
            {
                /*int idx = */heap1->RemoveMax();

                //if (t0.mTimeIdx == t1.mTimeIdx)
                //    // Same
                //    heap0->RemoveIndex(idx);
                
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
        SortedVec(vector<Tuple> *vector/*, int maxNumBins*/)
        {
            mVector = vector;

            mBinToVecIdx.resize(vector->size());
            //mBinToVecIdx.resize(maxNumBins);
            //for (int i = 0; i < mBinToVecIdx.size(); i++)
            //    mBinToVecIdx[i] = -1;
            
            for (int i = 0; i < vector->size(); i++)
                mBinToVecIdx[(*vector)[i].mBinIdx] = i;

            // Compute num valid values
            //mNumValidValues = mVector->size();
            mNumValidValues = 0;
            for (int i = 0; i < mVector->size(); i++)
            {
                if ((*mVector)[i].mIsValid == 1)
                    mNumValidValues++;
            }
        }

        virtual ~SortedVec() {}

        int Contains(int binIdx/*, int timeIdx*/)
        {
            if ((binIdx < 0) || (binIdx >= mBinToVecIdx.size()))
                return -1;
            
            int vecIdx = mBinToVecIdx[binIdx];
            //if (vecIdx < 0) //
            //    return -1;
            
            if ((*mVector)[vecIdx].mIsValid == 1)
                return vecIdx;
            
            return -1;
        }

        void Remove(int idx)
        {
            if ((idx < 0) || (idx >= mVector->size()))
                return;
            
            if ((*mVector)[idx].mIsValid == 0)
                return;
            
            (*mVector)[idx].mIsValid = 0;

            mNumValidValues--;
        }

        bool IsEmpty()
        {
#if 0
            // TODO: optimize this
            for (int i = 0; i < mVector->size(); i++)
            {
                if ((*mVector)[i].mIsValid == 1)
                    return false;
            }
            
            return true;
#endif

            bool empty = (mNumValidValues <= 0);
            
            return empty;
        }
        
    protected:
        vector<Tuple> *mVector;

        vector<int> mBinToVecIdx;

        int mNumValidValues;
    };
    
private:
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
};

#endif /* defined(__BL_PitchShift__PitchShiftPrusaFftObj__) */
