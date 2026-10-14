/*******************************************************************************************
*
*   Window Font Atlas
*
*   MODULE USAGE:
*       #define GUI_WINDOW_FONT_ATLAS_IMPLEMENTATION
*       #include "gui_window_font_atlas.h"
*
*       INIT: GuiWindowFontAtlasState state = InitGuiWindowFontAtlas();
*       DRAW: GuiWindowFontAtlas(&state);
*
*   LICENSE: Propietary License
*
*   Copyright (c) 2023 raylib technologies. All Rights Reserved.
*
*   Unauthorized copying of this file, via any medium is strictly prohibited
*   This project is proprietary and confidential unless the owner allows
*   usage in any other form by expresely written permission.
*
**********************************************************************************************/

#include "raylib.h"

// WARNING: raygui implementation is expected to be defined before including this header
#undef RAYGUI_IMPLEMENTATION
#include "raygui.h"

#ifndef GUI_WINDOW_FONT_ATLAS_H
#define GUI_WINDOW_FONT_ATLAS_H

typedef struct TextureView {
    Texture2D texture;
    Vector2 position;
    Vector2 prevPosition;
    Vector2 panOffset;
    float scale;
    bool panning;
} TextureView;

typedef struct {
    bool windowActive;
    bool initialized;

    bool btnLoadFontPressed;
    bool btnUnloadFontPressed;
    bool btnUnloadCharsetPressed;
    bool btnLoadCharsetPressed;
    bool fontGenSizeEditMode;
    int fontGenSizeValue;

    bool btnSaveFontAtlasPressed;

    bool selectWhiteRecActive;
    bool compressImageDataActive;
    bool compressRecDataActive;
    bool compressGlyphDataActive;

    int selectedCharset;
    int prevSelectedCharset;

    // Custom state variables (depend on development software)
    // NOTE: This variables should be added manually if required
    Rectangle fontWhiteRec;

    int *externalCodepointList;        // External charset codepoints loaded from UTF-8 file
    unsigned int externalCodepointListCount;

    bool fontAtlasRegen;

} GuiWindowFontAtlasState;

// Default control state
#define GUI_WINDOW_FONT_ATLAS_STATE_DEFAULT \
    { \
        .fontGenSizeValue = 10, \
        .compressImageDataActive = true, \
        .compressRecDataActive = true, \
        .compressGlyphDataActive = true, \
        .selectedCharset = 1, \
        .prevSelectedCharset = 1, \
    }

#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
void GuiWindowFontAtlas(Rectangle bounds, const char *text, GuiWindowFontAtlasState *state);

#ifdef __cplusplus
}
#endif

#endif // GUI_WINDOW_FONT_ATLAS_H

/***********************************************************************************
*
*   GUI_WINDOW_FONT_ATLAS IMPLEMENTATION
*
************************************************************************************/
#if defined(GUI_WINDOW_FONT_ATLAS_IMPLEMENTATION)

#include "raygui.h"

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
// Basic charset (95 codepoints)
static const char *charsetBasic = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
// Default charset: ISO-8859-15 (213 codepoints)
static const char *charsetDefault = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~¡¢£€¥Š§š©ª«¬®¯°±²³Žµ¶·ž¹º»ŒœŸ¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ";

static TextureView fontAtlasView = { 0 };
static Rectangle fontAtlasRec = { 0 };

static Rectangle fontWhiteRecScreen = { 0 };
static Vector2 fontWhiteRecStartPos = { 0 };
static bool prevSelectWhiteRecActive = false;
static int prevFontGenSizeValue = 10;

// Custom font variables
// NOTE: They have to be global to be used by style export functions
static Font customFont = { 0 };             // Custom font
static bool customFontLoaded = false;       // Custom font loaded flag (from font file or style file)
static char inFontFileName[512] = { 0 };    // Input font file name (required for font reloading on atlas regeneration)

static int *codepointList = NULL;           // Custom codepoint list
static int codepointListCount = 0;          // Custom codepoint list count

//----------------------------------------------------------------------------------
// Internal Module Functions Definition
//----------------------------------------------------------------------------------
// Scale/zoom texture view to target point (mouse)
static void ScaleTextureView(TextureView *view, float scaleFactor, Vector2 targetPoint);

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
void GuiWindowFontAtlas(Rectangle bounds, const char *text, GuiWindowFontAtlasState *state)
{
    if (state->windowActive)
    {
        // Update logic
        //--------------------------------------------------------------------------------------------------
        // Check if selected size actually changed to force atlas regen
        if ((prevFontGenSizeValue != state->fontGenSizeValue) && !state->fontGenSizeEditMode && (inFontFileName[0] != '\0')) state->fontAtlasRegen = true;

        Vector2 mousePosition = GetMousePosition();

        if (state->btnUnloadFontPressed)
        {
            memset(inFontFileName, 0, 512);
            customFontLoaded = false;
        }
        else if (state->btnUnloadCharsetPressed)
        {
            RL_FREE(state->externalCodepointList);
            state->externalCodepointList = NULL;
            state->externalCodepointListCount = 0;
            state->selectedCharset = 0;
            state->fontAtlasRegen = true;
        }

        if (IsKeyPressed(KEY_SPACE)) state->selectWhiteRecActive = !state->selectWhiteRecActive;

        if (!prevSelectWhiteRecActive && state->selectWhiteRecActive)
        {
            fontWhiteRecScreen.x = fontAtlasRec.x + state->fontWhiteRec.x*fontAtlasView.scale;
            fontWhiteRecScreen.y = fontAtlasRec.y + state->fontWhiteRec.y*fontAtlasView.scale,
                fontWhiteRecScreen.width = state->fontWhiteRec.width*fontAtlasView.scale;
            fontWhiteRecScreen.height = state->fontWhiteRec.height*fontAtlasView.scale;
        }

        if (state->selectWhiteRecActive && CheckCollisionPointRec(mousePosition, (Rectangle){ bounds.x, bounds.y + 64, bounds.width, bounds.height - 64 }))
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                fontWhiteRecStartPos = mousePosition;

                fontWhiteRecScreen.x = mousePosition.x;
                fontWhiteRecScreen.y = mousePosition.y;
            }
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
            {
                fontWhiteRecScreen.width = mousePosition.x - fontWhiteRecStartPos.x;
                fontWhiteRecScreen.height = mousePosition.y - fontWhiteRecStartPos.y;

                // Take care of rectangles drawn in different directions
                if (fontWhiteRecScreen.width < 0)
                {
                    fontWhiteRecScreen.x = mousePosition.x;
                    fontWhiteRecScreen.width *= -1;
                }

                if (fontWhiteRecScreen.height < 0)
                {
                    fontWhiteRecScreen.y = mousePosition.y;
                    fontWhiteRecScreen.height *= -1;
                }
            }

            // Get final rectangle from selection
            state->fontWhiteRec.x = (fontWhiteRecScreen.x - fontAtlasView.position.x)/fontAtlasView.scale;
            state->fontWhiteRec.y = (fontWhiteRecScreen.y - fontAtlasView.position.y)/fontAtlasView.scale;
            state->fontWhiteRec.width = fontWhiteRecScreen.width/fontAtlasView.scale;
            state->fontWhiteRec.height = fontWhiteRecScreen.height/fontAtlasView.scale;
            if (state->fontWhiteRec.x < 0) state->fontWhiteRec.x = 0;
            if (state->fontWhiteRec.y < 0) state->fontWhiteRec.y = 0;

            if (IsKeyPressed(KEY_ENTER))
            {
                state->selectWhiteRecActive = false;
                prevSelectWhiteRecActive = false;
            }
        }
        else
        {
            if ((state->fontWhiteRec.width <= 0) || (state->fontWhiteRec.height <= 0)) state->fontWhiteRec = (Rectangle){ 0 };

            // Only zoom atlas when mouse over
            // NOTE: Zoom to mouse position, instead of center of atlas
            if (CheckCollisionPointRec(mousePosition, bounds))
            {
                // Scale/zoom atlas texture
                float wheelDelta = GetMouseWheelMove();
                if ((int)wheelDelta != 0)
                {
                    float scaleFactor = wheelDelta*0.5f;
                    if (IsKeyDown(KEY_LEFT_SHIFT)) scaleFactor = wheelDelta;

                    ScaleTextureView(&fontAtlasView, scaleFactor, mousePosition);
                }

                // Font atlas panning with mouse logic
                if (CheckCollisionPointRec(mousePosition, fontAtlasRec))
                {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    {
                        fontAtlasView.panning = true;
                        fontAtlasView.panOffset = mousePosition;
                        fontAtlasView.prevPosition = fontAtlasView.position;
                    }
                }
                if (fontAtlasView.panning)
                {
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                    {
                        fontAtlasView.position.x = fontAtlasView.prevPosition.x + ((mousePosition.x - fontAtlasView.panOffset.x));
                        fontAtlasView.position.y = fontAtlasView.prevPosition.y + ((mousePosition.y - fontAtlasView.panOffset.y));
                    }

                    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) fontAtlasView.panning = false;
                }
            }

            if (IsKeyPressed(KEY_F) || IsWindowResized())
            {
                // Reset font atlas view position and scale
                fontAtlasView.scale = 1.0f;
                fontAtlasView.position.x = bounds.x + bounds.width/2 - fontAtlasView.texture.width*fontAtlasView.scale/2;
                fontAtlasView.position.y = bounds.y + bounds.height/2 - fontAtlasView.texture.height*fontAtlasView.scale/2;
                fontAtlasView.prevPosition = fontAtlasView.position;

                prevSelectWhiteRecActive = false;
                state->selectWhiteRecActive = false;

                prevFontGenSizeValue = state->fontGenSizeValue;
            }

            if (state->prevSelectedCharset != state->selectedCharset)
            {
                if (state->prevSelectedCharset != 2) UnloadCodepoints(codepointList);

                if (state->selectedCharset == 0) codepointList = LoadCodepoints(charsetBasic, &codepointListCount);
                else if (state->selectedCharset == 1) codepointList = LoadCodepoints(charsetDefault, &codepointListCount);
                else if (state->selectedCharset == 2)
                {
                    if (state->externalCodepointList != NULL)
                    {
                        codepointList = state->externalCodepointList;
                        codepointListCount = state->externalCodepointListCount;
                    }
                }

                state->fontAtlasRegen = true;
            }
        }

        // Calculate font atlas rectangle (considering transformations)
        fontAtlasRec = (Rectangle){
            fontAtlasView.position.x,
            fontAtlasView.position.y,
            fontAtlasView.texture.width*fontAtlasView.scale,
            fontAtlasView.texture.height*fontAtlasView.scale
        };

        // Reload font and generate new atlas at new size when required
        if (state->fontAtlasRegen)
        {
            if (inFontFileName[0] != '\0')
            {
                // Load new font file
                Font tempFont = LoadFontEx(inFontFileName, state->fontGenSizeValue, codepointList, codepointListCount);

                if (tempFont.texture.id > 0)
                {
                    if (customFontLoaded) UnloadFont(customFont);   // Unload previously loaded font
                    customFont = tempFont;
                    GuiSetFont(customFont);
                    fontAtlasView.texture = customFont.texture;

                    // NOTE: Generated fonts have a white rectangle at the bottom-right corner by default,
                    // 3x3 pixels, to be used for shapes rectangle
                    state->fontWhiteRec = (Rectangle){ (float)customFont.texture.width - 2, (float)customFont.texture.height - 2, 1, 1 };

                    customFontLoaded = true;
                }
                else memset(inFontFileName, 0, 512);
            }
            else
            {
                // Reset to default font
                if (customFontLoaded) UnloadFont(customFont);
                customFont = GetFontDefault();
                GuiSetFont(customFont);
                fontAtlasView.texture = customFont.texture;

                Rectangle whiteRec = customFont.recs[95];
                state->fontWhiteRec = (Rectangle){ whiteRec.x + 2, whiteRec.y + 2, 1, 1 };

                GuiSetStyle(DEFAULT, TEXT_SPACING, 1);

                customFontLoaded = false;
            }

            // Set shapes texture and rectangle
            SetShapesTexture(customFont.texture, state->fontWhiteRec);

            // Reset font atlas view position and scale
            fontAtlasView.scale = 1.0f;
            fontAtlasView.position.x = bounds.x + bounds.width/2 - fontAtlasView.texture.width*fontAtlasView.scale/2;
            fontAtlasView.position.y = bounds.y + bounds.height/2 - fontAtlasView.texture.height*fontAtlasView.scale/2;
            fontAtlasView.prevPosition = fontAtlasView.position;

            prevSelectWhiteRecActive = false;
            state->selectWhiteRecActive = false;
            prevFontGenSizeValue = state->fontGenSizeValue;

            state->fontAtlasRegen = false;  // Reset regen flag
        }

        // Recalculate font window bounds
        bounds.width = GetScreenWidth() - bounds.x - 160 - 20;
        //----------------------------------------------------------------------------------------------------------------------

        // Draw window logic
        //--------------------------------------------------------------------------------------------------
        state->windowActive = !GuiWindowBox(bounds, text);

        // White rectangle selection border
        if (state->selectWhiteRecActive)
        {
            DrawRectangleLinesEx((Rectangle){ bounds.x, bounds.y + 64, bounds.width, bounds.height - 64 - 68 - 20 }, 4, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_PRESSED)));
        }

        // Draw font atlas view
        BeginScissorMode(bounds.x + 1, bounds.y + 24 + 40, bounds.width - 2, bounds.height - 65 - 68 - 18);
            DrawRectangleRec(fontAtlasRec, BLACK);
            DrawTexturePro(fontAtlasView.texture,
                (Rectangle){ 0, 0, (float)fontAtlasView.texture.width, (float)fontAtlasView.texture.height },
                fontAtlasRec, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
            DrawRectangleLinesEx(fontAtlasRec, 1.0f, Fade(RED, 0.6f));

            if (state->selectWhiteRecActive)
            {
                DrawRectangleLinesEx(fontWhiteRecScreen, 1.0f, RED);

                // Draw values for convenience
                DrawTextEx(GuiGetFont(), TextFormat("[%i, %i]", (int)state->fontWhiteRec.x, (int)state->fontWhiteRec.y),
                    (Vector2){ fontWhiteRecScreen.x - 20, fontWhiteRecScreen.y - 20 }, GuiGetStyle(DEFAULT, TEXT_SIZE),
                    GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED)));
                DrawTextEx(GuiGetFont(), TextFormat("[%i, %i]", (int)state->fontWhiteRec.width, (int)state->fontWhiteRec.height),
                    (Vector2){ fontWhiteRecScreen.x + fontWhiteRecScreen.width - 20, fontWhiteRecScreen.y + fontWhiteRecScreen.height + 20 },
                    GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED)));
            }
            else
            {
                DrawRectangleRec((Rectangle){
                    fontAtlasRec.x + state->fontWhiteRec.x*fontAtlasView.scale,
                    fontAtlasRec.y + state->fontWhiteRec.y*fontAtlasView.scale,
                    state->fontWhiteRec.width*fontAtlasView.scale, state->fontWhiteRec.height*fontAtlasView.scale },
                    GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED)));
            }
        EndScissorMode();

        GuiLine((Rectangle){ bounds.x + 0, bounds.y + 24 + 40 - 2, bounds.width, 2 }, NULL);

        GuiEnableTooltip();
        GuiSetTooltip("Load font file");
        state->btnLoadFontPressed = GuiButton((Rectangle){ bounds.x + 12, bounds.y + 32, 24, 24 }, "#30#");
        if (inFontFileName[0] == '\0') GuiDisable();
        GuiSetTooltip("Unload font file");
        state->btnUnloadFontPressed = GuiButton((Rectangle){ bounds.x + 12 + 28, bounds.y + 32, 24, 24 }, "#9#");
        GuiEnable();
        GuiSetTooltip("Save font atlas image");
        state->btnSaveFontAtlasPressed = GuiButton((Rectangle){ bounds.x + 12 + 28 + 28, bounds.y + 32, 24, 24 }, "#12#");

        if (!FileExists(inFontFileName)) GuiDisable();
        GuiDisableTooltip();
        prevFontGenSizeValue = state->fontGenSizeValue;
        if (GuiSpinner((Rectangle){ bounds.x + 164, bounds.y + 32, 96, 24 }, "Gen Size: ", &state->fontGenSizeValue, 0, 100, state->fontGenSizeEditMode)) state->fontGenSizeEditMode = !state->fontGenSizeEditMode;
        GuiEnableTooltip();
        //GuiSetTooltip("Regenerate font atlas");
        //if (GuiButton((Rectangle){ state->anchor.x + 210, state->anchor.y + 32, 80, 24 }, "#142#Regen")) state->fontAtlasRegen = true;
        GuiEnable();

        DrawLine(bounds.x + 260 + 12, bounds.y + 24, bounds.x + 260 + 12, bounds.y + 24 + 40, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));

        if (!FileExists(inFontFileName)) GuiDisable();
        GuiSetTooltip("Load custom charset file");
        state->btnLoadCharsetPressed = GuiButton((Rectangle){ bounds.x + 284, bounds.y + 32, 24, 24 }, "#31#");
        if (state->externalCodepointList == NULL) GuiDisable();
        GuiSetTooltip("Unload custom charset file");
        state->btnUnloadCharsetPressed = GuiButton((Rectangle){ bounds.x + 312, bounds.y + 32, 24, 24 }, "#9#");
        if (FileExists(inFontFileName)) GuiEnable();
        state->prevSelectedCharset = state->selectedCharset;
        GuiSetTooltip("Select charset");
        GuiLabel((Rectangle){ bounds.x + 350, bounds.y + 32, 60, 24 }, "Charset: ");
        int comboBoxWidth = bounds.width - 350 - 60 - 196;
        GuiComboBox((Rectangle){ bounds.x + 348 + 56, bounds.y + 32, (float)comboBoxWidth, 24 }, (state->externalCodepointList != NULL)? "Basic;ISO-8859-15;Custom" : "Basic;ISO-8859-15", &state->selectedCharset);
        GuiEnable();

        DrawLine(bounds.x + bounds.width - 188, bounds.y + 24, bounds.x + bounds.width - 188, bounds.y + 24 + 40, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));

        GuiLabel((Rectangle){ bounds.x + bounds.width - 176, bounds.y + 32, 100, 24 }, "Shapes rec: ");
        GuiSetTooltip("Set bottom-right corner rectangle");
        if (GuiButton((Rectangle){ bounds.x + bounds.width - 176 + 82, bounds.y + 32, 24, 24 }, "#84#"))
        {
            // SOLUTION: Always add a white rectangle at the bottom-right corner, 3x3 pixels -> Added by raylib LoadFontEx()
            state->fontWhiteRec = (Rectangle){ fontAtlasView.texture.width - 2, fontAtlasView.texture.height - 2, 1, 1 };
        }
        GuiSetTooltip("Clear shapes rectangle");
        if (GuiButton((Rectangle){ bounds.x + bounds.width - 176 + 82 + 24 + 4, bounds.y + 32, 24, 24 }, "#79#"))
        {
            state->fontWhiteRec = (Rectangle){ 0 };

            // Reset shapes texture and rectangle
            SetShapesTexture((Texture2D){ 0 }, (Rectangle){ 0 });
        }
        GuiSetTooltip("Toggle shapes rectangle selection (SPACE)");
        prevSelectWhiteRecActive = state->selectWhiteRecActive;
        GuiToggle((Rectangle){ bounds.x + bounds.width - 176 + 82 + 48 + 8, bounds.y + 32, 24, 24 }, "#80#", &state->selectWhiteRecActive);

        GuiSetTooltip(NULL);

        //GuiToggle((Rectangle){ state->anchor.x + 360, state->anchor.y + 32, 24, 24 }, "#178#", &state->compressImageDataActive);
        //GuiToggle((Rectangle){ state->anchor.x + 360 + 24 + 4, state->anchor.y + 32, 24, 24 }, "#179#", &state->compressRecDataActive);
        //GuiToggle((Rectangle){ state->anchor.x + 360 + 48 + 8, state->anchor.y + 32, 24, 24 }, "#180#", &state->compressGlyphDataActive);
        //--------------------------------------------------------------------------------------------------
    }
    else
    {
        // Reset font atlas view position and scale
        fontAtlasView.scale = 1.0f;
        fontAtlasView.position.x = bounds.x + bounds.width/2 - fontAtlasView.texture.width*fontAtlasView.scale/2;
        fontAtlasView.position.y = bounds.y + bounds.height/2 - fontAtlasView.texture.height*fontAtlasView.scale/2;
        fontAtlasView.prevPosition = fontAtlasView.position;

        prevSelectWhiteRecActive = false;
        state->selectWhiteRecActive = false;

        prevFontGenSizeValue = state->fontGenSizeValue;
    }
}

// Scale/zoom texture view to target point (mouse)
static void ScaleTextureView(TextureView *view, float scaleFactor, Vector2 targetPoint)
{
    float prevWidth = (float)view->texture.width*view->scale;
    float prevHeight = (float)view->texture.height*view->scale;

    // Update scale by mouse factor
    view->scale += scaleFactor;

    if (view->scale < 1.0f) view->scale = 1.0f;
    else if (view->scale > 20.0f) view->scale = 20.0f;

    // Calculate the new width and height
    float newWidth = (float)view->texture.width*view->scale;
    float newHeight = (float)view->texture.height*view->scale;

    // Calculate the change in width and height
    float deltaWidth = newWidth - prevWidth;
    float deltaHeight = newHeight - prevHeight;

    // Calculate the relative position of the point within the rectangle
    float relativeX = (targetPoint.x - view->position.x)/prevWidth;
    float relativeY = (targetPoint.y - view->position.y)/prevHeight;

    // Update position as required
    view->position.x -= deltaWidth*relativeX;
    view->position.x = floorf(view->position.x);
    view->position.y -= deltaHeight*relativeY;
    view->position.y = floorf(view->position.y);
    view->prevPosition = view->position;
}

#endif // GUI_WINDOW_FONT_ATLAS_IMPLEMENTATION
