//
//  Spectrum.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

#ifndef __Denoiser__DbgSpectrogram__
#define __Denoiser__DbgSpectrogram__

#include <vector>
#include <deque>
using namespace std;

#include <BLTypes.h>

#include <Scale.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

#include "PPMFile.h"

// Simpler than BLSpectrogram3
// Does not contain phases, only magns
//
// For debugging
//
class DbgSpectrogram
{
public:
    DbgSpectrogram(int height, int maxCols = -1,
                   Scale::Type scale = Scale::MEL);
    
    virtual ~DbgSpectrogram();
    
    //void SetYLogScale(bool flag); //, BL_FLOAT factor);
    void SetYScale(Scale::Type scale);
    
    void SetAmpDb(bool flag);
    
    void Reset();
    
    void Reset(int height, int maxCols = -1);
    
    int GetMaxNumCols();
    
    int GetNumCols();
    
    int GetHeight();
    
    // Lines
    void AddLine(const WDL_TypedBuf<BL_FLOAT> &magns);
    
    bool GetLine(int index,
                 WDL_TypedBuf<BL_FLOAT> *magns);
    
    // Load and save
    static DbgSpectrogram *Load(const char *fileName);
    
    void Save(const char *filename);
    
    static DbgSpectrogram *LoadPPM(const char *filename);
    
    void SavePPM(const char *filename);
    
    static DbgSpectrogram *LoadPPM16(const char *filename);
    
    void SavePPM16(const char *filename);
    
    static DbgSpectrogram *LoadPPM32(const char *filename);
    
    void SavePPM32(const char *filename);
    
protected:
    void FillWithZeros();
    
    
    void SavePPM(const char *filename, int maxValue);
    
    static DbgSpectrogram *ImageToSpectrogram(PPMFile::PPMImage *image, bool is16Bits);
    
    static DbgSpectrogram *ImagesToSpectrogram(PPMFile::PPMImage *magnsImage);

    //
    int mHeight;
    int mMaxCols;
    
    //bool mYLogScale;
    //BL_FLOAT mYLogScaleFactor;
    
    bool mAmpDb;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMagns;
    
    Scale::Type mYScale;
};

#endif /* defined(__Denoiser__DbgSpectrogram__) */
