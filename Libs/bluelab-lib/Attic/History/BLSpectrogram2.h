//
//  Spectrum.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

#ifndef __Denoiser__BLSpectrogram2__
#define __Denoiser__BLSpectrogram2__

#include <vector>
#include <deque>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

#include "PPMFile.h"


class ColorMap2;

class BLSpectrogram2
{
public:
    BLSpectrogram2(int height, int maxCols = -1);
    
    virtual ~BLSpectrogram2();
    
    void Reset();
    
    void SetRange(BL_FLOAT range);
    
    void SetContrast(BL_FLOAT contrast);
    
    void SetYLogScale(bool flag);
    
    void SetDisplayMagns(bool flag);
    
    void SetDisplayPhasesX(bool flag);
    
    void SetDisplayPhasesY(bool flag);
    
    void SetDisplayDPhases(bool flag);
    
    int GetMaxNumCols();
    
    int GetNumCols();
    
    int GetHeight();
    
    
    // Lines
    void AddLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                 const WDL_TypedBuf<BL_FLOAT> &phases);
    
    bool GetLine(int index,
                 WDL_TypedBuf<BL_FLOAT> *magns,
                 WDL_TypedBuf<BL_FLOAT> *phases);
    
    
    // Image data
    void GetImageDataRGBA(int width, int height, unsigned char *buf);
    
    
    // Load and save
    static BLSpectrogram2 *Load(const char *fileName);
    
    void Save(const char *filename);
    
    static BLSpectrogram2 *LoadPPM(const char *filename);
    
    void SavePPM(const char *filename);
    
    static BLSpectrogram2 *LoadPPM16(const char *filename);
    
    void SavePPM16(const char *filename);
    
    static BLSpectrogram2 *LoadPPM32(const char *filename);
    
    void SavePPM32(const char *filename);
    
protected:
    void UnwrapAllPhases(const deque<WDL_TypedBuf<BL_FLOAT> > &inPhases,
                         vector<WDL_TypedBuf<BL_FLOAT> > *outPhases,
                         bool hozirontal, bool vertical);
    
    // Unwrap in place (slow)
    void UnwrapAllPhases(deque<WDL_TypedBuf<BL_FLOAT> > *ioPhases,
                         bool horizontal, bool vertical);
    
    void PhasesToStdVector(const deque<WDL_TypedBuf<BL_FLOAT> > &inPhases,
                           vector<WDL_TypedBuf<BL_FLOAT> > *outPhases);

    void StdVectorToPhases(const vector<WDL_TypedBuf<BL_FLOAT> > &inPhases,
                           deque<WDL_TypedBuf<BL_FLOAT> > *outPhases);
    
    void UnwrapLineX(WDL_TypedBuf<BL_FLOAT> *phases);
    
    void UnwrapLineY(WDL_TypedBuf<BL_FLOAT> *phases);
    
    void FillWithZeros();
    
    
    void SavePPM(const char *filename, int maxValue);
    
    static BLSpectrogram2 *ImageToSpectrogram(PPMFile::PPMImage *image, bool is16Bits);
    
    static BLSpectrogram2 *ImagesToSpectrogram(PPMFile::PPMImage *magnsImage,
                                               PPMFile::PPMImage *phasesImage);

    
    BL_FLOAT ComputeMaxPhaseValue(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW);
    
    void ComputeDPhases(vector<WDL_TypedBuf<BL_FLOAT> > *phasesUnW);
    
    void ComputeDPhasesX(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
                         vector<WDL_TypedBuf<BL_FLOAT> > *dPhasesX);
    
    void ComputeDPhasesY(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
                         vector<WDL_TypedBuf<BL_FLOAT> > *dPhasesY);
    
    
    int mHeight;
    int mMaxCols;
    
    BL_FLOAT mRange;
    
    BL_FLOAT mContrast;
    
    bool mYLogScale;
    
    bool mDisplayMagns;
    
    // Display the phases (+ wrap on x) ?
    bool mDisplayPhasesX;
    
    // Display the phases (+ wrap on y) ?
    bool mDisplayPhasesY;
    
    // Display derivative of phases instead of phase unwrap ?
    bool mDisplayDPhases;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMagns;
    
    // Raw phases
    deque<WDL_TypedBuf<BL_FLOAT> > mPhases;
    
    // Unwrapped phases
    deque<WDL_TypedBuf<BL_FLOAT> > mUnwrappedPhases;
    
    ColorMap2 *mColorMap;
    
    bool mProgressivePhaseUnwrap;
    
    static unsigned char mPhasesColor[4];
};

#endif /* defined(__Denoiser__BLSpectrogram2__) */
