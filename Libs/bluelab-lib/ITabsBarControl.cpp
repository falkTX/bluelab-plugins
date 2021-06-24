#include "ITabsBarControl.h"

#undef FillRect
#undef DrawText

//
// Tab
ITabsBarControl::Tab::Tab(const char *name)
{
    memset(mName, '\0', FILENAME_SIZE);
    if (name != NULL)
        strcpy(mName, name);

    memset(mShortName, '\0', FILENAME_SIZE);
    if (name != NULL)
    {
        char *shortName = BLUtilsFile::GetFileName(name);
        strcpy(mShortName, shortName);
    }
    
    mIsEnabled = false;
    
    mIsTabRollover = false;
    mIsCrossRollover = false;
}

ITabsBarControl::Tab::Tab(const Tab &other)
{
    memset(mName, '\0', FILENAME_SIZE);;
    strcpy(mName, other.mName);

    memset(mShortName, '\0', FILENAME_SIZE);;
    strcpy(mShortName, other.mShortName);
    
    mIsEnabled = other.mIsEnabled;

    mIsTabRollover = other.mIsTabRollover;
    mIsCrossRollover = other.mIsCrossRollover;
}
        
ITabsBarControl::Tab::~Tab() {}

const char *
ITabsBarControl::Tab::GetName() const
{
    return mName;
}

const char *
ITabsBarControl::Tab::GetShortName() const
{
    return mShortName;
}

void
ITabsBarControl::Tab::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

bool
ITabsBarControl::Tab::IsEnabled() const
{
    return mIsEnabled;
}

void
ITabsBarControl::Tab::SetTabRollover(bool flag)
{
    mIsTabRollover = flag;
}

bool
ITabsBarControl::Tab::IsTabRollover() const
{
    return mIsTabRollover;
}

void
ITabsBarControl::Tab::SetCrossRollover(bool flag)
{
    mIsCrossRollover = flag;
}

bool
ITabsBarControl::Tab::IsCrossRollover() const
{
    return mIsCrossRollover;
}

//
// ITabsBarControl

ITabsBarControl::ITabsBarControl(const IRECT& bounds, int paramIdx)
: IControl(bounds, paramIdx)
{
    mListener = NULL;

    mBGColor = IColor(255, 128, 128, 128);
    mTabEnabledColor = IColor(255, 200, 200, 200);
    mTabDisabledColor = IColor(255, 128, 128, 128);
    mTabLinesColor = IColor(255, 4, 4, 4);
    mTabsLinesWidth = 1.0;

    mTabRolloverColor = IColor(255, 164, 164, 164);
    
    mCrossColor = IColor(255, 4, 4, 4);
    mCrossRatio = 0.4; //0.5;
    mCrossLineWidth = 1.0;

    mCrossRolloverColor = IColor(255, 128, 128, 128);

    mNameColor = IColor(255, 255, 255, 255);
    mFontSize = 14.0;
}
    
ITabsBarControl::~ITabsBarControl() {}

void
ITabsBarControl::SetListener(ITabsBarListener *listener)
{
    mListener = listener;
}

void
ITabsBarControl::Draw(IGraphics& g)
{    
    DrawBackground(g);
    DrawTabs(g);
    DrawCrosses(g);
    DrawTabNames(g);
}

void
ITabsBarControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
    // Chck if we clicked a cross, then in this case, close the corresponding tab
    int tabToClose = MouseOverCrossIdx(x, y);
    
    if (tabToClose >= 0)
        // Clicked on a cross
    {
        if (mListener != NULL)
            mListener->OnTabClose(tabToClose);

        if (mTabs[tabToClose].IsEnabled())
            // Transmit enabled state before closing
        {
            // On the right by default
            if (tabToClose + 1 <= mTabs.size() - 1)
                mTabs[tabToClose + 1].SetEnabled(true);
            // Then try on the left otherwise
            else if (tabToClose - 1 >= 0)
                mTabs[tabToClose - 1].SetEnabled(true);
        }
        
        mTabs.erase(mTabs.begin() + tabToClose);
        mDirty = true;

        // Clicked on a cross done
        return;
    }

    // Check if we clicked on a tab (an not on a cross)
    int tabClicked = MouseOverTabIdx(x, y);
    if (tabClicked >= 0)
    {
        if (mListener != NULL)
            mListener->OnTabSelected(tabClicked);
        
        // Disable all the tabs
        for (int i = 0; i < mTabs.size(); i++)
        {
            Tab &t0 = mTabs[i];
            t0.SetEnabled(false);
        }

        mTabs[tabClicked].SetEnabled(true);

        mDirty = true;

        // Clicked on a tab to enable it: done 
        return;
    }
}

// For rollover
void
ITabsBarControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
    // Disable all tab rolloverq and cross rollovers
    DisableAllRollover();
    
    // Check tab rollover
    int tabIdx = MouseOverTabIdx(x, y);
    if (tabIdx >= 0)
    {
        mTabs[tabIdx].SetTabRollover(true);

        const char *fileName = mTabs[tabIdx].GetName();
        SetTooltip(fileName);
    }
    else
    {
        SetTooltip(NULL);
    }

    // Check cross rollover
    int crossIdx = MouseOverCrossIdx(x, y);
    if (crossIdx >= 0)
        mTabs[tabIdx].SetCrossRollover(true);

    mDirty = true;
}

void
ITabsBarControl::OnMouseOut()
{
    DisableAllRollover();
}

void
ITabsBarControl::NewTab(const char *name)
{
    // Disable all current tabs, and enable the new tab
    for (int i = 0; i < mTabs.size(); i++)
    {
        Tab &t0 = mTabs[i];
        t0.SetEnabled(false);
    }
    
    Tab t(name);
    t.SetEnabled(true);
    
    mTabs.push_back(t);

    mDirty = true;
}

void
ITabsBarControl::SelectTab(int tabNum)
{
    if (tabNum >= mTabs.size())
        return;

    // Disable all current tabs, and enable the new tab
    for (int i = 0; i < mTabs.size(); i++)
    {
        Tab &t0 = mTabs[i];
        t0.SetEnabled(false);
    }

    mTabs[tabNum].SetEnabled(true);

    mDirty = true;
}

void
ITabsBarControl::SetBackgroundColor(const IColor &color)
{
    mBGColor = color;
}

void
ITabsBarControl::SetTabEnabledColor(const IColor &color)
{
    mTabEnabledColor = color;
}
    
void
ITabsBarControl::SetTabDisabledColor(const IColor &color)
{
    mTabDisabledColor = color;
}
    
void
ITabsBarControl::SetTabLinesColor(const IColor &color)
{
    mTabLinesColor = color;
}

void
ITabsBarControl::SetTabsRolloverColor(const IColor &color)
{
    mTabRolloverColor = color;
}

void
ITabsBarControl::SetCrossColor(const IColor &color)
{
    mCrossColor = color;
}

void
ITabsBarControl::SetCrossRolloverColor(const IColor &color)
{
    mCrossRolloverColor = color;
}

void
ITabsBarControl::SetNameColor(const IColor &color)
{
    mNameColor = color;
}

void
ITabsBarControl::SetCrossLineWidth(float width)
{
    mCrossLineWidth = width;
}

void
ITabsBarControl::DrawBackground(IGraphics &g)
{
    g.FillRect(mBGColor, mRECT);
}

void
ITabsBarControl::DrawTabs(IGraphics &g)
{
    if (mTabs.empty())
        return;
    
    float tabSize = mRECT.W()/mTabs.size();
    for (int i = 0; i < mTabs.size(); i++)
    {
        const Tab &t = mTabs[i];
        
        IRECT rect(mRECT.L + i*tabSize, mRECT.T, mRECT.L + (i + 1)*tabSize, mRECT.B);
        
        if (t.IsEnabled())
            g.FillRect(mTabEnabledColor, rect);
        else
        {
            if (t.IsTabRollover())
                g.FillRect(mTabRolloverColor, rect);
            else
                g.FillRect(mTabDisabledColor, rect);
        }

        g.DrawRect(mTabLinesColor, rect, NULL, mTabsLinesWidth);
    }
}

void
ITabsBarControl::DrawCrosses(IGraphics &g)
{
    if (mTabs.empty())
        return;

    float tabSize = mRECT.W()/mTabs.size();
    float crossSize = mRECT.H()*mCrossRatio;
    float marginRight = (mRECT.H() - crossSize)*0.5;
    
    for (int i = 0; i < mTabs.size(); i++)
    {
        const Tab &t = mTabs[i];
        
        // Center
        float c = (i + 1)*tabSize - crossSize*0.5 - marginRight;

        IColor col = mCrossColor;
        if (t.IsCrossRollover())
            col = mCrossRolloverColor;
        
        // Draw the cross (2 lines)
        g.DrawLine(col,
                   mRECT.L + c - crossSize*0.5,
                   mRECT.T + mRECT.H()*0.5 - crossSize*0.5,
                   mRECT.L + c + crossSize*0.5,
                   mRECT.T + mRECT.H()*0.5 + crossSize*0.5,
                   NULL, mCrossLineWidth);

        g.DrawLine(col,
                   mRECT.L + c - crossSize*0.5,
                   mRECT.T + mRECT.H()*0.5 + crossSize*0.5,
                   mRECT.L + c + crossSize*0.5,
                   mRECT.T + mRECT.H()*0.5 - crossSize*0.5,
                   NULL, mCrossLineWidth);
    }
}

int
ITabsBarControl::MouseOverCrossIdx(float x, float y)
{
    // Check if we are on a cross
    float tabSize = mRECT.W()/mTabs.size();
    float crossSize = mRECT.H()*mCrossRatio;
    float marginRight = (mRECT.H() - crossSize)*0.5;

    int crossIdx = -1;
    for (int i = 0; i < mTabs.size(); i++)
    {
        // Center
        float c = (i + 1)*tabSize - crossSize*0.5 - marginRight;

        IRECT crossRect(mRECT.L + c - crossSize*0.5,
                        mRECT.T + mRECT.H()*0.5 - crossSize*0.5,
                        mRECT.L + c + crossSize*0.5,
                        mRECT.T + mRECT.H()*0.5 + crossSize*0.5);
                        
        if (crossRect.Contains(x, y))
        {
            crossIdx = i;
            break;
        }
    }

    return crossIdx;
}

int
ITabsBarControl::MouseOverTabIdx(float x, float y)
{
    float tabSize = mRECT.W()/mTabs.size();
    
    int tabIdx = -1;
    for (int i = 0; i < mTabs.size(); i++)
    {
        IRECT tabRect(mRECT.L + i*tabSize, mRECT.T,
                      mRECT.L + (i + 1)*tabSize, mRECT.B);
        
        if (tabRect.Contains(x, y))
        {
            tabIdx = i;
            break;
        }
    }

    return tabIdx;
}

void
ITabsBarControl::DrawTabNames(IGraphics &g)
{
    float tabSize = mRECT.W()/mTabs.size();
    float y = mRECT.T + mRECT.H()*0.5;
    float margin = mRECT.H()*0.5;
    
    IText text(mFontSize, mNameColor, "Roboto-Bold", EAlign::Near);
    // Make the text to be clipped by the tab bounds
    text.mClipToBounds = true;
    
    float crossSize = mRECT.H()*mCrossRatio;
    float marginRight = (mRECT.H() - crossSize)*0.5;

    float marginRight2 = margin*0.5;
    
    for (int i = 0; i < mTabs.size(); i++)
    {
        const Tab &t = mTabs[i];
        //const char *name = t.GetName();
        const char *name = t.GetShortName();

        //float x = mRECT.L + i*tabSize + margin;
        //g.DrawText(text, name, x, y);

        IRECT rect(mRECT.L + i*tabSize + margin,
                   mRECT.T,
                   mRECT.L + (i + 1)*tabSize - crossSize - marginRight - marginRight2,
                   mRECT.B);
        g.DrawText(text, name, rect);
    }
}

void
ITabsBarControl::DisableAllRollover()
{
    bool isRollover = false;
    for (int i = 0; i < mTabs.size(); i++)
    {
        Tab &t = mTabs[i];
        if (t.IsTabRollover())
        {
            isRollover = true;
            break;
        }

        if (t.IsCrossRollover())
        {
            isRollover = true;
            break;
        }
    }
    if (!isRollover)
        return;
    
    for (int i = 0; i < mTabs.size(); i++)
    {
        Tab &t = mTabs[i];
        t.SetTabRollover(false);
        t.SetCrossRollover(false);
    }

    mDirty = true;
}
