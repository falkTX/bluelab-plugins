#ifndef ITABS_BAR_CONTROL_H
#define ITABS_BAR_CONTROL_H

#include <vector>
using namespace std;

#include <IControl.h>

#include <BLUtilsFile.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class ITabsBarListener
{
public:
    virtual void OnTabSelected(int tabNum) = 0;
    virtual void OnTabClose(int tabNum) = 0;
};

class ITabsBarControl : public IControl
{
 public:
    ITabsBarControl(const IRECT &bounds, int paramIdx = kNoParameter);
    virtual ~ITabsBarControl();

    void SetListener(ITabsBarListener *listener);
    
    void Draw(IGraphics &g) override;
    void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    void OnMouseOver(float x, float y, const IMouseMod &mod) override;
    void OnMouseOut() override;
    
    void NewTab(const char *name);
    void SelectTab(int tabNum);
        
protected:
    void DrawBackground(IGraphics &g);
    void DrawTabs(IGraphics &g);
    void DrawCrosses(IGraphics &g);
    void DrawTabNames(IGraphics &g);
    
    void DisableAllRollover();
    
    int MouseOverCrossIdx(float x, float y);
    int MouseOverTabIdx(float x, float y);
    
    //
    // Bar background color 
    IColor mBGColor;
    // For the tab that is enabled
    IColor mTabEnabledColor;
    // For the tab that is not enabled
    IColor mTabDisabledColor;
    // Lines around tabs
    IColor mTabLinesColor;

    float mTabsLinesWidth;

    IColor mTabRolloverColor;
    
    IColor mCrossColor;
    float mCrossRatio;
    float mCrossLineWidth;

    IColor mCrossRolloverColor;

    IColor mNameColor;
    float mFontSize;
    
    //
    ITabsBarListener *mListener;

    class Tab
    {
    public:
        Tab(const char *name);
        Tab(const Tab &other);
        
        virtual ~Tab();

        const char *GetName() const;
        const char *GetShortName() const;
        
        void SetEnabled(bool flag);
        bool IsEnabled() const;

        // Rollover
        void SetTabRollover(bool flag);
        bool IsTabRollover() const;

        void SetCrossRollover(bool flag);
        bool IsCrossRollover() const;
        
    protected:        
        //
        char mName[FILENAME_SIZE];
        char mShortName[FILENAME_SIZE];

        bool mIsEnabled;

        bool mIsTabRollover;
        bool mIsCrossRollover;
    };

    vector<Tab> mTabs;
};

#endif /* ITABS_BAR_CONTROL_H */
