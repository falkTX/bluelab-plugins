#include <IIconLabelControl.h>

IIconLabelControl::IIconLabelControl(float x, float y,
                                     float iconOffsetX, float iconOffsetY,
                                     float textOffsetX, float textOffsetY,
                                     const IBitmap &bgBitmap,
                                     const IBitmap &iconBitmap,
                                     const IText &text, int paramIdx, EBlend blend)
: IBitmapControl(x, y, bgBitmap, paramIdx, blend),
  mIconBitmap(iconBitmap),
  mIconOffsetX(iconOffsetX),
  mIconOffsetY(iconOffsetY),
  mTextOffsetX(textOffsetX),
  mTextOffsetY(textOffsetY),
  mTextControl(NULL),
  mIconNum(-1)
{
    const IRECT controlRect = GetRECT();
    
    IRECT textRect(controlRect.L + mTextOffsetX, controlRect.T + mTextOffsetY,
                   controlRect.R - mTextOffsetX,
                   controlRect.B - mTextOffsetY);

    IColor bgColor(0, 0, 0, 0);
    mTextControl = new ITextControl(textRect, "", text, bgColor);
}

IIconLabelControl::~IIconLabelControl()
{
    if (mTextControl != NULL)
        delete mTextControl;
}
    
void
IIconLabelControl::Draw(IGraphics& g)
{
    // Draw background
    IBitmapControl::Draw(g);

    // Draw text
    if (mTextControl != NULL)
        mTextControl->Draw(g);
    
    // Draw icon
    if (mIconNum != -1)
    {
        const IRECT controlRect = GetRECT();

        float x = controlRect.L + mIconOffsetX;
        float y = controlRect.T + mIconOffsetY;
            
        int w = mIconBitmap.W();
        int h = mIconBitmap.H()/mIconBitmap.N();

        IRECT iconRect(x, y, x + w, y + h);
        
        IBlend blend = GetBlend();
        g.DrawBitmap(mIconBitmap, iconRect, (int)mIconNum + 1, &blend);
    }
}

void
IIconLabelControl::SetLabelText(const char *text)
{
    if (mTextControl != NULL)
        mTextControl->SetStr(text);

    mDirty = true;
}

void
IIconLabelControl::SetIconNum(int iconNum)
{
    mIconNum = iconNum;

    mDirty = true;
}
