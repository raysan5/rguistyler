/*******************************************************************************************
*
*   rGuiStyler v4.2 - A simple and easy-to-use raygui styles editor
*
*   CONFIGURATION:
*
*   #define CUSTOM_MODAL_DIALOGS
*       Use custom raygui generated modal dialogs instead of native OS ones
*       NOTE: Avoids including tinyfiledialogs depencency library
*
*   #define SUPPORT_COMPRESSED_FONT_ATLAS
*       Export font atlas image data compressed using raylib CompressData() DEFLATE algorythm,
*       NOTE: It requires to be decompressed with raylib DecompressData(),
*       that requires compiling raylib with SUPPORT_COMPRESSION_API config flag enabled
*
*   VERSIONS HISTORY:
*       4.2  (13-Dec-2022)  ADDED: Welcome window with sponsors info
*                           REDESIGNED: Main toolbar to add tooltips
*                           REVIEWED: Help window implementation
*       4.1  (10-Oct-2022)  ADDED: Sponsor window for tools support
*                           ADDED: Random style generator button (experimental)
*                           Updated to raylib 4.5-dev and raygui 3.5-dev
*       4.0  (02-Oct-2022)  ADDED: Main toolbar, for consistency with other tools
*                           ADDED: Multiple new styles as templates
*                           ADDED: Export style window with new options
*                           REVIEWED: Layout metrics
*                           Updated to raylib 4.2 and raygui 3.2
*                           Source code re-licensed to open-source
*       3.5  (29-Dec-2021)  Updated to raylib 4.0 and raygui 3.1
*
*   DEPENDENCIES:
*       raylib 4.2              - Windowing/input management and drawing
*       raygui 3.5-dev          - Immediate-mode GUI controls with custom styling and icons
*       rpng 1.0                - PNG chunks management
*       tinyfiledialogs 3.9.0   - Open/save file dialogs, it requires linkage with comdlg32 and ole32 libs
*
*   COMPILATION (Windows - MinGW):
*       gcc -o rguistyler.exe rguistyler.c external/tinyfiledialogs.c -s -O2 -std=c99 -DPLATFORM_DESKTOP
*           -lraylib -lopengl32 -lgdi32 -lcomdlg32 -lole32
*
*   COMPILATION (Linux - GCC):
*       gcc -o rguistyler rguistyler.c external/tinyfiledialogs.c -s -no-pie -D_DEFAULT_SOURCE /
*           -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
*
*   NOTE: On PLATFORM_ANDROID and PLATFORM_WEB file dialogs are not available
*
*   DEVELOPERS:
*       Ramon Santamaria (@raysan5):    Supervision, review, redesign, update and maintenance.
*       Adria Arranz (@Adri102):        Developer and designer (v2.0 - May.2018)
*       Jordi Jorba (@KoroBli):         Developer and designer (v2.0 - May.2018)
*       Sergio Martinez (@anidealgift): Developer and tester (v1.0 - 2015..2017)
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2015-2023 raylib technologies (@raylibtech) / Ramon Santamaria (@raysan5)
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

#define TOOL_NAME               "rGuiStyler"
#define TOOL_SHORT_NAME         "rGS"
#define TOOL_VERSION            "4.1"
#define TOOL_DESCRIPTION        "A simple and easy-to-use raygui styles editor"
#define TOOL_RELEASE_DATE       "Oct.2022"
#define TOOL_LOGO_COLOR         0x62bde3ff

#define SUPPORT_COMPRESSED_FONT_ATLAS

#include "raylib.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
    #include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"                         // Required for: IMGUI controls

#undef RAYGUI_IMPLEMENTATION                // Avoid including raygui implementation again

#define GUI_MAIN_TOOLBAR_IMPLEMENTATION
#include "gui_main_toolbar.h"               // GUI: Main toolbar

#define GUI_WINDOW_HELP_IMPLEMENTATION
#include "gui_window_help.h"                // GUI: Help Window

#define GUI_WINDOW_ABOUT_IMPLEMENTATION
#include "gui_window_about.h"               // GUI: About Window

#define GUI_WINDOW_SPONSOR_IMPLEMENTATION
#include "gui_window_sponsor.h"             // GUI: Sponsor Window

#define GUI_FILE_DIALOGS_IMPLEMENTATION
#include "gui_file_dialogs.h"               // GUI: File Dialogs

// raygui embedded styles (used as templates)
// NOTE: Included in the same order as selector
#define MAX_GUI_STYLES_AVAILABLE   12       // NOTE: Included light style
#include "styles/style_jungle.h"            // raygui style: jungle
#include "styles/style_candy.h"             // raygui style: candy
#include "styles/style_lavanda.h"           // raygui style: lavanda
#include "styles/style_cyber.h"             // raygui style: cyber
#include "styles/style_terminal.h"          // raygui style: terminal
#include "styles/style_ashes.h"             // raygui style: ashes
#include "styles/style_bluish.h"            // raygui style: bluish
#include "styles/style_dark.h"              // raygui style: dark
#include "styles/style_cherry.h"            // raygui style: cherry
#include "styles/style_sunny.h"             // raygui style: sunny
#include "styles/style_enefete.h"           // raygui style: enefete

#define RPNG_IMPLEMENTATION
#include "external/rpng.h"                  // PNG chunks management

// Standard C libraries
#include <stdlib.h>                         // Required for: malloc(), free()
#include <string.h>                         // Required for: strcmp(), memcpy()
#include <stdio.h>                          // Required for: fopen(), fclose(), fread()...

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#if (!defined(_DEBUG) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)))
bool __stdcall FreeConsole(void);       // Close console from code (kernel32.lib)
#endif

// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO) && defined(_DEBUG)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Style file type to export
// NOTE: Exported style files (.rgs, .h) always embed the custom font (if provided)
// and the custom font atlas image is always GRAY+ALPHA and saved compressed (DEFLATE)
typedef enum {
    STYLE_BINARY = 0,       // Style binary file (.rgs)
    STYLE_AS_CODE,          // Style as (ready-to-use) code (.h)
    STYLE_TABLE_IMAGE,      // Style controls table image (for reference)
    STYLE_TEXT              // Style text file (.rgs), only supported on command-line
} GuiStyleFileType;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const char *toolName = TOOL_NAME;
static const char *toolVersion = TOOL_VERSION;
static const char *toolDescription = TOOL_DESCRIPTION;

// Controls name text
// NOTE: Some styles are shared by multiple controls
static const char *guiControlText[RAYGUI_MAX_CONTROLS] = {
    "DEFAULT",
    "LABEL",        // LABELBUTTON
    "BUTTON",
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

// Controls properties name text (common to all controls)
// NOTE: +2 extra: Background color and Line color
static const char *guiPropsText[RAYGUI_MAX_PROPS_BASE] = {
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

// DEFAULT control properties name text
// NOTE: This list removes some of the common properties for all controls (BORDER_WIDTH, TEXT_PADDING, TEXT_ALIGNMENT)
// to force individual set of those ones and it also adds some DEFAULT extended properties for convenience (BACKGROUND_COLOR, LINE_COLOR)
static const char *guiPropsDefaultText[14] = {
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
    // Additional extended properties for DEFAULT control
    "BACKGROUND_COLOR",          // DEFAULT extended property
    "LINE_COLOR"                 // DEFAULT extended property
};

// Style template names
static const char *styleNames[12] = {
    "Light",
    "Jungle",
    "Candy",
    "Lavanda",
    "Cyber",
    "Terminal",
    "Ashes",
    "Bluish",
    "Dark",
    "Cherry",
    "Sunny",
    "Enefete"
};

// Default style backup to check changed properties
static unsigned int defaultStyle[RAYGUI_MAX_CONTROLS*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)] = { 0 };

// Current active style template
static unsigned int currentStyle[RAYGUI_MAX_CONTROLS*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)] = { 0 };

// Custom font variables
// NOTE: They have to be global to be used bys tyle export functions
static Font customFont = { 0 };             // Custom font
static bool customFontLoaded = false;       // Custom font loaded flag (from font file or style file)

static char inFontFileName[512] = { 0 };    // Input font file name (required for font reloading on atlas regeneration)

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
#if defined(PLATFORM_DESKTOP)
static void ShowCommandLineInfo(void);                      // Show command line usage info
static void ProcessCommandLine(int argc, char *argv[]);     // Process command line input
#endif

// Load/Save/Export data functions
static unsigned char *SaveStyleToMemory(int *size);         // Save style to memory buffer
static bool SaveStyle(const char *fileName, int format);    // Save style binary file binary (.rgs)
static void ExportStyleAsCode(const char *fileName, const char *styleName);        // Export gui style as color palette code
static Image GenImageStyleControlsTable(const char *styleName); // Draw controls table image

// Auxiliar functions
static int StyleChangesCounter(unsigned int *refStyle);     // Count changed properties in current style (comparing to ref style)
static Color GuiColorBox(Rectangle bounds, Color *colorPicker, Color color);    // Gui color box

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    char inFileName[512] = { 0 };       // Input file name (required in case of drag & drop over executable)
    char outFileName[512] = { 0 };      // Output file name (required for file save/export)

    bool inputFileLoaded = false;       // Flag to detect an input file has been loaded (required for fast save)
    bool inputFontFileLoaded = false;   // Flag to detect an input font file has been loaded (required for font atlas regen)
    bool outputFileCreated = false;     // Flag to detect if an output file has been created (required for fast save)

#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messsages
#endif
#if defined(PLATFORM_DESKTOP)
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
                strcpy(inFileName, argv[1]);        // Read input filename to open with gui interface
            }
        }
        else
        {
            ProcessCommandLine(argc, argv);
            return 0;
        }
    }
#endif  // PLATFORM_DESKTOP
#if (!defined(_DEBUG) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)))
    // WARNING (Windows): If program is compiled as Window application (instead of console),
    // no console is available to show output info... solution is compiling a console application
    // and closing console (FreeConsole()) when changing to GUI interface
    FreeConsole();
#endif

    // GUI usage mode - Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 748;
    const int screenHeight = 610;

    InitWindow(screenWidth, screenHeight, TextFormat("%s v%s | %s", toolName, toolVersion, toolDescription));
    SetExitKey(0);

    // General pourpose variables
    Vector2 mousePos = { 0.0f, 0.0f };
    int frameCounter = 0;

    int changedPropCounter = 0;
    bool obtainProperty = false;
    bool selectingColor = false;

    // Load file if provided (drag & drop over executable)
    if ((inFileName[0] != '\0') && (IsFileExtension(inFileName, ".rgs")))
    {
        GuiLoadStyle(inFileName);
        SetWindowTitle(TextFormat("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
        inputFileLoaded = true;
    }
    else
    {
        GuiLoadStyleDefault();
        customFont = GetFontDefault();
    }

    // Default light style + current style backups (used to track changes)
    memcpy(defaultStyle, guiStyle, RAYGUI_MAX_CONTROLS*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)*sizeof(int));
    memcpy(currentStyle, guiStyle, RAYGUI_MAX_CONTROLS*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)*sizeof(int));

    // Init color picker saved colors
    Color colorBoxValue[12] = { 0 };
    for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));
    Vector3 colorHSV = { 0.0f, 0.0f, 0.0f };

    // Style table variables
    Texture texStyleTable = { 0 };
    int styleTablePositionX = 0;

    // Font atlas view variables
    float fontScale = 1.0f;
    int fontSizeValue = 10;
    int prevFontSizeValue = fontSizeValue;

    // Style required variables
    bool saveChangesRequired = false;     // Flag to notice save changes are required

    // GUI: Main Layout
    //-----------------------------------------------------------------------------------
    Vector2 anchorMain = { 0, 0 };
    Vector2 anchorWindow = { 353, 52 };
    Vector2 anchorPropEditor = { 363, 92 };
    Vector2 anchorFontOptions = { 363, 465 };

    int currentSelectedControl = -1;
    int currentSelectedProperty = -1;
    int previousSelectedProperty = -1;
    int previousSelectedControl = -1;

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

    bool screenSizeActive = false;
    bool controlsWindowActive = true;   // Show window: controls
    //-----------------------------------------------------------------------------------

    // GUI: Main toolbar panel (file and visualization)
    //-----------------------------------------------------------------------------------
    GuiMainToolbarState mainToolbarState = InitGuiMainToolbar();
    //-----------------------------------------------------------------------------------

    // GUI: Help Window
    //-----------------------------------------------------------------------------------
    GuiWindowHelpState windowHelpState = InitGuiWindowHelp();
    //-----------------------------------------------------------------------------------

    // GUI: About Window
    //-----------------------------------------------------------------------------------
    GuiWindowAboutState windowAboutState = InitGuiWindowAbout();
    //-----------------------------------------------------------------------------------
    
    // GUI: Sponsor Window
    //-----------------------------------------------------------------------------------
    GuiWindowSponsorState windowSponsorState = InitGuiWindowSponsor();
    //-----------------------------------------------------------------------------------

    // GUI: Export Window
    //-----------------------------------------------------------------------------------
    bool windowExportActive = false;

    int exportFormatActive = 0;         // ComboBox file type selection
    char styleNameText[128] = "Unnamed"; // Style name text box
    bool styleNameEditMode = false;     // Style name text box edit mode
    bool embedFontChecked = true;       // Select to embed font into style file
    bool styleChunkChecked = false;     // Select to embed style as a PNG chunk (rGSf)
    //-----------------------------------------------------------------------------------

    // GUI: Exit Window
    //-----------------------------------------------------------------------------------
    bool closeWindow = false;
    bool windowExitActive = false;
    //-----------------------------------------------------------------------------------

    // GUI: Custom file dialogs
    //-----------------------------------------------------------------------------------
    bool showLoadFileDialog = false;
    bool showLoadFontFileDialog = false;
    bool showSaveFileDialog = false;
    bool showExportFileDialog = false;
    //-----------------------------------------------------------------------------------

//#define STYLES_SPINNING_DEMO
#if defined(STYLES_SPINNING_DEMO)
    int styleCounter = 0;
    char stylesList[8][64] = {
        "../styles/jungle.rgs\0",
        "../styles/candy.rgs\0",
        "../styles/bluish.rgs\0",
        "../styles/cherry.rgs\0",
        "../styles/ashes.rgs\0",
        "../styles/cyber.rgs\0",
        "../styles/lavanda.rgs\0",
        "../styles/terminal.rgs\0"
    };
#endif

    // Render texture to draw full screen, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    RenderTexture2D screenTarget = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    SetTextureFilter(screenTarget.texture, TEXTURE_FILTER_POINT);

    SetTargetFPS(60);       // Set our game desired framerate
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!closeWindow)    // Detect window close button
    {
        // WARNING: ASINCIFY requires this line,
        // it contains the call to emscripten_sleep() for PLATFORM_WEB
        if (WindowShouldClose()) windowExitActive = true;

        // Dropped files logic
        //----------------------------------------------------------------------------------
        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();

            // Supports loading .rgs style files (text or binary) and .png style palette images
            if (IsFileExtension(droppedFiles.paths[0], ".rgs"))
            {
                GuiLoadStyleDefault();                  // Reset to base default style
                GuiLoadStyle(droppedFiles.paths[0]);    // Load new style properties

                strcpy(inFileName, droppedFiles.paths[0]);
                SetWindowTitle(TextFormat("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
                inputFileLoaded = true;

                fontSizeValue = GuiGetStyle(DEFAULT, TEXT_SIZE);
                fontSpacingValue = GuiGetStyle(DEFAULT, TEXT_SPACING);

                // Load .rgs custom font in font
                customFont = GuiGetFont();
                memset(inFontFileName, 0, 512);
                inputFontFileLoaded = false;
                customFontLoaded = true;

                // Reset style backup for changes
                memcpy(currentStyle, guiStyle, RAYGUI_MAX_CONTROLS *(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED));
                changedPropCounter = 0;
                saveChangesRequired = false;
            }
            else if (IsFileExtension(droppedFiles.paths[0], ".ttf") || IsFileExtension(droppedFiles.paths[0], ".otf"))
            {
                // Unload previous font if it was file provided but
                // avoid unloading a font comming from some style tempalte
                if (inputFontFileLoaded) UnloadFont(customFont);

                // NOTE: Font generation size depends on spinner size selection
                customFont = LoadFontEx(droppedFiles.paths[0], fontSizeValue, NULL, 0);

                if (customFont.texture.id > 0)
                {
                    GuiSetFont(customFont);
                    strcpy(inFontFileName, droppedFiles.paths[0]);
                    inputFontFileLoaded = true;
                    customFontLoaded = true;
                }
            }

            for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));

            UnloadDroppedFiles(droppedFiles);   // Unload filepaths from memory

            currentSelectedControl = -1;    // Reset selected control
        }
        //----------------------------------------------------------------------------------

        // Keyboard shortcuts
        //----------------------------------------------------------------------------------
#if defined(STYLES_SPINNING_DEMO)
        if (IsKeyPressed(KEY_SPACE))
        {
            currentSelectedProperty = -1;

            //GuiLoadStyleDefault();          // Reset to base default style
            GuiLoadStyle(stylesList[styleCounter]);  // Load new style properties

            strcpy(inFileName, GetFileName(stylesList[styleCounter]));
            SetWindowTitle(TextFormat("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
            strcpy(styleNameText, GetFileNameWithoutExt(inFileName));

            genFontSizeValue = GuiGetStyle(DEFAULT, TEXT_SIZE);
            fontSpacingValue = GuiGetStyle(DEFAULT, TEXT_SPACING);

            // Load .rgs custom font in font
            customFont = GuiGetFont();
            memset(fontFilePath, 0, 512);
            fontFileProvided = false;
            customFontLoaded = true;

            // Regenerate style table
            UnloadTexture(texStyleTable);
            Image imStyleTable = GenImageStyleControlsTable(styleNameText);
            texStyleTable = LoadTextureFromImage(imStyleTable);
            UnloadImage(imStyleTable);

            styleCounter++;
            if (styleCounter > 7) styleCounter = 0;
        }
#endif

#if defined(PLATFORM_DESKTOP)
        // Toggle screen size (x2) mode
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_F)) screenSizeActive = !screenSizeActive;
#endif
        // New style file, previous in/out files registeres are reseted
        if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_N)) || mainToolbarState.btnNewFilePressed)
        {
            memset(inFileName, 0, 512);
            memset(outFileName, 0, 512);
            inputFileLoaded = false;
            outputFileCreated = false;

            // Force current style template reset
            mainToolbarState.btnReloadStylePressed = true;
        }

        // Show dialog: load input file (.rgs)
        if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) || mainToolbarState.btnLoadFilePressed) showLoadFileDialog = true;

        // Show dialog: save style file (.rgs)
        if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) || mainToolbarState.btnSaveFilePressed)
        {
#if defined(PLATFORM_DESKTOP)
            // NOTE: Fast-save only works for already loaded/saved .rgs styles
            if (inputFileLoaded || outputFileCreated)
            {
                // Priority to output file saving
                if (outputFileCreated)
                {
                    SaveStyle(outFileName, STYLE_BINARY);
                    SetWindowTitle(TextFormat("%s v%s - %s", toolName, toolVersion, GetFileName(outFileName)));
                }
                else
                {
                    SaveStyle(inFileName, STYLE_BINARY);
                    SetWindowTitle(TextFormat("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
                }

                saveChangesRequired = false;
            }
            else
#endif
            {
                // If no input/output file already loaded/saved, show save file dialog
                exportFormatActive = STYLE_BINARY;
                strcpy(outFileName, TextFormat("%s.rgs", TextToLower(styleNameText)));
                showSaveFileDialog = true;
            }
        }

        // Show dialog: export style file (.rgs, .png, .h)
        if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_E)) || mainToolbarState.btnExportFilePressed) windowExportActive = true;

        // Toggle window: help
        if (IsKeyPressed(KEY_F1)) windowHelpState.windowActive = !windowHelpState.windowActive;

        // Toggle window: about
        if (IsKeyPressed(KEY_F2)) windowAboutState.windowActive = !windowAboutState.windowActive;

        // Toggle window: sponsor
        if (IsKeyPressed(KEY_F3)) windowSponsorState.windowActive = !windowSponsorState.windowActive;

        // Show window: style table image
        if (IsKeyPressed(KEY_F5)) mainToolbarState.viewStyleTableActive = true;

        // Show window: font atlas
        if (IsKeyPressed(KEY_F6)) mainToolbarState.viewFontActive = true;

        // Show closing window on ESC
        if (IsKeyPressed(KEY_ESCAPE))
        {
            if (windowHelpState.windowActive) windowHelpState.windowActive = false;
            else if (windowAboutState.windowActive) windowAboutState.windowActive = false;
            else if (windowSponsorState.windowActive) windowSponsorState.windowActive = false;
            else if (windowExportActive) windowExportActive = false;
            else if (mainToolbarState.viewFontActive) mainToolbarState.viewFontActive = false;
            else if (mainToolbarState.viewStyleTableActive) mainToolbarState.viewStyleTableActive = false;
        #if defined(PLATFORM_DESKTOP)
            else if (changedPropCounter > 0) windowExitActive = !windowExitActive;
            else closeWindow = true;
        #else
            else if (showLoadFileDialog) showLoadFileDialog = false;
            else if (showSaveFileDialog) showSaveFileDialog = false;
            else if (showExportFileDialog) showExportFileDialog = false;
        #endif
        }

        // Select desired state for visualization
        if (IsKeyPressed(KEY_ONE)) mainToolbarState.propsStateActive = 0;
        else if (IsKeyPressed(KEY_TWO)) mainToolbarState.propsStateActive = 1;
        else if (IsKeyPressed(KEY_THREE)) mainToolbarState.propsStateActive = 2;
        else if (IsKeyPressed(KEY_FOUR)) mainToolbarState.propsStateActive = 3;

        // Reset to current style template
        if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R)) || mainToolbarState.btnReloadStylePressed)
        {
            if (mainToolbarState.visualStyleActive == 0) mainToolbarState.prevVisualStyleActive = 1;
            else mainToolbarState.prevVisualStyleActive = 0;
        }

        // Select visual style
        if (IsKeyPressed(KEY_LEFT)) mainToolbarState.visualStyleActive--;
        else if (IsKeyPressed(KEY_RIGHT)) mainToolbarState.visualStyleActive++;
        if (mainToolbarState.visualStyleActive < 0) mainToolbarState.visualStyleActive = MAX_GUI_STYLES_AVAILABLE - 1;
        else if (mainToolbarState.visualStyleActive > (MAX_GUI_STYLES_AVAILABLE - 1)) mainToolbarState.visualStyleActive = 0;
        //----------------------------------------------------------------------------------

        // Main toolbar logic
        //----------------------------------------------------------------------------------
        // File options logic
        if (mainToolbarState.btnRandomStylePressed)
        {
            // Generate random style
            float hueNormal = GetRandomValue(0, 360);
            float value = GetRandomValue(0, 100)/100.0f;

            float hueFocused = hueNormal;
            float huePressed = hueNormal;
            float hueDisabled = hueNormal;

            switch (GetRandomValue(0, 3))
            {
                case 0: hueFocused = hueNormal - 180; break;    // Focused items are complementary color
                case 1: huePressed = hueNormal - 180; break;    // Pressed items are complementary color
                case 2:                                         // Focused and pressed are split complementary
                {
                    int offset = GetRandomValue(60, 160);
                    int direction = GetRandomValue(0, 1);
                    if (direction == 0) direction = -1;
                    hueFocused = hueNormal + offset*direction;
                    huePressed = hueNormal + (offset*direction*-1);
                } break;
                default: break;
            }

            if (hueFocused < 0) hueFocused = 360 + hueFocused;
            else if (hueFocused > 360) hueFocused -= 360;

            if (huePressed < 0) huePressed = 360 + huePressed;
            else if (huePressed > 360) huePressed -= 360;

            Vector3 hsvNormal = { hueNormal, 0.8f, value };
            Vector3 hsvFocused = { hueFocused, 1.0f, 1.0f - hsvNormal.z };
            Vector3 hsvPressed = { huePressed, 0.5f, hsvFocused.z };
            Vector3 hsvDisabled = { hueDisabled, 0.2, value };

            // Update style default color values
            GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(ColorFromHSV(hsvNormal.x, hsvNormal.y, hsvNormal.z)));
            GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(ColorFromHSV(hsvNormal.x, GetRandomValue(4, 7) / 10.0f, (fabsf(0.5 - hsvNormal.z) < 0.2)? 1.0 + ((GetRandomValue(3,5) / 10.0f) * fabsf(0.5 - hsvNormal.z) / (0.5 - hsvNormal.z)) : 1 - hsvNormal.z)));
            GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(ColorFromHSV(hsvNormal.x, hsvNormal.y, hsvNormal.z)));

            GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, ColorToInt(ColorFromHSV(hsvFocused.x, hsvFocused.y, hsvFocused.z)));
            GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, ColorToInt(ColorFromHSV(hsvFocused.x, GetRandomValue(4, 7) / 10.0f, 1 - hsvFocused.z)));
            GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt(ColorFromHSV(hsvFocused.x, hsvFocused.y, hsvFocused.z)));

            GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, ColorToInt(ColorFromHSV(hsvPressed.x, hsvPressed.y, hsvPressed.z)));
            GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, ColorToInt(ColorFromHSV(hsvPressed.x, GetRandomValue(4, 7) / 10.0f, 1 - hsvPressed.z)));
            GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, ColorToInt(ColorFromHSV(hsvPressed.x, hsvPressed.y, hsvPressed.z)));

            GuiSetStyle(DEFAULT, BORDER_COLOR_DISABLED, ColorToInt(ColorFromHSV(hsvDisabled.x, hsvDisabled.y, hsvDisabled.z)));
            GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, ColorToInt(ColorFromHSV(hsvDisabled.x, hsvDisabled.y, 1 - hsvDisabled.z)));
            GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, ColorToInt(ColorFromHSV(hsvDisabled.x, hsvDisabled.y, hsvDisabled.z)));

            GuiSetStyle(DEFAULT, BACKGROUND_COLOR, GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL));
            GuiSetStyle(DEFAULT, LINE_COLOR, GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL));

            // Update color boxes palette
            for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));
        }

        // Visual options logic
        if (mainToolbarState.visualStyleActive != mainToolbarState.prevVisualStyleActive)
        {
            // When a new template style is selected, everything is reseted
            currentSelectedControl = -1;
            currentSelectedProperty = -1;

            // Reset to default internal style
            // NOTE: Required to unload any previously loaded font texture
            GuiLoadStyleDefault();

            switch (mainToolbarState.visualStyleActive)
            {
                case 1: GuiLoadStyleJungle(); break;
                case 2: GuiLoadStyleCandy(); break;
                case 3: GuiLoadStyleLavanda(); break;
                case 4: GuiLoadStyleCyber(); break;
                case 5: GuiLoadStyleTerminal(); break;
                case 6: GuiLoadStyleAshes(); break;
                case 7: GuiLoadStyleBluish(); break;
                case 8: GuiLoadStyleDark(); break;
                case 9: GuiLoadStyleCherry(); break;
                case 10: GuiLoadStyleSunny(); break;
                case 11: GuiLoadStyleEnefete(); break;
                default: break;
            }

            // Current style backup (used to track changes)
            memcpy(currentStyle, guiStyle, RAYGUI_MAX_CONTROLS *(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)*sizeof(int));

            customFont = GuiGetFont();
            customFontLoaded = true;
            fontSizeValue = GuiGetStyle(DEFAULT, TEXT_SIZE);
            fontSpacingValue = GuiGetStyle(DEFAULT, TEXT_SPACING);

            for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));

            changedPropCounter = 0;
            saveChangesRequired = false;

            mainToolbarState.prevVisualStyleActive = mainToolbarState.visualStyleActive;
        }

        // Help options logic
        if (mainToolbarState.btnHelpPressed) windowHelpState.windowActive = true;           // Help button logic
        if (mainToolbarState.btnAboutPressed) windowAboutState.windowActive = true;         // About window button logic
        if (mainToolbarState.btnSponsorPressed) windowSponsorState.windowActive = true;     // User sponsor logic
        //----------------------------------------------------------------------------------

        // Basic program flow logic
        //----------------------------------------------------------------------------------
        frameCounter++;                     // General usage frames counter
        mousePos = GetMousePosition();      // Get mouse position each frame

        // Check for changed properties
        changedPropCounter = StyleChangesCounter(currentStyle);
        if (changedPropCounter > 0) saveChangesRequired = true;

        // Reload font and generate new atlas at new size when required
        if (inputFontFileLoaded && (inFontFileName[0] != '\0') &&   // Check an external font file is provided (not internal custom one)
            !genFontSizeEditMode &&                                 // Check the spinner text editing has finished
            (prevFontSizeValue != fontSizeValue))                   // Check selected size actually changed
        {
            UnloadFont(customFont);
            customFont = LoadFontEx(inFontFileName, fontSizeValue, NULL, 0);
            GuiSetFont(customFont);
            customFontLoaded = true;
            saveChangesRequired = true;
        }

        GuiSetStyle(DEFAULT, TEXT_SIZE, fontSizeValue);
        GuiSetStyle(DEFAULT, TEXT_SPACING, fontSpacingValue);

        prevFontSizeValue = fontSizeValue;

        // Controls selection on list view logic
        //----------------------------------------------------------------------------------
        if ((previousSelectedControl != currentSelectedControl)) currentSelectedProperty = -1;

        if ((currentSelectedControl >= 0) && (currentSelectedProperty >= 0))
        {
            if ((previousSelectedProperty != currentSelectedProperty) && !obtainProperty) obtainProperty = true;

            if (obtainProperty)
            {
                // Get the previous style property for the control
                if (currentSelectedControl == DEFAULT)
                {
                    if (currentSelectedProperty <= TEXT_COLOR_DISABLED) colorPickerValue = GetColor(GuiGetStyle(currentSelectedControl, currentSelectedProperty));
                    else if (currentSelectedProperty == 13) colorPickerValue = GetColor(GuiGetStyle(currentSelectedControl, LINE_COLOR));
                    else if (currentSelectedProperty == 12) colorPickerValue = GetColor(GuiGetStyle(currentSelectedControl, BACKGROUND_COLOR));
                }
                else
                {
                    if (currentSelectedProperty <= TEXT_COLOR_DISABLED) colorPickerValue = GetColor(GuiGetStyle(currentSelectedControl, currentSelectedProperty));
                    else if ((currentSelectedProperty == BORDER_WIDTH) || (currentSelectedProperty == TEXT_PADDING)) propertyValue = GuiGetStyle(currentSelectedControl, currentSelectedProperty);
                    else if (currentSelectedProperty == TEXT_ALIGNMENT) textAlignmentActive = GuiGetStyle(currentSelectedControl, currentSelectedProperty);
                }

                obtainProperty = false;
            }

            // Set selected value for current selected property
            if (currentSelectedControl == DEFAULT)
            {
                // Update special default extended properties: BACKGROUND_COLOR and LINE_COLOR
                if (currentSelectedProperty <= TEXT_COLOR_DISABLED) GuiSetStyle(currentSelectedControl, currentSelectedProperty, ColorToInt(colorPickerValue));
                else if (currentSelectedProperty == 13) GuiSetStyle(currentSelectedControl, LINE_COLOR, ColorToInt(colorPickerValue));
                else if (currentSelectedProperty == 12) GuiSetStyle(currentSelectedControl, BACKGROUND_COLOR, ColorToInt(colorPickerValue));

            }
            else
            {
                // Update control property
                if (currentSelectedProperty <= TEXT_COLOR_DISABLED) GuiSetStyle(currentSelectedControl, currentSelectedProperty, ColorToInt(colorPickerValue));
                else if ((currentSelectedProperty == BORDER_WIDTH) || (currentSelectedProperty == TEXT_PADDING)) GuiSetStyle(currentSelectedControl, currentSelectedProperty, propertyValue);
                else if (currentSelectedProperty == TEXT_ALIGNMENT) GuiSetStyle(currentSelectedControl, currentSelectedProperty, textAlignmentActive);
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
        if (mainToolbarState.viewStyleTableActive && (mainToolbarState.prevViewStyleTableActive != mainToolbarState.viewStyleTableActive))
        {
            UnloadTexture(texStyleTable);

            Image imStyleTable = GenImageStyleControlsTable(styleNameText);
            texStyleTable = LoadTextureFromImage(imStyleTable);
            UnloadImage(imStyleTable);
        }

        if (mainToolbarState.viewStyleTableActive)
        {
            if (IsKeyDown(KEY_RIGHT)) styleTablePositionX += 5;
            else if (IsKeyDown(KEY_LEFT)) styleTablePositionX -= 5;

            styleTablePositionX += GetMouseWheelMove()*10;
            if (styleTablePositionX < 0) styleTablePositionX = 0;
            else if (styleTablePositionX > (texStyleTable.width - screenWidth)) styleTablePositionX = texStyleTable.width - screenWidth;
        }

        mainToolbarState.prevViewStyleTableActive = mainToolbarState.viewStyleTableActive;
        //----------------------------------------------------------------------------------

        // Font image scale logic
        //----------------------------------------------------------------------------------
        if (mainToolbarState.viewFontActive)
        {
            fontScale += GetMouseWheelMove();
            if (fontScale < 1.0f) fontScale = 1.0f;
            if (customFont.texture.width*fontScale > screenWidth) fontScale = screenWidth/customFont.texture.width;
        }
        //----------------------------------------------------------------------------------

        // Screen scale logic (x2)
        //----------------------------------------------------------------------------------
        if (screenSizeActive)
        {
            // Screen size x2
            if (GetScreenWidth() < screenWidth*2)
            {
                SetWindowSize(screenWidth*2, screenHeight*2);
                SetMouseScale(0.5f, 0.5f);
            }
        }
        else
        {
            // Screen size x1
            if (screenWidth*2 >= GetScreenWidth())
            {
                SetWindowSize(screenWidth, screenHeight);
                SetMouseScale(1.0f, 1.0f);
            }
        }
        //----------------------------------------------------------------------------------

        // WARNING: Some windows should lock the main screen controls when shown
        if (windowHelpState.windowActive ||
            windowAboutState.windowActive ||
            windowSponsorState.windowActive ||
            mainToolbarState.viewStyleTableActive ||
            mainToolbarState.viewFontActive ||
            mainToolbarState.propsStateEditMode ||
            windowExitActive ||
            windowExportActive ||
            showLoadFileDialog ||
            showSaveFileDialog ||
            showExportFileDialog) GuiLock();
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        // Render all screen to texture (for scaling)
        BeginTextureMode(screenTarget);
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

            // GUI: Main screen controls
            //---------------------------------------------------------------------------------------------------------
            // Set custom gui state if selected
            GuiSetState(mainToolbarState.propsStateActive);

            // In case a custom gui state is selected for review, we reset the selected property
            if (mainToolbarState.propsStateActive != STATE_NORMAL) currentSelectedProperty = -1;

            // List views
            currentSelectedControl = GuiListView((Rectangle){ anchorMain.x + 10, anchorMain.y + 52, 148, 520 }, TextJoin(guiControlText, RAYGUI_MAX_CONTROLS, ";"), NULL, currentSelectedControl);
            if (currentSelectedControl != DEFAULT) currentSelectedProperty = GuiListViewEx((Rectangle){ anchorMain.x + 163, anchorMain.y + 52, 180, 520 }, guiPropsText, RAYGUI_MAX_PROPS_BASE - 1, NULL, NULL, currentSelectedProperty);
            else currentSelectedProperty = GuiListViewEx((Rectangle){ anchorMain.x + 163, anchorMain.y + 52, 180, 520 }, guiPropsDefaultText, 14, NULL, NULL, currentSelectedProperty);

            // Controls window
            if (controlsWindowActive)
            {
                controlsWindowActive = !GuiWindowBox((Rectangle){ anchorWindow.x + 0, anchorWindow.y + 0, 385, 520 }, "#198#Sample raygui controls");

                GuiGroupBox((Rectangle){ anchorPropEditor.x + 0, anchorPropEditor.y + 0, 365, 357 }, "Property Editor");

                if ((mainToolbarState.propsStateActive == STATE_NORMAL) && (currentSelectedProperty != TEXT_PADDING) && (currentSelectedProperty != BORDER_WIDTH)) GuiDisable();
                if (currentSelectedControl == DEFAULT) GuiDisable();
                propertyValue = GuiSlider((Rectangle){ anchorPropEditor.x + 50, anchorPropEditor.y + 15, 235, 15 }, "Value:", NULL, propertyValue, 0, 20);
                if (GuiValueBox((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 10, 60, 25 }, NULL, &propertyValue, 0, 8, propertyValueEditMode)) propertyValueEditMode = !propertyValueEditMode;
                if (mainToolbarState.propsStateActive != STATE_DISABLED) GuiEnable();

                GuiLine((Rectangle){ anchorPropEditor.x + 0, anchorPropEditor.y + 35, 365, 15 }, NULL);
                colorPickerValue = GuiColorPicker((Rectangle){ anchorPropEditor.x + 10, anchorPropEditor.y + 55, 240, 240 }, NULL, colorPickerValue);

                GuiGroupBox((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 60, 60, 55 }, "RGBA");
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 65, 20, 20 }, TextFormat("R:  %03i", colorPickerValue.r));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 80, 20, 20 }, TextFormat("G:  %03i", colorPickerValue.g));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 95, 20, 20 }, TextFormat("B:  %03i", colorPickerValue.b));
                GuiGroupBox((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 125, 60, 55 }, "HSV");
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 130, 20, 20 }, TextFormat("H:  %.0f", colorHSV.x));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 145, 20, 20 }, TextFormat("S:  %.0f%%", colorHSV.y*100));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 160, 20, 20 }, TextFormat("V:  %.0f%%", colorHSV.z*100));

                if (GuiTextBox((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 275, 60, 20 }, hexColorText, 9, textHexColorEditMode))
                {
                    textHexColorEditMode = !textHexColorEditMode;
                    colorPickerValue = GetColor((int)strtoul(hexColorText, NULL, 16));
                }

                // Draw colors selector palette
                for (int i = 0; i < 12; i++) colorBoxValue[i] = GuiColorBox((Rectangle){ anchorPropEditor.x + 295 + 20*(i%3), anchorPropEditor.y + 190 + 20*(i/3), 20, 20 }, &colorPickerValue, colorBoxValue[i]);
                DrawRectangleLinesEx((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 190, 60, 80 }, 2, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL)));

                GuiLine((Rectangle){ anchorPropEditor.x + 0, anchorPropEditor.y + 300, 365, 15 }, NULL);

                if ((mainToolbarState.propsStateActive == STATE_NORMAL) && (currentSelectedProperty != TEXT_ALIGNMENT)) GuiDisable();
                GuiLabel((Rectangle){ anchorPropEditor.x + 10, anchorPropEditor.y + 320, 85, 24 }, "Text Alignment:");
                textAlignmentActive = GuiToggleGroup((Rectangle){ anchorPropEditor.x + 110, anchorPropEditor.y + 320, 80, 24 }, "#87#LEFT;#89#CENTER;#83#RIGHT", textAlignmentActive);
                if (mainToolbarState.propsStateActive != STATE_DISABLED) GuiEnable();

                // Font options
                GuiGroupBox((Rectangle){ anchorFontOptions.x + 0, anchorFontOptions.y + 0, 365, 90 }, "Font Options");
                if (GuiButton((Rectangle){ anchorFontOptions.x + 10, anchorFontOptions.y + 16, 85, 24 }, "#30#Load")) showLoadFontFileDialog = true;

                if (GuiSpinner((Rectangle){ anchorFontOptions.x + 135, anchorFontOptions.y + 16, 80, 24 }, "Size:", &fontSizeValue, 8, 32, genFontSizeEditMode)) genFontSizeEditMode = !genFontSizeEditMode;
                if (GuiSpinner((Rectangle){ anchorFontOptions.x + 275, anchorFontOptions.y + 16, 80, 24 }, "Spacing:", &fontSpacingValue, -4, 8, fontSpacingEditMode)) fontSpacingEditMode = !fontSpacingEditMode;

                if (GuiTextBox((Rectangle){ anchorFontOptions.x + 10, anchorFontOptions.y + 52, 345, 28 }, fontSampleText, 128, fontSampleEditMode)) fontSampleEditMode = !fontSampleEditMode;
            }
            else
            {
                GuiStatusBar((Rectangle){ anchorWindow.x + 0, anchorWindow.y + 0, 385, 24 }, "#198#Sample raygui controls");
                GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
                if (GuiButton((Rectangle){ anchorWindow.x + 385 - 16 - 5, anchorWindow.y + 3, 18, 18 }, "#53#")) controlsWindowActive = true;
                GuiSetStyle(BUTTON, BORDER_WIDTH, 2);
            }
            //---------------------------------------------------------------------------------------------------------

            // GUI: Status bar
            //----------------------------------------------------------------------------------------
            GuiStatusBar((Rectangle){ 0, GetScreenHeight() - 24, 160, 24 }, TextFormat("Name: %s", (changedPropCounter > 0)? styleNameText : styleNames[mainToolbarState.visualStyleActive]));
            GuiStatusBar((Rectangle){159, GetScreenHeight() - 24, 190, 24 }, TextFormat("CHANGED PROPERTIES: %i", changedPropCounter));

            if (inputFontFileLoaded) GuiStatusBar((Rectangle){ 348, GetScreenHeight() - 24, 405, 24 }, TextFormat("FONT: %s (%i x %i) - %i bytes", GetFileName(inFontFileName), customFont.texture.width, customFont.texture.height, GetPixelDataSize(customFont.texture.width, customFont.texture.height, customFont.texture.format)));
            else GuiStatusBar((Rectangle){ 348, GetScreenHeight() - 24, 405, 24 }, TextFormat("FONT: %s (%i x %i) - %i bytes", (customFontLoaded)? "style custom font" : "raylib default", customFont.texture.width, customFont.texture.height, GetPixelDataSize(customFont.texture.width, customFont.texture.height, customFont.texture.format)));
            //----------------------------------------------------------------------------------------

            // NOTE: If some overlap window is open and main window is locked, we draw a background rectangle
            if (GuiIsLocked()) DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)), 0.85f));

            // WARNING: Before drawing the windows, we unlock them
            GuiUnlock();

            // Set default NORMAL state for all controls not in main screen
            GuiSetState(STATE_NORMAL);

            // GUI: Main toolbar panel
            //----------------------------------------------------------------------------------
            GuiMainToolbar(&mainToolbarState);
            //----------------------------------------------------------------------------------

            // GUI: Show font texture
            //----------------------------------------------------------------------------------------
            if (mainToolbarState.viewFontActive)
            {
                DrawRectangle(screenWidth/2 - customFont.texture.width*fontScale/2, screenHeight/2 - customFont.texture.height*fontScale/2, customFont.texture.width*fontScale, customFont.texture.height*fontScale, BLACK);
                DrawRectangleLines(screenWidth/2 - customFont.texture.width*fontScale/2, screenHeight/2 - customFont.texture.height*fontScale/2, customFont.texture.width*fontScale, customFont.texture.height*fontScale, RED);
                DrawTextureEx(customFont.texture, (Vector2){ screenWidth/2 - customFont.texture.width*fontScale/2, screenHeight/2 - customFont.texture.height*fontScale/2 }, 0.0f, fontScale, WHITE);
            }
            //----------------------------------------------------------------------------------------

            // GUI: Show style table image (if active and reloaded)
            //----------------------------------------------------------------------------------------
            if (mainToolbarState.viewStyleTableActive && (mainToolbarState.prevViewStyleTableActive == mainToolbarState.viewStyleTableActive))
            {
                DrawTexture(texStyleTable, -styleTablePositionX, screenHeight/2 - texStyleTable.height/2, WHITE);
                DrawRectangleLines(-styleTablePositionX, screenHeight/2 - texStyleTable.height/2, texStyleTable.width, texStyleTable.height, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
                styleTablePositionX = GuiSlider((Rectangle){ 0, screenHeight/2 + texStyleTable.height/2, screenWidth, 15 }, NULL, NULL, styleTablePositionX, 0, texStyleTable.width - screenWidth);
            }
            //----------------------------------------------------------------------------------------
            
            // GUI: Help Window
            //----------------------------------------------------------------------------------------
            windowHelpState.windowBounds.x = (float)screenWidth/2 - windowHelpState.windowBounds.width/2;
            windowHelpState.windowBounds.y = (float)screenHeight/2 - windowHelpState.windowBounds.height/2;
            GuiWindowHelp(&windowHelpState);
            //----------------------------------------------------------------------------------------

            // GUI: About Window
            //----------------------------------------------------------------------------------------
            windowAboutState.windowBounds.x = (float)screenWidth/2 - windowAboutState.windowBounds.width/2;
            windowAboutState.windowBounds.y = (float)screenHeight/2 - windowAboutState.windowBounds.height/2;
            GuiWindowAbout(&windowAboutState);
            //----------------------------------------------------------------------------------------
            
            // GUI: Sponsor Window
            //----------------------------------------------------------------------------------------
            windowSponsorState.windowBounds.x = (float)screenWidth/2 - windowSponsorState.windowBounds.width/2;
            windowSponsorState.windowBounds.y = (float)screenHeight/2 - windowSponsorState.windowBounds.height/2;
            GuiWindowSponsor(&windowSponsorState);
            //----------------------------------------------------------------------------------------

            // GUI: Export Window
            //----------------------------------------------------------------------------------------
            if (windowExportActive)
            {
                Rectangle messageBox = { (float)screenWidth/2 - 248/2, (float)screenHeight/2 - 150, 248, 196 };
                int result = GuiMessageBox(messageBox, "#7#Export Style File", " ", "#7# Export Style");

                GuiLabel((Rectangle){ messageBox.x + 12, messageBox.y + 24 + 12, 106, 24 }, "Style Name:");
                if (GuiTextBox((Rectangle){ messageBox.x + 12 + 92, messageBox.y + 24 + 12, 132, 24 }, styleNameText, 128, styleNameEditMode)) styleNameEditMode = !styleNameEditMode;

                GuiLabel((Rectangle){ messageBox.x + 12, messageBox.y + 12 + 48 + 8, 106, 24 }, "Style Format:");
                exportFormatActive = GuiComboBox((Rectangle){ messageBox.x + 12 + 92, messageBox.y + 12 + 48 + 8, 132, 24 }, "Binary (.rgs);Code (.h);Image (.png)", exportFormatActive);

                GuiDisable();   // Font embedded by default!
                embedFontChecked = GuiCheckBox((Rectangle){ messageBox.x + 20, messageBox.y + 48 + 56, 16, 16 }, "Embed font atlas into style", embedFontChecked);
                GuiEnable();
                if (exportFormatActive != 2) GuiDisable();
                styleChunkChecked = GuiCheckBox((Rectangle){ messageBox.x + 20, messageBox.y + 72 + 32 + 24, 16, 16 }, "Embed style as rGSf chunk", styleChunkChecked);
                GuiEnable();

                if (result == 1)    // Export button pressed
                {
                    windowExportActive = false;
                    showExportFileDialog = true;
                }
                else if (result == 0) windowExportActive = false;
            }
            //----------------------------------------------------------------------------------

            // GUI: Exit Window
            //----------------------------------------------------------------------------------------
            if (windowExitActive)
            {
                int result = GuiMessageBox((Rectangle){ (float)screenWidth/2 - 125, (float)screenHeight/2 - 50, 250, 100 }, "#159#Closing rGuiStyler", "Do you really want to exit?", "Yes;No");

                if ((result == 0) || (result == 2)) windowExitActive = false;
                else if (result == 1) closeWindow = true;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Load File Dialog (and loading logic)
            //----------------------------------------------------------------------------------------
            if (showLoadFileDialog)
            {
#if defined(CUSTOM_MODAL_DIALOGS)
                int result = GuiFileDialog(DIALOG_MESSAGE, "Load raygui style file ...", inFileName, "Ok", "Just drag and drop your .rgs style file!");
#else
                int result = GuiFileDialog(DIALOG_OPEN_FILE, "Load raygui style file", inFileName, "*.rgs", "raygui Style Files (*.rgs)");
#endif
                if (result == 1)
                {
                    // Load style
                    GuiLoadStyle(inFileName);
                    SetWindowTitle(TextFormat("%s v%s - %s", toolName, toolVersion, GetFileName(inFileName)));
                    inputFileLoaded = true;

                    // Load .rgs custom font in font
                    customFont = GuiGetFont();
                    memset(inFontFileName, 0, 512);
                    inputFontFileLoaded = false;
                    customFontLoaded = true;

                    saveChangesRequired = false;
                }

                if (result >= 0) showLoadFileDialog = false;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Load Font File Dialog (and loading logic)
            //----------------------------------------------------------------------------------------
            if (showLoadFontFileDialog)
            {
#if defined(CUSTOM_MODAL_DIALOGS)
                int result = GuiFileDialog(DIALOG_MESSAGE, "Load font file ...", inFontFileName, "Ok", "Just drag and drop your .ttf/.otf font file!");
#else
                int result = GuiFileDialog(DIALOG_OPEN_FILE, "Load font file", inFontFileName, "*.ttf;*.otf", "Font Files (*.ttf, *.otf)");
#endif
                if (result == 1)
                {
                    // Load font file
                    Font tempFont = LoadFontEx(inFontFileName, fontSizeValue, NULL, 0);

                    if (tempFont.texture.id > 0)
                    {
                        if (inputFontFileLoaded) UnloadFont(customFont);   // Unload previously loaded font
                        customFont = tempFont;

                        GuiSetFont(customFont);
                        inputFontFileLoaded = true;
                        customFontLoaded = true;
                    }
                }

                if (result >= 0) showLoadFontFileDialog = false;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Save File Dialog (and saving logic)
            //----------------------------------------------------------------------------------------
            if (showSaveFileDialog)
            {
#if defined(CUSTOM_MODAL_DIALOGS)
                //int result = GuiFileDialog(DIALOG_TEXTINPUT, "Save raygui style file...", outFileName, "Ok;Cancel", NULL);
                int result = GuiTextInputBox((Rectangle){ screenWidth/2 - 280/2, screenHeight/2 - 112/2 - 30, 280, 112 }, "#2#Save raygui style file...", NULL, "#2#Save", outFileName, 512, NULL);
#else
                int result = GuiFileDialog(DIALOG_SAVE_FILE, "Save raygui style file...", outFileName, "*.rgs", "raygui Style Files (*.rgs)");
#endif
                if (result == 1)
                {
                    // Save file: outFileName
                    // Check for valid extension and make sure it is
                    if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".rgs")) strcat(outFileName, ".rgs\0");

                    // Save style file (text or binary)
                    SaveStyle(outFileName, STYLE_BINARY);
                    outputFileCreated = true;

                    // Set window title for future savings
                    SetWindowTitle(TextFormat("%s v%s - %s", toolName, toolVersion, GetFileName(outFileName)));

                #if defined(PLATFORM_WEB)
                    // Download file from MEMFS (emscripten memory filesystem)
                    // NOTE: Second argument must be a simple filename (we can't use directories)
                    emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", outFileName, GetFileName(outFileName)));
                #endif
                }

                if (result >= 0) showSaveFileDialog = false;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Export File Dialog (and saving logic)
            //----------------------------------------------------------------------------------------
            if (showExportFileDialog)
            {
#if defined(CUSTOM_MODAL_DIALOGS)
                //int result = GuiFileDialog(DIALOG_TEXTINPUT, "Export raygui style file...", outFileName, "Ok;Cancel", NULL);
                int result = GuiTextInputBox((Rectangle){ screenWidth/2 - 280/2, screenHeight/2 - 112/2 - 60, 280, 112 }, "#7#Export raygui style file...", NULL, "#7#Export", outFileName, 512, NULL);
#else
                // Consider different supported file types
                char filters[64] = { 0 };
                strcpy(outFileName, TextToLower(styleNameText));

                switch (exportFormatActive)
                {
                    //case STYLE_TEXT: strcpy(filters, "*.rgs"); strcat(outFileName, ".rgs"); break;
                    case STYLE_BINARY: strcpy(filters, "*.rgs"); strcat(outFileName, ".rgs"); break;
                    case STYLE_AS_CODE: strcpy(filters, "*.h"); strcat(outFileName, ".h"); break;
                    case STYLE_TABLE_IMAGE: strcpy(filters, "*.png"); strcat(outFileName, ".png");break;
                    default: break;
                }

                int result = GuiFileDialog(DIALOG_SAVE_FILE, "Export raygui style file...", outFileName, filters, TextFormat("File type (%s)", filters));
#endif
                if (result == 1)
                {
                    // Export file: outFileName
                    switch (exportFormatActive)
                    {
                        /*
                        case STYLE_TEXT:
                        {
                            // Check for valid extension and make sure it is
                            if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".rgs")) strcat(outFileName, ".rgs\0");
                            SaveStyle(outFileName, STYLE_TEXT);
                        } break;
                        */
                        case STYLE_BINARY:
                        {
                            // Check for valid extension and make sure it is
                            if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".rgs")) strcat(outFileName, ".rgs\0");
                            SaveStyle(outFileName, STYLE_BINARY);
                            outputFileCreated = true;

                        } break;
                        case STYLE_AS_CODE:
                        {
                            // Check for valid extension and make sure it is
                            if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".h")) strcat(outFileName, ".h\0");
                            ExportStyleAsCode(outFileName, styleNameText);
                        } break;
                        case STYLE_TABLE_IMAGE:
                        {
                            // Check for valid extension and make sure it is
                            if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".png")) strcat(outFileName, ".png\0");
                            Image imStyleTable = GenImageStyleControlsTable(styleNameText);
                            ExportImage(imStyleTable, outFileName);
                            UnloadImage(imStyleTable);

                            // Write a custom chunk - rGSf (rGuiStyler file)
                            if (styleChunkChecked)
                            {
                                rpng_chunk chunk = { 0 };
                                memcpy(chunk.type, "rGSf", 4);  // Chunk type FOURCC
                                chunk.data = SaveStyleToMemory(&chunk.length);
                                rpng_chunk_write(outFileName, chunk);
                                RPNG_FREE(chunk.data);
                            }

                        } break;
                        default: break;
                    }

                #if defined(PLATFORM_WEB)
                    // Download file from MEMFS (emscripten memory filesystem)
                    // NOTE: Second argument must be a simple filename (we can't use directories)
                    emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", outFileName, GetFileName(outFileName)));
                #endif
                }

                if (result >= 0) showExportFileDialog = false;
            }
            //----------------------------------------------------------------------------------------

        EndTextureMode();

        BeginDrawing();
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

            // Draw render texture to screen (scaled if required)
            if (screenSizeActive) DrawTexturePro(screenTarget.texture, (Rectangle){ 0, 0, (float)screenTarget.texture.width, -(float)screenTarget.texture.height }, (Rectangle){ 0, 0, (float)screenTarget.texture.width*2, (float)screenTarget.texture.height*2 }, (Vector2){ 0, 0 }, 0.0f, WHITE);
            else DrawTextureRec(screenTarget.texture, (Rectangle){ 0, 0, (float)screenTarget.texture.width, -(float)screenTarget.texture.height }, (Vector2){ 0, 0 }, WHITE);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadFont(customFont);     // Unload font data

    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------------
// Module functions definition
//--------------------------------------------------------------------------------------------
#if defined(PLATFORM_DESKTOP)
// Show command line usage info
static void ShowCommandLineInfo(void)
{
    printf("\n//////////////////////////////////////////////////////////////////////////////////\n");
    printf("//                                                                              //\n");
    printf("// %s v%s - %s             //\n", toolName, toolVersion, toolDescription);
    printf("// powered by raylib v%s and raygui v%s                             //\n", RAYLIB_VERSION, RAYGUI_VERSION);
    printf("// more info and bugs-report: github.com/raylibtech/rtools                      //\n");
    printf("// feedback and support:      ray[at]raylibtech.com                             //\n");
    printf("//                                                                              //\n");
    printf("// Copyright (c) 2017-2023 raylib technologies (@raylibtech)                    //\n");
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

    char inFileName[512] = { 0 };       // Input file name
    char outFileName[512] = { 0 };      // Output file name
    int outputFormat = STYLE_BINARY;    // Formats: STYLE_BINARY, STYLE_AS_CODE, STYLE_TABLE_IMAGE

    // Process command line arguments
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
                else LOG("WARNING: Input file extension not recognized\n");

                i++;
            }
            else LOG("WARNING: No input file provided\n");
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
                else LOG("WARNING: Output file extension not recognized\n");

                i++;
            }
            else LOG("WARNING: No output file provided\n");
        }
        else if ((strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--format") == 0))
        {
            // Check for valid argumment and valid parameters
            if (((i + 1) < argc) && (argv[i + 1][0] != '-'))
            {
                int format = TextToInteger(argv[i + 1]);

                if ((format >= 0) && (format <= 3)) outputFormat = format;

                i++;
            }
            else LOG("WARNING: Format parameters provided not valid\n");
        }
    }

    if (inFileName[0] != '\0')
    {
        // Set a default name for output in case not provided
        if (outFileName[0] == '\0') strcpy(outFileName, "output");

        LOG("\nInput file:       %s", inFileName);
        LOG("\nOutput file:      %s", outFileName);

        // Process input .rgs file
        GuiLoadStyle(inFileName);

        // Export style files with different formats
        switch (outputFormat)
        {
            case STYLE_TEXT: SaveStyle(TextFormat("%s%s", outFileName, ".rgs"), outputFormat); break;
            case STYLE_BINARY: SaveStyle(TextFormat("%s%s", outFileName, ".rgs"), outputFormat); break;
            case STYLE_AS_CODE: ExportStyleAsCode(TextFormat("%s%s", outFileName, ".h"), GetFileNameWithoutExt(outFileName)); break;
            case STYLE_TABLE_IMAGE:
            {
                Image imStyleTable = GenImageStyleControlsTable(GetFileNameWithoutExt(outFileName));
                ExportImage(imStyleTable, TextFormat("%s%s", outFileName, ".png"));
                UnloadImage(imStyleTable);
            } break;
            default: break;
        }
    }

    if (showUsageInfo) ShowCommandLineInfo();
}
#endif      // PLATFORM_DESKTOP

//--------------------------------------------------------------------------------------------
// Load/Save/Export data functions
//--------------------------------------------------------------------------------------------
// Save current style to memory data array
static unsigned char *SaveStyleToMemory(int *size)
{
    unsigned char *buffer = (unsigned char *)RL_CALLOC(1024*1024, 1);  // 1MB should be enough to save the style
    int dataSize = 0;

    char signature[5] = "rGS ";
    short version = 200;
    short reserved = 0;
    int changedPropCounter = StyleChangesCounter(defaultStyle);

    memcpy(buffer, signature, 4);
    memcpy(buffer + 4, &version, sizeof(short));
    memcpy(buffer + 6, &reserved, sizeof(short));
    memcpy(buffer + 8, &changedPropCounter, sizeof(int));
    dataSize += 12;

    short controlId = 0;
    short propertyId = 0;
    int propertyValue = 0;

    // Save first all properties that have changed in DEFAULT style
    for (int i = 0; i < (RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED); i++)
    {
        if (defaultStyle[i] != GuiGetStyle(0, i))
        {
            propertyId = (short)i;
            propertyValue = GuiGetStyle(0, i);

            memcpy(buffer + dataSize, &controlId, sizeof(short));
            memcpy(buffer + dataSize + 2, &propertyId, sizeof(short));
            memcpy(buffer + dataSize + 4, &propertyValue, sizeof(int));
            dataSize += 8;
        }
    }

    // Save all properties that have changed in comparison to DEFAULT style
    for (int i = 1; i < RAYGUI_MAX_CONTROLS; i++)
    {
        for (int j = 0; j < RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED; j++)
        {
            if ((defaultStyle[i*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED) + j] != GuiGetStyle(i, j)) && (GuiGetStyle(i, j) !=  GuiGetStyle(0, j)))
            {
                controlId = (short)i;
                propertyId = (short)j;
                propertyValue = GuiGetStyle(i, j);

                memcpy(buffer + dataSize, &controlId, sizeof(short));
                memcpy(buffer + dataSize + 2, &propertyId, sizeof(short));
                memcpy(buffer + dataSize + 4, &propertyValue, sizeof(int));
                dataSize += 8;
            }
        }
    }

    int fontSize = 0;

    // Write font data (embedding)
    if (customFontLoaded)
    {
        Image imFont = LoadImageFromTexture(customFont.texture);

        // Write font parameters
        int fontParamsSize = 32;
        int fontImageUncompSize = GetPixelDataSize(imFont.width, imFont.height, imFont.format);
        int fontImageCompSize = fontImageUncompSize;
        int fontGlyphDataSize = customFont.glyphCount*32;       // 32 bytes by char
        int fontDataSize = fontParamsSize + fontImageUncompSize + fontGlyphDataSize;
        int fontType = 0;       // 0-NORMAL, 1-SDF

#if defined(SUPPORT_COMPRESSED_FONT_ATLAS)
        // NOTE: If data is compressed using raylib CompressData() DEFLATE,
        // it requires to be decompressed with raylib DecompressData(), that requires
        // compiling raylib with SUPPORT_COMPRESSION_API config flag enabled

        // Make sure font atlas image data is GRAY + ALPHA for better compression
        if (imFont.format != PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA)
        {
            ImageFormat(&imFont, PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA);
            fontImageUncompSize = GetPixelDataSize(imFont.width, imFont.height, imFont.format);
        }

        // Compress font atlas image data
        unsigned char *compData = CompressData(imFont.data, fontImageUncompSize, &fontImageCompSize);

        // NOTE: Actually, fontDataSize is only used to check that there is font data included in the file
        fontDataSize = fontParamsSize + fontImageCompSize + fontGlyphDataSize;
#endif
        memcpy(buffer + dataSize, &fontDataSize, sizeof(int));
        memcpy(buffer + dataSize + 4, &customFont.baseSize, sizeof(int));
        memcpy(buffer + dataSize + 8, &customFont.glyphCount, sizeof(int));
        memcpy(buffer + dataSize + 12, &fontType, sizeof(int));

        // TODO: Define font white rectangle
        Rectangle rec = { 0 };
        memcpy(buffer + dataSize + 16, &rec, sizeof(Rectangle));
        dataSize += (16 + sizeof(Rectangle));

        // Write font image parameters
        memcpy(buffer + dataSize, &fontImageUncompSize, sizeof(int));
        memcpy(buffer + dataSize + 4, &fontImageCompSize, sizeof(int));
        memcpy(buffer + dataSize + 8, &imFont.width, sizeof(int));
        memcpy(buffer + dataSize + 12, &imFont.height, sizeof(int));
        memcpy(buffer + dataSize + 16, &imFont.format, sizeof(int));
#if defined(SUPPORT_COMPRESSED_FONT_ATLAS)
        memcpy(buffer + dataSize + 20, compData, fontImageCompSize);
        dataSize += (20 + fontImageCompSize);
        MemFree(compData);
#else
        memcpy(buffer + dataSize, imFont.data, fontImageUncompSize);
        dataSize += (20 + fontImageUncompSize);
#endif
        UnloadImage(imFont);

        // Write font recs data
        for (int i = 0; i < customFont.glyphCount; i++)
        {
            memcpy(buffer + dataSize, &customFont.recs[i], sizeof(Rectangle));
            dataSize += sizeof(Rectangle);
        }

        // Write font chars info data
        for (int i = 0; i < customFont.glyphCount; i++)
        {
            memcpy(buffer + dataSize, &customFont.glyphs[i].value, sizeof(int));
            memcpy(buffer + dataSize + 4, &customFont.glyphs[i].offsetX, sizeof(int));
            memcpy(buffer + dataSize + 8, &customFont.glyphs[i].offsetY, sizeof(int));
            memcpy(buffer + dataSize + 12, &customFont.glyphs[i].advanceX, sizeof(int));
            dataSize += 16;
        }
    }
    else
    {
        memcpy(buffer + dataSize, &fontSize, sizeof(int));
        dataSize += 4;
    }

    *size = dataSize;
    return buffer;
}

// Save raygui style binary file (.rgs)
// NOTE: By default style is saved as binary file but
// a text style mode is also available for debug (no font embedding)
static bool SaveStyle(const char *fileName, int format)
{
    int success = false;

    FILE *rgsFile = NULL;

    if (format == STYLE_BINARY)
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
            // 8       | 4       | int        | Num properties (only changed ones from default style)

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
            // 24+4*N  | 4       | int        | Font glyph count [glyphCount]
            // 28+4*N  | 4       | int        | Font type (0-NORMAL, 1-SDF)
            // 32+4*N  | 16      | Rectangle  | Font white rectangle

            // Custom Font Data : Image (20 bytes + imSize)
            // NOTE: Font image atlas is always converted to GRAY+ALPHA
            // and atlas image data can be compressed (DEFLATE)
            // 48+4*N  | 4       | int        | Image data size (uncompressed)
            // 52+4*N  | 4       | int        | Image data size (compressed)
            // 56+4*N  | 4       | int        | Image width
            // 60+4*N  | 4       | int        | Image height
            // 64+4*N  | 4       | int        | Image format
            // 68+4*N  | imSize  | *          | Image data (comp or uncomp)

            // Custom Font Data : Recs (32 bytes*glyphCount)
            // foreach (glyph)
            // {
            //   ...   | 16      | Rectangle  | Glyph rectangle (in image)
            // }

            // Custom Font Data : Glyph Info (32 bytes*glyphCount)
            // foreach (glyph)
            // {
            //   ...   | 4       | int        | Glyph value
            //   ...   | 4       | int        | Glyph offset X
            //   ...   | 4       | int        | Glyph offset Y
            //   ...   | 4       | int        | Glyph advance X
            // }
            // ------------------------------------------------------

            char signature[5] = "rGS ";
            short version = 200;
            short reserved = 0;

            fwrite(signature, 1, 4, rgsFile);
            fwrite(&version, sizeof(short), 1, rgsFile);
            fwrite(&reserved, sizeof(short), 1, rgsFile);

            int changedPropCounter = StyleChangesCounter(defaultStyle);

            fwrite(&changedPropCounter, sizeof(int), 1, rgsFile);

            short controlId = 0;
            short propertyId = 0;
            int propertyValue = 0;

            // Save first all properties that have changed in DEFAULT style
            for (int i = 0; i < (RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED); i++)
            {
                if (defaultStyle[i] != GuiGetStyle(0, i))
                {
                    propertyId = (short)i;
                    propertyValue = GuiGetStyle(0, i);

                    fwrite(&controlId, sizeof(short), 1, rgsFile);
                    fwrite(&propertyId, sizeof(short), 1, rgsFile);
                    fwrite(&propertyValue, sizeof(int), 1, rgsFile);
                }
            }

            // Save all properties that have changed in comparison to DEFAULT style
            for (int i = 1; i < RAYGUI_MAX_CONTROLS; i++)
            {
                for (int j = 0; j < RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED; j++)
                {
                    if ((defaultStyle[i*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED) + j] != GuiGetStyle(i, j)) && (GuiGetStyle(i, j) !=  GuiGetStyle(0, j)))
                    {
                        controlId = (short)i;
                        propertyId = (short)j;
                        propertyValue = GuiGetStyle(i, j);

                        fwrite(&controlId, sizeof(short), 1, rgsFile);
                        fwrite(&propertyId, sizeof(short), 1, rgsFile);
                        fwrite(&propertyValue, sizeof(int), 1, rgsFile);
                    }
                }
            }

            int fontSize = 0;

            // Write font data (embedding)
            if (customFontLoaded)
            {
                Image imFont = LoadImageFromTexture(customFont.texture);

                // Write font parameters
                int fontParamsSize = 32;
                int fontImageUncompSize = GetPixelDataSize(imFont.width, imFont.height, imFont.format);
                int fontImageCompSize = fontImageUncompSize;
                int fontGlyphDataSize = customFont.glyphCount*32;       // 32 bytes by char
                int fontDataSize = fontParamsSize + fontImageUncompSize + fontGlyphDataSize;
                int fontType = 0;       // 0-NORMAL, 1-SDF

#if defined(SUPPORT_COMPRESSED_FONT_ATLAS)
                // NOTE: If data is compressed using raylib CompressData() DEFLATE,
                // it requires to be decompressed with raylib DecompressData(), that requires
                // compiling raylib with SUPPORT_COMPRESSION_API config flag enabled

                // Make sure font atlas image data is GRAY + ALPHA for better compression
                if (imFont.format != PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA)
                {
                    ImageFormat(&imFont, PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA);
                    fontImageUncompSize = GetPixelDataSize(imFont.width, imFont.height, imFont.format);
                }

                // Compress font atlas image data
                unsigned char *compData = CompressData(imFont.data, fontImageUncompSize, &fontImageCompSize);

                // NOTE: Actually, fontDataSize is only used to check that there is font data included in the file
                fontDataSize = fontParamsSize + fontImageCompSize + fontGlyphDataSize;
#endif
                fwrite(&fontDataSize, sizeof(int), 1, rgsFile);
                fwrite(&customFont.baseSize, sizeof(int), 1, rgsFile);
                fwrite(&customFont.glyphCount, sizeof(int), 1, rgsFile);
                fwrite(&fontType, sizeof(int), 1, rgsFile);

                // TODO: Define font white rectangle
                Rectangle rec = { 0 };
                fwrite(&rec, sizeof(Rectangle), 1, rgsFile);

                // Write font image parameters
                fwrite(&fontImageUncompSize, sizeof(int), 1, rgsFile);
                fwrite(&fontImageCompSize, sizeof(int), 1, rgsFile);
                fwrite(&imFont.width, sizeof(int), 1, rgsFile);
                fwrite(&imFont.height, sizeof(int), 1, rgsFile);
                fwrite(&imFont.format, sizeof(int), 1, rgsFile);
#if defined(SUPPORT_COMPRESSED_FONT_ATLAS)
                fwrite(compData, 1, fontImageCompSize, rgsFile);
                MemFree(compData);
#else
                fwrite(imFont.data, 1, fontImageUncompSize, rgsFile);
#endif
                UnloadImage(imFont);

                // Write font recs data
                for (int i = 0; i < customFont.glyphCount; i++) fwrite(&customFont.recs[i], sizeof(Rectangle), 1, rgsFile);

                // Write font chars info data
                for (int i = 0; i < customFont.glyphCount; i++)
                {
                    fwrite(&customFont.glyphs[i].value, sizeof(int), 1, rgsFile);
                    fwrite(&customFont.glyphs[i].offsetX, sizeof(int), 1, rgsFile);
                    fwrite(&customFont.glyphs[i].offsetY, sizeof(int), 1, rgsFile);
                    fwrite(&customFont.glyphs[i].advanceX, sizeof(int), 1, rgsFile);
                }
            }
            else fwrite(&fontSize, sizeof(int), 1, rgsFile);

            fclose(rgsFile);
            success = true;
        }
    }
    else if (format == STYLE_TEXT)
    {
        rgsFile = fopen(fileName, "wt");

        if (rgsFile != NULL)
        {
            #define RGS_FILE_VERSION_TEXT  "4.0"

            // Write some description comments
            fprintf(rgsFile, "#\n# rgs style text file (v%s) - raygui style file generated using rGuiStyler\n#\n", RGS_FILE_VERSION_TEXT);
            fprintf(rgsFile, "# Info:  p <controlId> <propertyId> <propertyValue>  // Property description\n#\n");

            if (customFontLoaded)
            {
                fprintf(rgsFile, "# WARNING: This style uses a custom font, must be provided with style file\n");
                fprintf(rgsFile, "f %i %i %s\n", GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING), GetFileName(inFontFileName));
            }

            // Save DEFAULT properties that changed
            for (int j = 0; j < (RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED); j++)
            {
                if (defaultStyle[j] != GuiGetStyle(0, j))
                {
                    // NOTE: Control properties are written as hexadecimal values, extended properties names not provided
                    fprintf(rgsFile, "p 00 %02i 0x%08x    DEFAULT_%s \n", j, GuiGetStyle(0, j), (j < RAYGUI_MAX_PROPS_BASE)? guiPropsText[j] : TextFormat("EXT%02i", (j - RAYGUI_MAX_PROPS_BASE)));
                }
            }

            // Save other controls properties that changed
            for (int i = 1; i < RAYGUI_MAX_CONTROLS; i++)
            {
                for (int j = 0; j < (RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED); j++)
                {
                    // Check all properties that have changed in comparison to default style and add custom style sets for those properties
                    if ((defaultStyle[i*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED) + j] != GuiGetStyle(i, j)) && (GuiGetStyle(i, j) !=  GuiGetStyle(0, j)))
                    {
                        // NOTE: Control properties are written as hexadecimal values, extended properties names not provided
                        fprintf(rgsFile, "p %02i %02i 0x%08x    %s_%s \n", i, j, GuiGetStyle(i, j), guiControlText[i], (j < RAYGUI_MAX_PROPS_BASE)? guiPropsText[j] : TextFormat("EXT%02i", (j - RAYGUI_MAX_PROPS_BASE)));
                    }
                }
            }

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
    // DEFAULT extended properties
    static const char *guiPropsExText[RAYGUI_MAX_PROPS_EXTENDED] = {
        "TEXT_SIZE",
        "TEXT_SPACING",
        "LINE_COLOR",
        "BACKGROUND_COLOR",
        "EXTENDED01",
        "EXTENDED02",
        "EXTENDED03",
        "EXTENDED04",
    };

    FILE *txtFile = fopen(fileName, "wt");

    if (txtFile != NULL)
    {
        fprintf(txtFile, "//////////////////////////////////////////////////////////////////////////////////\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// StyleAsCode exporter v1.2 - Style data exported as a values array            //\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// USAGE: On init call: GuiLoadStyle%s();                             //\n", TextToPascal(styleName));
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// more info and bugs-report:  github.com/raysan5/raygui                        //\n");
        fprintf(txtFile, "// feedback and support:       ray[at]raylibtech.com                            //\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// Copyright (c) 2020-2023 raylib technologies (@raylibtech)                    //\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "//////////////////////////////////////////////////////////////////////////////////\n\n");

        // Export only properties that change from default style
        fprintf(txtFile, "#define %s_STYLE_PROPS_COUNT  %i\n\n", TextToUpper(styleName), StyleChangesCounter(defaultStyle));

        // Write byte data as hexadecimal text
        fprintf(txtFile, "// Custom style name: %s\n", styleName);
        fprintf(txtFile, "static const GuiStyleProp %sStyleProps[%s_STYLE_PROPS_COUNT] = {\n", styleName, TextToUpper(styleName));

        // Write all properties that have changed in default style
        for (int i = 0; i < (RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED); i++)
        {
            if (defaultStyle[i] != GuiGetStyle(0, i))
            {
                if (i < RAYGUI_MAX_PROPS_BASE) fprintf(txtFile, "    { 0, %i, 0x%08x },    // DEFAULT_%s \n", i, GuiGetStyle(DEFAULT, i), guiPropsText[i]);
                else fprintf(txtFile, "    { 0, %i, 0x%08x },    // DEFAULT_%s \n", i, GuiGetStyle(DEFAULT, i), guiPropsExText[i - RAYGUI_MAX_PROPS_BASE]);
            }
        }

        // Add to count all properties that have changed in comparison to default style
        for (int i = 1; i < RAYGUI_MAX_CONTROLS; i++)
        {
            for (int j = 0; j < (RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED); j++)
            {
                if ((defaultStyle[i*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED) + j] != GuiGetStyle(i, j)) && (GuiGetStyle(i, j) !=  GuiGetStyle(0, j)))
                {
                    if (j < RAYGUI_MAX_PROPS_BASE) fprintf(txtFile, "    { %i, %i, 0x%08x },    // %s_%s \n", i, j, GuiGetStyle(i, j), guiControlText[i], guiPropsText[j]);
                    else fprintf(txtFile, "    { %i, %i, 0x%08x },    // %s_%s \n", i, j, GuiGetStyle(i, j), guiControlText[i], TextFormat("EXTENDED%02i", j - RAYGUI_MAX_PROPS_BASE + 1));
                }
            }
        }

        fprintf(txtFile, "};\n\n");

        if (customFontLoaded)
        {
            fprintf(txtFile, "// WARNING: This style uses a custom font: %s (size: %i, spacing: %i)\n\n",
                    GetFileName(inFontFileName), GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING));
        }

        Image imFont = { 0 };

        if (customFontLoaded)
        {
            // Support font export and initialization
            // NOTE: This mechanism is highly coupled to raylib
            imFont = LoadImageFromTexture(customFont.texture);
            if (imFont.format != PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA) LOG("WARNING: Font image format is not GRAY+ALPHA!");
            int imFontSize = GetPixelDataSize(imFont.width, imFont.height, imFont.format);

            #define BYTES_TEXT_PER_LINE     20

#if defined(SUPPORT_COMPRESSED_FONT_ATLAS)
            // NOTE: If data is compressed using raylib CompressData() DEFLATE,
            // it requires to be decompressed with raylib DecompressData(), that requires
            // compiling raylib with SUPPORT_COMPRESSION_API config flag enabled

            // Image data is usually GRAYSCALE + ALPHA and can be reduced to GRAYSCALE
            //ImageFormat(&imFont, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);

            // Compress font image data
            int compDataSize = 0;
            unsigned char *compData = CompressData(imFont.data, imFontSize, &compDataSize);

            // Save font image data (compressed)
            fprintf(txtFile, "#define %s_COMPRESSED_DATA_SIZE %i\n\n", TextToUpper(styleName), compDataSize);
            fprintf(txtFile, "// Font image pixels data compressed (DEFLATE)\n");
            fprintf(txtFile, "// NOTE: Original pixel data simplified to GRAYSCALE\n");
            fprintf(txtFile, "static unsigned char %sFontData[%s_COMPRESSED_DATA_SIZE] = { ", styleName, TextToUpper(styleName));
            for (int i = 0; i < compDataSize - 1; i++) fprintf(txtFile, ((i%BYTES_TEXT_PER_LINE == 0)? "0x%02x,\n    " : "0x%02x, "), compData[i]);
            fprintf(txtFile, "0x%02x };\n\n", compData[compDataSize - 1]);
            MemFree(compData);
#else
            // Save font image data (uncompressed)
            fprintf(txtFile, "// Font image pixels data\n");
            fprintf(txtFile, "// NOTE: 2 bytes per pixel, GRAY + ALPHA channels\n");
            fprintf(txtFile, "static unsigned char %sFontImageData[%i] = { ", styleName, imFontSize);
            for (int i = 0; i < imFontSize - 1; i++) fprintf(txtFile, ((i%BYTES_TEXT_PER_LINE == 0)? "0x%02x,\n    " : "0x%02x, "), ((unsigned char *)imFont.data)[i]);
            fprintf(txtFile, "0x%02x };\n\n", ((unsigned char *)imFont.data)[imFontSize - 1]);
#endif
            // Save font recs data
            fprintf(txtFile, "// Font characters rectangles data\n");
            fprintf(txtFile, "static const Rectangle %sFontRecs[%i] = {\n", styleName, customFont.glyphCount);
            for (int i = 0; i < customFont.glyphCount; i++)
            {
                fprintf(txtFile, "    { %1.0f, %1.0f, %1.0f , %1.0f },\n", customFont.recs[i].x, customFont.recs[i].y, customFont.recs[i].width, customFont.recs[i].height);
            }
            fprintf(txtFile, "};\n\n");

            // Save font glyphs data
            // NOTE: Glyphs image data not saved (grayscale pixels),
            // it could be generated from image and recs
            fprintf(txtFile, "// Font glyphs info data\n");
            fprintf(txtFile, "// NOTE: No glyphs.image data provided\n");
            fprintf(txtFile, "static const GlyphInfo %sFontChars[%i] = {\n", styleName, customFont.glyphCount);
            for (int i = 0; i < customFont.glyphCount; i++)
            {
                fprintf(txtFile, "    { %i, %i, %i, %i, { 0 }},\n", customFont.glyphs[i].value, customFont.glyphs[i].offsetX, customFont.glyphs[i].offsetY, customFont.glyphs[i].advanceX);
            }
            fprintf(txtFile, "};\n\n");

            UnloadImage(imFont);
        }

        fprintf(txtFile, "// Style loading function: %s\n", styleName);
        fprintf(txtFile, "static void GuiLoadStyle%s(void)\n{\n", TextToPascal(styleName));
        fprintf(txtFile, "    // Load style properties provided\n");
        fprintf(txtFile, "    // NOTE: Default properties are propagated\n");
        fprintf(txtFile, "    for (int i = 0; i < %s_STYLE_PROPS_COUNT; i++)\n    {\n", TextToUpper(styleName));
        fprintf(txtFile, "        GuiSetStyle(%sStyleProps[i].controlId, %sStyleProps[i].propertyId, %sStyleProps[i].propertyValue);\n    }\n\n", styleName, styleName, styleName);

        if (customFontLoaded)
        {
            fprintf(txtFile, "    // Custom font loading\n");
#if defined(SUPPORT_COMPRESSED_FONT_ATLAS)
            fprintf(txtFile, "    // NOTE: Compressed font image data (DEFLATE), it requires DecompressData() function\n");
            fprintf(txtFile, "    int %sFontDataSize = 0;\n", styleName);
            fprintf(txtFile, "    unsigned char *data = DecompressData(%sFontData, %s_COMPRESSED_DATA_SIZE, &%sFontDataSize);\n", styleName, TextToUpper(styleName), styleName);
            fprintf(txtFile, "    Image imFont = { data, %i, %i, 1, %i };\n\n", imFont.width, imFont.height, imFont.format);
            //fprintf(txtFile, "    ImageFormat(&imFont, PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA);
#else
            fprintf(txtFile, "    Image imFont = { %sFontImageData, %i, %i, 1, %i };\n\n", styleName, imFont.width, imFont.height, imFont.format);
#endif
            fprintf(txtFile, "    Font font = { 0 };\n");
            fprintf(txtFile, "    font.baseSize = %i;\n", GuiGetStyle(DEFAULT, TEXT_SIZE));
            fprintf(txtFile, "    font.glyphCount = %i;\n\n", customFont.glyphCount);

            fprintf(txtFile, "    // Load texture from image\n");
            fprintf(txtFile, "    font.texture = LoadTextureFromImage(imFont);\n");
#if defined(SUPPORT_COMPRESSED_FONT_ATLAS)
            fprintf(txtFile, "    UnloadImage(imFont);  // Uncompressed data can be unloaded from memory\n\n");
#endif

            fprintf(txtFile, "    // Copy char recs data from global fontRecs\n");
            fprintf(txtFile, "    // NOTE: Required to avoid issues if trying to free font\n");
            fprintf(txtFile, "    font.recs = (Rectangle *)malloc(font.glyphCount*sizeof(Rectangle));\n");
            fprintf(txtFile, "    memcpy(font.recs, %sFontRecs, font.glyphCount*sizeof(Rectangle));\n\n", styleName);

            fprintf(txtFile, "    // Copy font char info data from global fontChars\n");
            fprintf(txtFile, "    // NOTE: Required to avoid issues if trying to free font\n");
            fprintf(txtFile, "    font.glyphs = (GlyphInfo *)malloc(font.glyphCount*sizeof(GlyphInfo));\n");
            fprintf(txtFile, "    memcpy(font.glyphs, %sFontChars, font.glyphCount*sizeof(GlyphInfo));\n\n", styleName);

            fprintf(txtFile, "    GuiSetFont(font);\n\n");

            fprintf(txtFile, "    // TODO: Setup a white rectangle on the font to be used on shapes drawing,\n");
            fprintf(txtFile, "    // this way we make sure all gui can be drawn on a single pass because no texture change is required\n");
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

    enum TableControlType {
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
    };

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
        DrawText(TextFormat("%s", styleName), TABLE_LEFT_PADDING + MeasureText("raygui style:  ", 10), 20, 10, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));

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

        GuiSetState(STATE_NORMAL);

        int offsetWidth = TABLE_LEFT_PADDING + tableStateNameWidth;

        // Draw basic controls
        for (int i = 0; i < TABLE_CONTROLS_COUNT; i++)
        {
            rec = (Rectangle){ offsetWidth - i - 1, TABLE_TOP_PADDING + 20, (controlWidth[i] + TABLE_CELL_PADDING*2), TABLE_CELL_HEIGHT/2 + 1 };

            // Draw grid lines: control name
            GuiGroupBox(rec, NULL);
            int labelTextAlignment = GuiGetStyle(LABEL, TEXT_ALIGNMENT);
            GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
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
                GuiSetState(STATE_NORMAL);

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

    Image imStyleTable = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&imStyleTable);

    UnloadRenderTexture(target);

    return imStyleTable;
}

//--------------------------------------------------------------------------------------------
// Auxiliar GUI functions
//--------------------------------------------------------------------------------------------

// Count changed properties in current style (raygui internal guiStyle) vs refStyle
// WARNING: refStyle must be a valid raygui style data array (expected size)
static int StyleChangesCounter(unsigned int *refStyle)
{
    int changes = 0;

    // Count all properties that have changed from reference style
    for (int i = 0; i < (RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED); i++) if (refStyle[i] != GuiGetStyle(0, i)) changes++;

    // Add to count all properties that have changed in comparison to default style
    for (int i = 1; i < RAYGUI_MAX_CONTROLS; i++)
    {
        for (int j = 0; j < (RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED); j++)
        {
            if ((refStyle[i*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED) + j] != GuiGetStyle(i, j)) && (GuiGetStyle(i, j) !=  GuiGetStyle(0, j))) changes++;
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
