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

#include <LockFreeQueue2.h>

#include <TransportListener.h>

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
class SpectrogramDisplayScroll4 : public GraphCustomDrawer,
                                  public TransportListener
{
public:
    struct SpectrogramDisplayScrollState
    {
        BL_FLOAT mDelayPercent;
        BL_FLOAT mDelayTimeSecRight; // From delay percent
        BL_FLOAT mDelayTimeSecLeft;  // 1 single col, or more...

        double mSpectroLineDurationSec;
        double mSpectroTotalDurationSec;
        
        double mSpectroTimeSec;
        
        BL_FLOAT mPrevOffsetSec;

        // Spectrogram
        BLSpectrogram4 *mSpectrogram;
        BL_FLOAT mSpectrogramBounds[4];

        // Variable speed
        int mSpeedMod;
        
        // Fft params
        int mBufferSize;
        int mOverlapping;
        BL_FLOAT mSampleRate;

        //
        BLTransport *mTransport;

        bool mSmoothScrollDisabled;
    };
    
    enum ViewOrientation
    {
        HORIZONTAL = 0,
        VERTICAL
    };
    
    // delayPercent: delay that we bufferize, to fix when the data is a bit late
    // It is a percent of the spectrogram full width
    SpectrogramDisplayScroll4(Plugin *plug,
                              SpectrogramDisplayScrollState *spectroState = NULL,
                              BL_FLOAT delayPercent = (BL_FLOAT)3.125/*25.0*/);
    
    virtual ~SpectrogramDisplayScroll4();

    SpectrogramDisplayScrollState *GetState();
    
    void SetTransport(BLTransport *transport);
    
    void Reset();
    
    // For InfrasonicViewer
    void ResetScroll();
    
    bool NeedUpdateSpectrogram();
    bool DoUpdateSpectrogram();

    // CustomDrawer
    void PreDraw(NVGcontext *vg, int width, int height) override;
    bool IsOwnedByGraph() override { return true; }
    bool NeedRedraw() override;
    
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
    void TransportPlayingChanged() override;
    
    // Variable speed
    void SetSpeedMod(int speedMod);
    int GetSpeedMod();

    // SpectrogramDisplayScroll4 upscale a bit the image, for hiding the borders.
    // We can get this scale to adapt some other objects, so they will scroll
    // exactly like the SpectrogramDisplayScroll4
    void GetTimeTransform(BL_FLOAT *timeOffsetSec, BL_FLOAT *timeScale);

    void GetTimeBoundsNorm(BL_FLOAT *tn0, BL_FLOAT *tn1);

    void SetViewOrientation(ViewOrientation orientation);

    void SetBypassed(bool flag);

    void SetSmoothScrollDisabled(bool flag);
    
protected:
    BL_FLOAT GetOffsetSec();

    void RecomputeParams(bool resetAll);

    BL_FLOAT SecsToPixels(BL_FLOAT secs, BL_FLOAT width);
    
    void AddSpectrogramLineLF(const WDL_TypedBuf<BL_FLOAT> &magns,
                              const WDL_TypedBuf<BL_FLOAT> &phases);

    void SetBypassedLF(bool flag);

    void SetSpeedModLF(int speedMod);
    
    // NanoVG
    NVGcontext *mVg;
    
    int mNvgSpectroImage;
    WDL_TypedBuf<unsigned char> mSpectroImageData;
    
    bool mNeedUpdateSpectrogram;
    bool mNeedUpdateSpectrogramData;
    
    bool mNeedUpdateColormapData;
    bool mShowSpectrogram;
    
    // Colormap
    int mNvgColormapImage;
    WDL_TypedBuf<unsigned int> mColormapImageData;

    // Get reference to plug, to know if the plug is currently playing
    Plugin *mPlug;

    ViewOrientation mViewOrientation;

    bool mIsBypassed;

    bool mNeedRedraw;
    
    SpectrogramDisplayScrollState *mState;
    
    // Lock free
    struct Command
    {
        enum Type
        {
            ADD_SPECTROGRAM_LINE = 0,
            SET_SPEED_MOD,
            SET_BYPASSED
        };

        Type mType;
        
        // For ADD_SPECTROGRAM_LINE
        WDL_TypedBuf<BL_FLOAT> mMagns;
        WDL_TypedBuf<BL_FLOAT> mPhases;

        // For SET_SPEED_MOD
        int mSpeedMod;

        // For SET_BYPASSED
        bool mBypassed;
    };
    
    LockFreeQueue2<Command> mLockFreeQueues[LOCK_FREE_NUM_BUFFERS];
    
private:
    WDL_TypedBuf<unsigned int> mTmpBuf0;
    Command mTmpBuf1;
    Command mTmpBuf2;
    Command mTmpBuf3;
    Command mTmpBuf4;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Chroma__SpectrogramDisplayScroll4__) */
