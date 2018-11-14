/*******************************************************************************************
*
*   rGuiStyler v2.5 - A simple and easy-to-use raygui styles editor
*
*   CONFIGURATION:
*
*   #define VERSION_ONE
*       Enable PRO features for the tool. Usually command-line and export options related.
*
*   DEPENDENCIES:
*       raylib 2.1-dev          - Windowing/input management and drawing.
*       raygui 2.0              - IMGUI controls (based on raylib).
*       tinyfiledialogs 3.3.7   - Open/save file dialogs, it requires linkage with comdlg32 and ole32 libs.
*
*   COMPILATION (Windows - MinGW):
*       gcc -o rguistyler.exe rguistyler.c external/tinyfiledialogs.c -s -O2 -std=c99
*           -lraylib -lopengl32 -lgdi32 -lcomdlg32 -lole32
*
*   COMPILATION (Linux - GCC):
*       gcc -o rguistyler rguistyler.c external/tinyfiledialogs.c -s -no-pie -D_DEFAULT_SOURCE /
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
*   Copyright (c) 2014-2018 raylib technologies (@raylibtech).
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
#include "external/raygui.h"            // Required for: IMGUI controls

#include "external/tinyfiledialogs.h"   // Required for: Native open/save file dialogs

#include <stdlib.h>                     // Required for: malloc(), free()
#include <string.h>                     // Required for: strcmp()
#include <stdio.h>                      // Required for: fopen(), fclose(), fread()...

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define VERSION_ONE             // Enable PRO version features

#define TOOL_VERSION_TEXT     "2.5"     // Tool version string

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Style file type to export
typedef enum {
    STYLE_FILE = 0,         // style binary file (.rgs)
    PALETTE_IMAGE,          // style color palette image (only default style)
    PALETTE_CODE,           // style color code
    CONTROLS_TABLE_IMAGE    // style controls table (for reference)
} GuiStyleFileType;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------

// Default Light style backup to check changed properties
static unsigned int styleBackup[NUM_CONTROLS*(NUM_CONTROL_PROPS_DEFAULT + NUM_CONTROL_PROPS_EX)] = { 0 };

// Controls name text
// NOTE: Some styles are shared by multiple controls
static const char *guiControlText[NUM_CONTROLS] = {
    "DEFAULT",
    "LABEL",        // LABELBUTTON
    "BUTTON",       // IMAGEBUTTON
    "TOGGLE",       // TOGGLEGROUP
    "SLIDER",       // SLIDERBAR
    "PROGRESSBAR",
    "CHECKBOX",
    "COMBOBOX",
    "DROPDOWNBOX",
    "TEXTBOX",      // VALUEBOX, SPINNER
    "LISTVIEW",
    "COLORPICKER"
};

// Controls default properties name text
static const char *guiPropsText[NUM_CONTROL_PROPS_DEFAULT] = {
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
    "TEXT_SIZE",
    "TEXT_SPACING",
    "BORDER_WIDTH",
    "INNER_PADDING",
};

static bool styleSaved = false;             // Show save dialog on closing if not saved
static bool styleLoaded = false;            // Register if we are working over a loaded style (auto-save)

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
#if defined(VERSION_ONE)
static void ShowCommandLineInfo(void);                      // Show command line usage info
static void ProcessCommandLine(int argc, char *argv[]);     // Process command line input
#endif

// Load/Save/Export data functions
static void SaveStyle(const char *fileName);                // Save style file (.rgs)

static void ExportStyle(const char *fileName, int type);    // Export style color palette
static void ExportStyleAsCode(const char *fileName);        // Export gui style as color palette code

static void DialogLoadStyle(void);                          // Show dialog: load style file
static void DialogSaveStyle(bool binary);                   // Show dialog: save style file
static void DialogExportStyle(int type);                    // Show dialog: export style file

// Auxiliar functions
static Color GuiColorBox(Rectangle bounds, Color *colorPicker, Color color);    // Gui color box

static Image GenImageStylePalette(void);                    // Generate raygui palette image by code
static Image GenImageStyleControlsTable(const char *styleName, const char *styleCreator); // Draw controls table image

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
        if ((argc == 2) &&
            (strcmp(argv[1], "-h") != 0) &&
            (strcmp(argv[1], "--help") != 0))       // One argument (file dropped over executable?)
        {
            if (IsFileExtension(argv[1], ".rgs") ||
                IsFileExtension(argv[1], ".png"))
            {
                strcpy(inFileName, argv[1]);        // Read input filename to open with gui interface
            }
        }
#if defined(VERSION_ONE)
        else
        {
            ProcessCommandLine(argc, argv);
            return 0;
        }
#endif      // VERSION_ONE
    }

    // GUI usage mode - Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 720;
    const int screenHeight = 640;

    SetTraceLog(0);                             // Disable trace log messsages
    //SetConfigFlags(FLAG_WINDOW_RESIZABLE);    // Window configuration flags
    InitWindow(screenWidth, screenHeight, FormatText("rGuiStyler v%s - A simple and easy-to-use raygui styles editor", TOOL_VERSION_TEXT));
    SetExitKey(0);

    // General pourpose variables
    Vector2 mousePos = { 0.0f, 0.0f };
    int framesCounter = 0;
    bool exitWindow = false;
    bool closingWindowActive = false;

    bool saveColor = false;
    int changedControlsCounter = 0;

    // Gui related variables
    //-----------------------------------------------------------
    Vector2 anchorMain = { 0, 0 };
    Vector2 anchorControls = { 345, 40 };

    // Define gui controls rectangles
    Rectangle bounds[NUM_CONTROLS] = {
        (Rectangle){ 0, 0, 0, 0 },                                              // DEFAULT
        (Rectangle){ anchorControls.x + 85, anchorControls.y + 35, 145, 25 },   // LABEL
        (Rectangle){ anchorControls.x + 195, anchorControls.y + 240, 160, 30 }, // BUTTON
        (Rectangle){ anchorControls.x + 10, anchorControls.y + 70, 65, 30 },    // TOGGLE
        (Rectangle){ anchorControls.x + 75, anchorControls.y + 115, 250, 15 },  // SLIDER
        (Rectangle){ anchorControls.x + 10, anchorControls.y + 165, 315, 15 },  // PROGRESSBAR
        (Rectangle){ anchorControls.x + 270, anchorControls.y + 38, 20, 20 },   // CHECKBOX
        (Rectangle){ anchorControls.x + 240, anchorControls.y + 195, 115, 30 }, // SPINNER
        (Rectangle){ anchorControls.x + 10, anchorControls.y + 195, 160, 30 },  // COMBOBOX
        (Rectangle){ anchorControls.x + 10, anchorControls.y + 240, 180, 30 },  // TEXTBOX
        (Rectangle){ anchorMain.x + 10, anchorMain.y + 40, 140, 560 },          // LISTVIEW
        (Rectangle){ anchorControls.x + 10, anchorControls.y + 300, 240, 240 }  // COLORPICKER
    };

    // GUI controls data
    bool toggleValue = false;
    const char *toggleGuiText[4] = { "toggle", "group", "selection", "options" };

    int dropdownBoxActive = false;
    const char *dropdownBoxList[3] = { "A", "B", "C" };

    float sliderValue = 50.0f;
    float sliderBarValue = 20.0f;
    float progressValue = 0.0f;
    bool checkedActive = false;
    bool selectingColor = false;
    int spinnerValue = 10;

    int comboNum = 5;
    const char *comboText[5] = { "Text (.rgs)", "Binary (.rgs)", "Palette (.png)", "Palette (.h)", "Controls Table (.png)" };
    int comboActive = 0;

    char guiText[32] =  "custom_style.rgs";
    
    int currentSelectedControl = -1;        // listViewActive
    int currentSelectedProperty = -1;       // listView2Active
    int previousSelectedProperty = -1;
    int previousSelectedControl = -1;

    // Edit mode
    bool textBoxEditMode = false;
    bool hexValueBoxEditMode = false;
    bool spinnerEditMode = false;
    bool dropDownEditMode = false;
    //-----------------------------------------------------------
    
    // GUI variables definition
    //-----------------------------------------------------------
    int listViewControlsActive = 0;
    const char *listViewControlsTextList[3] = { "ONE", "TWO", "THREE" };
    int listViewPropsActive = 0;
    const char *listViewPropsTextList[3] = { "ONE", "TWO", "THREE" };
    
    bool windowControlsActive = true;
    const char *dropdownStateTextList[4] = { "NORMAL", "FOCUSED", "PRESSED", "DISABLED" };
    int dropdownStateActive = 0;
    bool dropdownStateEditMode = false;
    //int spinnerValue = 0;
    char textBoxText[64] = "this is a text box";
    //float sliderValue = 50.0f;
    //float sliderBarValue = 50.0f;
    bool checkBoxChecked = false;
    float progressBarValue = 50.0f;
    int comboFormatActive = 0;
    const char *comboFormatTextList[3] = { "ONE", "TWO", "THREE" };
    
    const char *dropdownFormatTextList[5] = { "Style Text (.rgs)", "Style Binary (.rgs)", "Palette (.png)", "Palette (.h)", "Controls Table (.png)" };
    int dropdownFormatActive = 0;
    bool dropdownFormatEditMode = false;

    bool toggleActive = false;
    Color colorPickerValue = RED;
    char hexColorText[9] = "00000000";
    //----------------------------------------------------------------------------------
    
    Color colorBoxValue[12];
    for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));
    Vector3 colorHSV = { 0.0f, 0.0f, 0.0f };

    // TODO: Keep a backup for base style
    //memcpy(styleBackup, GuiGetStyleDefault(), NUM_CONTROLS*(NUM_CONTROL_PROPS_DEFAULT + NUM_CONTROL_PROPS_EX)*sizeof(unsigned int));

    Font font = { 0 };

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
            else if (IsFileExtension(droppedFiles[0], ".ttf"))
            {
                UnloadFont(font);
                font = LoadFontEx(droppedFiles[0], spinnerValue, 0, 0);
                GuiFont(font);
            }

            for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));

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

        if (IsKeyPressed(KEY_T)) ExportStyle("test_controls_table.png", CONTROLS_TABLE_IMAGE);
        if (IsKeyPressed(KEY_Y)) ExportStyle("test_palette.png", PALETTE_IMAGE);
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

        // TODO: Check for changed controls
        if ((framesCounter%120) == 0)
        {
            changedControlsCounter = 0;
            //for (int i = 0; i < NUM_PROPERTIES; i++) if (styleBackup[i] != style[i)) changedControlsCounter++;
        }

        // Controls selection on GuiListView logic
        if ((previousSelectedControl != currentSelectedControl)) currentSelectedProperty = -1;

        if ((currentSelectedControl == 0) && (currentSelectedProperty != -1))
        {
            if ((previousSelectedProperty != currentSelectedProperty) || (previousSelectedControl != currentSelectedControl)) saveColor = false;

            if (!saveColor)
            {
                colorPickerValue = GetColor(GuiGetStyle(currentSelectedControl, currentSelectedProperty));
                saveColor = true;
            }

            GuiSetStyle(currentSelectedControl, currentSelectedProperty, ColorToInt(colorPickerValue));

            // TODO: REVIEW: Resets all updated controls!
            GuiUpdateStyleComplete();
        }
        else if ((currentSelectedControl != -1) && (currentSelectedProperty != -1))
        {
            if ((previousSelectedProperty != currentSelectedProperty) || (previousSelectedControl != currentSelectedControl)) saveColor = false;

            if (!saveColor)
            {
                colorPickerValue = GetColor(GuiGetStyle(currentSelectedControl, currentSelectedProperty));
                saveColor = true;
            }

            GuiSetStyle(currentSelectedControl, currentSelectedProperty, ColorToInt(colorPickerValue));
        }

        previousSelectedProperty = currentSelectedProperty;
        previousSelectedControl = currentSelectedControl;

        // Update progress bar automatically
        progressValue += 0.0005f;
        if (progressValue > 1.0f) progressValue = 0.0f;

        // Get edited color from text box
        if (!hexValueBoxEditMode) sprintf(hexColorText, "%02X%02X%02X%02X", colorPickerValue.r, colorPickerValue.g, colorPickerValue.b, colorPickerValue.a);

        colorHSV = ColorToHSV(colorPickerValue);

        // Color selection cursor show/hide logic
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
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

            // Draw info bar top
            GuiStatusBar((Rectangle){ anchorMain.x + 0, anchorMain.y + 0, 720, 24 }, "CHOOSE CONTROL     >      CHOOSE PROPERTY STYLE      >      STYLE VIEWER", 35);

            // Draw Gui controls
            GuiListView(bounds[LISTVIEW], guiControlText, NUM_CONTROLS, NULL, &currentSelectedControl, true);

            // TODO: Replace by GuiListViewEx() with disabled gui elements
            GuiListViewEx((Rectangle){ anchorMain.x + 155, anchorMain.y + 40, 180, 560 }, guiPropsText, NULL, NUM_CONTROL_PROPS_DEFAULT - 4, NULL, &currentSelectedProperty, NULL, true);

            if (dropDownEditMode) GuiLock();

            GuiWindowBox((Rectangle){ anchorControls.x + 0, anchorControls.y + 0, 365, 560 }, "Sample raygui controls");

            // Draw selected control rectangles
            if (currentSelectedControl > 0) DrawRectangleLinesEx((Rectangle){ bounds[currentSelectedControl].x - 4, bounds[currentSelectedControl].y - 4, bounds[currentSelectedControl].width + 8, bounds[currentSelectedControl].height + 8 }, 2, RED);
            
            checkedActive = GuiCheckBoxEx(bounds[CHECKBOX], checkedActive, "DISABLED");

            if (checkedActive) GuiDisable();

            GuiLabel((Rectangle){ anchorControls.x + 11, anchorControls.y + 35, 80, 25 }, "rGuiStyler");
            if (GuiLabelButton(bounds[LABEL], "github.com/raysan5/raygui")) {}
            toggleActive = GuiToggle(bounds[TOGGLE], "toggle", toggleActive);
            toggleValue = GuiToggleGroup((Rectangle){ anchorControls.x + 90, anchorControls.y + 70, 262, 30 }, toggleGuiText, 4, toggleValue);
            sliderValue = GuiSliderEx(bounds[SLIDER], sliderValue, 0, 100, "SLIDER", true);
            sliderBarValue = GuiSliderBarEx(bounds[SLIDER], sliderBarValue, 0, 100, "SLIDERBAR", true);
            progressValue = GuiProgressBarEx(bounds[PROGRESSBAR], progressValue, 0, 1, true);
            if (GuiSpinner(bounds[TEXTBOX], &spinnerValue, 0, 32, 24, spinnerEditMode)) spinnerEditMode = !spinnerEditMode;
            comboActive = GuiComboBox(bounds[COMBOBOX], comboText, comboNum, comboActive);
            if (GuiTextBox(bounds[TEXTBOX], guiText, 32, textBoxEditMode)) textBoxEditMode = !textBoxEditMode;
            GuiLine((Rectangle){ anchorControls.x + 10, anchorControls.y + 275, 345, 20 }, 1);

            // Draw labels for GuiColorPicker information (RGBA)
            GuiGroupBox((Rectangle){ anchorControls.x + 295, anchorControls.y + 300, 60, 55 }, "RGBA");
            GuiLabel((Rectangle){ anchorControls.x + 305, anchorControls.y + 305, 15, 20 }, FormatText("R:   %03i", colorPickerValue.r));
            GuiLabel((Rectangle){ anchorControls.x + 305, anchorControls.y + 320, 15, 20 }, FormatText("G:   %03i", colorPickerValue.g));
            GuiLabel((Rectangle){ anchorControls.x + 305, anchorControls.y + 335, 15, 20 }, FormatText("B:   %03i", colorPickerValue.b));

            // Draw labels for GuiColorPicker information (HSV)
            GuiGroupBox((Rectangle){ anchorControls.x + 295, anchorControls.y + 365, 60, 55 }, "HSV");
            GuiLabel((Rectangle){ anchorControls.x + 305, anchorControls.y + 370, 15, 20 }, FormatText("H:  %.0f", colorHSV.x));
            GuiLabel((Rectangle){ anchorControls.x + 305, anchorControls.y + 385, 15, 20 }, FormatText("S:  %.0f%%", colorHSV.y*100));
            GuiLabel((Rectangle){ anchorControls.x + 305, anchorControls.y + 400, 15, 20 }, FormatText("V:  %.0f%%", colorHSV.z*100));

            if (GuiTextBox((Rectangle){ anchorControls.x + 295, anchorControls.y + 520, 60, 20 }, hexColorText, 8, hexValueBoxEditMode))
            {
                hexValueBoxEditMode = !hexValueBoxEditMode;
                colorPickerValue = GetColor((int)strtoul(hexColorText, NULL, 16));
            }

            for (int i = 0; i < 12; i++) colorBoxValue[i] = GuiColorBox((Rectangle){ anchorControls.x + 295 + 20*(i%3), anchorControls.y + 430 + 20*(i/3), 20, 20 }, &colorPickerValue, colorBoxValue[i]);
            DrawRectangleLinesEx((Rectangle){ anchorControls.x + 295, anchorControls.y + 430, 60, 80 }, 2, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL)));

            colorPickerValue = GuiColorPicker(bounds[COLORPICKER], colorPickerValue);

            // Draw save style button
            if (GuiButton(bounds[BUTTON], "Save Style")) DialogSaveStyle(comboActive);
            if (GuiDropdownBox((Rectangle){ anchorControls.x + 175, anchorControls.y + 195, 60, 30 }, dropdownBoxList, 3, &dropdownBoxActive, dropDownEditMode)) dropDownEditMode = !dropDownEditMode;

            GuiUnlock();

            /*
            //----------------------------------------------------------------------------------
            GuiStatusBar((Rectangle){ anchor01.x + 0, anchor01.y + 0, 720, 24 }, "CHOOSE CONTROL     >      CHOOSE PROPERTY STYLE      >      STYLE VIEWER", 10);

            GuiListView((Rectangle){ anchor01.x + 10, anchor01.y + 40, 140, 560 }, guiControlText, NUM_CONTROLS, NULL, &currentSelectedControl, true);
            GuiListView((Rectangle){ anchor01.x + 155, anchor01.y + 40, 180, 560 }, listViewPropsTextList, 3, &listViewPropsScrollIndex, &listViewPropsActive, true);
            //GuiListViewEx((Rectangle){ anchorMain.x + 155, anchorMain.y + 40, 180, 560 }, guiPropsText, GetEnabledProperties(currentSelectedControl), NUM_PROPERTIES, NULL, &currentSelectedProperty, NULL, true);
            
            if (windowControlsActive)
            {
                windowControlsActive = !GuiWindowBox((Rectangle){ anchor02.x + 0, anchor02.y + 0, 365, 560 }, "Sample raygui controls");

                GuiLabel((Rectangle){ anchor02.x + 25, anchor02.y + 35, 80, 30 }, "Default State:");
                if (GuiDropdownBox((Rectangle){ anchor02.x + 105, anchor02.y + 35, 120, 30 }, dropdownStateTextList, 3, &dropdownStateActive, dropdownStateEditMode)) dropdownStateEditMode = !dropdownStateEditMode; 
                if (GuiButton((Rectangle){ anchor02.x + 235, anchor02.y + 35, 120, 30 }, "About")) BtnAbout(); 
                GuiLine((Rectangle){ anchor02.x + 10, anchor02.y + 65, 345, 20 }, 1);
                if (GuiButton((Rectangle){ anchor02.x + 10, anchor02.y + 90, 85, 30 }, "Load Font")) BtnLoadFont(); 
                if (GuiSpinner((Rectangle){ anchor02.x + 105, anchor02.y + 90, 120, 30 }, spinnerValue, 0, 100, 25, spinnerEditMode)) spinnerEditMode = !spinnerEditMode;
                GuiTextBox((Rectangle){ anchor02.x + 235, anchor02.y + 90, 120, 30 }, textBoxText, 64, true);
                sliderValue = GuiSliderEx((Rectangle){ anchor02.x + 105, anchor02.y + 135, 220, 15 }, sliderValue, 0, 100, "SLIDER", true);
                sliderBarValue = GuiSliderBarEx((Rectangle){ anchor02.x + 105, anchor02.y + 155, 220, 15 }, sliderBarValue, 0, 100, "SLIDERBAR", true);
                checkBoxChecked = GuiCheckBoxEx((Rectangle){ anchor02.x + 25, anchor02.y + 175, 15, 15 }, checkBoxChecked, "PROGRESS");
                progressBarValue = GuiProgressBarEx((Rectangle){ anchor02.x + 105, anchor02.y + 175, 220, 15 }, progressBarValue, 0, 100, true);
                GuiLabel((Rectangle){ anchor02.x + 15, anchor02.y + 205, 81, 30 }, "RESET STYLE");
                comboFormatActive = GuiComboBox((Rectangle){ anchor02.x + 105, anchor02.y + 205, 120, 30 }, comboFormatTextList, 3, comboFormatActive);
                if (GuiDropdownBox((Rectangle){ anchor02.x + 235, anchor02.y + 205, 120, 30 }, dropdownFormatTextList, 3, &dropdownFormatActive, dropdownFormatEditMode)) dropdownFormatEditMode = !dropdownFormatEditMode; 
                toggleActive = GuiToggleButton((Rectangle){ anchor02.x + 10, anchor02.y + 245, 85, 30 }, "toggle", toggleActive);
                if (GuiButton((Rectangle){ anchor02.x + 105, anchor02.y + 245, 120, 30 }, "Load Style")) BtnLoadStyle(); 
                if (GuiButton((Rectangle){ anchor02.x + 235, anchor02.y + 245, 120, 30 }, "Save Style")) BtnSaveStyle(); 
                GuiLine((Rectangle){ anchor02.x + 10, anchor02.y + 280, 345, 20 }, 1);
                colorPickerValue = GuiColorPicker((Rectangle){ anchor02.x + 10, anchor02.y + 305, 240, 240 }, colorPickerValue);
                GuiGroupBox((Rectangle){ anchor02.x + 290, anchor02.y + 305, 65, 55 }, "RGBA");
                GuiLabel((Rectangle){ anchor02.x + 300, anchor02.y + 310, 20, 20 }, "R:");
                GuiLabel((Rectangle){ anchor02.x + 300, anchor02.y + 325, 20, 20 }, "G:");
                GuiLabel((Rectangle){ anchor02.x + 300, anchor02.y + 340, 20, 20 }, "B:");
                GuiGroupBox((Rectangle){ anchor02.x + 290, anchor02.y + 370, 65, 55 }, "HSV");
                GuiLabel((Rectangle){ anchor02.x + 300, anchor02.y + 375, 20, 20 }, "H:");
                GuiLabel((Rectangle){ anchor02.x + 300, anchor02.y + 390, 20, 20 }, "S:");
                GuiLabel((Rectangle){ anchor02.x + 300, anchor02.y + 405, 20, 20 }, "V:");
                GuiDummyRec((Rectangle){ anchor02.x + 290, anchor02.y + 435, 65, 80 }, "Panel");
                GuiTextBox((Rectangle){ anchor02.x + 290, anchor02.y + 525, 65, 20 }, hexColorText, 64, true);
            }
            
            GuiStatusBar((Rectangle){ anchor01.x + 0, anchor01.y + 616, 150, 24 }, "STYLE LOADED", 10);
            GuiStatusBar((Rectangle){ anchor01.x + 149, anchor01.y + 616, 186, 24 }, FormatText("CHANGED PROPERTIES: %03i", changedControlsCounter), 10);
            GuiStatusBar((Rectangle){ anchor01.x + 334, anchor01.y + 616, 386, 24 }, "", 10);
            //GuiStatusBar((Rectangle){ anchorMain.x + 334, anchorMain.y + 616, 386, 24 }, FormatText("EDITION TIME: %02i:%02i:%02i", (framesCounter/60)/(60*60), ((framesCounter/60)/60)%60, (framesCounter/60)%60), 10);
            //----------------------------------------------------------------------------------
            */
            
            // Draw font texture if available
            if (IsKeyDown(KEY_SPACE) && (font.texture.id > 0))
            {
                DrawRectangle(GetScreenWidth()/2 - font.texture.width/2, GetScreenHeight()/2 - font.texture.height/2, font.texture.width, font.texture.height, BLACK);
                DrawTexture(font.texture, GetScreenWidth()/2 - font.texture.width/2, GetScreenHeight()/2 - font.texture.height/2, WHITE);
            }

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
    UnloadFont(font);
    
    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//--------------------------------------------------------------------------------------------

#if defined(VERSION_ONE)
// Show command line usage info
static void ShowCommandLineInfo(void)
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
    //printf("    -e, --edit-prop <property> <value>\n");
    //printf("                                    : Edit specific property from input to output.\n");

    printf("\nEXAMPLES:\n\n");
    printf("    > rguistyler --input tools.rgs --output tools.png\n");
}

// Process command line input
static void ProcessCommandLine(int argc, char *argv[])
{
    // CLI required variables
    bool showUsageInfo = false;     // Toggle command line usage info

    char inFileName[256] = { 0 };   // Input file name
    char outFileName[256] = { 0 };  // Output file name
    int outputFormat = 0;           // Formats: STYLE_FILE, PALETTE_IMAGE, CONTROLS_TABLE_IMAGE, PALETTE_CODE

    // Arguments scan and processing
    for (int i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
        {
            showUsageInfo = true;
        }
        else if ((strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--input") == 0))
        {
            // Check for valid argumment and valid file extension: input
            if (((i + 1) < argc) && (argv[i + 1][0] != '-') &&
                IsFileExtension(inFileName, ".png"))
            {
                strcpy(inFileName, argv[i + 1]);    // Read input file
            }

            i++;
        }
        else if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--output") == 0))
        {
            // Check for valid argumment and valid file extension: output
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                strcpy(outFileName, argv[i + 1]);    // Read output filename
            }

            i++;
        }
        else if (strcmp(argv[i], "--format") == 0)
        {
            // Check for valid argumment and valid parameters
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
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

    if (showUsageInfo) ShowCommandLineInfo();
}
#endif      // VERSION_ONE

//--------------------------------------------------------------------------------------------
// Load/Save/Export data functions
//--------------------------------------------------------------------------------------------

// Save raygui style file (.rgs)
static void SaveStyle(const char *fileName)
{
    FILE *rgsFile = NULL;
#if defined(RAYGUI_STYLE_SAVE_AS_TEXT)
    rgsFile = fopen(fileName, "wt");
    
    if (rgsFile != NULL)
    {
        // Write some description comments
        fprintf(rgsFile, "\n//////////////////////////////////////////////////////////////////////////////////\n");
        fprintf(rgsFile, "//                                                                              //\n");
        fprintf(rgsFile, "// raygui style exporter v2.0 - Style export as text file                       //\n");
        fprintf(rgsFile, "// more info and bugs-report: github.com/raysan5/raygui                         //\n");
        fprintf(rgsFile, "//                                                                              //\n");
        fprintf(rgsFile, "// Copyright (c) 2018 Ramon Santamaria (@raysan5)                               //\n");
        fprintf(rgsFile, "//                                                                              //\n");
        fprintf(rgsFile, "//////////////////////////////////////////////////////////////////////////////////\n\n");
        
        fprintf(rgsFile, "## Style information\n");
        fprintf(rgsFile, "NUM_CONTROLS                       %i\n", NUM_CONTROLS);
        fprintf(rgsFile, "NUM_CONTROL_PROPERTIES_DEFAULT    %i\n", NUM_CONTROL_PROPS_DEFAULT);
        fprintf(rgsFile, "NUM_CONTROL_PROPERTIES_EXTENDED   %i\n", NUM_CONTROL_PROPS_EX);

        // NOTE: Control properties are just written as hexadecimal values, no control name info provided
        // To print control properties names, enum values hould be stored as string arrays also;
        // as long as .rgs text save mode is just intended for debug pourpose, not included that info.
        for (int i = 0; i < NUM_CONTROLS; i++)
        {
            if (i == 0) fprintf(rgsFile, "## DEFAULT properties\n");
            else fprintf(rgsFile, "## CONTROL %02i default properties\n", i);
            
            for (int j = 0; j < NUM_CONTROL_PROPS_DEFAULT + NUM_CONTROL_PROPS_EX; j++) 
            {
                if (j == NUM_CONTROL_PROPS_DEFAULT) fprintf(rgsFile, "## CONTROL %02i extended properties\n", i);
                fprintf(rgsFile, "0x%08x\n", GuiGetStyle(i, j));
            }
            
            fprintf(rgsFile, "\n");
        }
    }
#else
    rgsFile = fopen(fileName, "wb");

    if (rgsFile != NULL)
    {
        // Style File Structure (.rgs)
        // ------------------------------------------------------
        // Offset | Size  | Type       | Description
        // ------------------------------------------------------
        // 0      | 4     | char       | Signature: "rGS " // ID
        // 4      | 2     | short      | Version: 200
        // 6      | 2     | short      | reserved
        // 8      | 2     | short      | # Controls
        // 10     | 2     | short      | # Props Default
        // 12     | 2     | short      | # Props Extended
        // 14     | 2     | short      | reserved
        
        // 16     | N     | int        | Properties Data (NUM_CONTROLS*(NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED))
        
        // 16+N   | M     | -          | Custom Font Data
        // ------------------------------------------------------

        unsigned char value = 0;
        
        char signature[5] = "rGS ";
        short version = 200;
        short numControls = NUM_CONTROLS;
        short numPropsDefault = NUM_CONTROL_PROPS_DEFAULT;
        short numPropsExtended = NUM_CONTROL_PROPS_EX;

        fwrite(signature, 1, 4, rgsFile);
        fwrite(&version, 1, sizeof(short), rgsFile);
        fwrite(&numControls, 1, sizeof(short), rgsFile);
        fwrite(&numPropsDefault, 1, sizeof(short), rgsFile);
        fwrite(&numPropsExtended, 1, sizeof(short), rgsFile);

        for (int i = 0; i < NUM_CONTROLS; i++)
        {
            for (int j = 0; j < NUM_CONTROL_PROPS_DEFAULT + NUM_CONTROL_PROPS_EX; j++) 
            {
                value = GuiGetStyle(i, j);
                fwrite(&value, 1, 4, rgsFile);
            }
        }
        
        // TODO: Write font data (embedding)
        // Need to save IMAGE data (GRAYSCALE?) and CHAR data
    }
#endif
    if (rgsFile != NULL) fclose(rgsFile);
}

// Export style color palette
static void ExportStyle(const char *fileName, int type)
{
    switch (type)
    {
        case STYLE_FILE: GuiSaveStyle(fileName); break;
        case PALETTE_CODE: ExportStyleAsCode(fileName); break;
        case PALETTE_IMAGE:
        {
            Image imStylePal = GenImageStylePalette();
            ExportImage(imStylePal, fileName);
            UnloadImage(imStylePal);

        } break;
        case CONTROLS_TABLE_IMAGE:
        {
            Image imStyleTable = GenImageStyleControlsTable("raygui_light", "@raysan5");
            ExportImage(imStyleTable, fileName);
            UnloadImage(imStyleTable);
            
        } break;
        default: break;
    }
}

// Generate raygui palette image by code
static Image GenImageStylePalette(void)
{
    Image image = GenImageColor(64, 16, GetColor(GuiGetStyle(DEFAULT, LINES_COLOR)));
    
    for (int i = 0; i < 4; i++) 
    {
        ImageDrawRectangle(&image, (Rectangle){ 15*i + 1, 1, 14, 14 }, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        ImageDrawRectangle(&image, (Rectangle){ 15*i + 2, 2, 12, 12 }, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + 3*i)));
        ImageDrawRectangle(&image, (Rectangle){ 15*i + 3, 3, 10, 10 }, GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL + 3*i)));
        ImageDrawRectangle(&image, (Rectangle){ 15*i + 3, 3, 10, 10 }, GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL + 3*i)));
        
        // Draw "rl" characters
        ImageDrawRectangle(&image, (Rectangle){ 15*i + 5, 7, 3, 1 }, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL + 3*i)));
        ImageDrawRectangle(&image, (Rectangle){ 15*i + 5, 8, 1, 3 }, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL + 3*i)));
        ImageDrawRectangle(&image, (Rectangle){ 15*i + 9, 4, 1, 7 }, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL + 3*i)));
        ImageDrawRectangle(&image, (Rectangle){ 15*i + 9, 4, 1, 7 }, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL + 3*i)));
        ImageDrawRectangle(&image, (Rectangle){ 15*i + 10, 10, 1, 1 }, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL + 3*i)));
    }
    
    return image;
}

// Draw controls table image
static Image GenImageStyleControlsTable(const char *styleName, const char *styleCreator)
{
    #define TABLE_LEFT_PADDING      15
    #define TABLE_TOP_PADDING       20

    #define TABLE_CELL_HEIGHT       40
    #define TABLE_CELL_PADDING       4
    
    #define TABLE_CONTROLS_COUNT    12
    
    typedef enum {
        LABEL = 0,
        BUTTON,
        TOGGLE,
        CHECKBOX,
        SLIDER,
        SLIDERBAR,
        PROGRESSBAR,
        COMBOBOX,
        DROPDOWNBOX,
        TEXTBOX,
        VALUEBOX,
        SPINNER
    } TableControlType;
    
    static const char *tableStateName[4] = { "NORMAL", "FOCUSED", "PRESSED", "DISABLED" };
    static const char *tableControlsName[TABLE_CONTROLS_COUNT] = {
        "LABEL",        // LABELBUTTON
        "BUTTON",
        "TOGGLE",       // TOGGLEGROUP
        "CHECKBOX",
        "SLIDER",
        "SLIDERBAR",
        "PROGRESSBAR",
        "COMBOBOX",
        "DROPDOWNBOX",
        "TEXTBOX",      // TEXTBOXMULTI
        "VALUEBOX",
        "SPINNER"       // VALUEBOX + BUTTON
    };
    
    // TODO: Controls grid with should be calculated
    int controlGridWidth[TABLE_CONTROLS_COUNT] = {
        80,     // LABEL
        100,    // BUTTON
        100,    // TOGGLE
        160,    // CHECKBOX
        100,    // SLIDER
        100,    // SLIDERBAR
        100,    // PROGRESSBAR
        130,    // COMBOBOX,
        130,    // DROPDOWNBOX
        100,    // TEXTBOX
        100,    // VALUEBOX
        100,    // SPINNER
    };
    
    int tableControlsNameWidth = 85;
    
    int tableWidth = 0;
    int tableHeight = 256;
    
    // TODO: Compute proper texture size depending on font size and controls text!
    tableWidth = TABLE_LEFT_PADDING*2 + tableControlsNameWidth;
    for (int i = 0; i < TABLE_CONTROLS_COUNT; i++) tableWidth += (controlGridWidth[i] - 1);

    const char *comboBoxText[2] = { "ComboBox", "ComboBox" };
    const char *dropdownBoxText[2] = { "DropdownBox", "DropdownBox" };
    int dropdownActive = 0;
    int value = 40;

    Rectangle rec = { 0 };      // Current drawing rectangle space

    // NOTE: If loading texture when render-texture is active, it seem to fail 
    Image imStylePal = GenImageStylePalette();
    Texture2D texStylePal = LoadTextureFromImage(imStylePal);
    UnloadImage(imStylePal);
    
    RenderTexture2D target = LoadRenderTexture(tableWidth, tableHeight);
    
    GuiSetStyle(SLIDER, SLIDER_WIDTH, 10);

    // Texture rendering
    //--------------------------------------------------------------------------------------------
    BeginTextureMode(target);
    
        // Draw style title
        DrawText("raygui style table: ", TABLE_LEFT_PADDING, 20, 10, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_DISABLED)));
        DrawText(FormatText("%s by %s", styleName, styleCreator), TABLE_LEFT_PADDING + MeasureText("raygui style table: ", 10), 20, 10, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
        
        // Draw left column
        rec = (Rectangle){ TABLE_LEFT_PADDING, TABLE_TOP_PADDING + TABLE_CELL_HEIGHT/2 + 20, tableControlsNameWidth, TABLE_CELL_HEIGHT };
        
        for (int i = 0; i < 4; i++)
        {
            GuiGroupBox(rec, NULL);
            DrawTextureRec(texStylePal, (Rectangle){ 2 + i*15, 2, 12, 12 }, (Vector2){ rec.x + 6, rec.y + TABLE_CELL_HEIGHT/2 - 12/2 }, WHITE);
            GuiState(i); GuiLabelButton((Rectangle){ rec.x + 24, rec.y, rec.width, rec.height }, tableStateName[i]);
            rec.y += TABLE_CELL_HEIGHT - 1;             // NOTE: We add/remove 1px to draw lines overlapped!
        }
        
        GuiState(GUI_STATE_NORMAL);
        
        int offsetWidth = TABLE_LEFT_PADDING + tableControlsNameWidth;
        
        //GuiLock();
        
        // Draw basic controls
        for (int i = 0; i < TABLE_CONTROLS_COUNT; i++)
        {
            rec = (Rectangle){ offsetWidth - i - 1, TABLE_TOP_PADDING + 20, controlGridWidth[i], TABLE_CELL_HEIGHT/2 + 1 };

            // Draw grid lines: control name
            GuiGroupBox(rec, NULL);
            GuiLabelEx(rec, tableControlsName[i], 1, 0);
            rec.y += TABLE_CELL_HEIGHT/2;
            rec.height = TABLE_CELL_HEIGHT;
            
            // Draw control 4 states: DISABLED, NORMAL, FOCUSED, PRESSED
            for (int j = 0; j < 4; j++)
            {
                // Draw grid lines: control state
                GuiGroupBox(rec, NULL);

                GuiState(j);
                    // Draw control centered correctly in grid
                    switch(i)
                    {
                        case LABEL: GuiLabelButton((Rectangle){ rec.x + rec.width/2 - MeasureText("Label", 10)/2, rec.y, 80, 40 }, "Label"); break;
                        case BUTTON: GuiButton((Rectangle){ rec.x + rec.width/2 - 90/2, rec.y + rec.height/2 - 20/2, 90, 20 }, "Button"); break;
                        case TOGGLE: GuiToggle((Rectangle){ rec.x + rec.width/2 - 90/2, rec.y + rec.height/2 - 20/2, 90, 20 }, "Toggle", false); break;
                        case CHECKBOX: 
                        {
                            GuiCheckBoxEx((Rectangle){ rec.x + 10, rec.y + rec.height/2 - 15/2, 15, 15 }, false, "NoCheck");
                            DrawRectangle(rec.x + rec.width/2, rec.y, 1, TABLE_CELL_HEIGHT, GetColor(GuiGetStyle(DEFAULT, LINES_COLOR)));
                            GuiCheckBoxEx((Rectangle){ rec.x + 10 + controlGridWidth[i]/2, rec.y + rec.height/2 - 15/2, 15, 15 }, true, "Checked");
                        } break;
                        case SLIDER: GuiSlider((Rectangle){ rec.x + rec.width/2 - 90/2, rec.y + rec.height/2 - 10/2, 90, 10 }, 40, 0, 100); break;
                        case SLIDERBAR: GuiSliderBar((Rectangle){ rec.x + rec.width/2 - 90/2, rec.y + rec.height/2 - 10/2, 90, 10 }, 40, 0, 100); break;
                        case PROGRESSBAR: GuiProgressBar((Rectangle){ rec.x + rec.width/2 - 90/2, rec.y + rec.height/2 - 10/2, 90, 10 }, 60, 0, 100); break;
                        case COMBOBOX: GuiComboBox((Rectangle){ rec.x + rec.width/2 - 120/2, rec.y + rec.height/2 - 20/2, 120, 20 }, comboBoxText, 2, 0); break;
                        case DROPDOWNBOX: GuiDropdownBox((Rectangle){ rec.x + rec.width/2 - 120/2, rec.y + rec.height/2 - 20/2, 120, 20 }, dropdownBoxText, 2, &dropdownActive, false); break;
                        case TEXTBOX: GuiTextBox((Rectangle){ rec.x + rec.width/2 - 90/2, rec.y + rec.height/2 - 20/2, 90, 20 }, "text box", 32, false); break;
                        case VALUEBOX: GuiValueBox((Rectangle){ rec.x + rec.width/2 - 90/2, rec.y + rec.height/2 - 20/2, 90, 20 }, &value, 0, 100, false); break;
                        case SPINNER: GuiSpinner((Rectangle){ rec.x + rec.width/2 - 90/2, rec.y + rec.height/2 - 20/2, 90, 20 }, &value, 0, 100, 18, false); break;
                        default: break;
                    }
                GuiState(GUI_STATE_NORMAL);
                
                rec.y += TABLE_CELL_HEIGHT - 1;
            }
            
            offsetWidth += controlGridWidth[i];
        }
        
        //GuiUnlock();
        
        // Draw copyright and software info (bottom-right)
        DrawText("raygui style table automatically generated with rGuiStyler", TABLE_LEFT_PADDING, tableHeight - 30, 10, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_DISABLED)));
        DrawText("rGuiStyler created by raylib technologies (@raylibtech)", tableWidth - MeasureText("rGuiStyler created by raylib technologies (@raylibtech)", 10) - 20, tableHeight - 30, 10, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_DISABLED)));
    
    EndTextureMode();
    //--------------------------------------------------------------------------------------------

    Image imStyleTable = GetTextureData(target.texture);
    ImageFlipVertical(&imStyleTable);

    UnloadRenderTexture(target);
    UnloadTexture(texStylePal);
    
    return imStyleTable;
}

// Export gui style as color palette code
// NOTE: Currently only default color palette is supported
static void ExportStyleAsCode(const char *fileName)
{
    FILE *txtFile = fopen(fileName, "wt");

    fprintf(txtFile, "\n//////////////////////////////////////////////////////////////////////////////////\n");
    fprintf(txtFile, "//                                                                              //\n");
    fprintf(txtFile, "// StyleAsCode exporter v1.0 - Style data exported as an array values           //\n");
    fprintf(txtFile, "//                                                                              //\n");
    fprintf(txtFile, "// more info and bugs-report:  github.com/raysan5/rguistyler                    //\n");
    fprintf(txtFile, "// feedback and support:       ray[at]raylib.com                                //\n");
    fprintf(txtFile, "//                                                                              //\n");
    fprintf(txtFile, "// Copyright (c) 2018 Ramon Santamaria (@raysan5)                               //\n");
    fprintf(txtFile, "//                                                                              //\n");
    fprintf(txtFile, "//////////////////////////////////////////////////////////////////////////////////\n\n");

    fprintf(txtFile, "// raygui custom color style palette\n");
    fprintf(txtFile, "// NOTE: Only default colors defined, expanded to all properties\n");
    fprintf(txtFile, "// NOTE: Use GuiLoadStylePalette(stylePalette);\n");

    // Write byte data as hexadecimal text
    fprintf(txtFile, "static const unsigned int stylePalette[%i] = {\n", 14);
    fprintf(txtFile, "    0x%08x,    // DEFAULT_BACKGROUND_COLOR\n", GuiGetStyle(DEFAULT, BACKGROUND_COLOR));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_LINES_COLOR\n", GuiGetStyle(DEFAULT, LINES_COLOR));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_BORDER_COLOR_NORMAL\n", GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_BASE_COLOR_NORMAL\n", GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_TEXT_COLOR_NORMAL\n", GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_BORDER_COLOR_FOCUSED\n", GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_BASE_COLOR_FOCUSED\n", GuiGetStyle(DEFAULT, BASE_COLOR_FOCUSED));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_TEXT_COLOR_FOCUSED\n", GuiGetStyle(DEFAULT, TEXT_COLOR_FOCUSED));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_BORDER_COLOR_PRESSED\n", GuiGetStyle(DEFAULT, BORDER_COLOR_PRESSED));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_BASE_COLOR_PRESSED\n", GuiGetStyle(DEFAULT, BASE_COLOR_PRESSED));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_TEXT_COLOR_PRESSED\n", GuiGetStyle(DEFAULT, TEXT_COLOR_PRESSED));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_BORDER_COLOR_DISABLED\n", GuiGetStyle(DEFAULT, BORDER_COLOR_DISABLED));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_BASE_COLOR_DISABLED\n", GuiGetStyle(DEFAULT, BASE_COLOR_DISABLED));
    fprintf(txtFile, "    0x%08x,    // DEFAULT_TEXT_COLOR_DISABLED\n", GuiGetStyle(DEFAULT, TEXT_COLOR_DISABLED));
    fprintf(txtFile, "};\n");

    fclose(txtFile);
}

// Dialog load style file
static void DialogLoadStyle(void)
{
    // Open file dialog
    const char *filters[] = { "*.rgs" };
    const char *fileName = tinyfd_openFileDialog("Load raygui style file", "", 1, filters, "raygui Style Files (*.rgs)", 0);

    if (fileName != NULL)
    {
        GuiLoadStyle(fileName);
        SetWindowTitle(FormatText("rGuiStyler v%s - %s", TOOL_VERSION_TEXT, GetFileName(fileName)));

        // TODO: Register input fileName
        styleLoaded = true;
    }
}

// Dialog save style file
static void DialogSaveStyle(bool binary)
{
    // Save file dialog
    const char *filters[] = { "*.rgs" };
    const char *fileName = tinyfd_saveFileDialog("Save raygui style file", "style.rgs", 1, filters, "raygui Style Files (*.rgs)");

    if (fileName != NULL)
    {
        char outFileName[256] = { 0 };
        strcpy(outFileName, fileName);

        // Check for valid extension and make sure it is
        if ((GetExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".rgs")) strcat(outFileName, ".rgs\0");

        // Save style file (text or binary)
        GuiSaveStyle(outFileName);
        styleSaved = true;
    }
}

// Dialog export style file
static void DialogExportStyle(int type)
{
    // Save file dialog
    const char *filters[] = { "*.png", "*.h" };
    const char *fileName = tinyfd_saveFileDialog("Export raygui style file", "", 2, filters, "Style Files (*.rgs, *.png, *.h)");

    // TODO: Check file extension for type?

    if (fileName != NULL)
    {
        ExportStyle(fileName, type);
    }
}

//--------------------------------------------------------------------------------------------
// Auxiliar GUI functions
//--------------------------------------------------------------------------------------------

// Color box control to save color samples from color picker
// NOTE: It requires colorPicker pointer for updating in case of selection
static Color GuiColorBox(Rectangle bounds, Color *colorPicker, Color color)
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
    DrawRectangleLinesEx(bounds, 1, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL)));

    return color;
}

/*
// Load style text file
static LoadStyleText(const char *fileName)
{
    int i = 0, j = 0;
    unsigned int value = 0;
    char buffer[256] = { 0 };

    FILE *rgsFile = fopen(fileName, "rt");

    if (rgsFile != NULL)
    {
        fgets(buffer, 256, rgsFile);

        while (!feof(rgsFile))
        {
            if ((buffer[0] != '\n') && (buffer[0] != '#'))
            {
                sscanf(buffer, "0x%x", &value);
                GuiSetStyle(i, j, value);
                j++;
                
                if (j >= NUM_CONTROL_PROPS_DEFAULT + NUM_CONTROL_PROPS_EX) i++;
            }

            fgets(buffer, 256, rgsFile);
        }

        fclose(rgsFile);
    }
}
*/
