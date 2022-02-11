//
//  PanogramPlayFftObj.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__PanogramPlayFftObj__
#define __BL_Ghost__PanogramPlayFftObj__

#include <deque>
using namespace std;

#include "FftProcessObj16.h"

class HistoMaskLine2;

class PanogramPlayFftObj : public ProcessObj
{
public:
    enum Mode
    {
        BYPASS,
        PLAY,
        RECORD
    };
    
    PanogramPlayFftObj(int bufferSize, int oversampling, int freqRes,
                       double sampleRate);
    
    virtual ~PanogramPlayFftObj();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int bufferSize, int oversampling, int freqRes, double sampleRate);
    
    void SetMode(Mode mode);

    // Selection
    void SetNormSelection(double x0, double y0, double x1, double y1);

    void GetNormSelection(double selection[4]);
    
    void SetSelectionEnabled(bool flag); //
    
    // Play
    void RewindToStartSelection();

    void RewindToNormValue(double value);
    
    bool SelectionPlayFinished();
    
    double GetPlayPosition();
    
    // Get normalized pos of play selection,
    // normalization is done inside selection
    double GetSelPlayPosition();

    //
    void SetNumCols(int numCols);
    
    void SetIsPlaying(bool flag);
    void SetHostIsPlaying(bool flag);
    
    //
    void AddMaskLine(const HistoMaskLine2 &maskLine);
    
protected:
    void GetDataLine(const deque<WDL_TypedBuf<double> > &inData,
                     WDL_TypedBuf<double> *data, int lineCount);

    void GetDataLineMask(const deque<WDL_TypedBuf<double> > &inData,
                         WDL_TypedBuf<double> *data, int lineCount);
    
    // Shift a little, to have the sound played eactly when the bar passes
    // on the sound
    bool ShiftLineCount(int *lineCount);
    
    //
    long mLineCount;
    
    bool mSelectionEnabled;
    double mDataSelection[4];
    
    bool mSelectionPlayFinished;
    
    Mode mMode;
    
    // There are 4 magns and phases for one call to Process() if oversampling is 4
    
    deque<WDL_TypedBuf<double> > mCurrentMagns;
    deque<WDL_TypedBuf<double> > mCurrentPhases;
    
    deque<HistoMaskLine2> mMaskLines;
    
    int mNumCols;
    
    // Play button
    bool mIsPlaying;
    
    // Host play
    bool mHostIsPlaying;
};

#endif /* defined(__BL_Ghost__PanogramPlayFftObj__) */
