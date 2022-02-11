//
//  GUIHelpers3.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/06/17.
//
//

#ifndef __Transient__GUIHelpers3__
#define __Transient__GUIHelpers3__

#include "IPlug_include_in_plug_hdr.h"

#define GUI_OBJECTS_SORTING 1

#if GUI_OBJECTS_SORTING
#include <vector>
using namespace std;
#endif

#include <VuMeterControl.h>

// TODO: make a singleton from this class

// Title text
#define TITLE_TEXT_SIZE 20
#define TITLE_TEXT_OFFSET -15

#define TITLE_TEXT_COLOR_R 110
#define TITLE_TEXT_COLOR_G 110
#define TITLE_TEXT_COLOR_B 110

#define TITLE_TEXT_FONT "Tahoma"

// Value text
#define VALUE_TEXT_SIZE 14
#define VALUE_TEXT_OFFSET 3

#define VALUE_TEXT_OFFSET2 11

#define VALUE_TEXT_COLOR_R 240
#define VALUE_TEXT_COLOR_G 240
#define VALUE_TEXT_COLOR_B 255

#define VALUE_TEXT_FONT "Tahoma"

// Radio label text
#define RADIO_LABEL_TEXT_SIZE 15
#define RADIO_LABEL_TEXT_OFFSET 6

#define RADIO_LABEL_TEXT_COLOR_R 110
#define RADIO_LABEL_TEXT_COLOR_G 110
#define RADIO_LABEL_TEXT_COLOR_B 110

#define RADIO_LABEL_TEXT_FONT "Tahoma"

// Version text
#define VERSION_TEXT_SIZE 12
#define VERSION_TEXT_OFFSET 3

#define VERSION_TEXT_COLOR_R 110
#define VERSION_TEXT_COLOR_G 110
#define VERSION_TEXT_COLOR_B 110

#define VERSION_TEXT_FONT "Tahoma"

class GraphControl6;


// GUIHelpers3, for version 3.0 of the plugins (new interface)
// Compute as much as possible graphics, instead of getting them from png
// Factorization for widget creation
class GUIHelpers3
{
public:
    // If GUI_OBJECTS_SORTING is set, add all the created objects, in the correct order
    static void AddAllObjects(IGraphics *graphics);
    
    static void DirtyAllObjects();
    
    static ITextControl *CreateValueText(IPlug *plug, IGraphics *graphics,
                                         int width, int height, int x, int y,
                                         const char *title);
    
    // Create a knob with text. The text can be added to the graphics after, using AddText()
    static IKnobMultiControl *CreateKnob(IPlug *plug, IGraphics *graphics,
                                         int bmpId, const char *bmpFn, int bmpFrames,
                                         int x, int y, int param,
                                         const char *title,
                                         int shadowId = -1, const char *shadowFn = NULL,
                                         int diffuseId = -1, const char *diffuseFn = NULL,
                                         int textFieldId = -1, const char *textFieldFn = NULL,
                                         IText **valueText = NULL,
                                         bool addValueText = true,
                                         ITextControl **ioValueTextControl = NULL,
                                         int yoffset = 0);
    
    static IKnobMultiControl *CreateEnumKnob(IPlug *plug,
                                             IGraphics *graphics,
                                             IBitmap *bitmap,
                                             int x, int y, int param, BL_FLOAT sensivity,
                                             const char *title, IText **text = NULL,
                                             int yoffset = 0);
    
    static IRadioButtonsControl *
                    CreateRadioButtons(IPlug *plug, IGraphics *graphics,
                                       int bmpId, const char *bmpFn, int bmpFrames,
                                       int x, int y, int numButtons, int size, int param,
                                       bool horizontalFlag, const char *title,
                                       int diffuseId, const char *diffuseFn,
                                       IText::EAlign titleAlign = IText::kAlignCenter,
                                       const char **radioLabels = NULL, int numRadioLabels = 0);
    
    static IToggleButtonControl *CreateToggleButton(IPlug *plug,
                                                    IGraphics *graphics, IBitmap *bitmap,
                                                    int x, int y, int param, const char *title);
    
    static VuMeterControl *CreateVuMeter(IPlug *plug, IGraphics *graphics, IBitmap *bitmap,
                                         int x, int y, int param, const char *title,
                                         bool alignLeft);
    
    // Graph
#if 0 // Old
    static GraphControl4 *CreateGraph4(IPlug *plug, IGraphics *graphics,
                                       int bmpId, const char *bmpFn, int bmpFrames,
                                       int x , int y, int numCurves, int numPoints);
#endif
    
    static GraphControl6 *CreateGraph6(IPlug *plug, IGraphics *graphics,
                                       int bmpId, const char *bmpFn, int bmpFrames,
                                       int x , int y, int numCurves, int numPoints);
    
    // Version
    static void CreateVersion(IPlug *plug, IGraphics *graphics, const char *versionStr);
    
    // Generic bitmap
    static IBitmapControl *CreateBitmap(IPlug *plug, IGraphics *graphics,
                                        int x, int y, int bmpId, const char *bmpFn);
    
    // Shadows
    static IBitmapControl *CreateShadows(IPlug *plug, IGraphics *graphics,
                                         int bmpId, const char *bmpFn);
    
    // Logo
    static IBitmapControl *CreateLogo(IPlug *plug, IGraphics *graphics,
                                      int bmpId, const char *bmpFn);
    
    // Plugin name
    static IBitmapControl *CreatePlugName(IPlug *plug, IGraphics *graphics,
                                          int bmpId, const char *bmpFn);
    
    // Call it after a parameter change
    static void UpdateText(IPlug *plug, int paramIdx);
    
protected:
    static ITextControl *AddTitleText(IPlug *plug, IGraphics *graphics,
                                      IBitmap *bitmap, IText *text, const char *title,
                                      int x, int y);
   
    static ITextControl *AddValueText(IPlug *plug, IGraphics *graphics,
                                      IBitmap *bitmap, IText *text, int x, int y, int param);

    static ITextControl *AddRadioLabelText(IPlug *plug, IGraphics *graphics,
                                           IBitmap *bitmap, IText *text, const char *labelStr,
                                           int x, int y);
    
    static ITextControl *AddVersionText(IPlug *plug, IGraphics *graphics,
                                        IText *text, const char *versionStr, int x, int y);
    
    static IBitmapControl *CreateShadow(IPlug *plug, IGraphics *graphics,
                                        int bmpId, const char *bmpFn,
                                        int x, int y, IBitmap *objectBitmap,
                                        int nObjectFrames);
    
    static IBitmapControl *CreateDiffuse(IPlug *plug, IGraphics *graphics,
                                         int bmpId, const char *bmpFn,
                                         int x, int y, IBitmap *objectBitmap,
                                         int nObjectFrames);
    
    static bool CreateMultiDiffuse(IPlug *plug, IGraphics *graphics,
                                   int bmpId, const char *bmpFn,
                                   const WDL_TypedBuf<IRECT> &rects,
                                   IBitmap *objectBitmap,
                                   int nObjectFrames,
                                   vector<IBitmapControlExt *> *diffuseBmps);
    
    static IBitmapControl *CreateTextField(IPlug *plug, IGraphics *graphics,
                                          int bmpId, const char *bmpFn,
                                          int x, int y, IBitmap *objectBitmap,
                                          int nObjectFrames);
    
    static int GetTextOffset();
    
public:
    static IColor mValueTextColor;
    
protected:
    static IColor mTitleTextColor;
    static IColor mRadioLabelTextColor;
    static IColor mVersionTextColor;
    
#if GUI_OBJECTS_SORTING
    static vector<IControl *> mBackObjects;
    static vector<IControl *> mWidgets;
    static vector<IControl *> mTexts;
    static vector<IControl *> mDiffuse;
    static vector<IControl *> mShadows;
#endif
};

#endif /* defined(__Transient__GUIHelpers3__) */
