/*******************************************************************************************
*
*   rGuiStyler v2.2 - A simple and easy-to-use raygui styles editor
*
*   CONFIGURATION:
*
*   #define ENABLE_PRO_FEATURES
*       Enable PRO features for the tool. Usually command-line and export options related.
*
*   DEPENDENCIES:
*       raylib 2.1-dev          - Windowing/input management and drawing.
*       raygui 2.0              - IMGUI controls (based on raylib).
*       tinyfiledialogs 3.3.7   - Open/save file dialogs, it requires linkage with comdlg32 and ole32 libs.
*
*   COMPILATION (Windows - MinGW):
*       gcc -o rguistyler.exe rguistyler.c external/tinyfiledialogs.c -s -Iexternal / 
*           -lraylib -lopengl32 -lgdi32 -lcomdlg32 -lole32 -std=c99
* 
*   COMPILATION (Linux - GCC):
*       gcc -o rguistyler rguistyler.c external/tinyfiledialogs.c -s -Iexternal -no-pie -D_DEFAULT_SOURCE /
*           -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
*
*   DEVELOPERS:
*       Ramon Santamaria (@raysan5):    Supervision, review, redesign, update and maintainer.
*       Adria Arranz (@Adri102):        Developer and designer, implemented v2.0 (2018)
*       Jordi Jorba (@KoroBli):         Developer and designer, implemented v2.0 (2018)
*       Sergio Martinez (@anidealgift): Development and testing v1.0 (2015..2017)
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2014-2018 raylib technologies (@raysan5).
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_STYLE_SAVE_LOAD
#define RAYGUI_STYLE_DEFAULT_LIGHT
#include "raygui.h"                     // Required for: IMGUI controls

#include "external/tinyfiledialogs.h"   // Required for: Native open/save file dialogs

#include "raygui_style_light.h"         // Embedded Image: controls table

#include <stdlib.h>                     // Required for: malloc(), free()
#include <string.h>                     // Required for: strcmp()
#include <stdio.h>                      // Required for: fopen(), fclose(), fread()...

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define ENABLE_PRO_FEATURES             // Enable PRO version features

#define TOOL_VERSION_TEXT     "2.2"     // Tool version string

#define NUM_CONTROLS            13      // Number of GUI controls on the list

// NOTE: Not all controls expose the same number of configurable properties,
// depending on the control the number of properties available differs
#define NUM_PROPS_CONTROLS_A     4      // Number of properties for controls type A
#define NUM_PROPS_CONTROLS_B     8      // Number of properties for controls type B
#define NUM_PROPS_CONTROLS_C    14      // Number of properties for controls type C

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Control types available (NUM_CONTROLS)
// NOTE: Control name to prepend to property type
typedef enum { 
    DEFAULT = 0, 
    LABELBUTTON,
    BUTTON, 
    //IMAGEBUTTON,
    TOGGLE, 
    //TOGGLEGROUP, 
    SLIDER, 
    SLIDERBAR, 
    PROGRESSBAR, 
    CHECKBOX, 
    SPINNER, 
    COMBOBOX, 
    TEXTBOX,
    LISTVIEW,
    COLORPICKER
} GuiControlType;

// Available property types
// NOTE: Depending on control, name is prepended: BUTTON_BORDER_COLOR_NORMAL
typedef enum { 
    BORDER_COLOR_NORMAL = 0,
    BASE_COLOR_NORMAL,
    TEXT_COLOR_NORMAL,
    BORDER_COLOR_FOCUSED,
    BASE_COLOR_FOCUSED,
    TEXT_COLOR_FOCUSED,
    BORDER_COLOR_PRESSED,
    BASE_COLOR_PRESSED,
    TEXT_COLOR_PRESSED,
    BORDER_COLOR_DISABLED,
    BASE_COLOR_DISABLED,
    TEXT_COLOR_DISABLED
} GuiPropertyType;

// Style file type to export
typedef enum {
    STYLE_TEXT = 0,         // raygui style text file (.rgs)
    STYLE_BINARY,           // raygui style binary file (.rgs)
    PALETTE_IMAGE,
    PALETTE_CODE,
    CONTROLS_TABLE_IMAGE
} GuiStyleFileType;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------

// Default Light style backup to check changed properties
static int styleBackup[NUM_PROPERTIES] = { 0 };

// Controls name text
// NOTE: Some styles are shared by multiple controls
// LABEL = LABELBUTTON, BUTTON = IMAGEBUTTON, TOGGLE = TOGGLEGROUP
static const char *guiControlText[NUM_CONTROLS] = { 
    "DEFAULT", 
    "LABELBUTTON",
    "BUTTON", 
    "TOGGLE", 
    "SLIDER", 
    "SLIDERBAR", 
    "PROGRESSBAR", 
    "CHECKBOX", 
    "SPINNER", 
    "COMBOBOX", 
    "TEXTBOX",
    "LISTVIEW",
    "COLORPICKER"
};

// Controls type A properties name text
// NOTE: Used by LABEL, LABELBUTTON
static const char *guiPropsTextA[NUM_PROPS_CONTROLS_A] = { 
    "TEXT_COLOR_NORMAL",
    "TEXT_COLOR_FOCUSED",
    "TEXT_COLOR_PRESSED",
    "TEXT_COLOR_DISABLED"
};

// Controls type B properties name text
// NOTE: Used by SLIDER, SLIDERBAR, PROGRESSBAR, CHECKBOX, COLORPICKER
static const char *guiPropsTextB[NUM_PROPS_CONTROLS_B] = { 
    "BORDER_COLOR_NORMAL",
    "BASE_COLOR_NORMAL",
    "BORDER_COLOR_FOCUSED",
    "BASE_COLOR_FOCUSED",
    "BORDER_COLOR_PRESSED",
    "BASE_COLOR_PRESSED",
    "BORDER_COLOR_DISABLED",
    "BASE_COLOR_DISABLED",
};

// Controls type C properties name text
// NOTE: Used by BUTTON, TOGGLE, SPINNER, COMBOBOX, TEXTBOX, LISTVIEW
static const char *guiPropsTextC[NUM_PROPS_CONTROLS_C] = { 
    "BORDER_COLOR_NORMAL",
    "BASE_COLOR_NORMAL",
    "TEXT_COLOR_NORMAL",
    "BORDER_COLOR_FOCUSED",
    "BASE_COLOR_FOCUSED",
    "TEXT_COLOR_FOCUSED",
    "BORDER_COLOR_PRESSED",
    "BASE_COLOR_PRESSED",
    "TEXT_COLOR_PRESSED",
    "BORDER_COLOR_DISABLED",
    "BASE_COLOR_DISABLED",
    "TEXT_COLOR_DISABLED",
    "BACKGROUND_COLOR",
    "LINES_COLOR"
};

// Full property name text
// NOTE: Only used by SaveStyle() for text mode value comments
static const char *guiPropertyText[NUM_PROPERTIES] = {
    "DEFAULT_BACKGROUND_COLOR",
    "DEFAULT_LINES_COLOR",
    "DEFAULT_TEXT_FONT",
    "DEFAULT_TEXT_SIZE",
    "DEFAULT_BORDER_WIDTH",
    "DEFAULT_BORDER_COLOR_NORMAL",
    "DEFAULT_BASE_COLOR_NORMAL",
    "DEFAULT_TEXT_COLOR_NORMAL",
    "DEFAULT_BORDER_COLOR_FOCUSED",
    "DEFAULT_BASE_COLOR_FOCUSED",
    "DEFAULT_TEXT_COLOR_FOCUSED",
    "DEFAULT_BORDER_COLOR_PRESSED",
    "DEFAULT_BASE_COLOR_PRESSED",
    "DEFAULT_TEXT_COLOR_PRESSED",
    "DEFAULT_BORDER_COLOR_DISABLED",
    "DEFAULT_BASE_COLOR_DISABLED",
    "DEFAULT_TEXT_COLOR_DISABLED",
    "LABEL_TEXT_COLOR_NORMAL",
    "LABEL_TEXT_COLOR_FOCUSED",
    "LABEL_TEXT_COLOR_PRESSED",
    "LABEL_TEXT_COLOR_DISABLED",
    "BUTTON_BORDER_WIDTH",
    "BUTTON_BORDER_COLOR_NORMAL",
    "BUTTON_BASE_COLOR_NORMAL",
    "BUTTON_TEXT_COLOR_NORMAL",
    "BUTTON_BORDER_COLOR_FOCUSED",
    "BUTTON_BASE_COLOR_FOCUSED",
    "BUTTON_TEXT_COLOR_FOCUSED",
    "BUTTON_BORDER_COLOR_PRESSED",
    "BUTTON_BASE_COLOR_PRESSED",
    "BUTTON_TEXT_COLOR_PRESSED",
    "BUTTON_BORDER_COLOR_DISABLED",
    "BUTTON_BASE_COLOR_DISABLED",
    "BUTTON_TEXT_COLOR_DISABLED",
    "TOGGLE_BORDER_WIDTH",
    "TOGGLE_BORDER_COLOR_NORMAL",
    "TOGGLE_BASE_COLOR_NORMAL",
    "TOGGLE_TEXT_COLOR_NORMAL",
    "TOGGLE_BORDER_COLOR_FOCUSED",
    "TOGGLE_BASE_COLOR_FOCUSED",
    "TOGGLE_TEXT_COLOR_FOCUSED",
    "TOGGLE_BORDER_COLOR_PRESSED",
    "TOGGLE_BASE_COLOR_PRESSED",
    "TOGGLE_TEXT_COLOR_PRESSED",
    "TOGGLE_BORDER_COLOR_DISABLED",
    "TOGGLE_BASE_COLOR_DISABLED",
    "TOGGLE_TEXT_COLOR_DISABLED",
    "TOGGLEGROUP_PADDING",
    "SLIDER_BORDER_WIDTH",
    "SLIDER_SLIDER_WIDTH",
    "SLIDER_BORDER_COLOR_NORMAL",
    "SLIDER_BASE_COLOR_NORMAL",
    "SLIDER_BORDER_COLOR_FOCUSED",
    "SLIDER_BASE_COLOR_FOCUSED",
    "SLIDER_BORDER_COLOR_PRESSED",
    "SLIDER_BASE_COLOR_PRESSED",
    "SLIDER_BORDER_COLOR_DISABLED",
    "SLIDER_BASE_COLOR_DISABLED",
    "SLIDERBAR_INNER_PADDING",
    "SLIDERBAR_BORDER_WIDTH",
    "SLIDERBAR_BORDER_COLOR_NORMAL",
    "SLIDERBAR_BASE_COLOR_NORMAL",
    "SLIDERBAR_BORDER_COLOR_FOCUSED",
    "SLIDERBAR_BASE_COLOR_FOCUSED",
    "SLIDERBAR_BORDER_COLOR_PRESSED",
    "SLIDERBAR_BASE_COLOR_PRESSED",
    "SLIDERBAR_BORDER_COLOR_DISABLED",
    "SLIDERBAR_BASE_COLOR_DISABLED",
    "PROGRESSBAR_INNER_PADDING",
    "PROGRESSBAR_BORDER_WIDTH",
    "PROGRESSBAR_BORDER_COLOR_NORMAL",
    "PROGRESSBAR_BASE_COLOR_NORMAL",
    "PROGRESSBAR_BORDER_COLOR_FOCUSED",
    "PROGRESSBAR_BASE_COLOR_FOCUSED",
    "PROGRESSBAR_BORDER_COLOR_PRESSED",
    "PROGRESSBAR_BASE_COLOR_PRESSED",
    "PROGRESSBAR_BORDER_COLOR_DISABLED",
    "PROGRESSBAR_BASE_COLOR_DISABLED",
    "SPINNER_BUTTON_PADDING",
    "SPINNER_BUTTONS_WIDTH",
    "SPINNER_BORDER_COLOR_NORMAL",
    "SPINNER_BASE_COLOR_NORMAL",
    "SPINNER_TEXT_COLOR_NORMAL",
    "SPINNER_BORDER_COLOR_FOCUSED",
    "SPINNER_BASE_COLOR_FOCUSED",
    "SPINNER_TEXT_COLOR_FOCUSED",
    "SPINNER_BORDER_COLOR_PRESSED",
    "SPINNER_BASE_COLOR_PRESSED",
    "SPINNER_TEXT_COLOR_PRESSED",
    "SPINNER_BORDER_COLOR_DISABLED",
    "SPINNER_BASE_COLOR_DISABLED",
    "SPINNER_TEXT_COLOR_DISABLED",
    "COMBOBOX_BORDER_WIDTH",
    "COMBOBOX_BUTTON_PADDING",
    "COMBOBOX_SELECTOR_WIDTH",
    "COMBOBOX_BORDER_COLOR_NORMAL",
    "COMBOBOX_BASE_COLOR_NORMAL",
    "COMBOBOX_TEXT_COLOR_NORMAL",
    "COMBOBOX_BORDER_COLOR_FOCUSED",
    "COMBOBOX_BASE_COLOR_FOCUSED",
    "COMBOBOX_TEXT_COLOR_FOCUSED",
    "COMBOBOX_BORDER_COLOR_PRESSED",
    "COMBOBOX_BASE_COLOR_PRESSED",
    "COMBOBOX_TEXT_COLOR_PRESSED",
    "COMBOBOX_BORDER_COLOR_DISABLED",
    "COMBOBOX_BASE_COLOR_DISABLED",
    "COMBOBOX_TEXT_COLOR_DISABLED",
    "CHECKBOX_BORDER_WIDTH",
    "CHECKBOX_INNER_PADDING",
    "CHECKBOX_BORDER_COLOR_NORMAL",
    "CHECKBOX_BASE_COLOR_NORMAL",
    "CHECKBOX_BORDER_COLOR_FOCUSED",
    "CHECKBOX_BASE_COLOR_FOCUSED",
    "CHECKBOX_BORDER_COLOR_PRESSED",
    "CHECKBOX_BASE_COLOR_PRESSED",
    "CHECKBOX_BORDER_COLOR_DISABLED",
    "CHECKBOX_BASE_COLOR_DISABLED",
    "TEXTBOX_BORDER_WIDTH",
    "TEXTBOX_BORDER_COLOR_NORMAL",
    "TEXTBOX_BASE_COLOR_NORMAL",
    "TEXTBOX_TEXT_COLOR_NORMAL",
    "TEXTBOX_BORDER_COLOR_FOCUSED",
    "TEXTBOX_BASE_COLOR_FOCUSED",
    "TEXTBOX_TEXT_COLOR_FOCUSED",
    "TEXTBOX_BORDER_COLOR_PRESSED",
    "TEXTBOX_BASE_COLOR_PRESSED",
    "TEXTBOX_TEXT_COLOR_PRESSED",
    "TEXTBOX_BORDER_COLOR_DISABLED",
    "TEXTBOX_BASE_COLOR_DISABLED",
    "TEXTBOX_TEXT_COLOR_DISABLED",
    "COLORPICKER_BARS_THICK",
    "COLORPICKER_BARS_PADDING",
    "COLORPICKER_BORDER_COLOR_NORMAL",
    "COLORPICKER_BASE_COLOR_NORMAL",
    "COLORPICKER_BORDER_COLOR_FOCUSED",
    "COLORPICKER_BASE_COLOR_FOCUSED",
    "COLORPICKER_BORDER_COLOR_PRESSED",
    "COLORPICKER_BASE_COLOR_PRESSED",
    "COLORPICKER_BORDER_COLOR_DISABLED",
    "COLORPICKER_BASE_COLOR_DISABLED",
    "LISTVIEW_ELEMENTS_HEIGHT",
    "LISTVIEW_ELEMENTS_PADDING",
    "LISTVIEW_BAR_WIDTH",       
    "LISTVIEW_BORDER_COLOR_NORMAL",
    "LISTVIEW_BASE_COLOR_NORMAL",
    "LISTVIEW_TEXT_COLOR_NORMAL",
    "LISTVIEW_BORDER_COLOR_FOCUSED",
    "LISTVIEW_BASE_COLOR_FOCUSED",
    "LISTVIEW_TEXT_COLOR_FOCUSED",
    "LISTVIEW_BORDER_COLOR_PRESSED",
    "LISTVIEW_BASE_COLOR_PRESSED",
    "LISTVIEW_TEXT_COLOR_PRESSED",
    "LISTVIEW_BORDER_COLOR_DISABLED",
    "LISTVIEW_BASE_COLOR_DISABLED",
    "LISTVIEW_TEXT_COLOR_DISABLED"
};

static bool styleSaved = false;             // Show save dialog on closing if not saved

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void ShowUsageInfo(void);            // Show command line usage info

// Load/Save/Export data functions
static void DialogLoadStyle(void);          // Dialog load style file
static void DialogSaveStyle(bool binary);   // Dialog save style file
static void DialogExportStyle(int type);    // Dialog save style file
static void SaveStyle(const char *fileName, bool binary);   // Save raygui style (.rgs), text or binary
static void ExportStyle(const char *fileName, int type);    // Export style color palette

// Auxiliar GUI functions
static int GetGuiStylePropertyIndex(int control, int property);
static Color ColorBox(Rectangle bounds, Color *colorPicker, Color color);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    char inFileName[256] = { 0 };       // Input file name (required in case of drag & drop over executable)
    
    // Command-line usage mode
    //--------------------------------------------------------------------------------------
    if (argc > 1)
    {
        // CLI required variables
        bool showUsageInfo = false;     // Toggle command line usage info
        
        char outFileName[256] = { 0 };  // Output file name
        int outputFormat = 0;           // Formats: STYLE_TEXT = 0, STYLE_BINARY, PALETTE_IMAGE, CONTROLS_TABLE_IMAGE, PALETTE_CODE
        
        if (argc == 2)  // One file dropped over the executable or just one argument
        {
            if (IsFileExtension(argv[1], ".rgs") || 
                IsFileExtension(argv[1], ".png"))
            {
                // Open file with graphic interface
                strcpy(inFileName, argv[1]);        // Read input filename
            }
            else 
            {
                ShowUsageInfo();                    // Show command line usage info
                return 0;
            }
        }
        else
        {
            // Arguments scan and processing
            for (int i = 1; i < argc; i++)
            {
                if (strcmp(argv[i], "--help") == 0) showUsageInfo = true;
                else if ((strcmp(argv[i], "--input") == 0) || (strcmp(argv[i], "-i") == 0))
                {                   
                    // Verify file provided with supported extension
                    // NOTE: Also checking no "--" is comming after --input
                    if (((i + 1) < argc) && (argv[i + 1][0] != '-') && 
                        IsFileExtension(inFileName, ".png"))
                    {
                        strcpy(inFileName, argv[i + 1]);    // Read input file
                    }

                    i++;
                }
                else if (strcmp(argv[i], "--format") == 0) 
                {
                    if (((i + 1) < argc) && (argv[i + 1][0] != '-'))  // Check if next argument could be readed
                    {
                        int format = atoi(argv[i + 1]);
                        
                        if ((format >= 0) && (format <= 4)) outputFormat = format;
                        else { TraceLog(LOG_WARNING, "Output format not valid"); showUsageInfo = true; }
                    }
                    else { TraceLog(LOG_WARNING, "No valid parameter after --format"); showUsageInfo = true; }
                }
                else 
                {
                    TraceLog(LOG_WARNING, "No valid parameter: %s", argv[i]);
                    showUsageInfo = true;
                }
            }
            
            if (inFileName[0] != '\0')
            {
                // Process input .rgs file
                GuiLoadStyle(inFileName);
                
                // TODO: Setup output file name, based on input
                char outFileName[256] = { 0 };
                strcpy(outFileName, inFileName);
                
                // Export style files with different formats
                ExportStyle(outFileName, outputFormat);
            }
            
            if (showUsageInfo) ShowUsageInfo();
            
            return 0;
        }
    }
    
    // GUI usage mode - Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 720;
    const int screenHeight = 640;
    
    SetTraceLog(0);                             // Disable trace log messsages
    //SetConfigFlags(FLAG_WINDOW_RESIZABLE);    // Window configuration flags
    InitWindow(screenWidth, screenHeight, FormatText("rGuiStyler v%s - A simple and easy-to-use raygui styles editor", TOOL_VERSION_TEXT));
    SetExitKey(0);
    
    int framesCounter = 0;
    
    Vector2 anchor01 = { 0, 0 };
    Vector2 anchor02 = { 345, 40 };
    bool saveColor = false;

    int changedControlsCounter = 0;

    // Define gui controls rectangles
    Rectangle bounds[NUM_CONTROLS] = {
        (Rectangle){ 0 },                                           // DEFAULT
        (Rectangle){ anchor02.x + 85, anchor02.y + 35, 145, 25 },   // LABELBUTTON
        (Rectangle){ anchor02.x + 195, anchor02.y + 240, 160, 30 }, // BUTTON
        (Rectangle){ anchor02.x + 10, anchor02.y + 70, 65, 30 },    // TOGGLE
        (Rectangle){ anchor02.x + 75, anchor02.y + 115, 250, 15 },  // SLIDER
        (Rectangle){ anchor02.x + 75, anchor02.y + 140, 250, 15 },  // SLIDERBAR
        (Rectangle){ anchor02.x + 10, anchor02.y + 165, 315, 15 },  // PROGRESSBAR    
        (Rectangle){ anchor02.x + 270, anchor02.y + 38, 20, 20 },  // CHECKBOX
        (Rectangle){ anchor02.x + 240, anchor02.y + 195, 115, 30 },  // SPINNER
        (Rectangle){ anchor02.x + 10, anchor02.y + 195, 160, 30 },  // COMBOBOX
        (Rectangle){ anchor02.x + 10, anchor02.y + 240, 180, 30 }, // TEXTBOX
        (Rectangle){ anchor01.x + 10, anchor01.y + 40, 140, 560 },  // LISTVIEW
        (Rectangle){ anchor02.x + 10, anchor02.y + 300, 240, 240 }  // COLORPICKER
    };
    
    // Keep a backup for style    
    memcpy(styleBackup, style, NUM_PROPERTIES*sizeof(int));
    
    // GUI controls data
    //-----------------------------------------------------------
    bool toggleActive = false;
    bool toggleValue = false;
    const char *toggleGuiText[4] = { "toggle", "group", "selection", "options" };
    
    int dropdownBoxActive = false;
    const char *dropdownBoxList[3] = { "A", "B", "C" };

    float sliderValue = 50.0f;
    float sliderBarValue = 20.0f;
    float progressValue = 0.0f;
    
    bool checked = false;
    
    bool selectingColor = false;
    
    int spinnerValue = 28;
    
    Vector2 mousePos = { 0 };

    int comboNum = 5;
    const char *comboText[5] = { "Text (.rgs)", "Binary (.rgs)", "Palette (.png)", "Palette (.h)", "Controls Table (.png)" };
    int comboActive = 0;
    
    char guiText[32] =  "custom_style.rgs";
    
    Color colorPickerValue = RED;
    
    int currentSelectedControl = -1;
    int currentSelectedProperty = -1;
    int previousSelectedProperty = -1;
    int previousSelectedControl = -1;
    
    Color colorBoxValue[12];
    
    for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(style[DEFAULT_BORDER_COLOR_NORMAL + i]);
    
    char colorHex[9] = "00000000";
 
    Vector3 colorHSV = { 0.0f, 0.0f, 0.0f };
    
    bool editFilenameText = false;
    bool editHexColorText = false;
    
    // Exit variables
    bool exitWindow = false;
    bool closingWindowActive = false;

    SetTargetFPS(60);
    //------------------------------------------------------------
    
    // Main game loop
    while (!exitWindow)             // Detect window close button
    {
        // Dropped files logic
        //----------------------------------------------------------------------------------
        if (IsFileDropped())        
        {
            int dropsCount = 0;
            char **droppedFiles = GetDroppedFiles(&dropsCount);
            
            // Supports loading .rgs style files (text or binary) and .png style palette images
            if (IsFileExtension(droppedFiles[0], ".rgs")) GuiLoadStyle(droppedFiles[0]);
            else if (IsFileExtension(droppedFiles[0], ".png")) GuiLoadStylePaletteImage(droppedFiles[0]);
            
            for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(style[DEFAULT_BORDER_COLOR_NORMAL + i]);
            
            ClearDroppedFiles();
            
            // Reset selected control
            currentSelectedControl = -1;
        }
        //----------------------------------------------------------------------------------
        
        // Keyboard shortcuts
        //----------------------------------------------------------------------------------
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) DialogSaveStyle(false);    // Show save style dialog (.rgs text)
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) DialogLoadStyle();         // Show load style dialog (.rgs)
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_E)) DialogExportStyle(CONTROLS_TABLE_IMAGE); // Show export style dialog (.rgs, .png, .h)        
        //----------------------------------------------------------------------------------
    
        // Basic program flow logic
        //----------------------------------------------------------------------------------
        framesCounter++;                    // General usage frames counter
        mousePos = GetMousePosition();      // Get mouse position each frame
        if (WindowShouldClose()) exitWindow = true;
        
        // Show save layout message window on ESC
        if (IsKeyPressed(KEY_ESCAPE))
        {
            if (changedControlsCounter <= 0) exitWindow = true;
            else closingWindowActive = !closingWindowActive;
        }
        
        // Check for changed controls 
        if ((framesCounter%120) == 0) 
        {
            changedControlsCounter = 0;
            for (int i = 0; i < NUM_PROPERTIES; i++) if (styleBackup[i] != style[i]) changedControlsCounter++;
        }

        // Controls selection on GuiListView logic
        if ((previousSelectedControl != currentSelectedControl)) currentSelectedProperty = -1;
        
        if ((currentSelectedControl == 0) && (currentSelectedProperty != -1))
        {
            if ((previousSelectedProperty != currentSelectedProperty) || (previousSelectedControl != currentSelectedControl)) saveColor = false;
            
            if (!saveColor)
            {
                colorPickerValue = GetColor(style[GetGuiStylePropertyIndex(currentSelectedControl, currentSelectedProperty)]);
                saveColor = true;
            }

            style[GetGuiStylePropertyIndex(currentSelectedControl, currentSelectedProperty)] = ColorToInt(colorPickerValue);
            
            // TODO: REVIEW: Resets all updated controls!
            GuiUpdateStyleComplete();
        }
        else if ((currentSelectedControl != -1) && (currentSelectedProperty != -1))
        {
            if ((previousSelectedProperty != currentSelectedProperty) || (previousSelectedControl != currentSelectedControl)) saveColor = false;
            
            if (!saveColor)
            {
                colorPickerValue = GetColor(GuiGetStyleProperty(GetGuiStylePropertyIndex(currentSelectedControl, currentSelectedProperty)));
                saveColor = true;
            }

            GuiSetStyleProperty(GetGuiStylePropertyIndex(currentSelectedControl, currentSelectedProperty), ColorToInt(colorPickerValue));
        }
        
        previousSelectedProperty = currentSelectedProperty;
        previousSelectedControl = currentSelectedControl;
        
        // Update progress bar automatically
        progressValue += 0.0005f;
        if (progressValue > 1.0f) progressValue = 0.0f;

        // Get edited color from text box
        if (!editHexColorText) sprintf(colorHex, "%02X%02X%02X%02X", colorPickerValue.r, colorPickerValue.g, colorPickerValue.b, colorPickerValue.a);
        
        colorHSV = ColorToHSV(colorPickerValue);
        
        // Color picker color selection logic
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, bounds[COLORPICKER])) selectingColor = true;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) 
        {
            selectingColor = false;
            ShowCursor();
        }
        
        if (selectingColor)
        {
            HideCursor();
            if (mousePos.x < bounds[COLORPICKER].x) SetMousePosition((Vector2){ bounds[COLORPICKER].x, mousePos.y });
            else if (mousePos.x > bounds[COLORPICKER].x + bounds[COLORPICKER].width) SetMousePosition((Vector2){ bounds[COLORPICKER].x + bounds[COLORPICKER].width, mousePos.y });
            
            if (mousePos.y < bounds[COLORPICKER].y) SetMousePosition((Vector2){ mousePos.x, bounds[COLORPICKER].y });
            else if (mousePos.y > bounds[COLORPICKER].y + bounds[COLORPICKER].height) SetMousePosition((Vector2){ mousePos.x, bounds[COLORPICKER].y + bounds[COLORPICKER].height });

        }
        
        // Control TextBox edit mode
        if (CheckCollisionPointRec(GetMousePosition(), bounds[TEXTBOX]) && (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))) editFilenameText = !editFilenameText;
        if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){ anchor02.x + 290, anchor02.y + 530, 65, 20 }) && (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))) editHexColorText = !editHexColorText;
        //----------------------------------------------------------------------------------
        
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        
            ClearBackground(RAYWHITE);
            
            // Draw background rectangle
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), GetColor(GuiGetStyleProperty(DEFAULT_BACKGROUND_COLOR)));
            
            // Draw info bar top
            GuiStatusBar((Rectangle){ anchor01.x + 0, anchor01.y + 0, 720, 24 }, "CHOOSE CONTROL     >      CHOOSE PROPERTY STYLE      >                            STYLE VIEWER", 35);

            // Draw status bar bottom
            //GuiStatusBar((Rectangle){ anchor01.x + 334, anchor01.y + 616, 386, 24 }, FormatText("EDITION TIME: %02i:%02i:%02i", (framesCounter/60)/(60*60), ((framesCounter/60)/60)%60, (framesCounter/60)%60), 10);
            // TODO: Review style info...
        #if defined(RAYGUI_STYLE_DEFAULT_LIGHT)
            GuiStatusBar((Rectangle){ anchor01.x + 0, anchor01.y + 616, 150, 24 }, "BASE STYLE: LIGHT", 10);
        #elif defined(RAYGUI_STYLE_DEFAULT_DARK)
            GuiStatusBar((Rectangle){ anchor01.x + 0, anchor01.y + 616, 150, 24 }, "BASE STYLE: DARK", 10);
        #else
            GuiStatusBar((Rectangle){ anchor01.x + 0, anchor01.y + 616, 150, 24 }, "BASE STYLE: UNKNOWN", 10);
        #endif
            GuiStatusBar((Rectangle){ anchor01.x + 149, anchor01.y + 616, 186, 24 }, FormatText("CHANGED PROPERTIES: %03i", changedControlsCounter), 10);
            GuiStatusBar((Rectangle){ anchor01.x + 334, anchor01.y + 616, 386, 24 }, "powered by raylib and raygui", 226);
            
            // Draw Gui controls
            currentSelectedControl = GuiListView(bounds[LISTVIEW], guiControlText, NUM_CONTROLS, currentSelectedControl);
            
            if (currentSelectedControl < 0) GuiDisable();
            
            switch (currentSelectedControl)
            {
                case DEFAULT: currentSelectedProperty = GuiListView((Rectangle){ anchor01.x + 155, anchor01.y + 40, 180, 560 }, guiPropsTextC, NUM_PROPS_CONTROLS_C, currentSelectedProperty); break;
                case LABELBUTTON: currentSelectedProperty = GuiListView((Rectangle){ anchor01.x + 155, anchor01.y + 40, 180, 560 }, guiPropsTextA, NUM_PROPS_CONTROLS_A, currentSelectedProperty); break;
                case SLIDER: case SLIDERBAR: case PROGRESSBAR: case CHECKBOX:
                case COLORPICKER: currentSelectedProperty = GuiListView((Rectangle){ anchor01.x + 155, anchor01.y + 40, 180, 560 }, guiPropsTextB, NUM_PROPS_CONTROLS_B, currentSelectedProperty); break;
                case BUTTON: case TOGGLE: case COMBOBOX: case TEXTBOX: case SPINNER: case LISTVIEW:
                default: currentSelectedProperty = GuiListView((Rectangle){ anchor01.x + 155, anchor01.y + 40, 180, 560 }, guiPropsTextC, NUM_PROPS_CONTROLS_C - 2, currentSelectedProperty); break;
            }

            GuiEnable();
            
            GuiWindowBox((Rectangle){ anchor02.x + 0, anchor02.y + 0, 365, 560 }, "Sample raygui controls");
            
            checked = GuiCheckBoxEx(bounds[CHECKBOX], checked, "DISABLED");

            if (checked) GuiDisable();

            GuiLabel((Rectangle){ anchor02.x + 11, anchor02.y + 35, 80, 25 }, "rGuiStyler");

            if (GuiLabelButton(bounds[LABELBUTTON], "github.com/raysan5/raygui")) {}
            
            toggleActive = GuiToggleButton(bounds[TOGGLE], "toggle", toggleActive);
            
            toggleValue = GuiToggleGroup((Rectangle){ anchor02.x + 90, anchor02.y + 70, 262, 30 }, toggleGuiText, 4, toggleValue);
            
            sliderValue = GuiSliderEx(bounds[SLIDER], sliderValue, 0, 100, "SLIDER", true);
            
            sliderBarValue = GuiSliderBarEx(bounds[SLIDERBAR], sliderBarValue, 0, 100, "SLIDERBAR", true);
            
            progressValue = GuiProgressBarEx(bounds[PROGRESSBAR], progressValue, 0, 1, true);

            spinnerValue = GuiSpinner(bounds[SPINNER], spinnerValue, 32, 24);
            
            comboActive = GuiComboBox(bounds[COMBOBOX], comboText, comboNum, comboActive);

            GuiTextBox(bounds[TEXTBOX], guiText, spinnerValue, editFilenameText);
            
            GuiLine((Rectangle){ anchor02.x + 10, anchor02.y + 275, 345, 20 }, 1);
            
            // Draw labels for GuiColorPicker information (RGBA)
            GuiGroupBox((Rectangle){ anchor02.x + 295, anchor02.y + 300, 60, 55 }, "RGBA");
            GuiLabel((Rectangle){ anchor02.x + 305, anchor02.y + 305, 15, 20 }, FormatText("R:   %03i", colorPickerValue.r));
            GuiLabel((Rectangle){ anchor02.x + 305, anchor02.y + 320, 15, 20 }, FormatText("G:   %03i", colorPickerValue.g));
            GuiLabel((Rectangle){ anchor02.x + 305, anchor02.y + 335, 15, 20 }, FormatText("B:   %03i", colorPickerValue.b));
           
            // Draw labels for GuiColorPicker information (HSV)
            GuiGroupBox((Rectangle){ anchor02.x + 295, anchor02.y + 365, 60, 55 }, "HSV");
            GuiLabel((Rectangle){ anchor02.x + 305, anchor02.y + 370, 15, 20 }, FormatText("H:  %.0f", colorHSV.x));
            GuiLabel((Rectangle){ anchor02.x + 305, anchor02.y + 385, 15, 20 }, FormatText("S:  %.0f%%", colorHSV.y*100));
            GuiLabel((Rectangle){ anchor02.x + 305, anchor02.y + 400, 15, 20 }, FormatText("V:  %.0f%%", colorHSV.z*100));

            if (GuiTextBox((Rectangle){ anchor02.x + 295, anchor02.y + 520, 60, 20 }, colorHex, 8, editHexColorText)) colorPickerValue = GetColor((int)strtoul(colorHex, NULL, 16));
            
            for (int i = 0; i < 12; i++) colorBoxValue[i] = ColorBox((Rectangle){ anchor02.x + 295 + 20*(i%3), anchor02.y + 430 + 20*(i/3), 20, 20 }, &colorPickerValue, colorBoxValue[i]);
            DrawRectangleLinesEx((Rectangle){ anchor02.x + 295, anchor02.y + 430, 60, 80 }, 2, GetColor(style[DEFAULT_BORDER_COLOR_NORMAL]));

            GuiEnable();
            
            colorPickerValue = GuiColorPicker(bounds[COLORPICKER], colorPickerValue);
            
            if (checked) GuiDisable();
            
            // Draw save style button
            if (GuiButton(bounds[BUTTON], "Save Style")) DialogSaveStyle(comboActive);

            dropdownBoxActive = GuiDropdownBox((Rectangle){ anchor02.x + 175, anchor02.y + 195, 60, 30 }, dropdownBoxList, 3, dropdownBoxActive);
            
            GuiEnable();
            
            // Draw selected control rectangles
            if (currentSelectedControl >= 0) DrawRectangleLinesEx((Rectangle){ bounds[currentSelectedControl].x - 2, bounds[currentSelectedControl].y -2, bounds[currentSelectedControl].width + 4, bounds[currentSelectedControl].height + 4 }, 1, RED);
            
            // Draw ending message window (save)
            if (closingWindowActive)
            {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(WHITE, 0.7f));
                closingWindowActive = !GuiWindowBox((Rectangle){ GetScreenWidth()/2 - 125, GetScreenHeight()/2 - 50, 250, 100 }, "Closing rGuiStyler");
                
                GuiLabel((Rectangle){ GetScreenWidth()/2 - 95, GetScreenHeight()/2 - 60, 200, 100 }, "Do you want to save before quitting?");
                
                if (GuiButton((Rectangle){ GetScreenWidth()/2 - 94, GetScreenHeight()/2 + 10, 85, 25 }, "Yes")) 
                { 
                    styleSaved = false;
                    DialogSaveStyle(comboActive);
                    if (styleSaved) exitWindow = true;
                }
                else if (GuiButton((Rectangle){ GetScreenWidth()/2 + 10, GetScreenHeight()/2 + 10, 85, 25 }, "No")) { exitWindow = true; }
            }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }
    // De-Initialization
    //--------------------------------------------------------------------------------------    
    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    
    return 0;
}

//--------------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//--------------------------------------------------------------------------------------------

// Show command line usage info
static void ShowUsageInfo(void)
{
    printf("\n//////////////////////////////////////////////////////////////////////////////////\n");
    printf("//                                                                              //\n");
    printf("// rGuiStyler v%s - A simple and easy-to-use raygui styles editor              //\n", TOOL_VERSION_TEXT);
    printf("// powered by raylib v2.0 (www.raylib.com) and raygui v2.0                      //\n");
    printf("// more info and bugs-report: github.com/raysan5/rguistyler                     //\n");
    printf("//                                                                              //\n");
    printf("// Copyright (c) 2017-2018 raylib technologies (@raylibtech)                    //\n");
    printf("//                                                                              //\n");
    printf("//////////////////////////////////////////////////////////////////////////////////\n\n");

#if defined(ENABLE_PRO_FEATURES)
    printf("USAGE:\n\n");
    printf("    > rguistyler [--help] --input <filename.ext> [--output <filename.ext>]\n");
    printf("                 [--format <styleformat>] [--edit-prop <property> <value>]\n");
    
    printf("\nOPTIONS:\n\n");
    printf("    -h, --help                      : Show tool version and command line usage help\n");
    printf("    -i, --input <filename.ext>      : Define input file.\n");
    printf("                                      Supported extensions: .rgs, .png\n");
    printf("    -o, --output <filename.ext>     : Define output file.\n");
    printf("                                      Supported extensions: .rgs, .png, .h\n");
    printf("                                      NOTE: Extension could be modified depending on format\n\n");
    printf("    -f, --format <type_value>       : Define output file format to export style data.\n");
    printf("                                      Supported values:\n");
    printf("                                          0 - Style text format (.rgs)  \n");          
    printf("                                          1 - Style binary format (.rgs)\n");    
    printf("                                          2 - Palette image (.png)\n");
    printf("                                          3 - Palette as int array (.h)\n");
    printf("                                          4 - Controls table image (.png)\n\n");
    printf("    -e, --edit-prop <property> <value>\n");
    printf("                                    : Edit specific property from input to output.\n");
    
    printf("\nEXAMPLES:\n\n");
    printf("    > rguistyler --input tools.rgs --output tools.png\n");
#endif
}

//--------------------------------------------------------------------------------------------
// Load/Save/Export data functions
//--------------------------------------------------------------------------------------------

// Save raygui style file (.rgs), text or binary
static void SaveStyle(const char *fileName, bool binary)
{
    #define NUM_COLOR_PROPERTIES 130

    if (binary)
    {
        #define RGS_FILE_VERSION_BINARY   200
        
        FILE *rgsFile = fopen(fileName, "wb");
        
        if (rgsFile != NULL)
        {
            // Write some header info (12 bytes)
            // id: "RGS "            - 4 bytes
            // version: 200          - 2 bytes
            // reserved              - 2 bytes
            // total properties      - 2 bytes
            // changed properties    - 2 bytes
            
            char signature[5] = "RGS ";
            short version = RGS_FILE_VERSION_BINARY;
            short reserved = 0;
            short numProperties = NUM_COLOR_PROPERTIES;
            short changedProperties = 0;
            
            for (int i = 0; i < NUM_PROPERTIES; i++) if (styleBackup[i] != style[i]) changedProperties++;

            fwrite(signature, 1, 4, rgsFile);
            fwrite(&version, 1, sizeof(short), rgsFile);
            fwrite(&reserved, 1, sizeof(short), rgsFile);
            fwrite(&numProperties, 1, sizeof(short), rgsFile);
            fwrite(&changedProperties, 1, sizeof(short), rgsFile);

            short id = 0;
            
            for (int i = 0; i < NUM_PROPERTIES; i++)
            {
                if (styleBackup[i] != style[i])
                {
                    id = (short)i;
                    
                    fwrite(&id, 1, 2, rgsFile);
                    fwrite(&style[i], 1, sizeof(int), rgsFile);
                }
            }
            
            fclose(rgsFile);
        }
    }
    else
    {
        #define RGS_FILE_VERSION_TEXT   "2.0"
        
        int counter = 0;
        FILE *rgsFile = fopen(fileName, "wt");
        
        if (rgsFile != NULL)
        {
            for (int i = 0; i < NUM_PROPERTIES; i++) if (styleBackup[i] != style[i]) counter++;
            
            // Write some description comments
            fprintf(rgsFile, "#\n# rgst file (v%s) - raygui style text file generated using rGuiStyler\n#\n", RGS_FILE_VERSION_TEXT);
            fprintf(rgsFile, "# Total number of properties:     %i\n", NUM_COLOR_PROPERTIES);
            fprintf(rgsFile, "# Number of properties changed:   %i\n", counter);
            fprintf(rgsFile, "# Required base default style:    %s\n#\n", "LIGHT");     // TODO: check base style

            for (int i = 0; i < NUM_PROPERTIES; i++)
            {
                if (styleBackup[i] != style[i]) fprintf(rgsFile, "%03i 0x%08x // %s\n", i, style[i], guiPropertyText[i]);      
            }

            fclose(rgsFile);
        }
    }
}

// Export style color palette
static void ExportStyle(const char *fileName, int type)
{
    switch (type)
    {
        case STYLE_TEXT: SaveStyle(fileName, false); break;
        case STYLE_BINARY: SaveStyle(fileName, true); break;
        case PALETTE_IMAGE:
        {
            // Export style palette image
            // NOTE: We use embedded image image_raygui_style_palette_light
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_BACKGROUND_COLOR]), GetColor(style[DEFAULT_BACKGROUND_COLOR]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_LINES_COLOR]), GetColor(style[DEFAULT_LINES_COLOR]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_BORDER_COLOR_NORMAL]), GetColor(style[DEFAULT_BORDER_COLOR_NORMAL]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_BASE_COLOR_NORMAL]), GetColor(style[DEFAULT_BASE_COLOR_NORMAL]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_TEXT_COLOR_NORMAL]), GetColor(style[DEFAULT_TEXT_COLOR_NORMAL]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_BORDER_COLOR_FOCUSED]), GetColor(style[DEFAULT_BORDER_COLOR_FOCUSED]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_BASE_COLOR_FOCUSED]), GetColor(style[DEFAULT_BASE_COLOR_FOCUSED]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_TEXT_COLOR_FOCUSED]), GetColor(style[DEFAULT_TEXT_COLOR_FOCUSED]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_BORDER_COLOR_PRESSED]), GetColor(style[DEFAULT_BORDER_COLOR_PRESSED]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_BASE_COLOR_PRESSED]), GetColor(style[DEFAULT_BASE_COLOR_PRESSED]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_TEXT_COLOR_PRESSED]), GetColor(style[DEFAULT_TEXT_COLOR_PRESSED]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_BORDER_COLOR_DISABLED]), GetColor(style[DEFAULT_BORDER_COLOR_DISABLED]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_BASE_COLOR_DISABLED]), GetColor(style[DEFAULT_BASE_COLOR_DISABLED]));
            ImageColorReplace(&image_raygui_style_palette_light, GetColor(styleBackup[DEFAULT_TEXT_COLOR_DISABLED]), GetColor(style[DEFAULT_TEXT_COLOR_DISABLED]));
        
            ExportImage(image_raygui_style_palette_light, fileName);
        }
        case PALETTE_CODE:
        {
            // TODO: Export style palette as int colors array code
        }
        case CONTROLS_TABLE_IMAGE:
        {
            // Export style controls table image
            // NOTE: We use embedded image raygui_style_table_light 
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_BACKGROUND_COLOR]), GetColor(style[DEFAULT_BACKGROUND_COLOR]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_LINES_COLOR]), GetColor(style[DEFAULT_LINES_COLOR]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_BORDER_COLOR_NORMAL]), GetColor(style[DEFAULT_BORDER_COLOR_NORMAL]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_BASE_COLOR_NORMAL]), GetColor(style[DEFAULT_BASE_COLOR_NORMAL]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_TEXT_COLOR_NORMAL]), GetColor(style[DEFAULT_TEXT_COLOR_NORMAL]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_BORDER_COLOR_FOCUSED]), GetColor(style[DEFAULT_BORDER_COLOR_FOCUSED]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_BASE_COLOR_FOCUSED]), GetColor(style[DEFAULT_BASE_COLOR_FOCUSED]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_TEXT_COLOR_FOCUSED]), GetColor(style[DEFAULT_TEXT_COLOR_FOCUSED]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_BORDER_COLOR_PRESSED]), GetColor(style[DEFAULT_BORDER_COLOR_PRESSED]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_BASE_COLOR_PRESSED]), GetColor(style[DEFAULT_BASE_COLOR_PRESSED]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_TEXT_COLOR_PRESSED]), GetColor(style[DEFAULT_TEXT_COLOR_PRESSED]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_BORDER_COLOR_DISABLED]), GetColor(style[DEFAULT_BORDER_COLOR_DISABLED]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_BASE_COLOR_DISABLED]), GetColor(style[DEFAULT_BASE_COLOR_DISABLED]));
            ImageColorReplace(&image_raygui_style_table_light, GetColor(styleBackup[DEFAULT_TEXT_COLOR_DISABLED]), GetColor(style[DEFAULT_TEXT_COLOR_DISABLED]));
            
            ExportImage(image_raygui_style_table_light, fileName);
        }
        default: break;
    }
}

// Dialog load style file
static void DialogLoadStyle(void)
{
    char currentPath[256];

    // Add sample file name to currentPath
    strcpy(currentPath, GetWorkingDirectory());
    strcat(currentPath, "\\\0");
    
    // Open file dialog
    const char *filters[] = { "*.rgs" };
    const char *fileName = tinyfd_openFileDialog("Load raygui style file", currentPath, 1, filters, "raygui Style Files (*.rgs)", 0);

    if (fileName != NULL) GuiLoadStyle(fileName);
}

// Dialog save style file
static void DialogSaveStyle(bool binary)
{
    char currentPathFile[256];

    // Add sample file name to currentPath
    strcpy(currentPathFile, GetWorkingDirectory());
    strcat(currentPathFile, "\\style.rgs\0");
    
    // Save file dialog
    const char *filters[] = { "*.rgs" };
    const char *fileName = tinyfd_saveFileDialog("Save raygui style text file", currentPathFile, 1, filters, "raygui Style Files (*.rgs)");

    if (fileName != NULL)
    {
        char outFileName[256] = { 0 };
        strcpy(outFileName, fileName);
        if (GetExtension(outFileName) == NULL) strcat(outFileName, ".rgs\0");     // No extension provided
        if (outFileName != NULL) SaveStyle(outFileName, binary);               // Save style file (text or binary)
        
        styleSaved = true;
    }
}

// Dialog export style file
static void DialogExportStyle(int type)
{
    // Save file dialog
    const char *filters[] = { "*.rgs", "*.png", "*.h" };
    const char *fileName = tinyfd_saveFileDialog("Export raygui style file", "", 3, filters, "Style Files (*.rgs, *.png, *.h)");

    // TODO: Check file extension for type?
    
    if (fileName != NULL)
    {
        ExportStyle(fileName, type);
    }
}

//--------------------------------------------------------------------------------------------
// Auxiliar GUI functions
//--------------------------------------------------------------------------------------------

// Get GUI property index based on control type
static int GetGuiStylePropertyIndex(int control, int property)
{
    int guiProp = -1;
    
    switch (control)
    {
        case DEFAULT: 
        {
            if (property == 12) guiProp = 0;
            else if (property == 13) guiProp = 1;
            else guiProp = DEFAULT_BORDER_COLOR_NORMAL + property; 
            
        } break;
        case LABELBUTTON: guiProp = LABEL_TEXT_COLOR_NORMAL + property; break;
        case BUTTON: guiProp = BUTTON_BORDER_COLOR_NORMAL + property; break;
        //case IMAGEBUTTON: guiProp = BUTTON_BORDER_COLOR_NORMAL + property; break;
        case TOGGLE: guiProp = TOGGLE_BORDER_COLOR_NORMAL + property; break; 
        //case TOGGLEGROUP: guiProp = TOGGLE_BORDER_COLOR_NORMAL + property; break;
        case SLIDER: guiProp = SLIDER_BORDER_COLOR_NORMAL + property; break;
        case SLIDERBAR: guiProp = SLIDERBAR_BORDER_COLOR_NORMAL + property; break;
        case PROGRESSBAR: guiProp = PROGRESSBAR_BORDER_COLOR_NORMAL + property; break;
        case CHECKBOX: guiProp = CHECKBOX_BORDER_COLOR_NORMAL + property; break;
        case SPINNER: guiProp = VALUEBOX_BORDER_COLOR_NORMAL + property; break;
        case COMBOBOX: guiProp = COMBOBOX_BORDER_COLOR_NORMAL + property; break;
        case TEXTBOX: guiProp = TEXTBOX_BORDER_COLOR_NORMAL + property; break;
        case LISTVIEW: guiProp = LISTVIEW_BORDER_COLOR_NORMAL + property; break;
        case COLORPICKER: guiProp = COLORPICKER_BORDER_COLOR_NORMAL + property; break;
        default: break;
    }

    //guiProp = LABEL_TEXT_COLOR_NORMAL + property/3;                   // type A
    //guiProp = SLIDER_BORDER_COLOR_NORMAL + property + property/2;     // type B
    //guiProp = TOGGLE_BORDER_COLOR_NORMAL + property;                  // type C

    return guiProp;
}

// Color box control to save color samples from color picker
// NOTE: It requires colorPicker pointer for updating in case of selection
static Color ColorBox(Rectangle bounds, Color *colorPicker, Color color)
{
    Vector2 mousePoint = GetMousePosition();
    
    // Update color box
    if (CheckCollisionPointRec(mousePoint, bounds))
    {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) *colorPicker = (Color){ color.r, color.g, color.b, color.a };
        else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) color = *colorPicker;
    }
    
    // Draw color box
    DrawRectangleRec(bounds, color);
    DrawRectangleLinesEx(bounds, 1, GetColor(style[DEFAULT_BORDER_COLOR_NORMAL]));
    
    return color;
}