//
//  BLSpectrogram4.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

#ifndef __Denoiser__BLSpectrogram4__
#define __Denoiser__BLSpectrogram4__

#include <vector>
#include <deque>
using namespace std;

#include <bl_queue.h>

#include <PPMFile.h>
#include <ColorMapFactory.h>

#include <Scale.h>

#include "IPlug_include_in_plug_hdr.h"

#define OPTIM_SPECTROGRAM2 1

// BLSpectrogram4: from BLSpectrogram3
// - use ColormapFactory
class ColorMap4;
class Scale;
class BLSpectrogram4
{
public:
    BLSpectrogram4(BL_FLOAT sampleRate,
                   int height, int maxCols = -1,
                   bool useGLSL = true);
    
    virtual ~BLSpectrogram4();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Reset(BL_FLOAT sampleRate, int height, int maxCols = -1);
    
    void SetRange(BL_FLOAT range);
    void SetContrast(BL_FLOAT contrast);
    
    void TouchColorMap();
    void TouchData();
    
    void SetValueScale(Scale::Type scale);
    void SetYScale(Scale::Type yScale);
    
    void SetDisplayMagns(bool flag);
    
    void SetDisplayPhasesX(bool flag);
    
    void SetDisplayPhasesY(bool flag);
    
    void SetDisplayDPhases(bool flag);
    
    int GetMaxNumCols();
    int GetNumCols();
    int GetHeight();

    void SetFixedSize(bool flag);
    
    void SetColorMap(ColorMapFactory::ColorMap colorMapId);
    
    // Lines
    void AddLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                 const WDL_TypedBuf<BL_FLOAT> &phases);
    
    bool GetLine(int index,
                 WDL_TypedBuf<BL_FLOAT> *magns,
                 WDL_TypedBuf<BL_FLOAT> *phases);

    // Replace all the data
    void SetLines(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                  const vector<WDL_TypedBuf<BL_FLOAT> > &phases);
                  
    // Image data
    bool GetImageDataFloat(unsigned char *buf);
    
    // Colormap image data
    bool GetColormapImageDataRGBA(WDL_TypedBuf<unsigned int> *colormapImageData);
    
    // Load and save
    static BLSpectrogram4 *Load(BL_FLOAT sampleRate, const char *fileName);
    
    void Save(const char *filename);
    
    static BLSpectrogram4 *LoadPPM(BL_FLOAT sampleRate, const char *filename);
    
    void SavePPM(const char *filename);
    
    static BLSpectrogram4 *LoadPPM16(BL_FLOAT sampleRate, const char *filename);
    
    void SavePPM16(const char *filename);
    
    static BLSpectrogram4 *LoadPPM32(BL_FLOAT sampleRate, const char *filename);
    
    void SavePPM32(const char *filename);
    
    unsigned long long GetTotalLineNum();
    
    BL_FLOAT GetSampleRate();

    BL_FLOAT NormYToFreq(BL_FLOAT normY);
    
protected:
    void UnwrapAllPhases(const bl_queue<WDL_TypedBuf<BL_FLOAT> > &inPhases,
                         vector<WDL_TypedBuf<BL_FLOAT> > *outPhases,
                         bool hozirontal, bool vertical);
    
    // Unwrap in place (slow)
    void UnwrapAllPhases(bl_queue<WDL_TypedBuf<BL_FLOAT> > *ioPhases,
                         bool horizontal, bool vertical);
    
    void PhasesToStdVector(const bl_queue<WDL_TypedBuf<BL_FLOAT> > &inPhases,
                           vector<WDL_TypedBuf<BL_FLOAT> > *outPhases);

    void StdVectorToPhases(const vector<WDL_TypedBuf<BL_FLOAT> > &inPhases,
                           bl_queue<WDL_TypedBuf<BL_FLOAT> > *outPhases);
    
    void UnwrapLineX(WDL_TypedBuf<BL_FLOAT> *phases);
    
    void UnwrapLineY(WDL_TypedBuf<BL_FLOAT> *phases);

    //
    void SavePPM(const char *filename, int maxValue);
    
    static BLSpectrogram4 *ImageToSpectrogram(BL_FLOAT sampleRate,
                                              PPMFile::PPMImage *image,
                                              bool is16Bits);
    
    static BLSpectrogram4 *ImagesToSpectrogram(BL_FLOAT sampleRate,
                                               PPMFile::PPMImage *magnsImage,
                                               PPMFile::PPMImage *phasesImage);

    
    BL_FLOAT ComputeMaxPhaseValue(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW);
    
    void ComputeDPhases(vector<WDL_TypedBuf<BL_FLOAT> > *phasesUnW);
    
    void ComputeDPhasesX(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
                         vector<WDL_TypedBuf<BL_FLOAT> > *dPhasesX);
    
    void ComputeDPhasesY(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
                         vector<WDL_TypedBuf<BL_FLOAT> > *dPhasesY);

    void ResetQueues();
    
    //
    
    int mHeight;
    int mMaxCols;
    
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mRange;
    
    BL_FLOAT mContrast;
    
    //bool mYLogScale;
    Scale::Type mYScale;
    Scale::Type mValueScale;
    
    bool mDisplayMagns;
    
    // Display the phases (+ wrap on x) ?
    bool mDisplayPhasesX;
    
    // Display the phases (+ wrap on y) ?
    bool mDisplayPhasesY;
    
    // Display derivative of phases instead of phase unwrap ?
    bool mDisplayDPhases;
    
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mMagns;
    
    // Raw phases
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mPhases;
    
    // Unwrapped phases
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mUnwrappedPhases;
    
    // Count the total number of lines added
    unsigned long long mTotalLineNum;
    
    ColorMapFactory *mColorMapFactory;
    ColorMap4 *mColorMap;
    
    bool mProgressivePhaseUnwrap;
    
    static unsigned char mPhasesColor[4];
    
#if OPTIM_SPECTROGRAM2
    bool mSpectroDataChanged;
    bool mColormapDataChanged;
#endif
    
    Scale *mScale;

private:
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
};

#endif /* defined(__Denoiser__BLSpectrogram4__) */
