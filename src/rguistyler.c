/*******************************************************************************************
*
*   rGuiStyler v5.1 - A simple and easy-to-use raygui styles editor
*
*   FEATURES:
*       - Global and control specific styles edition
*       - Style preview in real time with individual test controls
*       - Style templates available to start customizing new styles
*       - Selectable controls state: NORMAL, FOCUSED, PRESSED, DISABLED
*       - Save and load as binary style file .rgs (font embedded!)
*       - Export style as an embeddable code file (.h) (font embedded!)
*       - Export style as a .png controls table image for showcase
*       - Embed style as custom rGSf png chunk (rgs file data)
*       - Import, configure and preview style fonts (.ttf/.otf)
*       - Color palette for quick color save/selection
*       - 12 custom style examples included
*
*   LIMITATIONS:
*       - Limitation 01
*       - Limitation 02
*
*   POSSIBLE IMPROVEMENTS:
*       - Improvement 01
*
*   CONFIGURATION:
*       #define CUSTOM_MODAL_DIALOGS
*           Use custom raygui generated modal dialogs instead of native OS ones
*           NOTE: Avoids including tinyfiledialogs depencency library
*
*       #define SUPPORT_COMPRESSED_FONT_ATLAS
*           Export font atlas image data compressed using raylib CompressData() DEFLATE algorythm,
*           NOTE: It requires to be decompressed with raylib DecompressData(),
*           that requires compiling raylib with SUPPORT_COMPRESSION_API config flag enabled
*
*   VERSIONS HISTORY:
*       5.1  (06-Apr-2024)  ADDED: Issue report window
*                           REMOVED: Sponsors window
*                           REVIEWED: Main toolbar and help window
*                           UPDATED: Using raylib 5.1-dev and raygui 4.1-dev
*
*       5.0  (20-Sep-2023)  ADDED: Support macOS builds (x86_64 + arm64)
*                           ADDED: New font atlas generation window
*                           ADDED: Shapes white rectangle definition visually
*                           ADDED: Support for custom font codepoints (Unicode)
*                           ADDED: Style table movement controls
*                           ADDED: Style name input in status bar
*                           REVIEWED: Style table controls exposed
*                           REVIEWED: Regenerated tool imagery
*                           REVIEWED: Disabled sponsors window at launch
*                           UPDATED: Using raygui 4.0 and latest raylib 4.6-dev
*
*       4.2  (13-Dec-2022)  ADDED: Welcome window with sponsors info
*                           REDESIGNED: Main toolbar to add tooltips
*                           REVIEWED: Help window implementation
*
*       4.1  (10-Oct-2022)  ADDED: Sponsor window for tools support
*                           ADDED: Random style generator button (experimental)
*                           UPDATED: Using raylib 4.5-dev and raygui 3.5-dev
*
*       4.0  (02-Oct-2022)  ADDED: Main toolbar, for consistency with other tools
*                           ADDED: Multiple new styles as templates
*                           ADDED: Export style window with new options
*                           REVIEWED: Layout metrics
*                           UPDATED: Using raylib 4.2 and raygui 3.2
*                           Source code re-licensed to open-source
*
*       3.5  (29-Dec-2021)  UPDATED: Using raylib 4.0 and raygui 3.1
*
*   DEPENDENCIES:
*       raylib 5.5-dev          - Windowing/input management and drawing
*       raygui 4.5-dev          - Immediate-mode GUI controls with custom styling and icons
*       rpng 1.1                - PNG chunks management
*       tinyfiledialogs 3.18    - Open/save file dialogs, it requires linkage with comdlg32 and ole32 libs
*
*   BUILDING:
*     - Windows (MinGW-w64):
*       gcc -o rguistyler.exe rguistyler.c external/tinyfiledialogs.c -s -O2 -std=c99 -DPLATFORM_DESKTOP
*           -lraylib -lopengl32 -lgdi32 -lcomdlg32 -lole32
*
*     - Linux (GCC):
*       gcc -o rguistyler rguistyler.c external/tinyfiledialogs.c -s -no-pie -D_DEFAULT_SOURCE /
*           -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
*
*   ADDITIONAL NOTES:
*       On PLATFORM_ANDROID and PLATFORM_WEB system file dialogs are not available
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
*   Copyright (c) 2015-2025 raylib technologies (@raylibtech) / Ramon Santamaria (@raysan5)
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
#define TOOL_VERSION            "5.1"
#define TOOL_DESCRIPTION        "A simple and easy-to-use raygui styles editor"
#define TOOL_DESCRIPTION_BREAK  "A simple and easy-to-use raygui\nstyles editor"
#define TOOL_RELEASE_DATE       "Apr.2024"
#define TOOL_LOGO_COLOR         0x62bde3ff

#define SUPPORT_COMPRESSED_FONT_ATLAS

#include "raylib.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
    #include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

#define RAYGUI_IMPLEMENTATION
#include "external/raygui.h"                // Required for: IMGUI controls

#undef RAYGUI_IMPLEMENTATION                // Avoid including raygui implementation again

#define GUI_MAIN_TOOLBAR_IMPLEMENTATION
#include "gui_main_toolbar.h"               // GUI: Main toolbar

#define GUI_WINDOW_FONT_ATLAS_IMPLEMENTATION
#include "gui_window_font_atlas.h"          // GUI: Window font atlas

#define GUI_WINDOW_HELP_IMPLEMENTATION
#include "gui_window_help.h"                // GUI: Help Window

#define GUI_WINDOW_ABOUT_IMPLEMENTATION
#include "gui_window_about.h"               // GUI: About Window

#define GUI_FILE_DIALOGS_IMPLEMENTATION
#include "gui_file_dialogs.h"               // GUI: File Dialogs

// raygui embedded styles (used as templates)
// NOTE: Included in the same order as selector
#define MAX_GUI_STYLES_AVAILABLE    14      // WARNING: Required for styleNames[]
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
#include "styles/style_amber.h"             // raygui style: amber
#include "styles/style_rltech.h"            // raygui style: rltech

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
static char *guiControlText[RAYGUI_MAX_CONTROLS] = {
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
    "CONTROL11",
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

static const char *guiPropsDefaultExtendedText[8] = {
    "TEXT_SIZE",
    "TEXT_SPACING",
    "LINE_COLOR",
    "BACKGROUND_COLOR",
    "TEXT_LINE_SPACING",
    "TEXT_ALIGNMENT_VERTICAL",
    "TEXT_WRAP_MODE",
    "EXT08"
};

// Style template names
static const char *styleNames[MAX_GUI_STYLES_AVAILABLE] = {
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
    "Enefete",
    "Amber",
    "RLTech"
};

// Default style backup to check changed properties
static unsigned int defaultStyle[RAYGUI_MAX_CONTROLS*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)] = { 0 };

// Current active style template
static unsigned int currentStyle[RAYGUI_MAX_CONTROLS*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)] = { 0 };

static bool fontEmbeddedChecked = true;         // Select to embed font into style file
static bool fontDataCompressedChecked = true;   // Export font data compressed (recs and glyphs)

static Rectangle fontWhiteRec = { 0 };          // Font white rectangle, required to be updated from window font atlas

static char currentStyleName[64] = { 0 };       // Current style name

// NOTE: Max length depends on OS, in Windows MAX_PATH = 256
static char inFileName[512] = { 0 };            // Input file name (required in case of drag & drop over executable)
static char outFileName[512] = { 0 };           // Output file name (required for file save/export)

static bool inputFileLoaded = false;            // Flag to detect an input file has been loaded (required for fast save)
static bool outputFileCreated = false;          // Flag to detect if an output file has been created (required for fast save)

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
#if defined(PLATFORM_DESKTOP)
static void ShowCommandLineInfo(void);                      // Show command line usage info
static void ProcessCommandLine(int argc, char *argv[]);     // Process command line input
#endif

// Load/Save/Export data functions
static int SaveStyle(const char *fileName, int format);     // Save style binary file binary (.rgs)
static char *SaveStyleToMemory(int *size);                  // Save style to memory buffer
static void ExportStyleAsCode(const char *fileName, const char *styleName); // Export gui style as properties array

static void DrawStyleControlsTable(int posX, int posY);     // Draw style controls table
static Image GenImageStyleControlsTable(int width, int height, const char *styleName); // Generate controls table image

// Auxiliar functions
static int StyleChangesCounter(unsigned int *refStyle);     // Count changed properties in current style (comparing to ref style)
static Color GuiColorBox(Rectangle bounds, Color *colorPicker, Color color);    // Gui color box

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
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
    const int screenHeight = 610 + 200;

    InitWindow(screenWidth, screenHeight, TextFormat("%s v%s | %s", toolName, toolVersion, toolDescription));
    //EnableEventWaiting();
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
        SetWindowTitle(TextFormat("%s v%s | File: %s", toolName, toolVersion, GetFileName(inFileName)));
        inputFileLoaded = true;
        strcpy(currentStyleName, GetFileNameWithoutExt(inFileName));
    }
    else
    {
        GuiLoadStyleDefault();
        customFont = GetFontDefault();
        strcpy(currentStyleName, "Light");
    }

    // Default light style + current style backups (used to track changes)
    memcpy(defaultStyle, guiStyle, RAYGUI_MAX_CONTROLS*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)*sizeof(int));
    memcpy(currentStyle, guiStyle, RAYGUI_MAX_CONTROLS*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)*sizeof(int));

    // Init color picker saved colors
    Color colorBoxValue[12] = { 0 };
    for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));
    Vector3 colorHSV = { 0.0f, 0.0f, 0.0f };

    // Style table variables
    Rectangle styleTableRec = { 0, 0, 1920, 256 };
    float styleTableOffsetX = 0.0f;
    float prevStyleTablePositionX = 0.0f;
    bool styleTablePanningMode = false;

    // Style required variables
    bool saveChangesRequired = false; // Flag to notice save changes are required

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

    // GUI: Font Atlas Window
    //-----------------------------------------------------------------------------------
    GuiWindowFontAtlasState windowFontAtlasState = InitGuiWindowFontAtlas();
    int fontDrawSizeValue = windowFontAtlasState.fontGenSizeValue;
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
    bool showIssueReportWindow = false;
    //-----------------------------------------------------------------------------------

    // GUI: Export Window
    //-----------------------------------------------------------------------------------
    bool showExportWindow = false;

    int exportFormatActive = 0;             // ComboBox file type selection
    //char styleNameText[128] = "Unnamed";    // Style name text box
    bool styleNameEditMode = false;         // Style name text box edit mode
    bool styleChunkChecked = true;          // Select to embed style as a PNG chunk (rGSf)
    //-----------------------------------------------------------------------------------

    // GUI: Exit Window
    //-----------------------------------------------------------------------------------
    bool closeWindow = false;
    bool showExitWindow = false;
    //-----------------------------------------------------------------------------------

    // GUI: Custom file dialogs
    //-----------------------------------------------------------------------------------
    bool showLoadStyleDialog = false;
    bool showSaveStyleDialog = false;
    bool showExportStyleDialog = false;

    bool showLoadFontDialog = false;
    bool showLoadCharsetDialog = false;
    //bool showFontAtlasWindow = false;
    bool showSaveFontAtlasDialog = false;
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
        if (WindowShouldClose()) showExitWindow = true;

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
                SetWindowTitle(TextFormat("%s v%s | File: %s", toolName, toolVersion, GetFileName(inFileName)));
                inputFileLoaded = true;

                fontDrawSizeValue = GuiGetStyle(DEFAULT, TEXT_SIZE);
                fontSpacingValue = GuiGetStyle(DEFAULT, TEXT_SPACING);
                windowFontAtlasState.fontGenSizeValue = fontDrawSizeValue;

                // Load .rgs custom font in font
                customFont = GuiGetFont();
                memset(inFontFileName, 0, 512);
                customFontLoaded = true;

                // Reset style backup for changes
                memcpy(currentStyle, guiStyle, RAYGUI_MAX_CONTROLS *(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED));
                changedPropCounter = 0;
                saveChangesRequired = false;
            }
            else if (IsFileExtension(droppedFiles.paths[0], ".ttf;.otf"))
            {
                strcpy(inFontFileName, droppedFiles.paths[0]);
                windowFontAtlasState.fontAtlasRegen = true;
            }
            else if (IsFileExtension(droppedFiles.paths[0], ".txt"))
            {
                // Load codepoints to generate the font
                // NOTE: A UTF8 text file should be provided, it will be processed to get codepoints
                char *text = LoadFileText(droppedFiles.paths[0]);
                if (text != NULL)
                {
                    int codepointsCount = 0;
                    int *codepoints = LoadCodepoints(text, &codepointsCount);
                    UnloadFileText(text);

                    if (codepointsCount > 0)
                    {
                        // Clear current custom codepoints list
                        if (windowFontAtlasState.externalCodepointList != NULL)
                        {
                            RL_FREE(windowFontAtlasState.externalCodepointList);
                            windowFontAtlasState.externalCodepointListCount = 0;
                            windowFontAtlasState.externalCodepointList = NULL;
                        }

                        // TODO: Make sure default charset is always available?
                        //codepoints = RL_REALLOC(codepoints, (codepointsCount + 95)*sizeof(int));
                        //for (int i = 0; i < 95; i++) codepoints[codepointsCount + i] = (int)charsetBasic[i];
                        //codepointsCount += 95;

                        // Create an array to store codepoints without duplicates
                        int codepointsClearCount = codepointsCount;
                        int *codepointsClear = (int *)RL_CALLOC(codepointsCount, sizeof(int));
                        memcpy(codepointsClear, codepoints, codepointsCount*sizeof(int));

                        // Remove duplicates
                        for (int i = 0; i < codepointsClearCount; i++)
                        {
                            for (int j = i + 1; j < codepointsClearCount; j++)
                            {
                                if (codepointsClear[i] == codepointsClear[j])
                                {
                                    for (int k = j; k < codepointsClearCount; k++) codepointsClear[k] = codepointsClear[k + 1];

                                    j--;
                                    codepointsClearCount--;
                                }
                            }
                        }

                        // Allocate memory to fit duplicates-cleared codepoints
                        windowFontAtlasState.externalCodepointList = (int *)RL_CALLOC(codepointsClearCount*sizeof(int), 1);

                        // Copy codepoints into our custom charset
                        for (int i = 0; (i < codepointsClearCount); i++) windowFontAtlasState.externalCodepointList[i] = codepointsClear[i];
                        windowFontAtlasState.externalCodepointListCount = codepointsClearCount;

                        RL_FREE(codepointsClear);

                        windowFontAtlasState.selectedCharset = 2;
                        windowFontAtlasState.selectedCharset = 2;
                        windowFontAtlasState.fontAtlasRegen = true;
                    }

                    UnloadCodepoints(codepoints);
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
            SetWindowTitle(TextFormat("%s v%s | File: %s", toolName, toolVersion, GetFileName(inFileName)));
            strcpy(currentStyleName, GetFileNameWithoutExt(inFileName));

            genFontSizeValue = GuiGetStyle(DEFAULT, TEXT_SIZE);
            fontSpacingValue = GuiGetStyle(DEFAULT, TEXT_SPACING);

            // Load .rgs custom font in font
            customFont = GuiGetFont();
            memset(fontFilePath, 0, 512);
            fontFileProvided = false;
            customFontLoaded = true;

            // Regenerate style table
            UnloadTexture(texStyleTable);
            Image imStyleTable = GenImageStyleControlsTable(1920, 256, currentStyleName);
            texStyleTable = LoadTextureFromImage(imStyleTable);
            UnloadImage(imStyleTable);

            styleCounter++;
            if (styleCounter > 7) styleCounter = 0;
        }
#endif

#if defined(PLATFORM_DESKTOP)
        // Toggle screen size (x2) mode
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_F)) screenSizeActive = !screenSizeActive;
/*
        // Save all style file formats for current style (convenience functionality)
        // TODO: Review shortcut and exposure of this function
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_B))
        {
            char styleNameLower[64] = { 0 };
            strcpy(styleNameLower, TextToLower(currentStyleName));
            if (!DirectoryExists(styleNameLower)) MakeDirectory(styleNameLower);

            // Style header: style_name.h
            ExportStyleAsCode(TextFormat("%s/style_%s.h", styleNameLower, styleNameLower), currentStyleName);

            //fontDataCompressedChecked = false;

            // Style binary: style_name.old.rgs (backward compatible)
            version = 300;
            SaveStyle(TextFormat("%s/style_%s.old.rgs", styleNameLower, styleNameLower), STYLE_BINARY);
            version = 400;

            // Style binary: style_name.rgs
            SaveStyle(TextFormat("%s/style_%s.rgs", styleNameLower, styleNameLower), STYLE_BINARY);

            // Style text (font required): style_name.txt.rgs
            SaveStyle(TextFormat("%s/style_%s.txt.rgs", styleNameLower, styleNameLower), STYLE_TEXT);

            // Style table (with style chunck): style_name.png
            Image imStyleTable = GenImageStyleControlsTable(1920, 256, currentStyleName);
            ExportImage(imStyleTable, TextFormat("%s/style_%s.png", styleNameLower, styleNameLower));
            UnloadImage(imStyleTable);
            // Write a custom chunk - rGSf (rGuiStyler file)
            rpng_chunk chunk = { 0 };
            memcpy(chunk.type, "rGSf", 4);  // Chunk type FOURCC
            chunk.data = SaveStyleToMemory(&chunk.length);
            rpng_chunk_write(TextFormat("%s/style_%s.png", styleNameLower, styleNameLower), chunk);
            RPNG_FREE(chunk.data);

            // Style screenshot: screenshot.png
            // Select LABEL, BORDER_COLOR_FOCUSED -> Update required?
            TakeScreenshot(TextFormat("%s/screenshot.png", styleNameLower));
        }
*/
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
        if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) || mainToolbarState.btnLoadFilePressed) showLoadStyleDialog = true;

        // Show dialog: save style file (.rgs)
        if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) || mainToolbarState.btnSaveFilePressed)
        {
#if defined(PLATFORM_DESKTOP)
            // NOTE: Fast-save only works for already loaded/saved .rgs styles
            if (inputFileLoaded || outputFileCreated)
            {
                // Priority to output file saving
                if (outputFileCreated) SaveStyle(outFileName, STYLE_BINARY);
                else SaveStyle(inFileName, STYLE_BINARY);

                SetWindowTitle(TextFormat("%s v%s | File: %s", toolName, toolVersion, GetFileName(inFileName)));
                saveChangesRequired = false;
            }
            else
#endif
            {
                // If no input/output file already loaded/saved, show save file dialog
                exportFormatActive = STYLE_BINARY;
                strcpy(outFileName, TextFormat("%s.rgs", TextToLower(currentStyleName)));
                showSaveStyleDialog = true;
            }
        }

        // Show dialog: export style file (.rgs, .png, .h)
        if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_E)) || mainToolbarState.btnExportFilePressed) showExportWindow = true;

        // Make sure shortcuts do not interfere with text-editing boxes
        if (!textHexColorEditMode && !genFontSizeEditMode && !fontSpacingEditMode && !fontSampleEditMode && !styleNameEditMode)
        {
            // Toggle window: help
            if (IsKeyPressed(KEY_F1)) windowHelpState.windowActive = !windowHelpState.windowActive;

            // Toggle window: about
            if (IsKeyPressed(KEY_F2)) windowAboutState.windowActive = !windowAboutState.windowActive;

            // Toggle window: issue
            if (IsKeyPressed(KEY_F3)) showIssueReportWindow = !showIssueReportWindow;

            // Show window: style table image
            //if (IsKeyPressed(KEY_F5)) mainToolbarState.viewStyleTableActive = !mainToolbarState.viewStyleTableActive;

            // Show window: font atlas
            if (IsKeyPressed(KEY_F6) || mainToolbarState.btnFontAtlasPressed) windowFontAtlasState.windowActive = !windowFontAtlasState.windowActive;

            // Show closing window on ESC
            if (IsKeyPressed(KEY_ESCAPE))
            {
                if (windowHelpState.windowActive) windowHelpState.windowActive = false;
                else if (windowAboutState.windowActive) windowAboutState.windowActive = false;
                else if (showIssueReportWindow) showIssueReportWindow = false;
                else if (windowFontAtlasState.windowActive) windowFontAtlasState.windowActive = false;
                else if (showExportWindow) showExportWindow = false;
            #if defined(PLATFORM_DESKTOP)
                else if (changedPropCounter > 0) showExitWindow = !showExitWindow;
                else closeWindow = true;
            #else
                else if (showLoadStyleDialog) showLoadStyleDialog = false;
                else if (showSaveStyleDialog) showSaveStyleDialog = false;
                else if (showExportStyleDialog) showExportStyleDialog = false;
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
        }
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
            Vector3 hsvDisabled = { hueDisabled, 0.2f, value };

            // Update style default color values
            GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(ColorFromHSV(hsvNormal.x, hsvNormal.y, hsvNormal.z)));
            GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(ColorFromHSV(hsvNormal.x, GetRandomValue(4, 7)/10.0f, (fabsf(0.5f - hsvNormal.z) < 0.2f)? 1.0f + ((GetRandomValue(3, 5)/10.0f)*fabsf(0.5f - hsvNormal.z) / (0.5 - hsvNormal.z)) : 1 - hsvNormal.z)));
            GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(ColorFromHSV(hsvNormal.x, hsvNormal.y, hsvNormal.z)));

            GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, ColorToInt(ColorFromHSV(hsvFocused.x, hsvFocused.y, hsvFocused.z)));
            GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, ColorToInt(ColorFromHSV(hsvFocused.x, GetRandomValue(4, 7)/10.0f, 1 - hsvFocused.z)));
            GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt(ColorFromHSV(hsvFocused.x, hsvFocused.y, hsvFocused.z)));

            GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, ColorToInt(ColorFromHSV(hsvPressed.x, hsvPressed.y, hsvPressed.z)));
            GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, ColorToInt(ColorFromHSV(hsvPressed.x, GetRandomValue(4, 7)/10.0f, 1 - hsvPressed.z)));
            GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, ColorToInt(ColorFromHSV(hsvPressed.x, hsvPressed.y, hsvPressed.z)));

            GuiSetStyle(DEFAULT, BORDER_COLOR_DISABLED, ColorToInt(ColorFromHSV(hsvDisabled.x, hsvDisabled.y, hsvDisabled.z)));
            GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, ColorToInt(ColorFromHSV(hsvDisabled.x, hsvDisabled.y, 1 - hsvDisabled.z)));
            GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, ColorToInt(ColorFromHSV(hsvDisabled.x, hsvDisabled.y, hsvDisabled.z)));

            GuiSetStyle(DEFAULT, BACKGROUND_COLOR, GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL));
            GuiSetStyle(DEFAULT, LINE_COLOR, GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL));

            // Update color boxes palette
            for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));
        }

        //styleFrameCounter++;
        //if ((styleFrameCounter%120) == 0) mainToolbarState.visualStyleActive++;
        //if (mainToolbarState.visualStyleActive > 11) mainToolbarState.visualStyleActive = 0;

        // Visual options logic
        if (mainToolbarState.visualStyleActive != mainToolbarState.prevVisualStyleActive)
        {
            LOG("INFO: Current Visual Style: %i\n", mainToolbarState.visualStyleActive);

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
                case 12: GuiLoadStyleAmber(); break;
                case 13: GuiLoadStyleRltech(); break;
                default: break;
            }

            // Current style backup (used to track changes)
            memcpy(currentStyle, guiStyle, RAYGUI_MAX_CONTROLS *(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)*sizeof(int));

            customFont = GuiGetFont();
            customFontLoaded = true;
            windowFontAtlasState.fontGenSizeValue = GuiGetStyle(DEFAULT, TEXT_SIZE);
            fontDrawSizeValue = GuiGetStyle(DEFAULT, TEXT_SIZE);
            fontSpacingValue = GuiGetStyle(DEFAULT, TEXT_SPACING);

            for (int i = 0; i < 12; i++) colorBoxValue[i] = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL + i));

            changedPropCounter = 0;
            saveChangesRequired = false;

            mainToolbarState.prevVisualStyleActive = mainToolbarState.visualStyleActive;

            windowFontAtlasState.fontWhiteRec = GetShapesTextureRectangle();

            // WARNING: Make sure styleNames[] size matches number of gui styles!
            memset(currentStyleName, 0, 64);
            strcpy(currentStyleName, styleNames[mainToolbarState.visualStyleActive]);
        }

        fontWhiteRec = windowFontAtlasState.fontWhiteRec;   // Register fontWhiteRec from fontAtlas window

        // Help options logic
        if (mainToolbarState.btnHelpPressed) windowHelpState.windowActive = true;
        if (mainToolbarState.btnAboutPressed) windowAboutState.windowActive = true;
        if (mainToolbarState.btnIssuePressed) showIssueReportWindow = true;
        //----------------------------------------------------------------------------------

        // Basic program flow logic
        //----------------------------------------------------------------------------------
        frameCounter++;                     // General usage frames counter
        mousePos = GetMousePosition();      // Get mouse position each frame

        // Check for changed properties
        changedPropCounter = StyleChangesCounter(currentStyle);
        if (changedPropCounter > 0) saveChangesRequired = true;

        // NOTE: Font reloading inside windowFontAtlas

        GuiSetStyle(DEFAULT, TEXT_SIZE, fontDrawSizeValue);
        GuiSetStyle(DEFAULT, TEXT_SPACING, fontSpacingValue);
        GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, (int)(1.5f*fontDrawSizeValue));     // TODO: Expose property?

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

        if (!windowFontAtlasState.windowActive && selectingColor)
        {
            HideCursor();
            if (mousePos.x < colorPickerRec.x) SetMousePosition(colorPickerRec.x, mousePos.y);
            else if (mousePos.x > colorPickerRec.x + colorPickerRec.width) SetMousePosition(colorPickerRec.x + colorPickerRec.width, mousePos.y);

            if (mousePos.y < colorPickerRec.y) SetMousePosition(mousePos.x, colorPickerRec.y);
            else if (mousePos.y > colorPickerRec.y + colorPickerRec.height) SetMousePosition(mousePos.x, colorPickerRec.y + colorPickerRec.height);
        }
        //----------------------------------------------------------------------------------

        // Font image atals logic
        //----------------------------------------------------------------------------------
        if (windowFontAtlasState.windowActive)
        {
            windowFontAtlasState.texFont = customFont.texture;
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
        if (mainToolbarState.propsStateEditMode ||
            windowHelpState.windowActive ||
            windowAboutState.windowActive ||
            windowFontAtlasState.windowActive ||
            showExitWindow ||
            showExportWindow ||
            showIssueReportWindow ||
            showLoadStyleDialog ||
            showSaveStyleDialog ||
            showExportStyleDialog) GuiLock();
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
            GuiListView((Rectangle){ anchorMain.x + 10, anchorMain.y + 52, 148, 520 }, TextJoin(guiControlText, RAYGUI_MAX_CONTROLS, ";"), NULL, &currentSelectedControl);
            if (currentSelectedControl != DEFAULT) GuiListViewEx((Rectangle){ anchorMain.x + 163, anchorMain.y + 52, 180, 520 }, guiPropsText, RAYGUI_MAX_PROPS_BASE - 1, NULL, &currentSelectedProperty, NULL);
            else GuiListViewEx((Rectangle){ anchorMain.x + 163, anchorMain.y + 52, 180, 520 }, guiPropsDefaultText, 14, NULL, &currentSelectedProperty, NULL);

            // Controls window
            if (controlsWindowActive)
            {
                controlsWindowActive = !GuiWindowBox((Rectangle){ anchorWindow.x + 0, anchorWindow.y + 0, 385, 520 }, "#198#Sample raygui controls");

                GuiGroupBox((Rectangle){ anchorPropEditor.x + 0, anchorPropEditor.y + 0, 365, 357 }, "Property Editor");

                //if ((mainToolbarState.propsStateActive == STATE_NORMAL) && (currentSelectedProperty != TEXT_PADDING) && (currentSelectedProperty != BORDER_WIDTH)) GuiDisable();
                //if (currentSelectedControl == DEFAULT) GuiDisable();
                float propValueFloat = (float)propertyValue;
                GuiSlider((Rectangle){ anchorPropEditor.x + 50, anchorPropEditor.y + 15, 235, 15 }, "Value:", NULL, &propValueFloat, 0, 20);
                propertyValue = (int)propValueFloat;
                if (GuiValueBox((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 10, 60, 25 }, NULL, &propertyValue, 0, 8, propertyValueEditMode)) propertyValueEditMode = !propertyValueEditMode;
                //if (mainToolbarState.propsStateActive != STATE_DISABLED) GuiEnable();

                GuiLine((Rectangle){ anchorPropEditor.x + 0, anchorPropEditor.y + 35, 365, 15 }, NULL);
                GuiColorPicker((Rectangle){ anchorPropEditor.x + 10, anchorPropEditor.y + 55, 240, 240 }, NULL, &colorPickerValue);

                GuiGroupBox((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 60, 60, 55 }, "RGBA");
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 65, 80, 20 }, TextFormat("R:  %03i", colorPickerValue.r));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 80, 80, 20 }, TextFormat("G:  %03i", colorPickerValue.g));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 95, 80, 20 }, TextFormat("B:  %03i", colorPickerValue.b));
                GuiGroupBox((Rectangle){ anchorPropEditor.x + 295, anchorPropEditor.y + 125, 60, 55 }, "HSV");
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 130, 80, 20 }, TextFormat("H:  %.0f", colorHSV.x));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 145, 80, 20 }, TextFormat("S:  %.0f%%", colorHSV.y*100));
                GuiLabel((Rectangle){ anchorPropEditor.x + 300, anchorPropEditor.y + 160, 80, 20 }, TextFormat("V:  %.0f%%", colorHSV.z*100));

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
                GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
                GuiLabel((Rectangle){ anchorPropEditor.x + 10, anchorPropEditor.y + 320, 104, 24 }, "Text Alignment:");
                GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
                GuiToggleGroup((Rectangle){ anchorPropEditor.x + 120, anchorPropEditor.y + 320, 76, 24 }, "#87#LEFT;#89#CENTER;#83#RIGHT", &textAlignmentActive);
                if (mainToolbarState.propsStateActive != STATE_DISABLED) GuiEnable();

                // Font options
                GuiGroupBox((Rectangle){ anchorFontOptions.x + 0, anchorFontOptions.y + 0, 365, 90 }, "Text Drawing Options");
                if (GuiButton((Rectangle){ anchorFontOptions.x + 10, anchorFontOptions.y + 16, 60, 24 }, "#30#Font")) windowFontAtlasState.windowActive = true;

                if (GuiSpinner((Rectangle){ anchorFontOptions.x + 110, anchorFontOptions.y + 16, 92, 24 }, "Size: ", &fontDrawSizeValue, 8, 32, genFontSizeEditMode)) genFontSizeEditMode = !genFontSizeEditMode;
                if (GuiSpinner((Rectangle){ anchorFontOptions.x + 262, anchorFontOptions.y + 16, 92, 24 }, "Spacing: ", &fontSpacingValue, -4, 8, fontSpacingEditMode)) fontSpacingEditMode = !fontSpacingEditMode;

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

            // GUI: Show style table image (if active and reloaded)
            //----------------------------------------------------------------------------------------
            //if (mainToolbarState.viewStyleTableActive && 
            //    (mainToolbarState.prevViewStyleTableActive == mainToolbarState.viewStyleTableActive))
            {
                // Style table panning with mouse logic
                if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){ 0, screenHeight - styleTableRec.height - 28, GetScreenWidth(), styleTableRec.height }))
                {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    {
                        styleTablePanningMode = true;
                        styleTableOffsetX = GetMousePosition().x;
                        prevStyleTablePositionX = styleTableRec.x;
                    }
                }
                if (styleTablePanningMode)
                {
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) styleTableRec.x = prevStyleTablePositionX - (GetMouseX() - styleTableOffsetX);

                    if (styleTableRec.x < 0) styleTableRec.x = 0;
                    else if (styleTableRec.x > (styleTableRec.width - GetScreenWidth())) styleTableRec.x = styleTableRec.width - GetScreenWidth();

                    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) styleTablePanningMode = false;
                }

                GuiSetStyle(SLIDER, SLIDER_WIDTH, 128);
                GuiSlider((Rectangle){ 0, screenHeight - 24 - 12 + 1, screenWidth, 12 }, NULL, NULL, &styleTableRec.x, 0.0f, (float)styleTableRec.width - screenWidth);
                GuiSetStyle(SLIDER, SLIDER_WIDTH, 16);

                DrawStyleControlsTable(-styleTableRec.x, screenHeight - 264);

                // Draw style palette as small rectangles for easy color reference
                // NOTE: We have 4 states with 3 color properties per state (BASE, BORDER, TEXT)
                //GuiLabel((Rectangle){ 12, screenHeight - 234, screenWidth, GuiGetFont().baseSize }, "Color palette (DEFAULT):");
                //for (int i = 0; i < 12; i++) DrawRectangle(12 + 150 + 8*i, screenHeight - 234, 8, 8, GetColor((unsigned int)GuiGetStyle(0, i)));
            }
            //----------------------------------------------------------------------------------------

            // GUI: Status bar
            //----------------------------------------------------------------------------------------
            GuiStatusBar((Rectangle){ 0, GetScreenHeight() - 24, 60, 24 }, "Name:"); //(changedPropCounter > 0)? currentStyleName : styleNames[mainToolbarState.visualStyleActive]));
            GuiStatusBar((Rectangle){159, GetScreenHeight() - 24, 190, 24 }, TextFormat("CHANGED PROPERTIES: %i", changedPropCounter));

            if (GuiTextBox((Rectangle){ 60 - 1, GetScreenHeight() - 24, 101, 24 }, currentStyleName, 128, styleNameEditMode)) styleNameEditMode = !styleNameEditMode;

            GuiStatusBar((Rectangle){ 348, GetScreenHeight() - 24, 400, 24 }, TextFormat("FONT: %i codepoints | %ix%i pixels", GuiGetFont().glyphCount, GuiGetFont().texture.width, GuiGetFont().texture.height));
            //----------------------------------------------------------------------------------------

            // NOTE: If some overlap window is open and main window is locked, we draw a background rectangle
            if (GuiIsLocked()) DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)), 0.85f));


            // WARNING: Before drawing the windows, we unlock them
            GuiUnlock();

            // GUI: Main toolbar panel
            //----------------------------------------------------------------------------------
            GuiMainToolbar(&mainToolbarState);
            //----------------------------------------------------------------------------------

            // Set default NORMAL state for all controls not in main screen
            GuiSetState(STATE_NORMAL);

            // GUI: Font Atlas Window
            //----------------------------------------------------------------------------------------
            int prevFontSize = windowFontAtlasState.fontGenSizeValue;

            GuiWindowFontAtlas(&windowFontAtlasState);

            if (windowFontAtlasState.fontGenSizeValue != prevFontSize)
            {
                fontDrawSizeValue = windowFontAtlasState.fontGenSizeValue;
            }

            if (windowFontAtlasState.btnLoadFontPressed) showLoadFontDialog = true;
            if (windowFontAtlasState.btnLoadCharsetPressed) showLoadCharsetDialog = true;
            if (windowFontAtlasState.btnSaveFontAtlasPressed) showSaveFontAtlasDialog = true;
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

            // GUI: Issue Report Window
            //----------------------------------------------------------------------------------------
            if (showIssueReportWindow)
            {
                Rectangle messageBox = { (float)GetScreenWidth()/2 - 300/2, (float)GetScreenHeight()/2 - 190/2 - 20, 300, 190 };
                int result = GuiMessageBox(messageBox, "#220#Report Issue",
                    "Do you want to report any issue or\nfeature request for this program?\n\ngithub.com/raysan5/rguistyler", "#186#Report on GitHub");

                if (result == 1)    // Report issue pressed
                {
                    OpenURL("https://github.com/raysan5/rguistyler/issues");
                    showIssueReportWindow = false;
                }
                else if (result == 0) showIssueReportWindow = false;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Export Window
            //----------------------------------------------------------------------------------------
            if (showExportWindow)
            {
                Rectangle messageBox = { (float)screenWidth/2 - 248/2, (float)screenHeight/2 - 150, 248, 220 };
                int result = GuiMessageBox(messageBox, "#7#Export Style File", " ", "#7# Export Style");

                GuiLabel((Rectangle){ messageBox.x + 12, messageBox.y + 24 + 12, 106, 24 }, "Style Name:");
                if (GuiTextBox((Rectangle){ messageBox.x + 12 + 92, messageBox.y + 24 + 12, 132, 24 }, currentStyleName, 128, styleNameEditMode)) styleNameEditMode = !styleNameEditMode;

                GuiLabel((Rectangle){ messageBox.x + 12, messageBox.y + 12 + 48 + 8, 106, 24 }, "Style Format:");
                GuiComboBox((Rectangle){ messageBox.x + 12 + 92, messageBox.y + 12 + 48 + 8, 132, 24 }, "Binary (.rgs);Code (.h);Image (.png)", &exportFormatActive);

                GuiCheckBox((Rectangle){ messageBox.x + 20, messageBox.y + 48 + 56, 16, 16 }, "Font data embedded into style", &fontEmbeddedChecked);
                GuiEnable();
                //if (exportFormatActive != 2) GuiDisable();
                GuiCheckBox((Rectangle){ messageBox.x + 20, messageBox.y + 72 + 32 + 24, 16, 16 }, "Font data compressed", &fontDataCompressedChecked);
                GuiEnable();
                if (exportFormatActive != 2) GuiDisable();
                GuiCheckBox((Rectangle){ messageBox.x + 20, messageBox.y + 72 + 32 + 24 + 24, 16, 16 }, "Style embedded as rGSf chunk", &styleChunkChecked);
                GuiEnable();

                if (result == 1)    // Export button pressed
                {
                    showExportWindow = false;
                    showExportStyleDialog = true;
                }
                else if (result == 0) showExportWindow = false;
            }
            //----------------------------------------------------------------------------------

            // GUI: Exit Window
            //----------------------------------------------------------------------------------------
            if (showExitWindow)
            {
                int result = GuiMessageBox((Rectangle){ (float)screenWidth/2 - 125, (float)screenHeight/2 - 50, 250, 100 }, "#159#Closing rGuiStyler", "Do you really want to exit?", "Yes;No");

                if ((result == 0) || (result == 2)) showExitWindow = false;
                else if (result == 1) closeWindow = true;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Load File Dialog (and loading logic)
            //----------------------------------------------------------------------------------------
            if (showLoadStyleDialog)
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
                    SetWindowTitle(TextFormat("%s v%s | File: %s", toolName, toolVersion, GetFileName(inFileName)));
                    inputFileLoaded = true;

                    // Load .rgs custom font in font
                    customFont = GuiGetFont();
                    memset(inFontFileName, 0, 512);
                    customFontLoaded = true;

                    saveChangesRequired = false;
                }

                if (result >= 0) showLoadStyleDialog = false;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Load Font File Dialog (and loading logic)
            //----------------------------------------------------------------------------------------
            if (showLoadFontDialog)
            {
#if defined(CUSTOM_MODAL_DIALOGS)
                int result = GuiFileDialog(DIALOG_MESSAGE, "Load font file ...", inFontFileName, "Ok", "Just drag and drop your .ttf/.otf font file!");
#else
                int result = GuiFileDialog(DIALOG_OPEN_FILE, "Load font file", inFontFileName, "*.ttf;*.otf", "Font Files (*.ttf, *.otf)");
#endif
                if (result == 1)
                {
                    windowFontAtlasState.fontAtlasRegen = true;
                    //fontDrawSizeValue = windowFontAtlasState.fontGenSizeValue;
                }

                if (result >= 0) showLoadFontDialog = false;

                windowFontAtlasState.btnLoadFontPressed = false;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Load Font Charset Dialog (and loading logic)
            //----------------------------------------------------------------------------------------
            if (showLoadCharsetDialog)
            {
#if defined(CUSTOM_MODAL_DIALOGS)
                int result = GuiFileDialog(DIALOG_MESSAGE, "Load font charset ...", inFileName, "Ok", "Just drag and drop your .txt charset!");
#else
                int result = GuiFileDialog(DIALOG_OPEN_FILE, "Load font charset", inFileName, "*.txt", "Charset data (*.txt)");
#endif
                if (result == 1)
                {
                    // Load codepoints to generate the font
                    // NOTE: A UTF8 text file should be provided, it will be processed to get codepoints
                    char *text = LoadFileText(inFileName);
                    if (text != NULL)
                    {
                        int codepointsCount = 0;
                        int *codepoints = LoadCodepoints(text, &codepointsCount);
                        UnloadFileText(text);

                        if (codepointsCount > 0)
                        {
                            // Clear current custom codepoints list
                            if (windowFontAtlasState.externalCodepointList != NULL)
                            {
                                RL_FREE(windowFontAtlasState.externalCodepointList);
                                windowFontAtlasState.externalCodepointListCount = 0;
                                windowFontAtlasState.externalCodepointList = NULL;
                            }

                            // Create an array to store codepoints without duplicates
                            int codepointsClearCount = codepointsCount;
                            int *codepointsClear = (int *)RL_CALLOC(codepointsCount, sizeof(int));
                            memcpy(codepointsClear, codepoints, codepointsCount*sizeof(int));

                            // Remove duplicates
                            for (int i = 0; i < codepointsClearCount; i++)
                            {
                                for (int j = i + 1; j < codepointsClearCount; j++)
                                {
                                    if (codepointsClear[i] == codepointsClear[j])
                                    {
                                        for (int k = j; k < codepointsClearCount; k++) codepointsClear[k] = codepointsClear[k + 1];

                                        j--;
                                        codepointsClearCount--;
                                    }
                                }
                            }

                            // Allocate memory to fit duplicates-cleared codepoints
                            windowFontAtlasState.externalCodepointList = (int *)RL_CALLOC(codepointsClearCount*sizeof(int), 1);

                            // Copy codepoints into our custom charset
                            for (int i = 0; (i < codepointsClearCount); i++) windowFontAtlasState.externalCodepointList[i] = codepointsClear[i];
                            windowFontAtlasState.externalCodepointListCount = codepointsClearCount;

                            RL_FREE(codepointsClear);

                            windowFontAtlasState.selectedCharset = 2;
                            windowFontAtlasState.fontAtlasRegen = true;
                        }

                        UnloadCodepoints(codepoints);
                    }
                }

                if (result >= 0) showLoadCharsetDialog = false;

                windowFontAtlasState.btnLoadCharsetPressed = false;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Save File Dialog (and saving logic)
            //----------------------------------------------------------------------------------------
            if (showSaveStyleDialog)
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
                    if (outFileName[0] == '\0') strcpy(outFileName, "style.rgs");   // Check for empty name
                    if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".rgs")) strcat(outFileName, ".rgs\0");

                    // Save style file (text or binary)
                    SaveStyle(outFileName, STYLE_BINARY);
                    outputFileCreated = true;

                    // Set window title for future savings
                    SetWindowTitle(TextFormat("%s v%s | File: %s", toolName, toolVersion, GetFileName(outFileName)));

                #if defined(PLATFORM_WEB)
                    // Download file from MEMFS (emscripten memory filesystem)
                    // NOTE: Second argument must be a simple filename (we can't use directories)
                    // NOTE: Included security check to (partially) avoid malicious code on PLATFORM_WEB
                    if (strchr(outFileName, '\'') == NULL) emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", outFileName, GetFileName(outFileName)));
                #endif
                }

                if (result >= 0) showSaveStyleDialog = false;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Export File Dialog (and saving logic)
            //----------------------------------------------------------------------------------------
            if (showExportStyleDialog)
            {
#if defined(CUSTOM_MODAL_DIALOGS)
                //int result = GuiFileDialog(DIALOG_TEXTINPUT, "Export raygui style file...", outFileName, "Ok;Cancel", NULL);
                int result = GuiTextInputBox((Rectangle){ screenWidth/2 - 280/2, screenHeight/2 - 112/2 - 60, 280, 112 }, "#7#Export raygui style file...", NULL, "#7#Export", outFileName, 512, NULL);
#else
                if (outFileName[0] == '\0') strcpy(outFileName, "style");   // Check for empty name

                // Consider different supported file types
                char filters[64] = { 0 };
                strcpy(outFileName, TextToLower(currentStyleName));

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
                            ExportStyleAsCode(outFileName, currentStyleName);
                        } break;
                        case STYLE_TABLE_IMAGE:
                        {
                            // Check for valid extension and make sure it is
                            if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".png")) strcat(outFileName, ".png\0");
                            
                            // Export table image
                            Image imStyleTable = GenImageStyleControlsTable(1920, 256, currentStyleName);
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
                    // NOTE: Included security check to (partially) avoid malicious code on PLATFORM_WEB
                    if (strchr(outFileName, '\'') == NULL) emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", outFileName, GetFileName(outFileName)));
                #endif
                }

                if (result >= 0) showExportStyleDialog = false;
            }
            //----------------------------------------------------------------------------------------

            // GUI: Save Font Atlas Image Dialog (and saving logic)
            //----------------------------------------------------------------------------------------
            if (showSaveFontAtlasDialog)
            {
#if defined(CUSTOM_MODAL_DIALOGS)
                //int result = GuiFileDialog(DIALOG_TEXTINPUT, "Save font atlas image...", outFileName, "Ok;Cancel", NULL);
                int result = GuiTextInputBox((Rectangle){ screenWidth/2 - 280/2, screenHeight/2 - 112/2 - 30, 280, 112 }, "#2#Save font atlas image...", NULL, "#2#Save", outFileName, 512, NULL);
#else
                int result = GuiFileDialog(DIALOG_SAVE_FILE, "Save font atlas image...", outFileName, "*.png", "Image File (*.png)");
#endif
                if (result == 1)
                {
                    // Save file: outFileName
                    // Check for valid extension and make sure it is
                    if (outFileName[0] == '\0') strcpy(outFileName, "style_font.png");   // Check for empty name
                    if ((GetFileExtension(outFileName) == NULL) || !IsFileExtension(outFileName, ".png")) strcat(outFileName, ".png\0");

                    Image image = LoadImageFromTexture(windowFontAtlasState.texFont);
                    ExportImage(image, outFileName);
                    UnloadImage(image);

#if defined(PLATFORM_WEB)
                    // Download file from MEMFS (emscripten memory filesystem)
                    // NOTE: Second argument must be a simple filename (we can't use directories)
                    // NOTE: Included security check to (partially) avoid malicious code on PLATFORM_WEB
                    if (strchr(outFileName, '\'') == NULL) emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", outFileName, GetFileName(outFileName)));
#endif
                }

                if (result >= 0) showSaveFontAtlasDialog = false;
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
    printf("// Copyright (c) 2017-2025 raylib technologies (@raylibtech)                    //\n");
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
                // Gen and export table image
                Image imStyleTable = GenImageStyleControlsTable(1920, 256, GetFileNameWithoutExt(outFileName));
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
// WARNING: Using globals: fontEmbeddedChecked, fontDataCompressed
static char *SaveStyleToMemory(int *size)
{
    #define GUI_STYLE_RGS_VERSION   400

    char *buffer = (char *)RL_CALLOC(1024*1024, 1);  // 1MB should be enough to save the style
    int dataSize = 0;

    char signature[5] = "rGS ";
    short version = GUI_STYLE_RGS_VERSION;
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

    // Embed font data if required
    if (fontEmbeddedChecked && customFontLoaded)
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

        // Save font white rectangle
        memcpy(buffer + dataSize + 16, &fontWhiteRec, sizeof(Rectangle));
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
        // NOTE: Version 400 always adds the compression size parameter
        if (version >= 400)
        {
            int recsDataCompSize = 0;

            if (fontDataCompressedChecked)
            {
                unsigned char *recsDataCompressed = CompressData((unsigned char *)customFont.recs, customFont.glyphCount*sizeof(Rectangle), &recsDataCompSize);

                memcpy(buffer + dataSize, &recsDataCompSize, sizeof(int));
                dataSize += 4;

                memcpy(buffer + dataSize, recsDataCompressed, recsDataCompSize);
                dataSize += recsDataCompSize;

                RL_FREE(recsDataCompressed);
            }
            else
            {
                memcpy(buffer + dataSize, &recsDataCompSize, sizeof(int));
                dataSize += 4;

                for (int i = 0; i < customFont.glyphCount; i++)
                {
                    memcpy(buffer + dataSize, &customFont.recs[i], sizeof(Rectangle));
                    dataSize += sizeof(Rectangle);
                }
            }
        }
        else
        {
            // Fallback for older versions, no compression and no compression size stored
            for (int i = 0; i < customFont.glyphCount; i++)
            {
                memcpy(buffer + dataSize, &customFont.recs[i], sizeof(Rectangle));
                dataSize += sizeof(Rectangle);
            }
        }

        // Write font chars info data
        // NOTE: Version 400 always adds the compression size parameter
        if (version >= 400)
        {
            int glyphsDataCompSize = 0;

            if (fontDataCompressedChecked)
            {
                // NOTE: We only want to save some fields from GlyphInfo struct
                int *glyphsData = (int *)RL_MALLOC(customFont.glyphCount*4*sizeof(int));

                for (int i = 0; i < customFont.glyphCount; i++)
                {
                    glyphsData[4*i + 0] = customFont.glyphs[i].value;
                    glyphsData[4*i + 1] = customFont.glyphs[i].offsetX;
                    glyphsData[4*i + 2] = customFont.glyphs[i].offsetY;
                    glyphsData[4*i + 3] = customFont.glyphs[i].advanceX;
                }

                unsigned char *glyphsDataCompressed = CompressData((unsigned char *)glyphsData, customFont.glyphCount*4*sizeof(int), &glyphsDataCompSize);

                memcpy(buffer + dataSize, &glyphsDataCompSize, sizeof(int));
                dataSize += 4;

                memcpy(buffer + dataSize, glyphsDataCompressed, glyphsDataCompSize);
                dataSize += glyphsDataCompSize;

                RL_FREE(glyphsDataCompressed);
                RL_FREE(glyphsData);
            }
            else
            {
                memcpy(buffer + dataSize, &glyphsDataCompSize, sizeof(int));
                dataSize += 4;

                for (int i = 0; i < customFont.glyphCount; i++)
                {
                    memcpy(buffer + dataSize, &customFont.glyphs[i].value, sizeof(int));
                    memcpy(buffer + dataSize + 4, &customFont.glyphs[i].offsetX, sizeof(int));
                    memcpy(buffer + dataSize + 8, &customFont.glyphs[i].offsetY, sizeof(int));
                    memcpy(buffer + dataSize + 12, &customFont.glyphs[i].advanceX, sizeof(int));
                    dataSize += 16;
                }
            }
        }
        else
        {
            // Fallback for older versions, no compression and no compression size stored
            for (int i = 0; i < customFont.glyphCount; i++)
            {
                memcpy(buffer + dataSize, &customFont.glyphs[i].value, sizeof(int));
                memcpy(buffer + dataSize + 4, &customFont.glyphs[i].offsetX, sizeof(int));
                memcpy(buffer + dataSize + 8, &customFont.glyphs[i].offsetY, sizeof(int));
                memcpy(buffer + dataSize + 12, &customFont.glyphs[i].advanceX, sizeof(int));
                dataSize += 16;
            }
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
static int SaveStyle(const char *fileName, int format)
{
    #define GUI_STYLE_RGS_VERSION   400

    int result = 0;

    if (format == STYLE_BINARY)
    {
        // Style File Structure (.rgs)
        // ------------------------------------------------------
        // Offset  | Size    | Type       | Description
        // ------------------------------------------------------
        // 0       | 4       | char       | Signature: "rGS "
        // 4       | 2       | short      | Version: 200, 400
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
        // 16+4*N  | 4       | int        | Font data size (0 - no font, no more fields added!)
        // 20+4*N  | 4       | int        | Font base size
        // 24+4*N  | 4       | int        | Font glyph count [glyphCount]
        // 28+4*N  | 4       | int        | Font type (0-NORMAL, 1-SDF)
        // 32+4*N  | 16      | Rectangle  | Font white rectangle

        // Custom Font Data : Image (20 bytes + imData)
        // NOTE: Font image atlas is always converted to GRAY+ALPHA
        // and atlas image data can be compressed (DEFLATE)
        // 48+4*N  | 4       | int        | Image data size (uncompressed)
        // 52+4*N  | 4       | int        | Image data size (compressed)
        // 56+4*N  | 4       | int        | Image width
        // 60+4*N  | 4       | int        | Image height
        // 64+4*N  | 4       | int        | Image format
        // 68+4*N  | imSize  | *          | Image data (comp or uncomp)

        // Custom Font Data : Recs (32 bytes*glyphCount)
        // NOTE: Font recs data can be compressed (DEFLATE)
        // if (version >= 400)
        // {
        //    ...  | 4       | int       | Recs data compressed size (0 - not compressed)
        // }
        // NOTE: Uncompressed size can be calculated: (glyphCount*16 byte)
        // foreach (glyph)
        // {
        //    ...  | 16      | Rectangle  | Glyph rectangle (in image)
        // }

        // Custom Font Data : Glyph Info (32 bytes*glyphCount)
        // NOTE: Font glyphs info data can be compressed (DEFLATE)
        // if (version >= 400)
        // {
        //    ...  | 4       | int       | Glyphs data compressed size (0 - not compressed)
        // }
        // NOTE: Uncompressed size can be calculated: (glyphCount*16 byte)
        // foreach (glyph)
        // {
        //   ...   | 4       | int        | Glyph value
        //   ...   | 4       | int        | Glyph offset X
        //   ...   | 4       | int        | Glyph offset Y
        //   ...   | 4       | int        | Glyph advance X
        // }
        // ------------------------------------------------------

        int rgsFileDataSize = 0;
        char *rgsFileData = SaveStyleToMemory(&rgsFileDataSize);

        result = SaveFileData(fileName, rgsFileData, rgsFileDataSize);

        RL_FREE(rgsFileData);
    }
    else if (format == STYLE_TEXT)
    {
        FILE *rgsFile = fopen(fileName, "wt");

        if (rgsFile != NULL)
        {
            #define RGS_FILE_VERSION_TEXT  "4.0"

            // Write some description comments
            fprintf(rgsFile, "#\n# rgs style text file (v%s) - raygui style file generated using rGuiStyler\n#\n", RGS_FILE_VERSION_TEXT);
            fprintf(rgsFile, "# Provided info:\n");
            fprintf(rgsFile, "#    f fontGenSize charsetFileName fontFileName\n");
            fprintf(rgsFile, "#    p <controlId> <propertyId> <propertyValue>  Property description\n#\n");

            if (customFontLoaded)
            {
                // Save charset into an external file
                // NOTE: Only saving charset if not basic one (95 codepoints)
                // WARNING: codepointList and codepointListCount are global variables in gui_window_font_atlas module
                if (codepointListCount > 95)
                {
                    char *textData = (char *)RL_CALLOC(1024*1024, 1);    // Allocate 1MB for text data

                    const char *value = NULL;
                    int valueSize = 0;

                    for (int i = 0, k = 0; i < codepointListCount; i++, k += valueSize)
                    {
                        value = CodepointToUTF8(codepointList[i], &valueSize);

                        for (int c = 0; c < valueSize; c++) textData[k + c] = value[c];
                    }

                    // Save charset data
                    SaveFileText(TextFormat("%s/charset.txt", GetDirectoryPath(fileName)), textData);

                    RL_FREE(textData);
                }

                fprintf(rgsFile, "# WARNING: This style uses a custom font, must be provided with style file\n#\n");

                if (FileExists(TextFormat("%s/charset.txt", GetDirectoryPath(fileName))))   // Check charset.txt saved successfully
                {
                    fprintf(rgsFile, "f %i %s %s\n", GuiGetStyle(DEFAULT, TEXT_SIZE), "charset.txt", GetFileName(inFontFileName));
                }
                else fprintf(rgsFile, "f %i 0 %s\n", GuiGetStyle(DEFAULT, TEXT_SIZE), GetFileName(inFontFileName));
            }

            // Save DEFAULT properties that changed
            for (int j = 0; j < (RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED); j++)
            {
                if (defaultStyle[j] != GuiGetStyle(0, j))
                {
                    // NOTE: Control properties are written as hexadecimal values, extended properties names not provided
                    if (j < RAYGUI_MAX_PROPS_BASE) fprintf(rgsFile, "p 00 %02i 0x%08x    DEFAULT_%s \n", j, GuiGetStyle(0, j), guiPropsText[j]);
                    else  fprintf(rgsFile, "p 00 %02i 0x%08x    %s \n", j, GuiGetStyle(0, j), guiPropsDefaultExtendedText[j - RAYGUI_MAX_PROPS_BASE]);
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
            result = 1;
        }
    }

    return result;
}

// Export gui style as properties array
// NOTE: Code file already implements a function to load style
static void ExportStyleAsCode(const char *fileName, const char *styleName)
{
    // DEFAULT extended properties
    static const char *guiPropsExtText[RAYGUI_MAX_PROPS_EXTENDED] = {
        "TEXT_SIZE",
        "TEXT_SPACING",
        "LINE_COLOR",
        "BACKGROUND_COLOR",
        "TEXT_LINE_SPACING",
        "TEXT_ALIGNMENT_VERTICAL",
        "TEXT_WRAP_MODE",
        "EXTENDED08",
    };

    FILE *txtFile = fopen(fileName, "wt");

    if (txtFile != NULL)
    {
        fprintf(txtFile, "//////////////////////////////////////////////////////////////////////////////////\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// StyleAsCode exporter v2.0 - Style data exported as a values array            //\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// USAGE: On init call: GuiLoadStyle%s();                                   //\n", TextToPascal(styleName));
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// more info and bugs-report:  github.com/raysan5/raygui                        //\n");
        fprintf(txtFile, "// feedback and support:       ray[at]raylibtech.com                            //\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "// Copyright (c) 2020-2025 raylib technologies (@raylibtech)                    //\n");
        fprintf(txtFile, "//                                                                              //\n");
        fprintf(txtFile, "//////////////////////////////////////////////////////////////////////////////////\n\n");

        char styleNameLower[64] = { 0 };
        strcpy(styleNameLower, TextToLower(styleName));

        // Export only properties that change from default style
        fprintf(txtFile, "#define %s_STYLE_PROPS_COUNT  %i\n\n", TextToUpper(styleName), StyleChangesCounter(defaultStyle));

        // Write byte data as hexadecimal text
        fprintf(txtFile, "// Custom style name: %s\n", styleName);
        fprintf(txtFile, "static const GuiStyleProp %sStyleProps[%s_STYLE_PROPS_COUNT] = {\n", styleNameLower, TextToUpper(styleName));

        // Write all properties that have changed in default style
        for (int i = 0; i < (RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED); i++)
        {
            if (defaultStyle[i] != GuiGetStyle(0, i))
            {
                if (i < RAYGUI_MAX_PROPS_BASE) fprintf(txtFile, "    { 0, %i, (int)0x%08x },    // DEFAULT_%s \n", i, GuiGetStyle(DEFAULT, i), guiPropsText[i]);
                else fprintf(txtFile, "    { 0, %i, (int)0x%08x },    // DEFAULT_%s \n", i, GuiGetStyle(DEFAULT, i), guiPropsExtText[i - RAYGUI_MAX_PROPS_BASE]);
            }
        }

        // Add to count all properties that have changed in comparison to default style
        for (int i = 1; i < RAYGUI_MAX_CONTROLS; i++)
        {
            for (int j = 0; j < (RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED); j++)
            {
                if ((defaultStyle[i*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED) + j] != GuiGetStyle(i, j)) && (GuiGetStyle(i, j) !=  GuiGetStyle(0, j)))
                {
                    if (j < RAYGUI_MAX_PROPS_BASE) fprintf(txtFile, "    { %i, %i, (int)0x%08x },    // %s_%s \n", i, j, GuiGetStyle(i, j), guiControlText[i], guiPropsText[j]);
                    else fprintf(txtFile, "    { %i, %i, (int)0x%08x },    // %s_%s \n", i, j, GuiGetStyle(i, j), guiControlText[i], TextFormat("EXTENDED%02i", j - RAYGUI_MAX_PROPS_BASE + 1));
                }
            }
        }

        fprintf(txtFile, "};\n\n");

        if (customFontLoaded)
        {
            fprintf(txtFile, "// WARNING: This style uses a custom font: \"%s\" (size: %i, spacing: %i)\n\n",
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

            // Font image data is usually GRAYSCALE + ALPHA

            // Compress font image data
            int compDataSize = 0;
            unsigned char *compData = CompressData(imFont.data, imFontSize, &compDataSize);

            // Save font image data (compressed)
            fprintf(txtFile, "#define %s_STYLE_FONT_ATLAS_COMP_SIZE %i\n\n", TextToUpper(styleName), compDataSize);
            fprintf(txtFile, "// Font atlas image pixels data: DEFLATE compressed\n");
            fprintf(txtFile, "static unsigned char %sFontData[%s_STYLE_FONT_ATLAS_COMP_SIZE] = { ", styleNameLower, TextToUpper(styleName));
            for (int i = 0; i < compDataSize - 1; i++) fprintf(txtFile, ((i%BYTES_TEXT_PER_LINE == 0)? "0x%02x,\n    " : "0x%02x, "), compData[i]);
            fprintf(txtFile, "0x%02x };\n\n", compData[compDataSize - 1]);
            MemFree(compData);
#else
            // Save font image data (uncompressed)
            fprintf(txtFile, "// Font image pixels data\n");
            fprintf(txtFile, "static unsigned char %sFontImageData[%i] = { ", styleNameLower, imFontSize);
            for (int i = 0; i < imFontSize - 1; i++) fprintf(txtFile, ((i%BYTES_TEXT_PER_LINE == 0)? "0x%02x,\n    " : "0x%02x, "), ((unsigned char *)imFont.data)[i]);
            fprintf(txtFile, "0x%02x };\n\n", ((unsigned char *)imFont.data)[imFontSize - 1]);
#endif
            // Save font recs data
            fprintf(txtFile, "// Font glyphs rectangles data (on atlas)\n");
            fprintf(txtFile, "static const Rectangle %sFontRecs[%i] = {\n", styleNameLower, customFont.glyphCount);
            for (int i = 0; i < customFont.glyphCount; i++)
            {
                fprintf(txtFile, "    { %1.0f, %1.0f, %1.0f , %1.0f },\n", customFont.recs[i].x, customFont.recs[i].y, customFont.recs[i].width, customFont.recs[i].height);
            }
            fprintf(txtFile, "};\n\n");

            // Save font glyphs data
            // NOTE: Individual glyphs image data not saved, it could be generated from atlas and recs
            fprintf(txtFile, "// Font glyphs info data\n");
            fprintf(txtFile, "// NOTE: No glyphs.image data provided\n");
            fprintf(txtFile, "static const GlyphInfo %sFontGlyphs[%i] = {\n", styleNameLower, customFont.glyphCount);
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
        fprintf(txtFile, "        GuiSetStyle(%sStyleProps[i].controlId, %sStyleProps[i].propertyId, %sStyleProps[i].propertyValue);\n    }\n\n", styleNameLower, styleNameLower, styleNameLower);

        if (customFontLoaded)
        {
            fprintf(txtFile, "    // Custom font loading\n");
#if defined(SUPPORT_COMPRESSED_FONT_ATLAS)
            fprintf(txtFile, "    // NOTE: Compressed font image data (DEFLATE), it requires DecompressData() function\n");
            fprintf(txtFile, "    int %sFontDataSize = 0;\n", styleNameLower);
            fprintf(txtFile, "    unsigned char *data = DecompressData(%sFontData, %s_STYLE_FONT_ATLAS_COMP_SIZE, &%sFontDataSize);\n", styleNameLower, TextToUpper(styleName), styleNameLower);
            fprintf(txtFile, "    Image imFont = { data, %i, %i, 1, %i };\n\n", imFont.width, imFont.height, imFont.format);
            //fprintf(txtFile, "    ImageFormat(&imFont, PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA);
#else
            fprintf(txtFile, "    Image imFont = { %sFontImageData, %i, %i, 1, %i };\n\n", styleNameLower, imFont.width, imFont.height, imFont.format);
#endif
            fprintf(txtFile, "    Font font = { 0 };\n");
            fprintf(txtFile, "    font.baseSize = %i;\n", GuiGetStyle(DEFAULT, TEXT_SIZE));
            fprintf(txtFile, "    font.glyphCount = %i;\n\n", customFont.glyphCount);

            fprintf(txtFile, "    // Load texture from image\n");
            fprintf(txtFile, "    font.texture = LoadTextureFromImage(imFont);\n");
#if defined(SUPPORT_COMPRESSED_FONT_ATLAS)
            fprintf(txtFile, "    UnloadImage(imFont);  // Uncompressed image data can be unloaded from memory\n\n");
#else
            fprintf(txtFile, "    // WARNING: Uncompressed global image data can not be freed\n\n");
#endif
            /*
            // Assign global recs/glyphs data to loaded font
            // NOTE: DO NOT DO THAT! GuiLoadStyleDefault() frees memory for styles loaded (other than default)
            // it can be used to reset/free any previous loaded style to default before loading a new one
            fprintf(txtFile, "    // Assign char recs data to global fontRecs\n");
            fprintf(txtFile, "    // WARNING: Font char rec data can not be freed\n");
            fprintf(txtFile, "    font.recs = %sFontRecs;\n\n", styleName);

            fprintf(txtFile, "    // Assign font char info data to global fontChars\n");
            fprintf(txtFile, "    // WARNING: Font char info data can not be freed\n");
            fprintf(txtFile, "    font.glyphs = %sFontChars;\n\n", styleName);
            */
            fprintf(txtFile, "    // Copy char recs data from global fontRecs\n");
            fprintf(txtFile, "    // NOTE: Required to avoid issues if trying to free font\n");
            fprintf(txtFile, "    font.recs = (Rectangle *)RAYGUI_MALLOC(font.glyphCount*sizeof(Rectangle));\n");
            fprintf(txtFile, "    memcpy(font.recs, %sFontRecs, font.glyphCount*sizeof(Rectangle));\n\n", styleNameLower);

            fprintf(txtFile, "    // Copy font char info data from global fontChars\n");
            fprintf(txtFile, "    // NOTE: Required to avoid issues if trying to free font\n");
            fprintf(txtFile, "    font.glyphs = (GlyphInfo *)RAYGUI_MALLOC(font.glyphCount*sizeof(GlyphInfo));\n");
            fprintf(txtFile, "    memcpy(font.glyphs, %sFontGlyphs, font.glyphCount*sizeof(GlyphInfo));\n\n", styleNameLower);

            fprintf(txtFile, "    GuiSetFont(font);\n\n");

            if ((fontWhiteRec.x > 0) && (fontWhiteRec.y > 0) && (fontWhiteRec.width > 0) && (fontWhiteRec.height > 0))
            {
                fprintf(txtFile, "    // Setup a white rectangle on the font to be used on shapes drawing,\n");
                fprintf(txtFile, "    // it makes possible to draw shapes and text (full UI) in a single draw call\n");
                fprintf(txtFile, "    Rectangle fontWhiteRec = { %.0f, %.0f, %.0f, %.0f };\n", fontWhiteRec.x, fontWhiteRec.y, fontWhiteRec.width, fontWhiteRec.height);
                fprintf(txtFile, "    SetShapesTexture(font.texture, fontWhiteRec);\n\n");
            }
            else
            {
                fprintf(txtFile, "    // TODO: Setup a white rectangle on the font to be used on shapes drawing,\n");
                fprintf(txtFile, "    // it makes possible to draw shapes and text (full UI) in a single draw call\n");
                fprintf(txtFile, "    // NOTE: rGuiStyler provides a visual tool to define this rectangle on loaded font\n");
                fprintf(txtFile, "    //Rectangle fontWhiteRec = { 0, 0, 0, 0 };\n");
                fprintf(txtFile, "    //SetShapesTexture(font.texture, fontWhiteRec);\n\n");
            }
        }

        fprintf(txtFile, "    //-----------------------------------------------------------------\n\n");
        fprintf(txtFile, "    // TODO: Custom user style setup: Set specific properties here (if required)\n");
        fprintf(txtFile, "    // i.e. Controls specific BORDER_WIDTH, TEXT_PADDING, TEXT_ALIGNMENT\n");

        fprintf(txtFile, "}\n");

        fclose(txtFile);
    }
}

// Draw style controls table
static void DrawStyleControlsTable(int posX, int posY)
{
    #define TABLE_LEFT_PADDING      12
    #define TABLE_TOP_PADDING       20

    #define TABLE_CELL_HEIGHT       40
    #define TABLE_CELL_PADDING       8          // Control padding inside cell

    #define TABLE_CONTROLS_COUNT    13

    enum TableControlType {
        TYPE_LABEL = 0,
        TYPE_BUTTON,
        TYPE_TOGGLE,
        TYPE_CHECKBOX,
        TYPE_SLIDER,
        TYPE_SLIDERBAR,
        TYPE_PROGRESSBAR,
        TYPE_TOGGLESLIDER,
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
        "TOGGLESLIDER",
        "COMBOBOX",
        "DROPDOWNBOX",
        "TEXTBOX",      // TEXTBOXMULTI
        "VALUEBOX",
        "SPINNER"       // VALUEBOX + BUTTON
    };

    // Controls grid width
    int controlWidth[TABLE_CONTROLS_COUNT] = {
        100,    // LABEL
        100,    // BUTTON
        100,    // TOGGLE
        200,    // CHECKBOX
        100,    // SLIDER
        100,    // SLIDERBAR
        100,    // PROGRESSBAR
        200,    // TOGGLESLIDER
        140,    // COMBOBOX,
        160,    // DROPDOWNBOX
        100,    // TEXTBOX
        100,    // VALUEBOX
        101,    // SPINNER
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

    int sliderWidth = GuiGetStyle(SLIDER, SLIDER_WIDTH);
    GuiSetStyle(SLIDER, SLIDER_WIDTH, 10);

    // Draw left column
    //----------------------------------------------------------------------------------------
    rec = (Rectangle){ posX + TABLE_LEFT_PADDING, posY + TABLE_TOP_PADDING + TABLE_CELL_HEIGHT/2 + 20, tableStateNameWidth, TABLE_CELL_HEIGHT };

    // Draw style palette as small rectangles for easy color reference
    for (int i = 0; i < 12; i++)
    {
        DrawRectangle(rec.x + 8*i, rec.y - 14, 8, 8, GetColor((unsigned int)GuiGetStyle(0, i)));
    }

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
        rec = (Rectangle){ posX + offsetWidth - i - 1, posY + TABLE_TOP_PADDING + 20, (controlWidth[i] + TABLE_CELL_PADDING*2), TABLE_CELL_HEIGHT/2 + 1 };

        // Draw grid lines: control name
        GuiGroupBox(rec, NULL);
        int labelTextAlignment = GuiGetStyle(LABEL, TEXT_ALIGNMENT);
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiLabel(rec, tableControlsName[i]);

        // Draw specific-control color palette, only if different than default
        for (int c = 0; c < 12; c++)
        {
            switch (i)
            {
                case TYPE_LABEL: if (GuiGetStyle(LABEL, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(LABEL, c))); break;
                case TYPE_BUTTON: if (GuiGetStyle(BUTTON, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(BUTTON, c))); break;
                case TYPE_TOGGLE: if (GuiGetStyle(TOGGLE, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(TOGGLE, c))); break;
                case TYPE_CHECKBOX: if (GuiGetStyle(CHECKBOX, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(CHECKBOX, c))); break;
                case TYPE_SLIDER: if (GuiGetStyle(SLIDER, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(SLIDER, c))); break;
                case TYPE_SLIDERBAR: if (GuiGetStyle(SLIDER, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(SLIDER, c))); break;
                case TYPE_PROGRESSBAR: if (GuiGetStyle(PROGRESSBAR, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(PROGRESSBAR, c))); break;
                case TYPE_TOGGLESLIDER: if (GuiGetStyle(TOGGLE, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(TOGGLE, c))); break;
                case TYPE_COMBOBOX: if (GuiGetStyle(COMBOBOX, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(COMBOBOX, c))); break;
                case TYPE_DROPDOWNBOX: if (GuiGetStyle(DROPDOWNBOX, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(DROPDOWNBOX, c))); break;
                case TYPE_TEXTBOX: if (GuiGetStyle(TEXTBOX, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(TEXTBOX, c))); break;
                case TYPE_VALUEBOX: if (GuiGetStyle(VALUEBOX, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(VALUEBOX, c))); break;
                case TYPE_SPINNER: if (GuiGetStyle(VALUEBOX, c) != GuiGetStyle(DEFAULT, c)) DrawRectangle(rec.x + c*4, rec.y - 6, 4, 4, GetColor(GuiGetStyle(VALUEBOX, c))); break;
                default: break;
            }
        }

        rec.y += TABLE_CELL_HEIGHT/2;
        rec.height = TABLE_CELL_HEIGHT;

        float tempFloat = 40.0f;

        // Draw control 4 states: DISABLED, NORMAL, FOCUSED, PRESSED
        for (int j = 0; j < 4; j++)
        {
            // Draw grid lines: control state
            GuiGroupBox(rec, NULL);

            bool tempBool = false;
            int tempInt = 0;

            GuiSetState(j);

            // Draw control centered correctly in grid
            switch (i)
            {
                case TYPE_LABEL: GuiLabelButton((Rectangle){ rec.x, rec.y, controlWidth[i], 40 }, "#10#Label"); break;
                case TYPE_BUTTON: GuiButton((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i], 24 }, "#2#Button"); break;
                case TYPE_TOGGLE: GuiToggle((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i], 24 }, "#39#Toggle", &tempBool); break;
                case TYPE_CHECKBOX:
                {
                    GuiCheckBox((Rectangle){ rec.x + 10, rec.y + rec.height/2 - 15/2, 15, 15 }, "NoCheck", &tempBool);
                    DrawRectangle(rec.x + rec.width/2, rec.y, 1, TABLE_CELL_HEIGHT, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
                    tempBool = true;
                    GuiCheckBox((Rectangle){ rec.x + rec.width/2 + 10, rec.y + rec.height/2 - 15/2, 15, 15 }, "Checked", &tempBool);
                } break;
                case TYPE_SLIDER: GuiSlider((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 10/2, controlWidth[i], 10 }, NULL, NULL, &tempFloat, 0, 100); break;
                case TYPE_SLIDERBAR: GuiSliderBar((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 10/2, controlWidth[i], 10 }, NULL, NULL, &tempFloat, 0, 100); break;
                case TYPE_PROGRESSBAR:
                {
                    if (j < 3) GuiSetState(0);
                    tempFloat = 60;
                    GuiProgressBar((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 10/2, controlWidth[i], 10 }, NULL, NULL, &tempFloat, 0, 100);
                    GuiSetState(j);
                } break;
                case TYPE_TOGGLESLIDER:
                {
                    GuiSetStyle(SLIDER, SLIDER_PADDING, 2);
                    GuiToggleSlider((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i]/2 - TABLE_CELL_PADDING, 24 }, "#87#OFF;#83#ON", &tempInt);
                    DrawRectangle(rec.x + rec.width/2, rec.y, 1, TABLE_CELL_HEIGHT, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
                    tempInt = 1;
                    GuiToggleSlider((Rectangle){ rec.x + rec.width/2 + TABLE_CELL_PADDING, rec.y + rec.height/2 - 24/2, controlWidth[i]/2 - TABLE_CELL_PADDING, 24 }, "#87#OFF;#83#ON", &tempInt);
                    GuiSetStyle(SLIDER, SLIDER_PADDING, 1);
                } break;
                case TYPE_COMBOBOX: GuiComboBox((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i], 24 }, "#40#ComboBox;ComboBox", 0); break;
                case TYPE_DROPDOWNBOX: GuiDropdownBox((Rectangle){ rec.x + rec.width/2 - controlWidth[i]/2, rec.y + rec.height/2 - 24/2, controlWidth[i], 24 }, "#41#DropdownBox;DropdownBox", &dropdownActive, false); break;
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

    // Reset required styling properties
    GuiSetStyle(SLIDER, SLIDER_WIDTH, sliderWidth);
}

// Generate controls table image
static Image GenImageStyleControlsTable(int width, int height, const char *styleName)
{
    #define STYLE_PALETTE_TILE_SIZE  8

    RenderTexture2D target = LoadRenderTexture(width, height);

    BeginTextureMode(target);

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        // Draw style title
        GuiLabel((Rectangle){ TABLE_LEFT_PADDING, 15, 200, 20 }, TextFormat("raygui style: %s", styleName));

        DrawStyleControlsTable(0, 0);

        // Draw copyright and software info (bottom-right)
        GuiLabel((Rectangle){ TABLE_LEFT_PADDING, height - 26, 400, 10 }, "raygui style table automatically generated with rGuiStyler");
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
        GuiLabel((Rectangle){ width - 400 - TABLE_LEFT_PADDING, height - 26, 400, 10 }, "rGuiStyler created by raylib technologies (@raylibtech)");
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    EndTextureMode();

    // Generate image from render texture
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
