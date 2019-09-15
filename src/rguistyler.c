/*******************************************************************************************
*
*   rGuiStyler v3.1 - A simple and easy-to-use raygui styles editor
*
*   CONFIGURATION:
*
*   #define VERSION_ONE
*       Enable PRO features for the tool. Usually command-line and export options related.
*
*   #define CUSTOM_MODAL_DIALOGS
*       Use custom raygui generated modal dialogs instead of native OS ones
*       NOTE: Avoids including tinyfiledialogs depencency library
*
*   DEPENDENCIES:
*       raylib 2.6-dev          - Windowing/input management and drawing.
*       raygui 2.6              - Immediate-mode GUI controls.
*       tinyfiledialogs 3.3.9   - Open/save file dialogs, it requires linkage with comdlg32 and ole32 libs.
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
*       Ramon Santamaria (@raysan5):    Supervision, review, redesign, update and maintenance.
*       Adria Arranz (@Adri102):        Developer and designer, implemented v2.0 (2018)
*       Jordi Jorba (@KoroBli):         Developer and designer, implemented v2.0 (2018)
*       Sergio Martinez (@anidealgift): Development and testing v1.0 (2015..2017)
*
*   LICENSE: Propietary License
*
*   Copyright (c) 2017-2019 raylib technologies (@raylibtech). All Rights Reserved.
*
*   Unauthorized copying of this file, via any medium is strictly prohibited
*   This project is proprietary and confidential unless the owner allows
*   usage in any other form by expresely written permission.
*
**********************************************************************************************/

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_RICONS
#include "external/raygui.h"                // Required for: IMGUI controls

#undef RAYGUI_IMPLEMENTATION                // Avoid including raygui implementation again

#define GUI_WINDOW_ABOUT_IMPLEMENTATION
#include "gui_window_about.h"               // GUI: About Window

#if !defined(PLATFORM_WEB) && !defined(PLATFORM_ANDROID)
    #include "external/tinyfiledialogs.h"   // Required for: Native open/save file dialogs
#endif

#include <stdlib.h>                         // Required for: malloc(), free()
#include <string.h>                         // Required for: strcmp()
#include <stdio.h>                          // Required for: fopen(), fclose(), fread()...

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
const char *toolName = "rGuiStyler";
const char *toolVersion = "3.1";
const char *toolDescription = "A simple and easy-to-use raygui styles editor";

#if (!defined(DEBUG) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)))
bool __stdcall FreeConsole(void);       // Close console from code (kernel32.lib)
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Style file type to export
typedef enum {
    STYLE_TEXT = 0,         // Style text file (.rgs)
    STYLE_TABLE_IMAGE,      // Style controls table image (for reference)
    STYLE_BINARY,           // Style binary file (.rgs)
    STYLE_AS_CODE           // Style as (ready-to-use) code (.h)
} GuiStyleFileType;

// Dialog type
typedef enum DialogType {
    DIALOG_OPEN = 0,
    DIALOG_SAVE,
    DIALOG_MESSAGE,
    DIALOG_TEXTINPUT,
    DIALOG_OTHER
} DialogType;
/*
// Style property
typedef struct StyleProp {
    int controlId;
    int propertyId;
    int propertyValue;
} StyleProp;
*/
//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------

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
    "TEXTBOX",      // TEXTBOXMULTI
    "VALUEBOX",
    "SPINNER",
    "LISTVIEW",
    "COLORPICKER",
    "SCROLLBAR",
    "STATUSBAR"
};

// Controls default properties name text
static const char *guiPropsText[NUM_PROPS_DEFAULT] = {
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
    "BORDER_WIDTH",
    "TEXT_PADDING",
    "TEXT_ALIGNMENT",
    "RESERVED"
};

// Controls default extended properties
static const char *guiPropsExText[NUM_PROPS_EXTENDED] = {
    "TEXT_SIZE",
    "TEXT_SPACING",
    "LINE_COLOR",
    "BACKGROUND_COLOR",
    "EXTENDED01",
    "EXTENDED02",
    "EXTENDED03",
    "EXTENDED04",
};

// Default style backup to check changed properties
static unsigned int styleBackup[NUM_CONTROLS*(NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED)] = { 0 };

// Custom font variables
static Font font = { 0 };               // Custom font
static bool customFont = false;         // Using custom font
static char fontFilePath[512] = { 0 };  // Font file path (register font path for reloading)
static bool fontFileProvided = false;   // Font loaded from a file provided (required for reloading)

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
#if defined(VERSION_ONE)
static void ShowCommandLineInfo(void);                      // Show command line usage info
static void ProcessCommandLine(int argc, char *argv[]);     // Process command line input
#endif

// Load/Save/Export data functions
static bool SaveStyle(const char *fileName, int format);    // Save style file text or binary (.rgs/.rgsb) 
static void ExportStyleAsCode(const char *fileName, const char *styleName);        // Export gui style as color palette code
static Image GenImageStyleControlsTable(const char *styleName); // Draw controls table image

// Auxiliar functions
static int StyleChangesCounter(void);                       // Count changed properties in current style
static Color GuiColorBox(Rectangle bounds, Color *colorPicker, Color color);    // Gui color box

// Multiplatform file dialogs
// NOTE 1: fileName parameters is used to display and store selected file name
// NOTE 2: Value returned is the operation result, on custom dialogs represents button option pressed
// NOTE 3: filters and message are used for buttons and dialog messages on DIALOG_MESSAGE and DIALOG_TEXTINPUT
static int GuiFileDialog(int dialogType, const char *title, char *fileName, const char *filters, const char *message);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
#if !defined(DEBUG)
    SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messsages
#endif
    char inFileName[512] = { 0 };       // Input file name (required in case of drag & drop over executable)
    char outFileName[512] = { 0 };      // Output file name (required for file save/export)

    // Command-line usage mode
    //--------------------------------------------------------------------------------------
    if (argc > 1)
    {
        if ((argc == 2) &&
            (strcmp(argv[1], "-h") != 0) &&
            (strcmp(argv[1], "--help") != 0))       // One argument (file dropped over executable?)
        {
            if (IsFileExtension(argv[1], ".rgs"))
            {
                strcpy(inFileName, argv[1]);    // Read input filename to open with gui interface
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

#if (!defined(DEBUG) && defined(VERSION_ONE) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)))
    // WARNING (Windows): If program is compiled as Window application (instead of console),
    // no console is available to show output info... solution is compiling a console application
    // and closing console (FreeConsole()) when changing to GUI interface
    FreeConsole();
#endif

    // GUI usage mode - Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 740;
    const int screenHeight = 660;

    InitWindow(screenWidth, screenHeight, FormatText("%s v%s - %s", toolName, toolVersion, toolDescription));
    SetExitKey(0);

    // General pourpose variables
    Vector2 mousePos = { 0.0f, 0.0f };
    int framesCounter = 0;

    int changedPropsCounter = 0;
    bool obtainProperty = false;
    bool selectingColor = false;
    
    // Load dropped file if provided
    if (inFileName[0] != '\0') 
    {
        GuiLoadStyle(inFileName);
        SetWindowTitle(FormatText("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
    }
    else
    {
        GuiLoadStyleDefault();
        font = GetFontDefault();
    }

    // Keep a backup for default light style (used to track changes)
    for (int i = 0; i < NUM_CONTROLS; i++)
    {
        for (int j = 0; j < (NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED); j++) styleBackup[i*(NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED) + j] = GuiGetStyle(i, j);
    }

    // Init color picker saved colors
    Color colorBoxValue[12] = { 0 };
    for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));
    Vector3 colorHSV = { 0.0f, 0.0f, 0.0f };

    Texture texStyleTable = { 0 };
    int styleTablePositionX = 0;

    float fontScale = 1.0f;
    int genFontSizeValue = 10;       // Generation font size
    int prevGenFontSize = genFontSizeValue;

    // Style required variables
    bool saveChangesRequired = false;     // Flag to notice save changes are required
    char styleNameText[32] = "default";   // Style name

    // GUI: Main Layout
    //-----------------------------------------------------------------------------------
    Vector2 anchorMain = { 0, 0 };
    Vector2 anchorWindow = { 345, 60 };
    Vector2 anchorPropEditor = { 355, 95 };
    Vector2 anchorFontOptions = { 355, 465 };

    bool viewStyleTableActive = false;
    bool viewFontActive = false;
    bool propsStateEditMode = false;
    int propsStateActive = 0;

    bool styleNameEditMode = false;

    bool prevViewStyleTableState = viewStyleTableActive;

    int currentSelectedControl = -1;
    int currentSelectedProperty = -1;
    int previousSelectedProperty = -1;
    int previousSelectedControl = -1;

    bool windowControlsActive = true;
    bool propertyValueEditMode = false;
    int propertyValue = 0;

    Color colorPickerValue = RED;
    bool textHexColorEditMode = false;
    char hexColorText[9] = "00000000";
    int textAlignmentActive = 0;
    bool genFontSizeEditMode = false;
    bool fontSpacingEditMode = false;
    int fontSpacingValue = GuiGetStyle(DEFAULT, TEXT_SPACING);
    bool fontSampleEditMode = false;
    char fontSampleText[128] = "sample text";
    int exportFormatActive = 0;
    //-----------------------------------------------------------------------------------

    // GUI: About Window
    //-----------------------------------------------------------------------------------
    GuiWindowAboutState windowAboutState = InitGuiWindowAbout();
    //-----------------------------------------------------------------------------------

    // GUI: Exit Window
    //-----------------------------------------------------------------------------------
    bool exitWindow = false;
    bool windowExitActive = false;
    //-----------------------------------------------------------------------------------
    
    // GUI: Custom file dialogs
    //-----------------------------------------------------------------------------------
    bool showLoadFileDialog = false;
    bool showLoadFontFileDialog = false;
    bool showSaveFileDialog = false;
    bool showExportFileDialog = false;
    //-----------------------------------------------------------------------------------
    
#if defined(STYLES_SPINNING_DEMO)
    int styleCounter = 0;
    char stylesList[8][64] = {
        "D:\\GitHub\\raygui/styles/jungle/jungle.rgs\0",
        "D:\\GitHub\\raygui/styles/candy/candy.rgs\0",
        "D:\\GitHub\\raygui/styles/bluish/bluish.rgs\0",
        "D:\\GitHub\\raygui/styles/cherry/cherry.rgs\0",
        "D:\\GitHub\\raygui/styles/ashes/ashes.rgs\0",
        "D:\\GitHub\\raygui/styles/cyber/cyber.rgs\0",
        "D:\\GitHub\\raygui/styles/lavanda/lavanda.rgs\0",
        "D:\\GitHub\\raygui/styles/terminal/terminal.rgs\0"
    };
#endif

    // Render texture to draw full screen, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    RenderTexture2D screenTarget = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(screenTarget.texture, FILTER_POINT);
    int screenScale = 1;

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
            if (IsFileExtension(droppedFiles[0], ".rgs")) 
            {
                GuiLoadStyleDefault();          // Reset to base default style
                GuiLoadStyle(droppedFiles[0]);  // Load new style properties

                strcpy(inFileName, droppedFiles[0]);
                SetWindowTitle(FormatText("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
                strcpy(styleNameText, GetFileNameWithoutExt(droppedFiles[0]));

                genFontSizeValue = GuiGetStyle(DEFAULT, TEXT_SIZE);
                fontSpacingValue = GuiGetStyle(DEFAULT, TEXT_SPACING);

                // Load .rgs custom font in font
                font = GuiGetFont();
                memset(fontFilePath, 0, 512);
                fontFileProvided = false;
                customFont = true;
                
                // TODO: Reset style backup for changes?
                //changedPropsCounter = 0;
                //saveChangesRequired = false;
            }
            else if (IsFileExtension(droppedFiles[0], ".ttf") || IsFileExtension(droppedFiles[0], ".otf"))
            {
                UnloadFont(font);

                // NOTE: Font generation size depends on spinner size selection
                font = LoadFontEx(droppedFiles[0], genFontSizeValue, NULL, 0);
                
                if (font.texture.id > 0)
                {
                    GuiSetFont(font);
                    strcpy(fontFilePath, droppedFiles[0]);
                    fontFileProvided = true;
                    customFont = true;
                }
            }

            for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));

            ClearDroppedFiles();

            currentSelectedControl = -1;    // Reset selected control
        }
        //----------------------------------------------------------------------------------

        // Keyboard shortcuts
        //----------------------------------------------------------------------------------
#if defined(STYLES_SPINNING_DEMO)
        if (IsKeyPressed(KEY_RIGHT))
        {           
            GuiLoadStyleDefault();          // Reset to base default style
            GuiLoadStyle(stylesList[styleCounter]);  // Load new style properties

            strcpy(inFileName, GetFileName(stylesList[styleCounter]));
            SetWindowTitle(FormatText("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
            strcpy(styleNameText, GetFileNameWithoutExt(inFileName));

            genFontSizeValue = GuiGetStyle(DEFAULT, TEXT_SIZE);
            fontSpacingValue = GuiGetStyle(DEFAULT, TEXT_SPACING);

            // Load .rgs custom font in font
            font = GuiGetFont();
            memset(fontFilePath, 0, 512);
            fontFileProvided = false;
            customFont = true;
            
            styleCounter++;
            if (styleCounter > 7) styleCounter = 0;
        }
#endif

        // Show dialog: load input file (.rgs)
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) showLoadFileDialog = true;

        // Show dialog: save style file (.rgs)
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S))
        {
            if (inFileName[0] == '\0')
            {
                exportFormatActive = STYLE_TEXT;
                showSaveFileDialog = true;
            }
            else
            {
                // TODO: Use same style type as loaded or previously saved
                // ISSUE: Style is loaded by raygui, GuiLoadStyle() checks for format internally!
                SaveStyle(inFileName, STYLE_TEXT);
                SetWindowTitle(FormatText("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
                saveChangesRequired = false;
            }
        }

        // Show dialog: export style file (.rgs, .png, .h)
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_E)) showExportFileDialog = true;

        // Show window: about
        if (IsKeyPressed(KEY_F1)) windowAboutState.windowAboutActive = true;

        // Show closing window on ESC
        if (IsKeyPressed(KEY_ESCAPE))
        {
            if (windowAboutState.windowAboutActive) windowAboutState.windowAboutActive = false;
            else if (changedPropsCounter > 0) windowExitActive = !windowExitActive;
            else exitWindow = true;
        }
        
        // Reset to default light style
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R))
        {
            currentSelectedControl = -1;
            currentSelectedProperty = -1;
            
            GuiLoadStyleDefault();
            
            memset(inFileName, 0, 512);
            SetWindowTitle(FormatText("%s v%s", toolName, toolVersion));
            strcpy(styleNameText, "default");
            memset(fontFilePath, 0, 512);
            customFont = false;
            
            genFontSizeValue = GuiGetStyle(DEFAULT, TEXT_SIZE);
            fontSpacingValue = GuiGetStyle(DEFAULT, TEXT_SPACING);
            
            for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));
        }   
        //----------------------------------------------------------------------------------

        // Basic program flow logic
        //----------------------------------------------------------------------------------
        framesCounter++;                    // General usage frames counter
        mousePos = GetMousePosition();      // Get mouse position each frame
        if (WindowShouldClose()) exitWindow = true;

        // Check for changed properties
        changedPropsCounter = StyleChangesCounter();
        if (changedPropsCounter > 0) saveChangesRequired = true;
        
        // Reload font to new size if required
        if (fontFileProvided && !genFontSizeEditMode && (prevGenFontSize != genFontSizeValue) && (fontFilePath != NULL))
        {
            UnloadFont(font);
            font = LoadFontEx(fontFilePath, genFontSizeValue, NULL, 0);
            GuiSetFont(font);
        }
        
        GuiSetStyle(DEFAULT, TEXT_SIZE, genFontSizeValue);
        GuiSetStyle(DEFAULT, TEXT_SPACING, fontSpacingValue);
        
        prevGenFontSize = genFontSizeValue;

        // Controls selection on list view logic
        //----------------------------------------------------------------------------------
        if ((previousSelectedControl != currentSelectedControl)) currentSelectedProperty = -1;

        if ((currentSelectedControl >= 0) && (currentSelectedProperty >= 0))
        {
            if ((previousSelectedProperty != currentSelectedProperty) && !obtainProperty) obtainProperty = true;

            if (obtainProperty)
            {
                if (currentSelectedProperty <= TEXT_COLOR_DISABLED) colorPickerValue = GetColor(GuiGetStyle(currentSelectedControl, currentSelectedProperty));
                else if ((currentSelectedProperty == BORDER_WIDTH) || (currentSelectedProperty == TEXT_PADDING)) propertyValue = GuiGetStyle(currentSelectedControl, currentSelectedProperty);
                else if (currentSelectedProperty == TEXT_ALIGNMENT) textAlignmentActive = GuiGetStyle(currentSelectedControl, currentSelectedProperty);

                obtainProperty = false;
            }

            // Update control property
            // NOTE: In case DEFAULT control selected, we propagate changes to all controls
            if (currentSelectedProperty <= TEXT_COLOR_DISABLED)
            {
                if (currentSelectedControl == DEFAULT) 
                {
                    GuiSetStyle(0, currentSelectedProperty, ColorToInt(colorPickerValue));
                    for (int i = 1; i < NUM_CONTROLS; i++) GuiSetStyle(i, currentSelectedProperty, ColorToInt(colorPickerValue));
                }
                else GuiSetStyle(currentSelectedControl, currentSelectedProperty, ColorToInt(colorPickerValue));
            }
            else if ((currentSelectedProperty == BORDER_WIDTH) || (currentSelectedProperty == TEXT_PADDING))
            {
                if (currentSelectedControl == DEFAULT) 
                {
                    GuiSetStyle(0, currentSelectedProperty, propertyValue);
                    for (int i = 1; i < NUM_CONTROLS; i++) GuiSetStyle(i, currentSelectedProperty, propertyValue);
                }
                else GuiSetStyle(currentSelectedControl, currentSelectedProperty, propertyValue);
            }
            else if (currentSelectedProperty == TEXT_ALIGNMENT)
            {
                if (currentSelectedControl == DEFAULT) 
                {
                    GuiSetStyle(0, currentSelectedProperty, textAlignmentActive);
                    for (int i = 1; i < NUM_CONTROLS; i++) GuiSetStyle(i, currentSelectedProperty, textAlignmentActive);
                }
                else GuiSetStyle(currentSelectedControl, currentSelectedProperty, textAlignmentActive);
            }
        }

        previousSelectedProperty = currentSelectedProperty;
        previousSelectedControl = currentSelectedControl;
        //----------------------------------------------------------------------------------

        // Color selection logic (text box and color picker)
        //----------------------------------------------------------------------------------
        if (!textHexColorEditMode) sprintf(hexColorText, "%02X%02X%02X%02X", colorPickerValue.r, colorPickerValue.g, colorPickerValue.b, colorPickerValue.a);

        colorHSV = ColorToHSV(colorPickerValue);

        // Color selection cursor show/hide logic
        Rectangle colorPickerRec = { anchorPropEditor.x + 10, anchorPropEditor.y + 55, 240, 240 };
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, colorPickerRec)) selectingColor = true;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            selectingColor = false;
            ShowCursor();
        }

        if (selectingColor)
        {
            HideCursor();
            if (mousePos.x < colorPickerRec.x) SetMousePosition(colorPickerRec.x, mousePos.y);
            else if (mousePos.x > colorPickerRec.x + colorPickerRec.width) SetMousePosition(colorPickerRec.x + colorPickerRec.width, mousePos.y);

            if (mousePos.y < colorPickerRec.y) SetMousePosition(mousePos.x, colorPickerRec.y);
            else if (mousePos.y > colorPickerRec.y + colorPickerRec.height) SetMousePosition(mousePos.x, colorPickerRec.y + colorPickerRec.height);
        }
        //----------------------------------------------------------------------------------

        // Style table image generation (only on toggle activation) and logic
        //----------------------------------------------------------------------------------
        if (viewStyleTableActive && (prevViewStyleTableState != viewStyleTableActive))
        {
            UnloadTexture(texStyleTable);

            Image imStyleTable = GenImageStyleControlsTable(styleNameText);
            texStyleTable = LoadTextureFromImage(imStyleTable);
            UnloadImage(imStyleTable);
        }

        if (viewStyleTableActive)
        {
            if (IsKeyDown(KEY_RIGHT)) styleTablePositionX += 5;
            else if (IsKeyDown(KEY_LEFT)) styleTablePositionX -= 5;
        }

        prevViewStyleTableState = viewStyleTableActive;
        //----------------------------------------------------------------------------------

        // Font image scale logic
        //----------------------------------------------------------------------------------
        if (viewFontActive)
        {
            fontScale += GetMouseWheelMove();
            if (fontScale < 1.0f) fontScale = 1.0f;
            if (font.texture.width*fontScale > GetScreenWidth()) fontScale = GetScreenWidth()/font.texture.width;
        }
        //----------------------------------------------------------------------------------
        
        // Screen scale logic (x2)
        //----------------------------------------------------------------------------------
        
        // TODO: Use an interface button -> main bar?
        if (IsKeyPressed(KEY_ONE))
        {
            screenScale = 1;
            SetWindowSize(screenWidth, screenHeight);
            SetMouseScale(1.0f, 1.0f);
        }
        else if (IsKeyPressed(KEY_TWO))
        {
            screenScale = 2;
            SetWindowSize(screenWidth*2, screenHeight*2);
            SetMouseScale(0.5f, 0.5f);
        }
        
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        
            // Render all screen to a texture (for scaling)
            BeginTextureMode(screenTarget);
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

            if (windowAboutState.windowAboutActive || windowExitActive) GuiDisable();
            else GuiEnable();
            
            // Main GUI
            //---------------------------------------------------------------------------------------------------------

            // Main toolbar panel
            GuiPanel((Rectangle){ 0, 0, 740, 50 });
            if (GuiButton((Rectangle){ anchorMain.x + 10, anchorMain.y + 10, 30, 30 }, "#1#")) showLoadFileDialog = true;
            if (GuiButton((Rectangle){ 45, 10, 30, 30 }, "#2#")) showSaveFileDialog = true;
            if (GuiButton((Rectangle){ 80, 10, 70, 30 }, "#191#ABOUT")) windowAboutState.windowAboutActive = true;

            if (GuiTextBox((Rectangle){ 155, 10, 180, 30 }, styleNameText, 32, styleNameEditMode)) styleNameEditMode = !styleNameEditMode;

            viewStyleTableActive = GuiToggle((Rectangle){ 345, 10, 30, 30 }, "#101#", viewStyleTableActive);
            viewFontActive = GuiToggle((Rectangle){ 380, 10, 30, 30 }, "#31#", viewFontActive);
            windowControlsActive = GuiToggle((Rectangle){ 415, 10, 30, 30 }, "#198#", windowControlsActive);

            // NOTE: Supporting custom gui state set makes a bit difficult to disable all gui on WindowAbout,
            // tried different options and allowing direct state change is the less problematic,
            // left other tests commented just in case
            //if ((GuiGetState() != GUI_STATE_DISABLED) || (propsStateActive == GUI_STATE_DISABLED)) 
            GuiSetState(propsStateActive);

            if (viewStyleTableActive || viewFontActive || propsStateEditMode) GuiLock();

            currentSelectedControl = GuiListView((Rectangle){ anchorMain.x + 10, anchorMain.y + 60, 140, 560 }, TextJoin(guiControlText, NUM_CONTROLS, ";"), NULL, currentSelectedControl);

            if ((currentSelectedControl == -1) && (propsStateActive == 0)) GuiDisable();
            currentSelectedProperty = GuiListViewEx((Rectangle){ anchorMain.x + 155, anchorMain.y + 60, 180, 560 }, guiPropsText, NUM_PROPS_DEFAULT - 1, NULL, NULL, currentSelectedProperty);
            if ((propsStateActive == GUI_STATE_NORMAL) && !(windowAboutState.windowAboutActive || windowExitActive)) GuiEnable();

            if (windowControlsActive)
            {
                windowControlsActive = !GuiWindowBox((Rectangle){ anchorWindow.x + 0, anchorWindow.y + 0, 385, 560 }, "#198#Sample raygui controls");

                GuiGroupBox((Rectangle){ anchorPropEditor.x + 0, anchorPropEditor.y + 0, 365, 357 }, "Property Editor");

                if ((currentSelectedProperty != TEXT_PADDING) && (currentSelectedProperty != BORDER_WIDTH) && (propsStateActive == 0)) GuiDisable();
                propertyValue = GuiSlider((Rectangle){ anchorPropEditor.x + 45, anchorPropEditor.y + 15, 235, 15 }, "Value:", NULL, propertyValue, 0, 20);
                if (GuiValueBox((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 10, 60, 25 }, NULL, &propertyValue, 0, 8, propertyValueEditMode)) propertyValueEditMode = !propertyValueEditMode;
                if ((propsStateActive == GUI_STATE_NORMAL) && !(windowAboutState.windowAboutActive || windowExitActive)) GuiEnable();

                GuiLine((Rectangle){ anchorPropEditor.x + 0, anchorPropEditor.y + 35, 365, 15 }, NULL);
                colorPickerValue = GuiColorPicker((Rectangle){ anchorPropEditor.x + 10, anchorPropEditor.y + 55, 240, 240 }, colorPickerValue);

                GuiGroupBox((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 60, 60, 55 }, "RGBA");
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 65, 20, 20 }, FormatText("R:   %03i", colorPickerValue.r));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 80, 20, 20 }, FormatText("G:   %03i", colorPickerValue.g));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 95, 20, 20 }, FormatText("B:   %03i", colorPickerValue.b));
                GuiGroupBox((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 125, 60, 55 }, "HSV");
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 130, 20, 20 }, FormatText("H:  %.0f", colorHSV.x));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 145, 20, 20 }, FormatText("S:  %.0f%%", colorHSV.y*100));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 160, 20, 20 }, FormatText("V:  %.0f%%", colorHSV.z*100));

                if (GuiTextBox((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 275, 60, 20 }, hexColorText, 9, textHexColorEditMode))
                {
                    textHexColorEditMode = !textHexColorEditMode;
                    colorPickerValue = GetColor((int)strtoul(hexColorText, NULL, 16));
                }

                // Draw colors selector palette
                for (int i = 0; i < 12; i++) colorBoxValue[i] = GuiColorBox((Rectangle){ anchorPropEditor.x + 295 + 20*(i%3), anchorPropEditor.y + 190 + 20*(i/3), 20, 20 }, &colorPickerValue, colorBoxValue[i]);
                DrawRectangleLinesEx((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 190, 60, 80 }, 2, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL)));

                GuiLine((Rectangle){ anchorPropEditor.x + 0, anchorPropEditor.y + 300, 365, 15 }, NULL);

                if ((currentSelectedProperty != TEXT_ALIGNMENT) && (propsStateActive == 0)) GuiDisable();
                GuiLabel((Rectangle){ anchorPropEditor.x + 10, anchorPropEditor.y + 320, 85, 25 }, "Text Alignment:");
                textAlignmentActive = GuiToggleGroup((Rectangle){ anchorPropEditor.x + 95, anchorPropEditor.y + 320, 85, 25 }, "#87#LEFT;#89#CENTER;#83#RIGHT", textAlignmentActive);
                if ((propsStateActive == GUI_STATE_NORMAL) && !(windowAboutState.windowAboutActive || windowExitActive)) GuiEnable();

                GuiGroupBox((Rectangle){ anchorFontOptions.x + 0, anchorFontOptions.y + 0, 365, 100 }, "Font Options");
                if (GuiButton((Rectangle){ anchorFontOptions.x + 10, anchorFontOptions.y + 15, 85, 30 }, "#30#Load")) showLoadFontFileDialog = true;

                if (GuiSpinner((Rectangle){ anchorFontOptions.x + 135, anchorFontOptions.y + 15, 80, 30 }, "Size:", &genFontSizeValue, 8, 32, genFontSizeEditMode)) genFontSizeEditMode = !genFontSizeEditMode;
                if (GuiSpinner((Rectangle){ anchorFontOptions.x + 275, anchorFontOptions.y + 15, 80, 30 }, "Spacing:", &fontSpacingValue, 0, 8, fontSpacingEditMode)) fontSpacingEditMode = !fontSpacingEditMode;

                if (GuiTextBox((Rectangle){ anchorFontOptions.x + 10, anchorFontOptions.y + 55, 345, 35 }, fontSampleText, 128, fontSampleEditMode)) fontSampleEditMode = !fontSampleEditMode;
#if defined(VERSION_ONE)
                exportFormatActive = GuiComboBox((Rectangle){ anchorPropEditor.x, 575, 190, 30 }, "Style Text (.rgs);Style Table (.png);Style Binary (.rgsb);Style Code (.h)", exportFormatActive);
#else
                exportFormatActive = GuiComboBox((Rectangle){ anchorPropEditor.x, 575, 190, 30 }, "Style Text (.rgs);Style Table (.png)", exportFormatActive);
#endif
                if (GuiButton((Rectangle){ anchorPropEditor.x + 195, 575, 170, 30 }, "#7#Export Style")) showExportFileDialog = true;
            }

            GuiStatusBar((Rectangle){ anchorMain.x + 0, anchorMain.y + 635, 151, 25 }, NULL);
            GuiStatusBar((Rectangle){ anchorMain.x + 150, anchorMain.y + 635, 186, 25 }, FormatText("CHANGED PROPERTIES: %i", changedPropsCounter));

            if (fontFileProvided) GuiStatusBar((Rectangle){ anchorMain.x + 335, anchorMain.y + 635, 405, 25 }, FormatText("FONT: %s (%i x %i) - %i bytes", GetFileName(fontFilePath), font.texture.width, font.texture.height, GetPixelDataSize(font.texture.width, font.texture.height, font.texture.format)));
            else GuiStatusBar((Rectangle){ anchorMain.x + 335, anchorMain.y + 635, 405, 25 }, FormatText("FONT: %s (%i x %i) - %i bytes", (customFont)? "style custom font" : "raylib default", font.texture.width, font.texture.height, GetPixelDataSize(font.texture.width, font.texture.height, font.texture.format)));

            //if (GuiGetState() != GUI_STATE_DISABLED) 
            GuiSetState(GUI_STATE_NORMAL);

            GuiUnlock();

            GuiLabel((Rectangle){ 580 - MeasureTextEx(font, "State:", genFontSizeValue, fontSpacingValue).x - 10, 10, 35, 30 }, "State:");
            if (GuiDropdownBox((Rectangle){ 580, 10, 150, 30 }, "NORMAL;FOCUSED;PRESSED;DISABLED", &propsStateActive, propsStateEditMode)) propsStateEditMode = !propsStateEditMode;
            //------------------------------------------------------------------------------------------------------------------------

            // Draw font texture
            if (viewFontActive)
            {
                DrawRectangle(0, 50, GetScreenWidth(), GetScreenHeight() - 75, Fade(GRAY, 0.8f));
                DrawRectangle(GetScreenWidth()/2 - font.texture.width*fontScale/2, GetScreenHeight()/2 - font.texture.height*fontScale/2, font.texture.width*fontScale, font.texture.height*fontScale, BLACK);
                DrawRectangleLines(GetScreenWidth()/2 - font.texture.width*fontScale/2, GetScreenHeight()/2 - font.texture.height*fontScale/2, font.texture.width*fontScale, font.texture.height*fontScale, RED);
                DrawTextureEx(font.texture, (Vector2){ GetScreenWidth()/2 - font.texture.width*fontScale/2, GetScreenHeight()/2 - font.texture.height*fontScale/2 }, 0.0f, fontScale, WHITE);
            }

            // Draw style table image (if active and reloaded)
            if (viewStyleTableActive && (prevViewStyleTableState == viewStyleTableActive))
            {
                DrawRectangle(0, 50, GetScreenWidth(), GetScreenHeight() - 75, Fade(GRAY, 0.8f));
                DrawTexture(texStyleTable, -styleTablePositionX, GetScreenHeight()/2 - texStyleTable.height/2, WHITE);

                styleTablePositionX = GuiSlider((Rectangle){ 0, GetScreenHeight()/2 + texStyleTable.height/2, GetScreenWidth(), 15 }, NULL, NULL, styleTablePositionX, 0, texStyleTable.width - GetScreenWidth());
            }

            // GUI: About Window
            //----------------------------------------------------------------------------------------
            GuiWindowAbout(&windowAboutState);
            //----------------------------------------------------------------------------------------

            // GUI: Exit Window
            //----------------------------------------------------------------------------------------
            if (windowExitActive)
            {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(WHITE, 0.7f));
                windowExitActive = !GuiWindowBox((Rectangle){ GetScreenWidth()/2 - 125, GetScreenHeight()/2 - 50, 250, 100 }, "Closing rGuiStyler");

                GuiLabel((Rectangle){ GetScreenWidth()/2 - 95, GetScreenHeight()/2 - 60, 200, 100 }, "Do you want to save before quitting?");

                if (GuiButton((Rectangle){ GetScreenWidth()/2 - 94, GetScreenHeight()/2 + 10, 85, 25 }, "Yes")) showExportFileDialog = true;
                else if (GuiButton((Rectangle){ GetScreenWidth()/2 + 10, GetScreenHeight()/2 + 10, 85, 25 }, "No")) { exitWindow = true; }
            }
            //----------------------------------------------------------------------------------------
            
            // GUI: Load File Dialog (and loading logic)
            //----------------------------------------------------------------------------------------
            if (showLoadFileDialog)
            {
#if defined(CUSTOM_MODAL_DIALOGS)
                int result = GuiFileDialog(DIALOG_MESSAGE, "Load raygui style file ...", inFileName, "Ok", "Just drag and drop your .rgs style file!");
#else
                int result = GuiFileDialog(DIALOG_OPEN, "Load raygui style file", inFileName, "*.rgs", "raygui Style Files (*.rgs)");
#endif
                if (result == 1)
                {
                    // Load style
                    GuiLoadStyle(inFileName);
                    
                    SetWindowTitle(FormatText("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
                    saveChangesRequired = false;

                    // Load .rgs custom font in font
                    font = GuiGetFont();
                    memset(fontFilePath, 0, 512);
                    fontFileProvided = false;
                    customFont = true;
                }
                
                if (result >= 0) showLoadFileDialog = false;
            }
            //----------------------------------------------------------------------------------------
            
            // GUI: Load Font File Dialog (and loading logic)
            //----------------------------------------------------------------------------------------
            if (showLoadFontFileDialog)
            {
#if defined(CUSTOM_MODAL_DIALOGS)
                int result = GuiFileDialog(DIALOG_MESSAGE, "Load font file ...", inFileName, "Ok", "Just drag and drop your .ttf/.otf font file!");
#else
                int result = GuiFileDialog(DIALOG_OPEN, "Load font file", inFileName, "*.ttf;*.otf", "Font Files (*.ttf, *.otf)");
#endif
                if (result == 1)
                {
                    // Load font file
                    Font tempFont = LoadFontEx(inFileName, genFontSizeValue, NULL, 0);
                    
                    if (tempFont.texture.id > 0)
                    {
                        UnloadFont(font);
                        font = tempFont;
                        
                        GuiSetFont(font);
                        strcpy(fontFilePath, inFileName);
                        fontFileProvided = true;
                        customFont = true;
                    }
                }
                
                if (result >= 0) showLoadFontFileDialog = false;
            }
            //----------------------------------------------------------------------------------------
            
            // GUI: Save File Dialog (and saving logic)
            //----------------------------------------------------------------------------------------
            if (showSaveFileDialog)
            {
                strcpy(outFileName, TextFormat("%s.rgs", styleNameText));
#if defined(CUSTOM_MODAL_DIALOGS)
                int result = GuiFileDialog(DIALOG_TEXTINPUT, "Save raygui style file...", outFileName, "Ok;Cancel", NULL);
#else
                int result = GuiFileDialog(DIALOG_SAVE, "Save raygui style file...", outFileName, "*.rgs", "raygui Style Files (*.rgs)");
#endif
                if (result == 1)
                {
                    // Save file: outFileName
                    // Check for valid extension and make sure it is
                    if ((GetExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".rgs")) strcat(outFileName, ".rgs\0");

                    // Save style file (text or binary)
                    SaveStyle(outFileName, STYLE_TEXT);

                #if defined(PLATFORM_WEB)
                    // Download file from MEMFS (emscripten memory filesystem)
                    // NOTE: Second argument must be a simple filename (we can't use directories)
                    emscripten_run_script(TextFormat("SaveFileFromMEMFSToDisk('%s','%s')", outFileName, GetFileName(outFileName)));
                #endif
                }
                
                if (result >= 0) showSaveFileDialog = false;
            }
            //----------------------------------------------------------------------------------------
            
            // GUI: Export File Dialog (and saving logic)
            //----------------------------------------------------------------------------------------
            if (showExportFileDialog)
            {
                // Consider different supported file types
                char filters[64] = { 0 };
                strcpy(outFileName, styleNameText);
                
                switch (exportFormatActive)
                {
                    case STYLE_TEXT: strcpy(filters, "*.rgs"); strcat(outFileName, ".rgs"); break;
                    case STYLE_BINARY: strcpy(filters, "*.rgsb"); strcat(outFileName, ".rgsb"); break;
                    case STYLE_AS_CODE: strcpy(filters, "*.h"); strcat(outFileName, ".h"); break;
                    case STYLE_TABLE_IMAGE: strcpy(filters, "*.png"); strcat(outFileName, ".png");break;
                    default: break;
                }

#if defined(CUSTOM_MODAL_DIALOGS)
                int result = GuiFileDialog(DIALOG_TEXTINPUT, "Export raygui style file...", outFileName, "Ok;Cancel", NULL);
#else
                int result = GuiFileDialog(DIALOG_SAVE, "Export raygui style file...", outFileName, filters, TextFormat("File type (%s)", filters));
#endif
                if (result == 1)
                {
                    // Export file: outFileName
                    switch (exportFormatActive)
                    {
                        case STYLE_TEXT: SaveStyle(outFileName, STYLE_TEXT); break;
                        case STYLE_BINARY: SaveStyle(outFileName, STYLE_BINARY); break;
                        case STYLE_AS_CODE: ExportStyleAsCode(outFileName, styleNameText); break;
                        case STYLE_TABLE_IMAGE:
                        {
                            Image imStyleTable = GenImageStyleControlsTable(styleNameText);
                            ExportImage(imStyleTable, outFileName);
                            UnloadImage(imStyleTable);
                        } break;
                        default: break;
                    }
     
                #if defined(PLATFORM_WEB)
                    // Download file from MEMFS (emscripten memory filesystem)
                    // NOTE: Second argument must be a simple filename (we can't use directories)
                    emscripten_run_script(TextFormat("SaveFileFromMEMFSToDisk('%s','%s')", outFileName, GetFileName(outFileName)));
                #endif
                }
                
                if (result >= 0) showExportFileDialog = false;
            }
            //----------------------------------------------------------------------------------------
            
            EndTextureMode();
            
            // Draw render texture to screen (scaled if required)
            DrawTexturePro(screenTarget.texture, (Rectangle){ 0, 0, screenTarget.texture.width, -screenTarget.texture.height }, (Rectangle){ 0, 0, screenTarget.texture.width*screenScale, screenTarget.texture.height*screenScale }, (Vector2){ 0, 0 }, 0.0f, WHITE);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadFont(font);           // Unload font data

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
    printf("// %s v%s ONE - %s             //\n", toolName, toolVersion, toolDescription);
    printf("// powered by raylib v2.4 (www.raylib.com) and raygui v2.0                      //\n");
    printf("// more info and bugs-report: github.com/raysan5/rguistyler                     //\n");
    printf("//                                                                              //\n");
    printf("// Copyright (c) 2017-2019 raylib technologies (@raylibtech)                    //\n");
    printf("//                                                                              //\n");
    printf("//////////////////////////////////////////////////////////////////////////////////\n\n");

    printf("USAGE:\n\n");
    printf("    > rguistyler [--help] --input <filename.ext> [--output <filename.ext>]\n");
    printf("                 [--format <styleformat>] [--edit-prop <property> <value>]\n");

    printf("\nOPTIONS:\n\n");
    printf("    -h, --help                      : Show tool version and command line usage help\n");
    printf("    -i, --input <filename.ext>      : Define input file.\n");
    printf("                                      Supported extensions: .rgs (text or binary)\n");
    printf("    -o, --output <filename.ext>     : Define output file.\n");
    printf("                                      Supported extensions: .rgs, .png, .h\n");
    printf("                                      NOTE: Extension could be modified depending on format\n\n");
    printf("    -f, --format <type_value>       : Define output file format to export style data.\n");
    printf("                                      Supported values:\n");
    printf("                                          0 - Style text format (.rgs)  \n");
    printf("                                          1 - Style binary format (.rgs)\n");
    printf("                                          2 - Style as code (.h)\n");
    printf("                                          3 - Controls table image (.png)\n\n");
    //printf("    -e, --edit-prop <controlId>,<propertyId>,<propertyValue>\n");
    //printf("                                    : Edit specific property from input to output.\n");

    printf("\nEXAMPLES:\n\n");
    printf("    > rguistyler --input tools.rgs --output tools.png\n");
}

// Process command line input
static void ProcessCommandLine(int argc, char *argv[])
{
    // CLI required variables
    bool showUsageInfo = false;         // Toggle command line usage info

    char inFileName[256] = { 0 };       // Input file name
    char outFileName[256] = { 0 };      // Output file name
    int outputFormat = STYLE_BINARY;    // Formats: STYLE_TEXT, STYLE_BINARY, STYLE_AS_CODE, STYLE_TABLE_IMAGE

    // Arguments scan and processing
    for (int i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
        {
            showUsageInfo = true;
        }
        else if ((strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--input") == 0))
        {
            // Check for valid argument and valid file extension
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                if (IsFileExtension(argv[i + 1], ".rgs"))
                {
                    strcpy(inFileName, argv[i + 1]);    // Read input filename
                }
                else printf("WARNING: Input file extension not recognized\n");

                i++;
            }
            else printf("WARNING: No input file provided\n");
        }
        else if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--output") == 0))
        {
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                if (IsFileExtension(argv[i + 1], ".rgs") ||
                    IsFileExtension(argv[i + 1], ".h") ||
                    IsFileExtension(argv[i + 1], ".png"))
                {
                    strcpy(outFileName, argv[i + 1]);   // Read output filename
                }
                else printf("WARNING: Output file extension not recognized\n");

                i++;
            }
            else printf("WARNING: No output file provided\n");
        }
        else if ((strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--format") == 0))
        {
            // Check for valid argumment and valid parameters
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                int format = atoi(argv[i + 1]);

                if ((format >= 0) && (format <= 3)) outputFormat = format;
            }
            else printf("WARNING: Format parameters provided not valid\n");
        }
        
        // TODO: Support font parameters and individual property edition
    }

    if (inFileName[0] != '\0')
    {
        // Set a default name for output in case not provided
        if (outFileName[0] == '\0') strcpy(outFileName, "output.ext");

        printf("\nInput file:       %s", inFileName);
        printf("\nOutput file:      %s", outFileName);

        // Process input .rgs file
        GuiLoadStyle(inFileName);

        // Export style files with different formats
        switch (outputFormat)
        {
            case STYLE_TEXT: SaveStyle(FormatText("%s%s", outFileName, ".rgsb"), outputFormat); break;
            case STYLE_BINARY: SaveStyle(FormatText("%s%s", outFileName, ".rgs"), outputFormat); break;
            case STYLE_AS_CODE: ExportStyleAsCode(FormatText("%s%s", outFileName, ".h"), GetFileNameWithoutExt(outFileName)); break;
            case STYLE_TABLE_IMAGE:
            {
                Image imStyleTable = GenImageStyleControlsTable(GetFileNameWithoutExt(outFileName));
                ExportImage(imStyleTable, FormatText("%s%s", outFileName, ".png"));
                UnloadImage(imStyleTable);
            } break;
            default: break;
        }
    }

    if (showUsageInfo) ShowCommandLineInfo();
}
#endif      // VERSION_ONE

//--------------------------------------------------------------------------------------------
// Load/Save/Export data functions
//--------------------------------------------------------------------------------------------

// Save raygui style file (.rgs)
static bool SaveStyle(const char *fileName, int format)
{
    int success = false;

    FILE *rgsFile = NULL;

    if (format == STYLE_TEXT)
    {
        rgsFile = fopen(fileName, "wt");

        if (rgsFile != NULL)
        {
            #define RGS_FILE_VERSION_TEXT  "3.1"
            
            // Write some description comments
            fprintf(rgsFile, "#\n# rgs style text file (v%s) - raygui style file generated using rGuiStyler\n#\n", RGS_FILE_VERSION_TEXT);
            fprintf(rgsFile, "# Info:  p <controlId> <propertyId> <propertyValue>  // Property description\n#\n");
            
            if (customFont)
            {
                fprintf(rgsFile, "# WARNING: This style uses a custom font, must be provided with style file\n");
                fprintf(rgsFile, "f %i %i %s\n", GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING), GetFileName(fontFilePath));
            }

            // Save DEFAULT properties that changed
            for (int j = 0; j < (NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED); j++)
            {
                if (styleBackup[j] != GuiGetStyle(0, j))
                {
                    // NOTE: Control properties are written as hexadecimal values, extended properties names not provided
                    fprintf(rgsFile, "p 00 %02i 0x%08x    DEFAULT_%s \n", j, GuiGetStyle(0, j), (j < NUM_PROPS_DEFAULT)? guiPropsText[j] : FormatText("EXT%02i", (j - NUM_PROPS_DEFAULT)));
                }
            }

            // Save other controls properties that changed
            for (int i = 1; i < NUM_CONTROLS; i++)
            {
                for (int j = 0; j < (NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED); j++)
                {
                    // Check all properties that have changed in comparison to default style and add custom style sets for those properties
                    if ((styleBackup[i*(NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED) + j] != GuiGetStyle(i, j)) && (GuiGetStyle(i, j) !=  GuiGetStyle(0, j)))
                    {
                        // NOTE: Control properties are written as hexadecimal values, extended properties names not provided
                        fprintf(rgsFile, "p %02i %02i 0x%08x    %s_%s \n", i, j, GuiGetStyle(i, j), guiControlText[i], (j < NUM_PROPS_DEFAULT)? guiPropsText[j] : FormatText("EXT%02i", (j - NUM_PROPS_DEFAULT)));
                    }
                }
            }

            fclose(rgsFile);
            success = true;
        }
    }
    else if (format == STYLE_BINARY)
    {
        rgsFile = fopen(fileName, "wb");

        if (rgsFile != NULL)
        {
            // Style File Structure (.rgs)
            // ------------------------------------------------------
            // Offset  | Size    | Type       | Description
            // ------------------------------------------------------
            // 0       | 4       | char       | Signature: "rGS "
            // 4       | 2       | short      | Version: 200
            // 6       | 2       | short      | reserved
            // 8       | 4       | int        | Num properties (N)

            // Properties Data: (controlId (2 byte) +  propertyId (2 byte) + propertyValue (4 bytes))*N
            // foreach (property)
            // {
            //   8+8*i  | 2       | short      | ControlId
            //   8+8*i  | 2       | short      | PropertyId
            //   8+8*i  | 4       | int        | PropertyValue
            // }

            // Custom Font Data : Parameters (32 bytes)
            // 16+4*N  | 4       | int        | Font data size (0 - no font)
            // 20+4*N  | 4       | int        | Font base size
            // 24+4*N  | 4       | int        | Font chars count [charCount]
            // 28+4*N  | 4       | int        | Font type (0-NORMAL, 1-SDF)
            // 32+4*N  | 16      | Rectangle  | Font white rectangle

            // Custom Font Data : Image (16 bytes + imSize)
            // 48+4*N  | 4       | int        | Image size [imSize = IS]
            // 52+4*N  | 4       | int        | Image width
            // 56+4*N  | 4       | int        | Image height
            // 60+4*N  | 4       | int        | Image format
            // 64+4*N  | imSize  | *          | Image data
            
            // Custom Font Data : Recs (32 bytes * charCount)
            // foreach (charCount)
            // {
            //   ...   | 16      | Rectangle  | Char rectangle (in image)
            // }

            // Custom Font Data : Chars Info (32 bytes * charCount)
            // foreach (charCount)
            // {
            //   ...   | 4       | int        | Char value
            //   ...   | 4       | int        | Char offset X
            //   ...   | 4       | int        | Char offset Y
            //   ...   | 4       | int        | Char advance X
            // }
            // ------------------------------------------------------

            char signature[5] = "rGS ";
            short version = 200;
            short reserved = 0;

            fwrite(signature, 1, 4, rgsFile);
            fwrite(&version, 1, sizeof(short), rgsFile);
            fwrite(&reserved, 1, sizeof(short), rgsFile);
            
            int changedPropsCounter = StyleChangesCounter();
            
            fwrite(&changedPropsCounter, 1, sizeof(int), rgsFile);
            
            short controlId = 0;
            short propertyId = 0;
            int propertyValue = 0;
            
            // Save first all properties that have changed in default style
            for (int i = 0; i < (NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED); i++) 
            {
                if (styleBackup[i] != GuiGetStyle(0, i))
                {
                    propertyId = (short)i;
                    propertyValue = GuiGetStyle(0, i);
                    
                    fwrite(&controlId, 1, sizeof(short), rgsFile);
                    fwrite(&propertyId, 1, sizeof(short), rgsFile);
                    fwrite(&propertyValue, 1, sizeof(int), rgsFile);
                }
            }

            // Save all properties that have changed in comparison to default style
            for (int i = 1; i < NUM_CONTROLS; i++)
            {
                for (int j = 0; j < NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED; j++)
                {
                    if ((styleBackup[i*(NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED) + j] != GuiGetStyle(i, j)) && (GuiGetStyle(i, j) !=  GuiGetStyle(0, j)))
                    {
                        controlId = (short)i;
                        propertyId = (short)j;
                        propertyValue = GuiGetStyle(i, j);
                        
                        fwrite(&controlId, 1, sizeof(short), rgsFile);
                        fwrite(&propertyId, 1, sizeof(short), rgsFile);
                        fwrite(&propertyValue, 1, sizeof(int), rgsFile);
                    }
                }
            }
            
            int fontSize = 0;

            // Write font data (embedding)
            if (customFont)
            {
                Image imFont = GetTextureData(font.texture);
                //ImageFormat(&imFont, UNCOMPRESSED_R8G8B8A8);    // TODO: WARNING: It could be required on OpenGL ES 2.0

                // Write font parameters
                int fontParamsSize = 32;
                int fontImageSize = GetPixelDataSize(imFont.width, imFont.height, imFont.format);
                int fontCharsDataSize = font.charsCount*32;       // 32 bytes by char
                int fontDataSize = fontParamsSize + fontImageSize + fontCharsDataSize;
                int fontType = 0;       // 0-NORMAL, 1-SDF

                fwrite(&fontDataSize, 1, sizeof(int), rgsFile);
                fwrite(&font.baseSize, 1, sizeof(int), rgsFile);
                fwrite(&font.charsCount, 1, sizeof(int), rgsFile);
                fwrite(&fontType, 1, sizeof(int), rgsFile);

                // TODO: Define font white rectangle?
                Rectangle rec = { 0 };
                fwrite(&rec, 1, sizeof(Rectangle), rgsFile);

                // Write font image parameters
                fwrite(&fontImageSize, 1, sizeof(int), rgsFile);
                fwrite(&imFont.width, 1, sizeof(int), rgsFile);
                fwrite(&imFont.height, 1, sizeof(int), rgsFile);
                fwrite(&imFont.format, 1, sizeof(int), rgsFile);
                fwrite(imFont.data, 1, fontImageSize, rgsFile);

                UnloadImage(imFont);
                
                // Write font recs data
                for (int i = 0; i < font.charsCount; i++) fwrite(&font.recs[i], 1, sizeof(Rectangle), rgsFile);

                // Write font chars info data
                for (int i = 0; i < font.charsCount; i++)
                {
                    fwrite(&font.chars[i].value, 1, sizeof(int), rgsFile);
                    fwrite(&font.chars[i].offsetX, 1, sizeof(int), rgsFile);
                    fwrite(&font.chars[i].offsetY, 1, sizeof(int), rgsFile);
                    fwrite(&font.chars[i].advanceX, 1, sizeof(int), rgsFile);
                }
            }
            else fwrite(&fontSize, 1, sizeof(int), rgsFile);
            
            fclose(rgsFile);
            success = true;
        }
    }
    
    return success;
}

// Export gui style as (ready-to-use) code file
// NOTE: Code file already implements a function to load style
static void ExportStyleAsCode(const char *fileName, const char *styleName)
{
    FILE *txtFile = fopen(fileName, "wt");

    if (txtFile != NULL)
    {
        fprintf(txtFile, "//////////////////////////////////////////////////////////////////////////////////\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// StyleAsCode exporter v1.1 - Style data exported as a values array            //\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// USAGE: On init call: GuiLoadStyle%s();                             //\n", TextToPascal(styleName));
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// more info and bugs-report:  github.com/raysan5/raygui                        //\n");
        fprintf(txtFile, "// feedback and support:       ray[at]raylibtech.com                            //\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// Copyright (c) 2019 raylib technologies (@raylibtech)                         //\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "//////////////////////////////////////////////////////////////////////////////////\n\n");

        // TODO: Export only properties that change from default style
        // Probably using an array of integers won't be enough, it would require and structure more aligned with
        // .rgs text data information: struct Property { int controlId; int propertyId, int propertyValue; }
        // Remove GuiLoadStyleProps(), it's a very bad design!
        
        // Write byte data as hexadecimal text
        fprintf(txtFile, "// Custom style palette: %s\n", styleName);
        fprintf(txtFile, "static const int style%s[%i] = {\n", TextToPascal(styleName), NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED);
        for (int i = 0; i < (NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED); i++)
        {
            if (i < NUM_PROPS_DEFAULT) fprintf(txtFile, "    0x%08x,    // DEFAULT_%s \n", GuiGetStyle(DEFAULT, i), guiPropsText[i]);
            else fprintf(txtFile, "    0x%08x,    // DEFAULT_%s \n", GuiGetStyle(DEFAULT, i), guiPropsExText[i - NUM_PROPS_DEFAULT]);
        }
        fprintf(txtFile, "};\n\n");
        
        if (customFont)
        {
            fprintf(txtFile, "// WARNING: This style uses a custom font: %s (size: %i, spacing: %i)\n\n", 
                    GetFileName(fontFilePath), GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING));
        }

        Image imFont = { 0 };
        
        if (customFont)
        {
            // Support font export and initialization
            // NOTE: This mechanism is highly coupled to raylib
            imFont = GetTextureData(font.texture);
            int imFontSize = GetPixelDataSize(imFont.width, imFont.height, imFont.format);

            #define BYTES_TEXT_PER_LINE     20
            
            // TODO: Compress font image data
            // Image data is usually Grayscale + Alpha that can be reduced to Grayscale and 
            // greatly compressed just using a simple RLE algorythm 
            
            // Save font image data
            fprintf(txtFile, "// Font image pixels data\n");
            fprintf(txtFile, "// NOTE: 2 bytes per pixel, GRAY + ALPHA channels\n");
            fprintf(txtFile, "static unsigned char %sFontImageData[%i] = { ", styleName, imFontSize);
            for (int i = 0; i < imFontSize - 1; i++) fprintf(txtFile, ((i%BYTES_TEXT_PER_LINE == 0)? "0x%02x,\n    " : "0x%02x, "), ((unsigned char *)imFont.data)[i]);
            fprintf(txtFile, "0x%02x };\n\n", ((unsigned char *)imFont.data)[imFontSize - 1]);

            // Save font recs data
            fprintf(txtFile, "// Font characters rectangles data\n");
            fprintf(txtFile, "static const Rectangle %sFontRecs[%i] = {\n", styleName, font.charsCount);
            for (int i = 0; i < font.charsCount; i++) 
            {
                fprintf(txtFile, "    { %1.0f, %1.0f, %1.0f , %1.0f },\n", font.recs[i].x, font.recs[i].y, font.recs[i].width, font.recs[i].height);
            }
            fprintf(txtFile, "};\n\n");

            // Save font chars data
            // NOTE: Characters Image data not saved (grayscale pixels), 
            // it could be generated from image and recs
            fprintf(txtFile, "// Font characters info data\n");
            fprintf(txtFile, "// NOTE: No chars.image data provided\n");
            fprintf(txtFile, "static const CharInfo %sFontChars[%i] = {\n", styleName, font.charsCount);
            for (int i = 0; i < font.charsCount; i++) 
            {
                fprintf(txtFile, "    { %i, %i, %i, %i, { 0 }},\n", font.chars[i].value, font.chars[i].offsetX, font.chars[i].offsetY, font.chars[i].advanceX);
            }
            fprintf(txtFile, "};\n\n");
            
            UnloadImage(imFont);
        }

        fprintf(txtFile, "// Style loading function: %s\n", styleName);
        fprintf(txtFile, "static void GuiLoadStyle%s(void)\n{\n", TextToPascal(styleName));
        fprintf(txtFile, "    // Load an populate global default style\n");
        fprintf(txtFile, "    GuiLoadStyleProps(style%s, 20);\n", TextToPascal(styleName));
        fprintf(txtFile, "    GuiUpdateStyleComplete();\n\n");
        fprintf(txtFile, "    // Set additional style properties (if required)\n");
        
        // Check all properties that have changed in comparison to default style
        // and add custom style sets for those properties
        for (int i = 1; i < NUM_CONTROLS; i++)
        {
            for (int j = 0; j < (NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED); j++)
            {
                if ((styleBackup[i*(NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED) + j] != GuiGetStyle(i, j)) && (GuiGetStyle(i, j) !=  GuiGetStyle(0, j)))
                {
                    if (j < NUM_PROPS_DEFAULT) fprintf(txtFile, "    GuiSetStyle(%s, %s, 0x%08x);\n", guiControlText[i], guiPropsText[j], GuiGetStyle(i, j));
                    else fprintf(txtFile, "    GuiSetStyle(%s, %i, 0x%08x); // %s_%s\n", guiControlText[i], j, GuiGetStyle(i, j), guiControlText[i], FormatText("EXT%02i", (j - NUM_PROPS_EXTENDED + 1)));
                }
            }
        }

        if (customFont)
        {
            fprintf(txtFile, "\n    // Custom font loading\n");
            fprintf(txtFile, "    Image imFont = { %sFontImageData, %i, %i, 1, %i };\n\n", styleName, imFont.width, imFont.height, imFont.format);
            fprintf(txtFile, "    Font font = { 0 };\n");
            fprintf(txtFile, "    font.baseSize = %i;\n", GuiGetStyle(DEFAULT, TEXT_SIZE));
            fprintf(txtFile, "    font.charsCount = %i;\n\n", font.charsCount);
    
            fprintf(txtFile, "    // Load texture from image\n");
            fprintf(txtFile, "    font.texture = LoadTextureFromImage(imFont);\n\n");
    
            fprintf(txtFile, "    // Copy char recs data from global fontRecs\n");
            fprintf(txtFile, "    // NOTE: Required to avoid issues if trying to free font\n");
            fprintf(txtFile, "    font.recs = (Rectangle *)malloc(font.charsCount*sizeof(Rectangle));\n");
            fprintf(txtFile, "    memcpy(font.recs, %sFontRecs, font.charsCount*sizeof(Rectangle));\n\n", styleName);
            
            fprintf(txtFile, "    // Copy font char info data from global fontChars\n");
            fprintf(txtFile, "    // NOTE: Required to avoid issues if trying to free font\n");
            fprintf(txtFile, "    font.chars = (CharInfo *)malloc(font.charsCount*sizeof(CharInfo));\n");
            fprintf(txtFile, "    memcpy(font.chars, %sFontChars, font.charsCount*sizeof(CharInfo));\n\n", styleName);
            
            fprintf(txtFile, "    GuiSetFont(font);\n\n");
            
            fprintf(txtFile, "    // TODO: Setup a white rectangle on the font to be used on shapes drawing,\n");
            fprintf(txtFile, "    // this way we make sure all gui can be drawn on a single pass because no texture cahnge is required\n");
            fprintf(txtFile, "    // NOTE: Setting up this rectangle is a manual process (for the moment)\n");
            fprintf(txtFile, "    //Rectangle whiteChar = { 0, 0, 0, 0 };\n");
            fprintf(txtFile, "    //SetShapesTexture(font.texture, whiteChar);\n\n");
        }
        
        fprintf(txtFile, "    //-----------------------------------------------------------------\n\n");
        fprintf(txtFile, "    // TODO: Custom user style setup: Set specific properties here (if required)\n");
        fprintf(txtFile, "    // i.e. Controls specific BORDER_WIDTH, TEXT_PADDING, TEXT_ALIGNMENT\n");

        fprintf(txtFile, "}\n");
        
        fclose(txtFile);
    }
}

// Draw controls table image
static Image GenImageStyleControlsTable(const char *styleName)
{
    #define TABLE_LEFT_PADDING      15
    #define TABLE_TOP_PADDING       20

    #define TABLE_CELL_HEIGHT       40
    #define TABLE_CELL_PADDING       5          // Control padding inside cell

    #define TABLE_CONTROLS_COUNT    12

    typedef enum {
        TYPE_LABEL = 0,
        TYPE_BUTTON,
        TYPE_TOGGLE,
        TYPE_CHECKBOX,
        TYPE_SLIDER,
        TYPE_SLIDERBAR,
        TYPE_PROGRESSBAR,
        TYPE_COMBOBOX,
        TYPE_DROPDOWNBOX,
        TYPE_TEXTBOX,
        TYPE_VALUEBOX,
        TYPE_SPINNER
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

    // TODO: Calculate controls grid widths depending on font size and spacing
    int controlWidth[TABLE_CONTROLS_COUNT] = {
        100,    // LABEL
        100,    // BUTTON
        100,    // TOGGLE
        200,    // CHECKBOX
        100,    // SLIDER
        100,    // SLIDERBAR
        100,    // PROGRESSBAR
        140,    // COMBOBOX,
        160,    // DROPDOWNBOX
        100,    // TEXTBOX
        100,    // VALUEBOX
        100,    // SPINNER
    };

    int tableStateNameWidth = 100;   // First column with state name width

    int tableWidth = 0;
    int tableHeight = 256;

    tableWidth = TABLE_LEFT_PADDING*2 + tableStateNameWidth;
    for (int i = 0; i < TABLE_CONTROLS_COUNT; i++) tableWidth += ((controlWidth[i] + TABLE_CELL_PADDING*2) - 1);

    // Controls required variables
    int dropdownActive = 0;
    int value = 40;

    Rectangle rec = { 0 };      // Current drawing rectangle space

    RenderTexture2D target = LoadRenderTexture(tableWidth, tableHeight);

    int sliderWidth = GuiGetStyle(SLIDER, SLIDER_WIDTH);
    GuiSetStyle(SLIDER, SLIDER_WIDTH, 10);

    // Texture rendering
    //--------------------------------------------------------------------------------------------
    BeginTextureMode(target);

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        // Draw style title
        DrawText("raygui style:  ", TABLE_LEFT_PADDING, 20, 10, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_DISABLED)));
        DrawText(FormatText("%s", styleName), TABLE_LEFT_PADDING + MeasureText("raygui style:  ", 10), 20, 10, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));

        // Draw left column
        //----------------------------------------------------------------------------------------
        rec = (Rectangle){ TABLE_LEFT_PADDING, TABLE_TOP_PADDING + TABLE_CELL_HEIGHT/2 + 20, tableStateNameWidth, TABLE_CELL_HEIGHT };

        for (int i = 0; i < 4; i++)
        {
            GuiGroupBox(rec, NULL);
            
            // Draw style rectangle
            GuiSetState(i); GuiLabelButton((Rectangle){ rec.x + 28, rec.y, rec.width, rec.height }, tableStateName[i]);
            rec.y += TABLE_CELL_HEIGHT - 1;             // NOTE: We add/remove 1px to draw lines overlapped!
        }
        //----------------------------------------------------------------------------------------

        GuiSetState(GUI_STATE_NORMAL);

        int offsetWidth = TABLE_LEFT_PADDING + tableStateNameWidth;

        // Draw basic controls
        for (int i = 0; i < TABLE_CONTROLS_COUNT; i++)
        {
            rec = (Rectangle){ offsetWidth - i - 1, TABLE_TOP_PADDING + 20, (controlWidth[i] + TABLE_CELL_PADDING*2), TABLE_CELL_HEIGHT/2 + 1 };

            // Draw grid lines: control name
            GuiGroupBox(rec, NULL);
            int labelTextAlignment = GuiGetStyle(LABEL, TEXT_ALIGNMENT);
            GuiSetStyle(LABEL, TEXT_ALIGNMENT, GUI_TEXT_ALIGN_CENTER);
            GuiLabel(rec, tableControlsName[i]);

            rec.y += TABLE_CELL_HEIGHT/2;
            rec.height = TABLE_CELL_HEIGHT;

            // Draw control 4 states: DISABLED, NORMAL, FOCUSED, PRESSED
            for (int j = 0; j < 4; j++)
            {
                // Draw grid lines: control state
                GuiGroupBox(rec, NULL);

                GuiSetState(j);
                    // Draw control centered correctly in grid
                    switch (i)
                    {
                        case TYPE_LABEL: GuiLabelButton((Rectangle){ rec.x, rec.y, controlWidth[i], 40 }, "Label"); break;
                        case TYPE_BUTTON: GuiButton((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i], 24 }, "Button"); break;
                        case TYPE_TOGGLE: GuiToggle((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i], 24 }, "Toggle", false); break;
                        case TYPE_CHECKBOX:
                        {
                            GuiCheckBox((Rectangle){ rec.x + 10, rec.y + rec.height/2 - 15/2, 15, 15 }, "NoCheck", false);
                            DrawRectangle(rec.x + rec.width/2, rec.y, 1, TABLE_CELL_HEIGHT, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
                            GuiCheckBox((Rectangle){ rec.x + rec.width/2 + 10, rec.y + rec.height/2 - 15/2, 15, 15 }, "Checked", true);
                        } break;
                        case TYPE_SLIDER: GuiSlider((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 10/2, controlWidth[i], 10 }, NULL, NULL, 40, 0, 100); break;
                        case TYPE_SLIDERBAR: GuiSliderBar((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 10/2, controlWidth[i], 10 }, NULL, NULL, 40, 0, 100); break;
                        case TYPE_PROGRESSBAR: GuiProgressBar((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 10/2, controlWidth[i], 10 }, NULL, NULL, 60, 0, 100); break;
                        case TYPE_COMBOBOX: GuiComboBox((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i], 24 }, "ComboBox;ComboBox", 0); break;
                        case TYPE_DROPDOWNBOX: GuiDropdownBox((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i], 24 }, "DropdownBox;DropdownBox", &dropdownActive, false); break;
                        case TYPE_TEXTBOX: GuiTextBox((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i], 24 }, "text box", 32, false); break;
                        case TYPE_VALUEBOX: GuiValueBox((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i], 24 }, NULL, &value, 0, 100, false); break;
                        case TYPE_SPINNER: GuiSpinner((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i], 24 }, NULL, &value, 0, 100, false); break;
                        default: break;
                    }
                GuiSetState(GUI_STATE_NORMAL);

                rec.y += TABLE_CELL_HEIGHT - 1;
            }

            GuiSetStyle(LABEL, TEXT_ALIGNMENT, labelTextAlignment);

            offsetWidth += (controlWidth[i] + TABLE_CELL_PADDING*2);
        }

        // Draw copyright and software info (bottom-right)
        DrawText("raygui style table automatically generated with rGuiStyler", TABLE_LEFT_PADDING, tableHeight - 30, 10, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_DISABLED)));
        DrawText("rGuiStyler created by raylib technologies (@raylibtech)", tableWidth - MeasureText("rGuiStyler created by raylib technologies (@raylibtech)", 10) - 20, tableHeight - 30, 10, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_DISABLED)));

    EndTextureMode();
    //--------------------------------------------------------------------------------------------

    GuiSetStyle(SLIDER, SLIDER_WIDTH, sliderWidth);

    Image imStyleTable = GetTextureData(target.texture);
    ImageFlipVertical(&imStyleTable);

    UnloadRenderTexture(target);

    return imStyleTable;
}

//--------------------------------------------------------------------------------------------
// Auxiliar GUI functions
//--------------------------------------------------------------------------------------------

// Count changed properties in current style (in reference to default light style)
static int StyleChangesCounter(void)
{
    int changes = 0;
    
    // Count all properties that have changed in default style
    for (int i = 0; i < (NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED); i++) if (styleBackup[i] != GuiGetStyle(0, i)) changes++;
    
    // Add to count all properties that have changed in comparison to default style
    for (int i = 1; i < NUM_CONTROLS; i++)
    {
        for (int j = 0; j < (NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED); j++)
        {
            if ((styleBackup[i*(NUM_PROPS_DEFAULT + NUM_PROPS_EXTENDED) + j] != GuiGetStyle(i, j)) && (GuiGetStyle(i, j) !=  GuiGetStyle(0, j))) changes++;
        }
    }
    
    return changes;
}

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

// Multiplatform file dialogs
static int GuiFileDialog(int dialogType, const char *title, char *fileName, const char *filters, const char *message)
{
    int result = -1;

#if defined(CUSTOM_MODAL_DIALOGS)
    static char tempFileName[256] = { 0 };
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)), 0.85f));

    switch (dialogType)
    {
        case DIALOG_OPEN: /* TODO: Load file modal dialog */ break;
        case DIALOG_SAVE: /* TODO: Load file modal dialog */ break;
        case DIALOG_MESSAGE: result = GuiMessageBox((Rectangle){ GetScreenWidth()/2 - 120, GetScreenHeight()/2 - 60, 240, 120 }, GuiIconText(RICON_FILE_OPEN, title), message, filters); break;
        case DIALOG_TEXTINPUT: result = GuiTextInputBox((Rectangle){ GetScreenWidth()/2 - 120, GetScreenHeight()/2 - 60, 240, 120 }, GuiIconText(RICON_FILE_SAVE, title), message, filters, tempFileName); break;
        default: break;
    }
    
    if ((result == 1) && (tempFileName[0] != '\0')) strcpy(fileName, tempFileName);
    
#else   // Use native OS dialogs (tinyfiledialogs)

    const char *tempFileName = NULL;
    int filterCount = 0;
    const char **filterSplit = TextSplit(filters, ';', &filterCount);
    
    switch (dialogType)
    {
        case DIALOG_OPEN: tempFileName = tinyfd_openFileDialog(title, fileName, filterCount, filterSplit, message, 0); break;
        case DIALOG_SAVE: tempFileName = tinyfd_saveFileDialog(title, fileName, filterCount, filterSplit, message); break;
        case DIALOG_MESSAGE: result = tinyfd_messageBox(title, message, "ok", "info", 0); break;
        case DIALOG_TEXTINPUT: tempFileName = tinyfd_inputBox(title, message, ""); break;
        default: break;
    }

    if (tempFileName != NULL) 
    {
        strcpy(fileName, tempFileName);
        result = 1;
    }
    else result = 0;
#endif

    return result;
}

#if 0
// PNG file management info
// PNG file-format: https://en.wikipedia.org/wiki/Portable_Network_Graphics#File_format

// PNG Signature
unsigned char pngSign[8] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };

// After signature we have a series of chunks, every chunck has the same structure:
typedef struct {
    unsigned int length;    // Big endian!
    unsigned char type[4];  // Chunck type: IDHR, PLTE, IDAT, IEND  /  gAMA, sRGB, tEXt, tIME...
    unsigned char *data;    // Chunck data pointer
    unsigned int crc;       // 32bit CRC (computed over type and data)
} PNGChunk;

// A minimal PNG only requires: pngSign | PNGChunk(IHDR) | PNGChunk(IDAT) | PNGChunk(IEND)

// IHDR mandatory chunck: image info (13 bytes)
typedef struct {
    unsigned int width;         // Image width
    unsigned int height;        // Image width
    unsigned char bitDepth;     // Bit depth
    unsigned char colorType;    // Pixel format: 0 - Grayscale, 2 - RGB, 3 - Indexed, 4 - GrayAlpha, 6 - RGBA
    unsigned char compression;  // Compression method: 0
    unsigned char filter;       // Filter method: 0 (default)
    unsigned char interlace;    // Interlace scheme (optional): 0 (none)
} IHDRChunkData;

// IDATChunkData > PNGChunk with pixel data
// IENDChunkData > PNGChunk (empty), it should be LAST
// PLTEChunkData > An array of RGB (3 bytes) values (optional for truecolor, before IDAT)

// Embedd gui style inside PNG file as rGSt chunk
void ExportStyleInPNG(const char *fileName, const char *rgsFileName)
{
    unsigned char signature[8] = { 0 };

    FILE *pngFile = fopen(fileName, "rb");

    if (pngFile != NULL)
    {
        fseek(pngFile, 0, SEEK_END);    // Place cursor at the end of file
        long pngSize = ftell(pngFile);  // Get file size
        fseek(pngFile, 0, SEEK_SET);    // Reset file pointer to beginning

        fread(signature, 8, 1, pngFile);

        if (strcmp(signature, pngSign) == 0)    // Check valid PNG file
        {
            unsigned char *pngData = (unsigned char *)malloc(pngSize);
            fread(pngData, pngSize, 1, pngFile); // Read full PNG file into buffer

            // Now we can start composing our new PNG file with rGSt chunk
            char *pngrgsFileName = GetFileNameWithoutExt(fileName);   // Requires manually free
            FILE *pngrgsFile = fopen(TextFormat("%s.rgs.png", pngrgsFileName), "wb");
            free(pngrgsFileName);

            // Write PNG file Signature (8 bytes) + IHDR chunk (4 + 4 + 13 + 4 bytes)
            fwrite(pngData, 8 + 4 + 4 + 13 + 4, 1, pngrgsFile);

            // Write rGSt chunk into file
            //---------------------------------------------------------------------------
            FILE *rgsFile = fopen(rgsFileName, "rb");

            if (rgsFile != NULL)
            {
                // TODO: Verify rGS file is a valid file

                fseek(rgsFile, 0, SEEK_END);    // Place cursor at the end of file
                long rgsSize = ftell(rgsFile);  // Get file size
                fseek(rgsFile, 0, SEEK_SET);    // Reset file pointer to beginning

                unsigned char *rgsData = (unsigned char *)malloc(rgsSize);
                fread(rgsData, rgsSize, 1, rgsFile);

                unsigned char rgsType[4] = { 'r', 'G', 'S', 't' };

                fwrite(&rgsSize, 4, 1, pngrgsFile);
                fwrite(&rgsType, 4, 1, pngrgsFile);
                fwrite(&rgsData, rgsSize, 1, pngrgsFile);

                unsigned int rgsDataCRC = 0;    // TODO: Compute CRC32 for rgsData
                fwrite(&rgsDataCRC, 4, 1, pngrgsFile);
                
                fclose(rgsFile);
            }
            //---------------------------------------------------------------------------

            // Write the rest of the original PNG file
            fwrite(pngData + 33, pngSize - 33, 1, pngrgsFile);

            fclose(pngrgsFile);
            free(pngData);
            free(rgsData);
        }

        fclose(pngFile);
    }
}
#endif
