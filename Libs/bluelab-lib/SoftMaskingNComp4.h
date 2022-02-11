//
//  SoftMaskingNComp4.h
//  BL-DUET
//
//  Created by applematuer on 5/8/20.
//
//

#ifndef __BL_DUET__SoftMaskingNComp4__
#define __BL_DUET__SoftMaskingNComp4__

#include <bl_queue.h>

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

// Wiener soft masking
//
// See: https://github.com/TUIlmenauAMS/ASP/blob/master/MaskingMethods.py
// and: http://www.jonathanleroux.org/pdf/Erdogan2015ICASSP04.pdf
// and: https://www.researchgate.net/publication/220736985_Degenerate_Unmixing_Estimation_Technique_using_the_Constant_Q_Transform
// and: https://hal.inria.fr/inria-00544949/document
// and: https://hal.inria.fr/hal-01881425/document

// From SoftMasking, but with complex numbers. Returns a complex mask.
//

// SoftMaskingComp2: from SoftMaskingComp
// - code clean
//
// SoftMaskingComp3: from SoftMaskingComp2
// - try to remove the gating effect
//
// SoftMaskingComp3:
// - simpler class
// - avoid may preparation computations outside the class
// - optimizes
//
// SoftMaskingCompN4: from SoftMaskingComp4
// (and not from SoftMaskingNComp)
// NOTE: works well!
class SoftMaskingNComp4
{
public:
    // autoGenerateRestMask: set to true to generate a "rest" mask
    // rest = 1.0 - sigma(masks)
    // Useful if the sum of the masks is not 1
    SoftMaskingNComp4(int bufferSize, int overlapping,
                      int historySize, int numMasks,
                      bool autoGenerateRestMask);
    
    virtual ~SoftMaskingNComp4();

    void Reset();
    
    void Reset(int bufferSize, int overlapping);
    
    int GetHistorySize();
    
    void SetProcessingEnabled(bool flag);
    bool IsProcessingEnabled();

    int GetLatency();
    
    // Returns the centered data value in ioSum
    // Returns the centered masked data in ioMaskedResult
    void ProcessCentered(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioSum,
                         const vector<WDL_TypedBuf<BL_FLOAT> > &masks,
                         vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *ioMaskedResult);
               
protected:
    void ComputeSigma2(int maskNum, WDL_TypedBuf<WDL_FFT_COMPLEX> *outSigma2);
                       
    //
    class HistoryLine
    {
    public:
        HistoryLine();
        HistoryLine(const HistoryLine &other);

        virtual ~HistoryLine();

        void Resize(int size, int numMasks);
        int GetSize();
        int GetNumMasks();
        
    public:
        WDL_TypedBuf<WDL_FFT_COMPLEX> mSum;
        
        vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > mMaskedSquare;

    protected:
        int mSize;

        int mNumMasks;
    };

    //
    int mBufferSize;
    int mOverlapping;
    
    int mHistorySize;
    int mNumMasks;
    
    bl_queue<HistoryLine> mHistory;
    
    //
    WDL_TypedBuf<BL_FLOAT> mWindow;
    
    bool mProcessingEnabled;

    bool mAutoGenerateRestMask;
    
private:
    HistoryLine mTmpHistoryLine;
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > mTmpBuf0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf1;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf2;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf3;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf4;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > mTmpBuf8;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf9;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf10;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf11;
};

#endif /* defined(__BL_DUET__SoftMaskingNComp4__) */
