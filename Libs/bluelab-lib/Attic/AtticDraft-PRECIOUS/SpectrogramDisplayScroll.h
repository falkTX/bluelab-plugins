//
//  SpectrogramDisplayScroll.h
//  BL-Chroma
//
//  Created by Pan on 14/06/18.
//
//

#ifndef __BL_Chroma__SpectrogramDisplayScroll__
#define __BL_Chroma__SpectrogramDisplayScroll__

#include <deque>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

// From SpectrogramDisplay
//
// Removed specificities that were for Ghost
// (MiniView, Background and Foreground spectrogram)
class BLSpectrogram3;

class SpectrogramDisplayScroll
{
public:
    SpectrogramDisplayScroll(NVGcontext *vg);
    
    virtual ~SpectrogramDisplayScroll();
    
    void Reset();
    
    bool DoUpdateSpectrogram();
    
    void DrawSpectrogram(int width, int height);
    
    // Spectrogram
    void SetSpectrogram(BLSpectrogram3 *spectro,
                        double left, double top, double right, double bottom);
    
    void SetFftParams(int bufferSize, int overlapping, double sampleRate);
    
    // Add and bufferize spectrogram line
    // the lines will be added progressively
    // (so with overlap > 1, the scrolling will be smoother)
    void AddSpectrogramLine(const WDL_TypedBuf<double> &magns,
                            const WDL_TypedBuf<double> &phases);
    
    void ShowSpectrogram(bool flag);
    
    void UpdateSpectrogram(bool flag);
    void UpdateColormap(bool flag);
    
    void SetIsPlaying(bool flag);
    void Update();
    
protected:
    void AddSpectrogramLines(double numLines);
    
    double ComputeScrollOffsetPixels(int width);
    
    // NanoVG
    NVGcontext *mVg;
    
    // Spectrogram
    BLSpectrogram3 *mSpectrogram;
    double mSpectrogramBounds[4];
    
    int mNvgSpectroImage;
    WDL_TypedBuf<unsigned char> mSpectroImageData;
    
    bool mMustUpdateSpectrogram;
    bool mMustUpdateSpectrogramData;
    
    // NEW
    bool mMustUpdateColormapData;
    
    //
    bool mShowSpectrogram;
    
    double mSpectrogramGain;
    
    // Colormap
    int mNvgColormapImage;
    WDL_TypedBuf<unsigned int> mColormapImageData;
    
    int mBufferSize;
    int mOverlapping;
    double mSampleRate;
    unsigned long long mPrevSpectroLineNum;
    
    unsigned long long mPrevTimeMillis;
    
    // Offset for scrolling in "line" units
    double mLinesOffset;
    
    // Add progressively spectrogram lines
    deque<WDL_TypedBuf<double> > mSpectroMagns;
    deque<WDL_TypedBuf<double> > mSpectroPhases;
    
    double mAddLineRemainder;
    
    bool mIsPlaying;
    bool mPrevIsPlaying;
    //long long mPrevElapsedMillis;
    double mPrevPixelOffset;
};

#endif /* defined(__BL_Chroma__SpectrogramDisplayScroll__) */
