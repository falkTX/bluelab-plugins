//
//  SpectrogramDisplayScroll4.h
//  BL-Chroma
//
//  Created by Pan on 14/06/18.
//
//

#ifndef __BL_Chroma__SpectrogramDisplayScroll4__
#define __BL_Chroma__SpectrogramDisplayScroll4__

#ifdef IGRAPHICS_NANOVG

#include <deque>
using namespace std;

#include <BLTypes.h>
#include <GraphControl12.h>

#include <LockFreeQueue.h>

#include "IPlug_include_in_plug_hdr.h"


using namespace iplug;

// From SpectrogramDisplay
//
// Removed specificities that were for Ghost
// (MiniView, Background and Foreground spectrogram)
//
// SpectrogramDisplayScroll3: from SpectrogramDisplayScroll
// For GraphControl12
//

class BLSpectrogram4;
class NVGcontext;
class BLTransport;
class SpectrogramDisplayScroll4 : public GraphCustomDrawer
{
public:
    // delayPercent: delay that we bufferize, to fix when the data is a bit late
    // It is a percent of the spectrogram full width
    SpectrogramDisplayScroll4(Plugin *plug, BL_FLOAT delayPercent = 3.125/*25.0*/);
    
    virtual ~SpectrogramDisplayScroll4();

    void SetTransport(BLTransport *transport);
    
    void Reset();
    
    // For InfrasonicViewer
    void ResetScroll();
    
    bool NeedUpdateSpectrogram();
    bool DoUpdateSpectrogram();

    // CustomDrawer
    void PreDraw(NVGcontext *vg, int width, int height) override;
    bool IsOwnedByGraph() override { return true; }

    // LockFreeObj
    void PushData() override;
    void PullData() override;
    void ApplyData() override;
    
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

    // Hack
    void TransportPlayingChanged();
    
    // Variable speed
    void SetSpeedMod(int speedMod);
    int GetSpeedMod();

    // SpectrogramDisplayScroll4 upscale a bit the image, for hiding the borders.
    // We can get this scale to adapt some other objects, so they will scroll
    // exactly like the SpectrogramDisplayScroll4
    void GetTimeTransform(BL_FLOAT *timeOffsetSec, BL_FLOAT *timeScale);
    
protected:
    BL_FLOAT GetOffsetSec();

    void RecomputeParams();

    BL_FLOAT SecsToPixels(BL_FLOAT secs, BL_FLOAT width);

    void ResynchTransport();

    void LFAddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                              const WDL_TypedBuf<BL_FLOAT> &phases);
    
    // NanoVG
    NVGcontext *mVg;
    
    // Spectrogram
    BLSpectrogram4 *mSpectrogram;
    BL_FLOAT mSpectrogramBounds[4];
    
    int mNvgSpectroImage;
    WDL_TypedBuf<unsigned char> mSpectroImageData;
    
    bool mNeedUpdateSpectrogram;
    bool mNeedUpdateSpectrogramData;
    
    bool mNeedUpdateColormapData;
    bool mShowSpectrogram;
    
    // Colormap
    int mNvgColormapImage;
    WDL_TypedBuf<unsigned int> mColormapImageData;
    
    int mBufferSize;
    int mOverlapping;
    BL_FLOAT mSampleRate;

    // Variable speed
    int mSpeedMod;
    
    // Get reference to plug, to know if the plug is currently playing
    Plugin *mPlug;
    
    BLTransport *mTransport;
    
    //
    BL_FLOAT mDelayPercent;
    BL_FLOAT mDelayTimeSecRight; // From delay percent
    BL_FLOAT mDelayTimeSecLeft;  // 1 single col, or more...

    BL_FLOAT mSpectroLineDurationSec;
    BL_FLOAT mSpectroTotalDurationSec;

    //
    BL_FLOAT mSpectroTimeSec;    

    BL_FLOAT mPrevOffsetSec;

    // Lock free
    struct SpectrogramLine
    {
        WDL_TypedBuf<BL_FLOAT> mMagns;
        WDL_TypedBuf<BL_FLOAT> mPhases;
    };
    
    LockFreeQueue<SpectrogramLine> mLockFreeQueues[LOCK_FREE_NUM_BUFFERS];
    
private:
    WDL_TypedBuf<unsigned int> mTmpBuf0;
    SpectrogramLine mTmpBuf1;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Chroma__SpectrogramDisplayScroll4__) */
