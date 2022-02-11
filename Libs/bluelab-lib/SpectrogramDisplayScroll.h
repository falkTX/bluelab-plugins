//
//  SpectrogramDisplayScroll.h
//  BL-Chroma
//
//  Created by Pan on 14/06/18.
//
//

#ifndef __BL_Chroma__SpectrogramDisplayScroll__
#define __BL_Chroma__SpectrogramDisplayScroll__

#ifdef IGRAPHICS_NANOVG

#include <deque>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#include "resource.h"

#define USE_VARIABLE_SPEED 1

using namespace iplug;

// From SpectrogramDisplay
//
// Removed specificities that were for Ghost
// (MiniView, Background and Foreground spectrogram)
class BLSpectrogram4;
class NVGcontext;

class SpectrogramDisplayScroll
{
public:
    SpectrogramDisplayScroll(Plugin *plug, NVGcontext *vg);
    
    virtual ~SpectrogramDisplayScroll();
    
    void SetNvgContext(NVGcontext *vg);

    void ResetGfx();
    void RefreshGfx();
    
    void Reset();
    
    // For InfrasonicViewer
    void ResetScroll();
    
    bool NeedUpdateSpectrogram();
    bool DoUpdateSpectrogram();
    
    void DrawSpectrogram(int width, int height);
    
    // Spectrogram
    void SetSpectrogram(BLSpectrogram4 *spectro,
                        BL_FLOAT left, BL_FLOAT top, BL_FLOAT right, BL_FLOAT bottom);
    
    void SetFftParams(int bufferSize, int overlapping, BL_FLOAT sampleRate);
    
    // Add and bufferize spectrogram line
    // the lines will be added progressively
    // (so with overlap > 1, the scrolling will be smoother)
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void ShowSpectrogram(bool flag);
    
    void UpdateSpectrogram(bool flag);
    void UpdateColormap(bool flag);
    
    // For Logic
    void SetIsPlaying(bool flag);
    
#if USE_VARIABLE_SPEED
    void SetSpeedMod(int speedMod);
    int GetSpeedMod();
#endif
    
    // We scale up a bit, to hide the borders
    BL_FLOAT GetScaleRatio();
    
protected:
    void AddSpectrogramLines(BL_FLOAT numLines);
    
    BL_FLOAT ComputeScrollOffsetPixels(int width);
    
    // NanoVG
    NVGcontext *mVg;
    
    // Spectrogram
    BLSpectrogram4 *mSpectrogram;
    BL_FLOAT mSpectrogramBounds[4];
    
    int mNvgSpectroImage;
    WDL_TypedBuf<unsigned char> mSpectroImageData;
    
    bool mNeedUpdateSpectrogram;
    bool mNeedUpdateSpectrogramData;
    
    // NEW
    bool mNeedUpdateColormapData;
    
    //
    bool mShowSpectrogram;
    
    BL_FLOAT mSpectrogramGain;
    
    // Colormap
    int mNvgColormapImage;
    WDL_TypedBuf<unsigned int> mColormapImageData;
    
    int mBufferSize;
    int mOverlapping;
    BL_FLOAT mSampleRate;
    unsigned long long mPrevSpectroLineNum;
    
    unsigned long long mPrevTimeMillis;
    
    // Offset for scrolling in "line" units
    BL_FLOAT mLinesOffset;
    
    // Add progressively spectrogram lines
    deque<WDL_TypedBuf<BL_FLOAT> > mSpectroMagns;
    deque<WDL_TypedBuf<BL_FLOAT> > mSpectroPhases;
    
    BL_FLOAT mAddLineRemainder;
    
    // Members to avoid jumps in scrolling when restarting playback
    //
    
    // Get reference to plug, to know if the plug is currently playing
    Plugin *mPlug;
    bool mPrevIsPlaying;
    BL_FLOAT mPrevPixelOffset;
    
    bool mIsPlaying;
    
#if USE_VARIABLE_SPEED
    int mSpeedMod;
#endif
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Chroma__SpectrogramDisplayScroll__) */
