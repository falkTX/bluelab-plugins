//
//  SpectroEditFftObj2EXPE.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__SpectroEditFftObj2EXPE__
#define __BL_Ghost__SpectroEditFftObj2EXPE__

#include <vector>
using namespace std;

#include "FftProcessObj16.h"

class SpectroEditFftObj2EXPE : public ProcessObj
{
public:
    enum Mode
    {
        BYPASS,
        PLAY,
        GEN_DATA,
        REPLACE_DATA
    };
    
    SpectroEditFftObj2EXPE(int bufferSize, int oversampling, int freqRes,
                       BL_FLOAT sampleRate);
    
    virtual ~SpectroEditFftObj2EXPE();
    
    // Set a reference to external samples
    void SetSamples(WDL_TypedBuf<BL_FLOAT> *samples);
    
    // In EDIT mode, replace the input samples buffer, provide by the app,
    // by the internal samples buffer, pointed at the mLineCount index
    void PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                 const WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int oversampling, int freqRes, BL_FLOAT sampleRate);
    
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
    long GetLineCount();
    void SetLineCount(long lineCount);
    
    void GetGeneratedData(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                          vector<WDL_TypedBuf<BL_FLOAT> > *phases);
    void ClearGeneratedData();
    
    void SetReplaceData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                        const vector<WDL_TypedBuf<BL_FLOAT> > &phases);
    
    void ClearReplaceData();

    // EXPE
    void SetSmoothFactor(BL_FLOAT factor);
    void SetFreqAmpRatio(BL_FLOAT ratio);
    void SetTransThreshold(BL_FLOAT thrs);
    
protected:
    void GetData(const WDL_TypedBuf<BL_FLOAT> &currentData,
                 WDL_TypedBuf<BL_FLOAT> *data);
    
    BL_FLOAT GetNumLines();
    
    long LineCountToSampleId(long lineCount);
    
    
    long mLineCount;
    
    WDL_TypedBuf<BL_FLOAT> *mSamples;
    
    BL_FLOAT mSelectionEnabled;
    BL_FLOAT mDataSelection[4];
    
    bool mSelectionPlayFinished;
    
    Mode mMode;
    
    // There are 4 magns and phases for one call to Process() if oversampling is 4
    
    // For GEN_DATA
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentMagns;
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentPhases;
    
    // For REPLACE_DATA
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentReplaceMagns;
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentReplacePhases;
    
    // EXPE
    WDL_TypedBuf<BL_FLOAT> mPrevPhases;
    
    BL_FLOAT mSmoothFactor;
    BL_FLOAT mFreqAmpRatio;
    
    BL_FLOAT mTransThreshold;
};

#endif /* defined(__BL_Ghost__SpectroEditFftObj2EXPE__) */
