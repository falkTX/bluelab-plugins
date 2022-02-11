//
//  Spectrum.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

#ifndef __Denoiser__BLSpectrogram__
#define __Denoiser__BLSpectrogram__

#include <vector>
#include <deque>
using namespace std;

#define CLASS_PROFILE 0

#if CLASS_PROFILE
#include <BlaTimer.h>
#include <Debug.h>
#endif

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

#include "PPMFile.h"


class ColorMap;

class BLSpectrogram
{
public:
    BLSpectrogram(int height, int maxCols = -1);
    
    virtual ~BLSpectrogram();
    
    void SetMultipliers(BL_FLOAT magnMult, BL_FLOAT phaseMult);
    
    void SetGain(BL_FLOAT gain);
    
    void SetYLogScale(bool flag);
    
    void SetDisplayMagns(bool flag);
    
    void SetDisplayPhasesX(bool flag);
    
    void SetDisplayPhasesY(bool flag);
    
    void SetDisplayDPhases(bool flag);
    
    void Reset();
    
    int GetMaxNumCols();
    
    int GetNumCols();
    
    int GetHeight();
    
    // Load and save
    static BLSpectrogram *Load(const char *fileName);
    
    void Save(const char *filename);
    
    static BLSpectrogram *LoadPPM(const char *filename,
                                  BL_FLOAT magnsMultiplier, BL_FLOAT phasesMultiplier);
    
    void SavePPM(const char *filename);
    
    static BLSpectrogram *LoadPPM16(const char *filename,
                                    BL_FLOAT magnsMultiplier, BL_FLOAT phasesMultiplier);
    
    void SavePPM16(const char *filename);
    
    static BLSpectrogram *LoadPPM32(const char *filename);
    
    void SavePPM32(const char *filename);
    
    // Lines
    void AddLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                 const WDL_TypedBuf<BL_FLOAT> &phases);
    
    bool GetLine(int index,
                 WDL_TypedBuf<BL_FLOAT> *magns,
                 WDL_TypedBuf<BL_FLOAT> *phases);
    
    // Image data
    void GetImageDataRGBA(int width, int height, unsigned char *buf);
    
protected:
    void UnwrapPhases3(const deque<WDL_TypedBuf<BL_FLOAT> > &inPhases,
                       vector<WDL_TypedBuf<BL_FLOAT> > *outPhases,
                       bool hozirontal, bool vertical);
    
    void FillWithZeros();
    
    void SavePPM(const char *filename, int maxValue);
    
    static BLSpectrogram *ImageToSpectrogram(PPMFile::PPMImage *image, bool is16Bits,
                                             BL_FLOAT magnsMultiplier, BL_FLOAT phasesMultiplier);
    
    static BLSpectrogram *ImagesToSpectrogram(PPMFile::PPMImage *magnsImage,
                                              PPMFile::PPMImage *phasesImage);

    
    void ComputeDPhases(vector<WDL_TypedBuf<BL_FLOAT> > *phasesUnW);
    
    void ComputeDPhasesX(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
                         vector<WDL_TypedBuf<BL_FLOAT> > *dPhasesX);
    
    void ComputeDPhasesY(const vector<WDL_TypedBuf<BL_FLOAT> > &phasesUnW,
                         vector<WDL_TypedBuf<BL_FLOAT> > *dPhasesY);
    
#if 0 // GARBAGE
    
    // Phases
    void UnwrapPhases();
    
    void UnwrapPhases(deque<WDL_TypedBuf<BL_FLOAT> > *ioPhases);
    
    void UnwrapPhases2(deque<WDL_TypedBuf<BL_FLOAT> > *ioPhases,
                       bool hozirontal, bool vertical);
    
    static BL_FLOAT MapToPi(BL_FLOAT val);
#endif
    
    int mHeight;
    int mMaxCols;
    
    BL_FLOAT mMagnsMultiplier;
    BL_FLOAT mPhasesMultiplier;

    BL_FLOAT mPhasesMultiplier2;
    
    BL_FLOAT mGain;
    
    bool mYLogScale;
    
    bool mDisplayMagns;
    
    // Display the phases (+ wrap on x) ?
    bool mDisplayPhasesX;
    
    // Display the phases (+ wrap on y) ?
    bool mDisplayPhasesY;
    
    // Display derivative of phases instead of phase unwrap ?
    bool mDisplayDPhases;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMagns;
    deque<WDL_TypedBuf<BL_FLOAT> > mPhases;
    
    ColorMap *mColorMap;
    
#if CLASS_PROFILE
    BlaTimer mTimer;
    long mTimerCount;
#endif
};

#endif /* defined(__Denoiser__BLSpectrogram__) */
