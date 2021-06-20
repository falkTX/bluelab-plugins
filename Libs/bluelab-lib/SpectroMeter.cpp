#include <IControls.h>

#include <GUIHelper12.h>

#include "SpectroMeter.h"

#define TEXT_FIELD_V_SIZE 14 //16 //40

#define TEXT_FIELD_H_SPACING1 10

#define TEXT_FIELD_V_SPACING 10

#define FONT "Roboto-Bold"

#define DEFAULT_TEXT "------------------"

#define TEXT_ALIGN EAlign::Far

SpectroMeter::SpectroMeter(BL_FLOAT x, BL_FLOAT y,
                           int timeParamIdx, int freqParamIdx,
                           int bufferSize, BL_FLOAT sampleRate,
                           DisplayType type)
{
#ifdef _DEBUG
    // When debugging, we prefer samples
    mTimeMode = SPECTRO_METER_TIME_SAMPLES;
#else
    // When releasing, h/m/s is better
    mTimeMode = SPECTRO_METER_TIME_HMS;
#endif
    
    mFreqMode = SPECTRO_METER_FREQ_HZ;

    mDisplayType = type;
        
    mTimeParamIdx = timeParamIdx;
    mFreqParamIdx = freqParamIdx;

    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mX = x;
    mY = y;

    // Style
    mTextFieldHSpacing = TEXT_FIELD_H_SPACING1;
    mTextFieldVSpacing = TEXT_FIELD_V_SPACING;

    //mBGColor = IColor(255, 64, 64, 64);
    mBGColor = IColor(255, 255, 0, 0);

    mBorderColor = IColor(255, 0, 0, 0);
    mBorderWidth = -1.0;
    
    ClearUI();

    mSelectionActive = false;
    
    //
    mPrevCursorTimeX = 0.0;
    mPrevCursorFreqY = 0.0;

    mPrevSelTimeX = 0.0;
    mPrevSelFreqY = 0.0;
    
    mPrevSelTimeW = 0.0;
    mPrevSelFreqH = 0.0;
}

SpectroMeter::~SpectroMeter() {}

void
SpectroMeter::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;

    RefreshValues();
}

void
SpectroMeter::GenerateUI(GUIHelper12 *guiHelper,
                         IGraphics *graphics,
                         int offsetX, int offsetY)
{
    IColor valueColor;
    guiHelper->GetValueTextColor(&valueColor);

    // Cursor pos
    mCursorPosTexts[0] =
        guiHelper->CreateTextButton(graphics,
                                    mX + offsetX, mY + offsetY,
                                    mTimeParamIdx,
                                    DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                                    FONT,
                                    valueColor, TEXT_ALIGN,
                                    0.0, 0.0, mBorderColor, mBorderWidth);

    IRECT cp0 = mCursorPosTexts[0]->GetRECT();
    mCursorPosTexts[1] =
        guiHelper->CreateTextButton(graphics,
                                    cp0.R + mTextFieldHSpacing,
                                    //TEXT_FIELD_H_SPACING1,
                                    mY + offsetY,
                                    mFreqParamIdx,
                                    DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                                    FONT,
                                    valueColor, TEXT_ALIGN,
                                    0.0, 0.0, mBorderColor, mBorderWidth);

    if (mDisplayType == SPECTRO_METER_DISPLAY_SELECTION)
    {
        // Selection pos
        mSelPosTexts[0] =
            guiHelper->CreateTextButton(graphics,
                                        mX + offsetX, cp0.B + mTextFieldVSpacing,
                                        //TEXT_FIELD_V_SPACING,
                                        mTimeParamIdx,
                                        DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                                        FONT,
                                        valueColor, TEXT_ALIGN,
                                        0.0, 0.0, mBorderColor, mBorderWidth);
        IRECT sp0 = mSelPosTexts[0]->GetRECT();
        
        mSelPosTexts[1] =
            guiHelper->CreateTextButton(graphics,
                                        cp0.R + mTextFieldHSpacing,
                                        //TEXT_FIELD_H_SPACING1,
                                        cp0.B + mTextFieldVSpacing,
                                        //TEXT_FIELD_V_SPACING,
                                        mFreqParamIdx,
                                        DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                                        FONT,
                                        valueColor, TEXT_ALIGN,
                                        0.0, 0.0, mBorderColor, mBorderWidth);
    
        // Selection size
        mSelSizeTexts[0] =
            guiHelper->CreateTextButton(graphics,
                                        mX + offsetX,
                                        sp0.B + mTextFieldVSpacing,
                                        //TEXT_FIELD_V_SPACING,
                                        mTimeParamIdx,
                                        DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                                        FONT,
                                        valueColor, TEXT_ALIGN,
                                        0.0, 0.0, mBorderColor, mBorderWidth);
        IRECT ss0 = mSelSizeTexts[0]->GetRECT();
        
        mSelSizeTexts[1] =
            guiHelper->CreateTextButton(graphics,
                                        ss0.R + mTextFieldHSpacing,
                                        //TEXT_FIELD_H_SPACING1,
                                        sp0.B + mTextFieldVSpacing,
                                        //TEXT_FIELD_V_SPACING,
                                        mFreqParamIdx,
                                        DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                                        FONT,
                                        valueColor, TEXT_ALIGN,
                                        0.0, 0.0, mBorderColor, mBorderWidth);
    }
    
    UpdateTextBGColor();
}

void
SpectroMeter::ClearUI()
{
    for (int i = 0; i < 2; i++)
        mCursorPosTexts[i] = NULL;

    for (int i = 0; i < 2; i++)
        mSelPosTexts[i] = NULL;

    for (int i = 0; i < 2; i++)
        mSelSizeTexts[i] = NULL; 
}

void
SpectroMeter::SetCursorPosition(BL_FLOAT timeX, BL_FLOAT freqY)
{
    mPrevCursorTimeX = timeX;
    mPrevCursorFreqY = freqY;
    
    freqY = AdjustFreq(freqY);
    
    if (mCursorPosTexts[0] != NULL)
    {
        char cpX[256];
        TimeToStr(timeX, cpX);
    
        mCursorPosTexts[0]->SetStr(cpX);
    }

    if (mCursorPosTexts[1] != NULL)
    {
        char cpY[256];
        FreqToStr(freqY, cpY);
        
        mCursorPosTexts[1]->SetStr(cpY);
    }
}

void
SpectroMeter::ResetCursorPosition()
{
    if (mCursorPosTexts[0] != NULL)
        mCursorPosTexts[0]->SetStr(DEFAULT_TEXT);

    if (mCursorPosTexts[1] != NULL)
        mCursorPosTexts[1]->SetStr(DEFAULT_TEXT);
}

void
SpectroMeter::SetSelectionValues(BL_FLOAT timeX, BL_FLOAT freqY,
                                 BL_FLOAT timeW, BL_FLOAT freqH)
{
    mSelectionActive = true;
    
    //
    mPrevSelTimeX = timeX;
    mPrevSelFreqY = freqY;
    
    mPrevSelTimeW = timeW;
    mPrevSelFreqH = freqH;

    //
    freqY = AdjustFreq(freqY);
    freqH = AdjustFreq(freqH);
    
    // Position
    if (mSelPosTexts[0] != NULL)
    {
        char spX[256];
        TimeToStr(timeX, spX);
    
        mSelPosTexts[0]->SetStr(spX);
    }

    if (mSelPosTexts[1] != NULL)
    {
        char spY[256];
        FreqToStr(freqY, spY);
        
        mSelPosTexts[1]->SetStr(spY);
    }

    // Size
    //
    if (mSelSizeTexts[0] != NULL)
    {
        char ssW[256];
        TimeToStr(timeW, ssW);
        
        mSelSizeTexts[0]->SetStr(ssW);
    }
    
    if (mSelSizeTexts[1] != NULL)
    {
        char ssH[256];
        FreqToStr(freqH, ssH);
    
        mSelSizeTexts[1]->SetStr(ssH);
    }
}

void
SpectroMeter::ResetSelectionValues()
{
    if (mSelPosTexts[0] != NULL)
        mSelPosTexts[0]->SetStr(DEFAULT_TEXT);

    if (mSelPosTexts[1] != NULL)
        mSelPosTexts[1]->SetStr(DEFAULT_TEXT);

    if (mSelSizeTexts[0] != NULL)
        mSelSizeTexts[0]->SetStr(DEFAULT_TEXT);

    if (mSelSizeTexts[1] != NULL)
        mSelSizeTexts[1]->SetStr(DEFAULT_TEXT);

    mSelectionActive = false;
}

void
SpectroMeter::SetTimeMode(TimeMode mode)
{
    mTimeMode = mode;

    RefreshValues();
}

void
SpectroMeter::SetFreqMode(FreqMode mode)
{
    mFreqMode = mode;

    RefreshValues();
}

void
SpectroMeter::SetTextFieldHSpacing(int spacing)
{
    mTextFieldHSpacing = spacing;
}

void
SpectroMeter::SetTextFieldVSpacing(int spacing)
{
    mTextFieldVSpacing = spacing;
}

void
SpectroMeter::SetBackgroundColor(const IColor &color)
{
    mBGColor = color;
}

void
SpectroMeter::SetBorderColor(const IColor &color)
{
    mBorderColor = color;
}

void
SpectroMeter::SetBorderWidth(float borderWidth)
{
    mBorderWidth = borderWidth;
}

void
SpectroMeter::UpdateTextBGColor()
{
    for (int i = 0; i < 2; i++)
    {
        if (mCursorPosTexts[i] != NULL)
            mCursorPosTexts[i]->SetTextBGColor(mBGColor);
    }

    for (int i = 0; i < 2; i++)
    {
        if (mSelPosTexts[i] != NULL)
            mSelPosTexts[i]->SetTextBGColor(mBGColor);
    }

    for (int i = 0; i < 2; i++)
    {
        if (mSelSizeTexts[i] != NULL)
            mSelSizeTexts[i]->SetTextBGColor(mBGColor);
    }
}

void
SpectroMeter::ConvertToHMS(BL_FLOAT timeSec,
                           int *h, int *m, int *s, int *ms)
{
    *h = 0;
    *m = 0;
    *s = 0;
    *ms = 0;

    BL_FLOAT h0 = timeSec/3600;
    if (h0 >= 1)
    {
        *h = (int)h0;

        timeSec -= *h*3600;
    }
    
    BL_FLOAT m0 = timeSec/60;
    if (m0 >= 1)
    {
        *m = (int)m0;

        timeSec -= *m*60;
    }

    BL_FLOAT s0 = timeSec;
    if (s0 >= 1)
    {
        *s = (int)s0;

        timeSec -= *s;
    }

    *ms = timeSec*1000;

    // Hack, See GraphTimeAxis6
    if (*ms < 0)
        *ms = -*ms;
    if (*ms >= 1000)
    {
        *s += *ms/1000;
        *ms -= (*ms/1000)*1000;
    }
}

void
SpectroMeter::TimeToStr(BL_FLOAT timeSec, char buf[256])
{
    if (mTimeMode == SPECTRO_METER_TIME_HMS)
        HMSStr(timeSec, buf);
    else if (mTimeMode == SPECTRO_METER_TIME_SAMPLES)
        SamplesStr(timeSec, buf);
}

void
SpectroMeter::FreqToStr(BL_FLOAT freqHz, char buf[256])
{
    // NOTE: add a whitespace at the end, so the text doesn't touch the right border
    
    if (mFreqMode == SPECTRO_METER_FREQ_HZ)
        sprintf(buf, "%g Hz  ", freqHz);
    else if (mFreqMode == SPECTRO_METER_FREQ_BIN)
    {
        int binNum = (freqHz/(mSampleRate*0.5))*(mBufferSize*0.5);
            
        // Don't knpw if we want "bin" or "bins"
        // => "bin" seems more neutral (e.g for position, this is "bin num"
        sprintf(buf, "%d bin  ", binNum);
    }
}

void
SpectroMeter::HMSStr(BL_FLOAT timeSec, char buf[256])
{
    int sign = 1;
    if (timeSec < 0.0)
    {
        timeSec = -timeSec;
        sign = -1;
    }
    
    int h;
    int m;
    int s;
    int ms;
    ConvertToHMS(timeSec, &h, &m, &s, &ms);

    const char *signStr = (sign >= 0) ? "" : "-";

    // NOTE: add a whitespace at the end, so the text doesn't touch the right border
    sprintf(buf, "%s%02d:%02d:%02d.%03d  ", signStr, h, m, s, ms);
}

void
SpectroMeter::SamplesStr(BL_FLOAT timeSec, char buf[256])
{
    int numSamples = timeSec*mSampleRate;

    // NOTE: add a whitespace at the end, so the text doesn't touch the right border
    sprintf(buf, "%d smp  ", numSamples);
}

BL_FLOAT
SpectroMeter::AdjustFreq(BL_FLOAT freq)
{
    // For frequencies, keep 1 digit
    freq = ((int)(freq*10))*0.1;

    return freq;
}

void
SpectroMeter::RefreshValues()
{
    // Update with prev values
    SetCursorPosition(mPrevCursorTimeX, mPrevCursorFreqY);

    if (mSelectionActive)
    {
        SetSelectionValues(mPrevSelTimeX, mPrevSelFreqY,
                           mPrevSelTimeW, mPrevSelFreqH);
    }
}
