//
//  SpectroEditFftObj2.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__SpectroEditFftObj2__
#define __BL_Ghost__SpectroEditFftObj2__

#include <vector>
using namespace std;

#include "FftProcessObj16.h"

class SpectroEditFftObj2 : public ProcessObj
{
public:
    enum Mode
    {
        BYPASS,
        PLAY,
        PLAY_RENDER,
        GEN_DATA,
        REPLACE_DATA
    };
    
    SpectroEditFftObj2(int bufferSize, int oversampling, int freqRes,
                       BL_FLOAT sampleRate);
    
    virtual ~SpectroEditFftObj2();
    
    // Set a reference to external samples
    void SetSamples(WDL_TypedBuf<BL_FLOAT> *samples);
    
    // In EDIT mode, replace the input samples buffer, provide by the app,
    // by the internal samples buffer, pointed at the mLineCount index
    void PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                 const WDL_TypedBuf<BL_FLOAT> *scBuffer) override;
    
    void
    ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL) override;
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    void SetMode(Mode mode);
    
    Mode GetMode();

    void SetDataSelection(BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1);
    
    void SetSelectionEnabled(bool flag);
    
    bool IsSelectionEnabled();

    void GetNormSelection(BL_FLOAT selection[4]);
    
    void SetNormSelection(const BL_FLOAT selection[4]);
    
    
    void RewindToStartSelection();

    void RewindToNormValue(BL_FLOAT value);
    
    bool SelectionPlayFinished();
    
    BL_FLOAT GetPlayPosition();
    
    // Get normalized pos of play selection,
    // normalization is done inside selection
    BL_FLOAT GetSelPlayPosition();
    
    // BUGGY ?
    // Get normalized pos of play selection,
    // normalization is done on visible data only
    BL_FLOAT GetViewPlayPosition(BL_FLOAT startDataPos, BL_FLOAT endDataPos);
    
    // Can be use to save and restore states
    /*long*/ BL_FLOAT GetLineCount();
    void SetLineCount(BL_FLOAT/*long*/ lineCount);
    
    void GetGeneratedData(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                          vector<WDL_TypedBuf<BL_FLOAT> > *phases);
    void ClearGeneratedData();
    
    void SetReplaceData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                        const vector<WDL_TypedBuf<BL_FLOAT> > &phases);
    
    void ClearReplaceData();

    
protected:
    void GetData(const WDL_TypedBuf<BL_FLOAT> &currentData,
                 WDL_TypedBuf<BL_FLOAT> *data);
    
    BL_FLOAT GetNumLines();
    
    long LineCountToSampleId(BL_FLOAT/*long*/ lineCount);
    
    // Swap the selection if necessary
    void SwapSelection(BL_FLOAT selection[4]);


    // Line count is the number of lines of spectrogram
    // At each call, we process bs/ov samples, and we get
    // 1 new line of spectrogram
    
    //long mLineCount;
    BL_FLOAT mLineCount;
    
    WDL_TypedBuf<BL_FLOAT> *mSamples;
    
    bool mSelectionEnabled;
    BL_FLOAT mDataSelection[4];
    
    bool mSelectionPlayFinished;
    
    Mode mMode;
    
    // There are 4 magns and phases for one call to Process() if oversampling is 4
    
    // For GEN_DATA
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentMagns;
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentPhases;
    
    // For REPLACE_DATA
    //vector<WDL_TypedBuf<BL_FLOAT> > mCurrentReplaceMagns;
    //vector<WDL_TypedBuf<BL_FLOAT> > mCurrentReplacePhases;
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentReplaceMagns;
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentReplacePhases;
    
private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf5;
};

#endif /* defined(__BL_Ghost__SpectroEditFftObj2__) */
