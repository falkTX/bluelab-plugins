//
//  SpectroEditFftObj3.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__SpectroEditFftObj3__
#define __BL_Ghost__SpectroEditFftObj3__

#include <vector>
using namespace std;

#include <FftProcessObj16.h>

// From SpectroEditFftobj2
//
// Use mSamplesPos instead of mLineCount
// (count the number of samples instead of spectrogram lines)
class SpectroEditFftObj3 : public ProcessObj
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
    
    SpectroEditFftObj3(int bufferSize, int oversampling,
                       int freqRes, BL_FLOAT sampleRate);
    
    virtual ~SpectroEditFftObj3();
    
    // Set a reference to external samples
    void SetSamples(WDL_TypedBuf<BL_FLOAT> *samples);

    void SetForceMono(bool flag);
    void SetSamplesForMono(vector<WDL_TypedBuf<BL_FLOAT> > *samples);
    
    // In EDIT mode, replace the input samples buffer, provide by the app,
    // by the internal samples buffer, pointed at the mSamplesPos index
    void PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                 const WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    void SetStep(BL_FLOAT step);
    
    void SetMode(Mode mode);
    Mode GetMode();

    // Set data selection area, in samples
    void SetDataSelection(BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1);
    
    void SetSelectionEnabled(bool flag);
    bool IsSelectionEnabled();

    void RewindToStartSelection();
    
    // Normalized selection
    // x = 1 gives the last sample
    void GetNormSelection(BL_FLOAT selection[4]);
    void SetNormSelection(const BL_FLOAT selection[4]);

    void RewindToNormValue(BL_FLOAT value);
    
    bool SelectionPlayFinished();
    
    BL_FLOAT GetPlayPosition();
    
    // Get normalized pos of play selection,
    // normalization is done inside selection
    BL_FLOAT GetSelPlayPosition();
    
    // Can be use to save and restore states
    BL_FLOAT GetSamplesPos();
    void SetSamplesPos(BL_FLOAT samplePos);
    
    void GetGeneratedData(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                          vector<WDL_TypedBuf<BL_FLOAT> > *phases);
    void ClearGeneratedData();
    
    void SetReplaceData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                        const vector<WDL_TypedBuf<BL_FLOAT> > &phases);
    void ClearReplaceData();

    // Fill some parts with zeros if selection is partially out of bounds
    static void FillFromSelection(WDL_TypedBuf<BL_FLOAT> *result,
                                  const WDL_TypedBuf<BL_FLOAT> &inBuf,
                                  int selStartSamples,
                                  int selSizeSamples);
    
protected:
    void GetData(const WDL_TypedBuf<BL_FLOAT> &currentData,
                 WDL_TypedBuf<BL_FLOAT> *data);

    // At each call, we process bs/ov samples,
    // and increment mSamplesPos by bs/ov
    BL_FLOAT mSamplesPos;
    
    WDL_TypedBuf<BL_FLOAT> *mSamples;
    
    vector<WDL_TypedBuf<BL_FLOAT> > *mSamplesForMono;
    bool mForceMono;
    
    bool mSelectionEnabled;
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

    BL_FLOAT mStep;
    
private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
};

#endif /* defined(__BL_Ghost__SpectroEditFftObj3__) */
