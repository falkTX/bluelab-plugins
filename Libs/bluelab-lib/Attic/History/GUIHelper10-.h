 //
//  GUIHelper10.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/06/17.
//
//

#ifndef __Transient__GUIHelper10__
#define __Transient__GUIHelper10__

#include "IPlug_include_in_plug_hdr.h"

#define GUI_OBJECTS_SORTING 1

#if GUI_OBJECTS_SORTING
#include <vector>
using namespace std;
#endif

#include <VuMeterControl.h>

#include "resource.h"

// #bl-iplug2
#define USE_GRAPH_OGL 1

// #bl-iplug2
//#ifndef USE_GRAPH_OGL
//Error, USE_GRAPH_OGL must be defined (0 or 1)
//#endif

// The macro USE_GRAPH_OGL is used to disable graph and open for plugin that does not use it
// Otherwise, we must add GraphControl, Graphcurve, GLContext
// and add nanovg, liceglew, glfw
// and link to about 4 system frameworks (opengl etc.)

// GUIHelper4: from GUIHelpers3, but no more static fields
// GUIHelper5: for GraphControl7
// GUIHelper6: for GraphControl8
// GUIHelper7: for file selector control
//             and for GraphControl9
// GUIHelper8: for GraphControl10 (SpectrogramDisplay)
//
// GUIHelper9: TODO:
// - rollover buttons
// - alt-click or double-click to reset parameter
// - editable text fields
//
// GuiHelper10: for UST (Ultimate Stero Tool)

// Origin
#ifndef WIN32 // Mac
// It seems that on mac, the font used is the default font: "Monaco"
// On Mac, this is good, on Windows if replace "Tahoma" by "Monaco",
// "bold" seems to not exist for some font size
#define TITLE_TEXT_FONT  "Tahoma"

// Title text
#define TITLE_TEXT_SIZE 20
#define TITLE_TEXT_OFFSET_Y -15

#define TITLE_TEXT_OFFSET_X 15

#define TITLE_TEXT_COLOR_R 110
#define TITLE_TEXT_COLOR_G 110
#define TITLE_TEXT_COLOR_B 110

// Value text
#define VALUE_TEXT_SIZE 14
#define VALUE_TEXT_OFFSET 3

#define VALUE_TEXT_OFFSET2 11
#define VALUE2_TEXT_OFFSET 6

#define VALUE_TEXT_COLOR_R 240
#define VALUE_TEXT_COLOR_G 240
#define VALUE_TEXT_COLOR_B 255

#define VALUE_TEXT_FONT "Tahoma"

// Radio label text
#define RADIO_LABEL_TEXT_SIZE 15
#define RADIO_LABEL_TEXT_OFFSET 6

#define RADIO_LABEL_TEXT_COLOR_R 100
#define RADIO_LABEL_TEXT_COLOR_G 100
#define RADIO_LABEL_TEXT_COLOR_B 161

#define RADIO_LABEL_TEXT_FONT "Tahoma"

// Version text
#define VERSION_TEXT_SIZE 12
#define VERSION_TEXT_OFFSET 3

#define VERSION_TEXT_COLOR_R 110
#define VERSION_TEXT_COLOR_G 110
#define VERSION_TEXT_COLOR_B 110

#define VERSION_TEXT_FONT "Tahoma"

#define HILIGHT_TEXT_COLOR_R 248
#define HILIGHT_TEXT_COLOR_G 248
#define HILIGHT_TEXT_COLOR_B 248

#define EDIT_TEXT_COLOR_R 42
#define EDIT_TEXT_COLOR_G 42
#define EDIT_TEXT_COLOR_B 42

// For "small" text
#define SMALL_SIZE_RATIO 0.65 // 0.5

// For "tiny" text
#define TINY_SIZE_RATIO 0.55 //0.55

// For "huge" text
#define HUGE_SIZE_RATIO 1.2

#endif

#ifdef WIN32
// It seems that on mac, the font used is the default font: "Monaco"
// On Mac, this is good, on Windows if replace "Tahoma" by "Monaco",
// "bold" seems to not exist for some font size
#define TITLE_TEXT_FONT  "Verdana"

// Title text
#define TITLE_TEXT_SIZE 18 //20
#define TITLE_TEXT_OFFSET_Y -15
#define TITLE_TEXT_OFFSET_X 15

#define TITLE_TEXT_COLOR_R 110
#define TITLE_TEXT_COLOR_G 110
#define TITLE_TEXT_COLOR_B 110

// Value text
#define VALUE_TEXT_SIZE 14
#define VALUE_TEXT_OFFSET 3

#define VALUE_TEXT_OFFSET2 11
#define VALUE2_TEXT_OFFSET 6

#define VALUE_TEXT_COLOR_R 240
#define VALUE_TEXT_COLOR_G 240
#define VALUE_TEXT_COLOR_B 255

#define VALUE_TEXT_FONT "Verdana"

// Radio label text
#define RADIO_LABEL_TEXT_SIZE 15
#define RADIO_LABEL_TEXT_OFFSET 6

#define RADIO_LABEL_TEXT_COLOR_R 100
#define RADIO_LABEL_TEXT_COLOR_G 100
#define RADIO_LABEL_TEXT_COLOR_B 161

#define RADIO_LABEL_TEXT_FONT "Verdana"

// Version text
#define VERSION_TEXT_SIZE 12
#define VERSION_TEXT_OFFSET 3

#define VERSION_TEXT_COLOR_R 110
#define VERSION_TEXT_COLOR_G 110
#define VERSION_TEXT_COLOR_B 110

#define VERSION_TEXT_FONT "Verdana"

#define HILIGHT_TEXT_COLOR_R 248
#define HILIGHT_TEXT_COLOR_G 248
#define HILIGHT_TEXT_COLOR_B 248

#define EDIT_TEXT_COLOR_R 42
#define EDIT_TEXT_COLOR_G 42
#define EDIT_TEXT_COLOR_B 42

// For "small" text
#define SMALL_SIZE_RATIO 0.71 //0.65 // 0.5

// TODO: check well this value (for UST, Windows)
// For "tiny" text
#define TINY_SIZE_RATIO 0.58

// TODO: check well this value (for UST, Windows)
// For "huge" text
#define HUGE_SIZE_RATIO 1.2

#endif


// Plug name
//#define PLUG_NAME_OFFSET_X -6
//#define PLUG_NAME_OFFSET_X 3
#define PLUG_NAME_OFFSET_X 5
#define PLUG_NAME_OFFSET_Y 6

#define LOGO_OFFSET_Y 1

#define TRIAL_Y_OFFSET 7

#define HELP_BUTTON_X_OFFSET 44
#define HELP_BUTTON_Y_OFFSET 4

#define FILE_SELECTOR_TEXT_OFFSET_X 3
#define FILE_SELECTOR_TEXT_OFFSET_Y 3

#define BUTTON_LABEL_TEXT_OFFSET_X 3
#define BUTTON_LABEL_TEXT_OFFSET_Y 3

using namespace iplug;
using namespace iplug::igraphics;

class GraphControl11;

// GUIHelpers3, for version 3.0 of the plugins (new interface)
// Compute as much as possible graphics, instead of getting them from png
// Factorization for widget creation
class GUIHelper10
{
public:
    enum Position
    {
        TOP,
        BOTTOM,
        LOWER_LEFT,
        LOWER_RIGHT
    };
    
    enum Size
    {
        SIZE_STANDARD,
        SIZE_SMALL,
        SIZE_TINY,
        SIZE_HUGE
    };
    
    enum TitlePosition
    {
        TITLE_TOP,
        TITLE_RIGHT
    };
    
    GUIHelper10();
    
    virtual ~GUIHelper10();
    
    // If GUI_OBJECTS_SORTING is set, add all the created objects, in the correct order
    void AddAllObjects(IGraphics *graphics);
    

    ITextControl *CreateValueText(Plugin *plug, IGraphics *graphics,
                                  int width, int height, int x, int y,
                                  const char *title);
    
    // #bl-iplug2
#if 0
    // Create a knob with text. The text can be added to the graphics after, using AddText()
    // Create title, then knob, then value
    IKnobMultiControl *CreateKnob(Plugin *plug, IGraphics *graphics,
                                  int bmpId, const char *bmpFn, int bmpFrames,
                                  int x, int y, int param,
                                  const char *title,
                                  int shadowId = -1, const char *shadowFn = NULL,
                                  int diffuseId = -1, const char *diffuseFn = NULL,
                                  int textFieldId = -1, const char *textFieldFn = NULL,
                                  Size size = SIZE_STANDARD,
                                  IText **valueText = NULL,
                                  bool addValueText = true,
                                  ITextControl **ioValueTextControl = NULL,
                                  int yoffset = 0, bool doubleTextField = false,
                                  BL_FLOAT sensivity = 1.0);
    
    // Create knob, then title, then value text
    // (for Spatializer)
    IKnobMultiControl *CreateKnob2(Plugin *plug, IGraphics *graphics,
                                   int bmpId, const char *bmpFn, int bmpFrames,
                                   int x, int y, int param,
                                   const char *title,
                                   int shadowId = -1, const char *shadowFn = NULL,
                                   int diffuseId = -1, const char *diffuseFn = NULL,
                                   int textFieldId = -1, const char *textFieldFn = NULL,
                                   IText **valueText = NULL,
                                   bool addValueText = true,
                                   ITextControl **ioValueTextControl = NULL,
                                   int yoffset = 0, bool BL_FLOATTextField = false,
                                   BL_FLOAT sensivity = 1.0);
    
    // From CreateKnob, for editable text value
    IKnobMultiControl *CreateKnob3(Plugin *plug, IGraphics *graphics,
                                   int bmpId, const char *bmpFn, int bmpFrames,
                                   int x, int y, int param,
                                   const char *title,
                                   int shadowId = -1, const char *shadowFn = NULL,
                                   int diffuseId = -1, const char *diffuseFn = NULL,
                                   int textFieldId = -1, const char *textFieldFn = NULL,
                                   Size size = SIZE_STANDARD,
                                   IText **valueText = NULL,
                                   bool addValueText = true,
                                   ICaptionControl **ioValueTextControl = NULL,
                                   int yoffset = 0, bool BL_FLOATTextField = false,
                                   BL_FLOAT sensivity = 1.0);
    
    IKnobMultiControl * CreateKnob3Ex(Plugin *plug, IGraphics *graphics,
                                      int bmpId, const char *bmpFn, int bmpFrames,
                                      int x, int y, int param, const char *title,
                                      int shadowId = -1, const char *shadowFn = NULL,
                                      int diffuseId = -1, const char *diffuseFn = NULL,
                                      int textFieldId = -1, const char *textFieldFn = NULL,
                                      Size size = SIZE_STANDARD, int titleYOffset = 0,
                                      IText **ioValueText = NULL, bool addValueText = true,
                                      ICaptionControl **ioValueTextControl = NULL,
                                      int textValueYoffset = 0,
                                      bool BL_FLOATTextField = false, BL_FLOAT sensivity = 1.0,
                                      EDirection dragDirection = kVertical);
    
    // Create knob, then title, then value text
    // (for Spatializer)
    // => added text editable value 
    IKnobMultiControl *CreateKnob4(Plugin *plug, IGraphics *graphics,
                                   int bmpId, const char *bmpFn, int bmpFrames,
                                   int x, int y, int param,
                                   const char *title,
                                   int shadowId = -1, const char *shadowFn = NULL,
                                   int diffuseId = -1, const char *diffuseFn = NULL,
                                   int textFieldId = -1, const char *textFieldFn = NULL,
                                   IText **valueText = NULL,
                                   bool addValueText = true,
                                   ITextControl **ioValueTextControl = NULL,
                                   int yoffset = 0, bool BL_FLOATTextField = false,
                                   BL_FLOAT sensivity = 1.0);
    
    // For Precedence
    //
    // From CreateKnob3
    IKnobMultiControl *CreateKnob5(Plugin *plug, IGraphics *graphics,
                                  int bmpId, const char *bmpFn, int bmpFrames,
                                  int x, int y, int param,
                                  const char *title,
                                  int shadowId = -1, const char *shadowFn = NULL,
                                  int diffuseId = -1, const char *diffuseFn = NULL,
                                  int textFieldId = -1, const char *textFieldFn = NULL,
                                  Size size = SIZE_STANDARD,
                                  IText **valueText = NULL,
                                  bool addValueText = true,
                                  ICaptionControl **ioValueTextControl = NULL,
                                  int yoffset = 0, bool BL_FLOATTextField = false,
                                  BL_FLOAT sensivity = 1.0);
    
    IKnobMultiControl *CreateEnumKnob(Plugin *plug, IGraphics *graphics,
                                      int bmpId, const char *bmpFn, int bmpFrames,
                                      int x, int y, int param, BL_FLOAT sensivity,
                                      const char *title,
                                      int shadowId = -1, const char *shadowFn = NULL,
                                      int diffuseId = -1, const char *diffuseFn = NULL,
                                      int textFieldId = -1, const char *textFieldFn = NULL,
                                      IText **ioValueText = NULL, bool addValueText = true,
                                      ITextControl **ioValueTextControl = NULL, int yoffset = 0);
#endif
    
    // #bl-iplug2
#if 0
    IRadioButtonsControl *
                    CreateRadioButtons(Plugin *plug, IGraphics *graphics,
                                       int bmpId, const char *bmpFn, int bmpFrames,
                                       int x, int y, int numButtons, int size, int param,
                                       bool horizontalFlag, const char *title,
                                       int diffuseId, const char *diffuseFn,
                                       IText::EAlign align = IText::Center,
                                       IText::EAlign titleAlign = IText::Center,
                                       const char **radioLabels = NULL, int numRadioLabels = 0,
                                       BL_FLOAT titleTextOffsetY = TITLE_TEXT_OFFSET_Y);
    
    IToggleButtonControl *CreateToggleButton(Plugin *plug, IGraphics *graphics,
                                             int bmpId, const char *bmpFn, int bmpFrames,
                                             int x, int y, int param, const char *title,
                                             int diffuseId, const char *diffuseFn,
                                             Size size = SIZE_STANDARD);
#endif
    
    /*IToggleButtonControl*/
    IControl *CreateToggleButtonEx(Plugin *plug, IGraphics *graphics,
                                   int bmpId, const char *bmpFn, int bmpFrames,
                                   int x, int y, int param, const char *title,
                                   int diffuseId, const char *diffuseFn,
                                   Size size = SIZE_STANDARD,
                                   int textOffsetX = 0, int textOffsetY = 0,
                                   TitlePosition position = TITLE_TOP,
                                   bool oneWay = false);
    
    VuMeterControl *CreateVuMeter(Plugin *plug, IGraphics *graphics,
                                  int bmpId, const char *bmpFn, int bmpFrames,
                                  int x, int y, int param, const char *title,
                                  bool alignLeft, bool decreaseWhenIdle = true,
                                  // Flag to inform host when vu-meter value changes
                                  // Used for AutoGain, to enable the host to write automation
                                  // when the gain value is changed from the plugin
                                  // (informing host for automation must be done in the GUI thread)
                                  bool informHostParamChange = false);
    
    
#if USE_GRAPH_OGL
    GraphControl11 *CreateGraph11(Plugin *plug, IGraphics *graphics,
                                  int param, const char *paramName,
                                  int bmpId, const char *bmpFn, int bmpFrames,
                                  int x , int y,
                                  int numCurves, int numPoints,
                                  int shadowsId = -1, const char *shadowsFn = NULL,
                                  IBitmap **oBitmap = NULL,
                                  bool useBgBitmap = false);
    
    GraphControl11 *CreateGraph11(Plugin *plug, IGraphics *graphics, int param,
                                  int x , int y, int width, int height,
                                  int numCurves, int numPoints,
                                  IBitmap *bgBitmap = NULL);
#endif
    
    IControl *CreateFileSelector(Plugin *plug, IGraphics *graphics,
                                 int bmpId, const char *bmpFn, int bmpFrames,
                                 int x, int y, int param,
                                 EFileAction action,
                                 char *label,
                                 char* dir = "", char* extensions = "",
                                 bool promptForFile = true);
    
    IControl *CreateButton(Plugin *plug, IGraphics *graphics,
                           int bmpId, const char *bmpFn, int bmpFrames,
                           int x, int y, int param,
                           char *label);

    IControl *CreateGUIResizeButton(Plugin *plug, IGraphics *graphics,
                                    int bmpId, const char *bmpFn, int bmpFrames,
                                    int x, int y, int param,
                                    int resizeWidth, int resizeHeight,
                                    char *label);
    
    IControl *CreateRolloverButton(Plugin *plug, IGraphics *graphics,
                                   int bmpId, const char *bmpFn, int bmpFrames,
                                   int x, int y, int param,
                                   char *label, bool toggleFlag = true);
    
    IControl *CreateRolloverButtonContainer(Plugin *plug, IGraphics *graphics,
                                            int bmpId, const char *bmpFn, int bmpFrames,
                                            int x, int y, int param,
                                            char *label, IControl *child, bool toggleFlag);

    
    // Version
    void CreateVersion(Plugin *plug, IGraphics *graphics,
                       const char *versionStr, Position pos = BOTTOM);
    
    // Generic bitmap
    IBitmapControl *CreateBitmap(Plugin *plug, IGraphics *graphics,
                                 int x, int y,
                                 int bmpId, const char *bmpFn,
                                 int shadowsId = -1, const char *shadowsFn = NULL,
                                 IBitmap **ioBmp = NULL);
    
    // Shadows
    IBitmapControl *CreateShadows(Plugin *plug, IGraphics *graphics,
                                  int bmpId, const char *bmpFn);
    
    // Logo
    IBitmapControl *CreateLogo(Plugin *plug, IGraphics *graphics,
                               int bmpId, const char *bmpFn, Position pos = BOTTOM);
    
    IBitmapControl *CreateLogoAnim(Plugin *plug, IGraphics *graphics,
                                   int bmpId, const char *bmpFn, Position pos = BOTTOM);
    
    // Plugin name
    IBitmapControl *CreatePlugName(Plugin *plug, IGraphics *graphics,
                                   int bmpId, const char *bmpFn, Position pos = BOTTOM);
    
    IBitmapControl *CreateHelpButton(Plugin *plug, IGraphics *graphics,
                                     int bmpId, const char *bmpFn,
                                     int manResId, const char *fileName,
                                     Position pos = BOTTOM);
    
    // #bl-iplug2
#if 0
    void AddTrigger(ITriggerControl *trigger);
    
    // Call it after a parameter change
    static void UpdateText(PluginBase *plug, int paramIdx);
    
    static void ResetParameter(PluginBase *plug, int paramIdx);

    // Return true if the resulting file name is valid 
    static bool PromptForFile(PluginBase *plug, EFileAction action, WDL_String *result,
                              char* dir = "", char* extensions = "");
#endif
    
    int GetTextOffset();
    
    // Create generic text
    ITextControl *CreateText(Plugin *plug, IGraphics *graphics,
                            const char *textStr,
                            int x, int y,
                             int size, char *font,
                             ETextStyle style,
                             const IColor &color,
                             EAlign align);
    
    ITextControl *CreateTitleText(Plugin *plug, IGraphics *graphics, int width,
                                  const char *title, int x, int y, EAlign align,
                                  Size size = SIZE_STANDARD,
                                  BL_FLOAT titleTextOffsetY = TITLE_TEXT_OFFSET_Y);
    
    // UST
    ITextControl * CreateUSTText(Plugin *plug, IGraphics *graphics, int textSize, int width,
                                 const IColor &color,
                                 const char *str, int x, int y, EAlign align,
                                 BL_FLOAT textOffsetX, BL_FLOAT textOffsetY);

    
    // UST
    IControl *CreateSwitch(Plugin *plug, IGraphics *graphics,
                           int bmpId, const char *bmpFn, int bmpFrames,
                           int x, int y, int param);
    
    // Convenient function when changing GUI size
    void AddControl(IControl *control);
    
    // Create or update the frequeny axis
    // FIX for project at 88200 Hz for example
    static void UpdateFrequencyAxis(GraphControl11 *graph,
                                    int nativeBufferSize,
                                    BL_FLOAT sampleRate,
                                    bool highResLogCurves);
    
    static int MeasureText(IGraphics *graphics, const char *text, Size size);
    
protected:
    ITextControl *AddTitleText(Plugin *plug, IGraphics *graphics,
                               IBitmap *bitmap, const char *title,
                               int x, int y, EAlign align,
                               Size size = SIZE_STANDARD,
                               BL_FLOAT titleTextOffsetY = TITLE_TEXT_OFFSET_Y);
   
    ITextControl *AddTitleTextRight(Plugin *plug, IGraphics *graphics,
                                    IBitmap *bitmap, const char *title,
                                    int x, int y, EAlign align,
                                    Size size = SIZE_STANDARD,
                                    BL_FLOAT titleTextOffsetX = 0,
                                    BL_FLOAT titleTextOffsetY = 0);
    
    ITextControl *AddValueText(Plugin *plug, IGraphics *graphics,
                               IBitmap *bitmap, IText *text, int x, int y, int param,
                               IBitmap *tfBitmap);
   
    // For editbale text
    ICaptionControl *AddValueText3(Plugin *plug, IGraphics *graphics,
                                   IBitmap *bitmap, IText *text, int x, int y, int param,
                                   IBitmap *tfBitmap);
    
    ITextControl *AddRadioLabelText(Plugin *plug, IGraphics *graphics,
                                    IBitmap *bitmap, IText *text, const char *labelStr,
                                    int x, int y,
                                    EAlign align = EAlign::Near);
    
    ITextControl *AddVersionText(Plugin *plug, IGraphics *graphics,
                                  IText *text, const char *versionStr, int x, int y);
    
    IBitmapControl *CreateShadow(Plugin *plug, IGraphics *graphics,
                                 int bmpId, const char *bmpFn,
                                 int x, int y, IBitmap *objectBitmap,
                                 int nObjectFrames);
    
    IBitmapControl *CreateDiffuse(Plugin *plug, IGraphics *graphics,
                                  int bmpId, const char *bmpFn,
                                  int x, int y, IBitmap *objectBitmap,
                                  int nObjectFrames);
    
    // #bl-iplug2
#if 0
    bool CreateMultiDiffuse(Plugin *plug, IGraphics *graphics,
                            int bmpId, const char *bmpFn,
                            const WDL_TypedBuf<IRECT> &rects,
                            IBitmap *objectBitmap,
                            int nObjectFrames,
                            vector<IBitmapControlExt *> *diffuseBmps);
#endif
    
    // For UST
    void CreateGraphOverlay(Plugin *plug, IGraphics *graphics,
                            int bmpId, const char *bmpFn,
                            int x, int y, IBitmap *objectBitmap,
                            int nObjectFrames, IBitmap **overlayBmp);

    
    IBitmapControl *CreateTextField(Plugin *plug, IGraphics *graphics,
                                    int bmpId, const char *bmpFn,
                                    int x, int y, IBitmap *objectBitmap,
                                    int nObjectFrames, IBitmap *outBmp = NULL);
    
    // #bl-iplug2
#if 0
    void CreateKnobInternal(Plugin *plug, IGraphics *graphics,
                            IBitmap *bitmap, IKnobMultiControl *control,
                            int x, int y, int param, const char *title,
                            int shadowId, const char *shadowFn,
                            int diffuseId, const char *diffuseFn,
                            int textFieldId, const char *textFieldFn,
                            Size size,
                            IText **ioValueText, bool addValueText,
                            ITextControl **ioValueTextControl, int yoffset,
                            bool BL_FLOATTextField = false);
    
    void CreateKnobInternal2(Plugin *plug, IGraphics *graphics,
                             IBitmap *bitmap, IKnobMultiControl *control,
                             int x, int y, int param, const char *title,
                             int shadowId, const char *shadowFn,
                             int diffuseId, const char *diffuseFn,
                             int textFieldId, const char *textFieldFn,
                             IText **ioValueText, bool addValueText,
                             ITextControl **ioValueTextControl, int yoffset,
                             bool BL_FLOATTextField = false);

    // For editable text value
    void CreateKnobInternal3(Plugin *plug, IGraphics *graphics,
                             IBitmap *bitmap, IKnobMultiControl *control,
                             int x, int y, int param, const char *title,
                             int shadowId, const char *shadowFn,
                             int diffuseId, const char *diffuseFn,
                             int textFieldId, const char *textFieldFn,
                             Size size,
                             IText **ioValueText, bool addValueText,
                             ICaptionControl **ioValueTextControl, int yoffset,
                             bool BL_FLOATTextField = false);
    
    void CreateKnobInternal3Ex(Plugin *plug, IGraphics *graphics,
                               IBitmap *bitmap, IKnobMultiControl *control,
                               int x, int y, int param, const char *title,
                               int shadowId, const char *shadowFn,
                               int diffuseId, const char *diffuseFn,
                               int textFieldId, const char *textFieldFn,
                               Size size, int titleYOffset,
                               IText **ioValueText, bool addValueText,
                               ICaptionControl **ioValueTextControl, int textValueYoffset,
                               bool BL_FLOATTextField = false);
    
    void CreateKnobInternal4(Plugin *plug, IGraphics *graphics,
                             IBitmap *bitmap, IKnobMultiControl *control,
                             int x, int y, int param, const char *title,
                             int shadowId, const char *shadowFn,
                             int diffuseId, const char *diffuseFn,
                             int textFieldId, const char *textFieldFn,
                             IText **ioValueText, bool addValueText,
                             ITextControl **ioValueTextControl, int yoffset,
                             bool BL_FLOATTextField = false);
    
    // For Precedence
    //
    // Will create the text field appart
    //
    void CreateKnobInternal5(Plugin *plug, IGraphics *graphics,
                             IBitmap *bitmap, IKnobMultiControl *control,
                             int x, int y, int param, const char *title,
                             int shadowId, const char *shadowFn,
                             int diffuseId, const char *diffuseFn,
                             int textFieldId, const char *textFieldFn,
                             Size size,
                             IText **ioValueText, bool addValueText,
                             ICaptionControl **ioValueTextControl, int yoffset,
                             bool BL_FLOATTextField = false);
#endif
    
    ITextControl *AddValueTextInternal(Plugin *plug, IGraphics *graphics, IBitmap *bitmap,
                                       int x, int y, int param,
                                       bool addValueText, IText **ioValueText,
                                       IBitmap *tfBitmap);
    
    // For editable text
    ICaptionControl *AddValueTextInternal3(Plugin *plug, IGraphics *graphics, IBitmap *bitmap,
                                           int x, int y, int param,
                                           bool addValueText, IText **ioValueText,
                                           IBitmap *tfBitmap);
    
    ICaptionControl *AddValueTextInternal5(Plugin *plug, IGraphics *graphics, IBitmap *bitmap,
                                           int x, int y, int param,
                                           bool addValueText, IText **ioValueText,
                                           IBitmap *tfBitmap);
    
    void CreateKnobLightingInternal(Plugin *plug, IGraphics *graphics,
                                    int shadowId, const char *shadowFn,
                                    int diffuseId, const char *diffuseFn,
                                    int x, int y, IBitmap *bitmap, int nObjectFrames);
    
    IBitmapControl *CreateTextFieldInternal(Plugin *plug, IGraphics *graphics,
                                            int textFieldId, const char *textFieldFn,
                                            int x, int y, IBitmap *bitmap, int nObjectFrames,
                                            bool doubleTextField, IBitmap *outBmp = NULL);
                                
public:
    static IColor mValueTextColor;
    
protected:
    IColor mTitleTextColor;
    IColor mRadioLabelTextColor;
    IColor mVersionTextColor;
    IColor mHilightTextColor;
    IColor mEditTextColor;
    
#if GUI_OBJECTS_SORTING
    vector<IControl *> mBackObjects;
    vector<IControl *> mWidgets;
    vector<IControl *> mTexts;
    vector<IControl *> mDiffuse;
    vector<IControl *> mShadows;
    vector<IControl *> mTriggers;
#endif
};

#endif /* defined(__Transient__GUIHelper10__) */
