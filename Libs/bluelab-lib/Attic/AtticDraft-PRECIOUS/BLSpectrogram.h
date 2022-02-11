//
//  Spectrum.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

#ifndef __Denoiser__BLSpectrogram__
#define __Denoiser__BLSpectrogram__

#include <deque>
using namespace std;

#define CLASS_PROFILE 1

#if CLASS_PROFILE
#include <BlaTimer.h>
#include <Debug.h>
#endif

#include "../../WDL/IPlug/Containers.h"

struct _PPMImage;
typedef struct _PPMImage PPMImage;

class ColorMap;

class BLSpectrogram
{
public:
    BLSpectrogram(int height, int maxCols = -1);
    
    virtual ~BLSpectrogram();
    
    void SetMultipliers(double magnMult, double phaseMult);
    
    void SetGain(double gain);
    
    void SetDisplayMagns(bool flag);
    
    void SetDisplayPhasesX(bool flag);
    
    void SetDisplayPhasesY(bool flag);
    
    void Reset();
    
    int GetMaxNumCols();
    
    int GetNumCols();
    
    int GetHeight();
    
    // Load and save
    static BLSpectrogram *Load(const char *fileName);
    
    void Save(const char *filename);
    
    static BLSpectrogram *LoadPPM(const char *filename,
                                  double magnsMultiplier, double phasesMultiplier);
    
    void SavePPM(const char *filename);
    
    static BLSpectrogram *LoadPPM16(const char *filename,
                                    double magnsMultiplier, double phasesMultiplier);
    
    void SavePPM16(const char *filename);
    
    static BLSpectrogram *LoadPPM32(const char *filename);
    
    void SavePPM32(const char *filename);
    
    // Lines
    void AddLine(const WDL_TypedBuf<double> &magns,
                 const WDL_TypedBuf<double> &phases);
    
    bool GetLine(int index,
                 WDL_TypedBuf<double> *magns,
                 WDL_TypedBuf<double> *phases);
    
    // Phases
    void UnwrapPhases();
    
    // Image data
    void GetImageDataRGBA(int width, int height, unsigned char *buf);
    
protected:
    void UnwrapPhases(deque<WDL_TypedBuf<double> > *ioPhases);
    
    void UnwrapPhases2(deque<WDL_TypedBuf<double> > *ioPhases,
                       bool hozirontal, bool vertical);
    
    void UnwrapPhases3(const deque<WDL_TypedBuf<double> > &inPhases,
                       vector<WDL_TypedBuf<double> > *outPhases,
                       bool hozirontal, bool vertical);
    
    void SavePPM(const char *filename, int maxValue);
    
    static BLSpectrogram *ImageToSpectrogram(PPMImage *image, bool is16Bits,
                                           double magnsMultiplier, double phasesMultiplier);
    
    static BLSpectrogram *ImagesToSpectrogram(PPMImage *magnsImage, PPMImage *phasesImage);
    
    static double MapToPi(double val);
    
    int mHeight;
    int mMaxCols;
    
    double mMagnsMultiplier;
    double mPhasesMultiplier;

    double mPhasesMultiplier2;
    
    double mGain;
    
    bool mDisplayMagns;
    
    // Display the phases (+ wrap on x) ?
    bool mDisplayPhasesX;
    
    // Display the phases (+ wrap on y) ?
    bool mDisplayPhasesY;
    
    
    deque<WDL_TypedBuf<double> > mMagns;
    deque<WDL_TypedBuf<double> > mPhases;
    
    ColorMap *mColorMap;
    
#if CLASS_PROFILE
    BlaTimer mTimer;
    long mTimerCount;
#endif
};

#endif /* defined(__Denoiser__BLSpectrogram__) */
