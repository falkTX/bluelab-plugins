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

#include "../../WDL/IPlug/Containers.h"

#include "PPMFile.h"


class ColorMap2;

class BLSpectrogram2
{
public:
    BLSpectrogram2(int height, int maxCols = -1);
    
    virtual ~BLSpectrogram2();
    
    void Reset();
    
    void SetGain(double gain);
    
    void SetYLogScale(bool flag);
    
    void SetDisplayMagns(bool flag);
    
    void SetDisplayPhasesX(bool flag);
    
    void SetDisplayPhasesY(bool flag);
    
    void SetDisplayDPhases(bool flag);
    
    int GetMaxNumCols();
    
    int GetNumCols();
    
    int GetHeight();
    
    
    // Lines
    void AddLine(const WDL_TypedBuf<double> &magns,
                 const WDL_TypedBuf<double> &phases);
    
    bool GetLine(int index,
                 WDL_TypedBuf<double> *magns,
                 WDL_TypedBuf<double> *phases);
    
    
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
    void UnwrapAllPhases(const deque<WDL_TypedBuf<double> > &inPhases,
                         vector<WDL_TypedBuf<double> > *outPhases,
                         bool hozirontal, bool vertical);
    
    // Unwrap in place (slow)
    void UnwrapAllPhases(deque<WDL_TypedBuf<double> > *ioPhases,
                         bool horizontal, bool vertical);
    
    void PhasesToStdVector(const deque<WDL_TypedBuf<double> > &inPhases,
                           vector<WDL_TypedBuf<double> > *outPhases);

    void StdVectorToPhases(const vector<WDL_TypedBuf<double> > &inPhases,
                           deque<WDL_TypedBuf<double> > *outPhases);
    
    void UnwrapLineX(WDL_TypedBuf<double> *phases);
    
    void UnwrapLineY(WDL_TypedBuf<double> *phases);
    
    void FillWithZeros();
    
    static void FindNextPhase(double *phase, double refPhase);

    
    void SavePPM(const char *filename, int maxValue);
    
    static BLSpectrogram2 *ImageToSpectrogram(PPMFile::PPMImage *image, bool is16Bits);
    
    static BLSpectrogram2 *ImagesToSpectrogram(PPMFile::PPMImage *magnsImage,
                                               PPMFile::PPMImage *phasesImage);

    
    double ComputeMaxPhaseValue(const vector<WDL_TypedBuf<double> > &phasesUnW);
    
    void ComputeDPhases(vector<WDL_TypedBuf<double> > *phasesUnW);
    
    void ComputeDPhasesX(const vector<WDL_TypedBuf<double> > &phasesUnW,
                         vector<WDL_TypedBuf<double> > *dPhasesX);
    
    void ComputeDPhasesY(const vector<WDL_TypedBuf<double> > &phasesUnW,
                         vector<WDL_TypedBuf<double> > *dPhasesY);
    
    
    int mHeight;
    int mMaxCols;
    
    double mGain;
    
    bool mYLogScale;
    
    bool mDisplayMagns;
    
    // Display the phases (+ wrap on x) ?
    bool mDisplayPhasesX;
    
    // Display the phases (+ wrap on y) ?
    bool mDisplayPhasesY;
    
    // Display derivative of phases instead of phase unwrap ?
    bool mDisplayDPhases;
    
    deque<WDL_TypedBuf<double> > mMagns;
    
    // Raw phases
    deque<WDL_TypedBuf<double> > mPhases;
    
    // Unwrapped phases
    deque<WDL_TypedBuf<double> > mUnwrappedPhases;
    
    ColorMap2 *mColorMap;
    
    bool mProgressivePhaseUnwrap;
};

#endif /* defined(__Denoiser__BLSpectrogram2__) */
