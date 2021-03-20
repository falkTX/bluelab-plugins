#include <IControls.h>

#include <GUIHelper12.h>

#include "GhostMeter.h"

#define TEXT_FIELD_V_SIZE 14 //16 //40

#define TEXT_FIELD_H_SPACING1 10
#define TEXT_FIELD_H_SPACING2 10

#define TEXT_FIELD_V_SPACING 10

#define FONT "Roboto-Bold"

#define DEFAULT_TEXT "------------------"

//#define TEXT_ALIGN EAlign::Center
#define TEXT_ALIGN EAlign::Far

GhostMeter::GhostMeter(BL_FLOAT x, BL_FLOAT y,
                       BL_FLOAT sampleRate)
{
    //mMode = GHOST_METER_HMS;
    mMode = GHOST_METER_SAMPLES;

    mSampleRate = sampleRate;
    
    mX = x;
    mY = y;
    
    ClearUI();
}

GhostMeter::~GhostMeter() {}

void
GhostMeter::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

void
GhostMeter::GenerateUI(GUIHelper12 *guiHelper,
                       IGraphics *graphics)
{
    IColor valueColor;
    guiHelper->GetValueTextColor(&valueColor);

    // Cursor pos
    mCursorPosTexts[0] =
        guiHelper->CreateText(graphics,
                              mX, mY,
                              DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                              FONT,
                              valueColor, TEXT_ALIGN);

    IRECT cp0 = mCursorPosTexts[0]->GetRECT();
    mCursorPosTexts[1] =
        guiHelper->CreateText(graphics,
                              cp0.R + TEXT_FIELD_H_SPACING1, mY,
                              DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                              FONT,
                              valueColor, TEXT_ALIGN);

    // Selection pos
    mSelPosTexts[0] =
        guiHelper->CreateText(graphics,
                              mX, cp0.B + TEXT_FIELD_V_SPACING,
                              DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                              FONT,
                              valueColor, TEXT_ALIGN);
    IRECT sp0 = mSelPosTexts[0]->GetRECT();
    
    mSelPosTexts[1] =
        guiHelper->CreateText(graphics,
                              cp0.R + TEXT_FIELD_H_SPACING1,
                              cp0.B + TEXT_FIELD_V_SPACING,
                              DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                              FONT,
                              valueColor, TEXT_ALIGN);
    
    // Selection size
    mSelSizeTexts[0] =
        guiHelper->CreateText(graphics,
                              mX,
                              sp0.B + TEXT_FIELD_V_SPACING,
                              DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                              FONT,
                              valueColor, TEXT_ALIGN);
    IRECT ss0 = mSelSizeTexts[0]->GetRECT();
    
    mSelSizeTexts[1] =
        guiHelper->CreateText(graphics,
                              ss0.R + TEXT_FIELD_H_SPACING1,
                              sp0.B + TEXT_FIELD_V_SPACING,
                              DEFAULT_TEXT, TEXT_FIELD_V_SIZE,
                              FONT,
                              valueColor, TEXT_ALIGN);

    UpdateTextBGColor();
}

void
GhostMeter::ClearUI()
{
    for (int i = 0; i < 2; i++)
        mCursorPosTexts[i] = NULL;

    for (int i = 0; i < 2; i++)
        mSelPosTexts[i] = NULL;

    for (int i = 0; i < 2; i++)
        mSelSizeTexts[i] = NULL; 
}

void
GhostMeter::SetCursorPosition(BL_FLOAT x, BL_FLOAT y)
{
    y = AdjustFreq(y);
    
    if (mCursorPosTexts[0] != NULL)
    {
        char cpX[256];
        TimeToStr(x, cpX);
        //HMSStr(x, cpX);
    
        mCursorPosTexts[0]->SetStr(cpX);
    }

    if (mCursorPosTexts[1] != NULL)
    {
        char cpY[256];
        sprintf(cpY, "%g Hz", y);
    
        mCursorPosTexts[1]->SetStr(cpY);
    }
}

void
GhostMeter::ResetCursorPosition()
{
    if (mCursorPosTexts[0] != NULL)
        mCursorPosTexts[0]->SetStr(DEFAULT_TEXT);

    if (mCursorPosTexts[1] != NULL)
        mCursorPosTexts[1]->SetStr(DEFAULT_TEXT);
}

void
GhostMeter::SetSelectionValues(BL_FLOAT x, BL_FLOAT y,
                               BL_FLOAT w, BL_FLOAT h)
{
    y = AdjustFreq(y);
    h = AdjustFreq(h);
    
    // Position
    if (mSelPosTexts[0] != NULL)
    {
        char spX[256];
        //HMSStr(x, spX);
        TimeToStr(x, spX);
    
        mSelPosTexts[0]->SetStr(spX);
    }

    if (mSelPosTexts[1] != NULL)
    {
        char spY[256];
        sprintf(spY, "%g Hz", y);
    
        mSelPosTexts[1]->SetStr(spY);
    }

    // Size
    //
    if (mSelSizeTexts[0] != NULL)
    {
        char ssW[256];
        //HMSStr(w, ssW);
        TimeToStr(w, ssW);
        
        mSelSizeTexts[0]->SetStr(ssW);
    }
    
    if (mSelSizeTexts[1] != NULL)
    {
        char ssH[256];
        sprintf(ssH, "%g Hz", h);
    
        mSelSizeTexts[1]->SetStr(ssH);
    }
}

void
GhostMeter::ResetSelectionValues()
{
    if (mSelPosTexts[0] != NULL)
        mSelPosTexts[0]->SetStr(DEFAULT_TEXT);

    if (mSelPosTexts[1] != NULL)
        mSelPosTexts[1]->SetStr(DEFAULT_TEXT);

    if (mSelSizeTexts[0] != NULL)
        mSelSizeTexts[0]->SetStr(DEFAULT_TEXT);

    if (mSelSizeTexts[1] != NULL)
        mSelSizeTexts[1]->SetStr(DEFAULT_TEXT);
}

void
GhostMeter::UpdateTextBGColor()
{
    IColor color(255, 64, 64, 64);

    for (int i = 0; i < 2; i++)
        mCursorPosTexts[i]->SetTextBGColor(color);

    for (int i = 0; i < 2; i++)
        mSelPosTexts[i]->SetTextBGColor(color);

    for (int i = 0; i < 2; i++)
        mSelSizeTexts[i]->SetTextBGColor(color); 
}

void
GhostMeter::ConvertToHMS(BL_FLOAT timeSec,
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
GhostMeter::TimeToStr(BL_FLOAT timeSec, char buf[256])
{
    if (mMode == GHOST_METER_HMS)
        HMSStr(timeSec, buf);
    if (mMode == GHOST_METER_SAMPLES)
        SamplesStr(timeSec, buf);
}

void
GhostMeter::HMSStr(BL_FLOAT timeSec, char buf[256])
{
    int h;
    int m;
    int s;
    int ms;
    ConvertToHMS(timeSec, &h, &m, &s, &ms);

    sprintf(buf, "%02d:%02d:%02d.%03d", h, m, s, ms);
}

void
GhostMeter::SamplesStr(BL_FLOAT timeSec, char buf[256])
{
    int numSamples = timeSec*mSampleRate;

    sprintf(buf, "%d smp", numSamples);
}

BL_FLOAT
GhostMeter::AdjustFreq(BL_FLOAT freq)
{
    // For frequencies, keep 1 digit
    freq = ((int)(freq*10))*0.1;

    return freq;
}
