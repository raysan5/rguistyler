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

typedef struct {
    Vector2 anchor;
    
    bool windowActive;

    bool btnLoadFontPressed;
    bool btnLoadCharsetPressed;
    bool fontGenSizeEditMode;
    int fontGenSizeValue;

    bool btnExportFontAtlasPressed;
    bool btnCropAtlasPressed;

    bool selectWhiteRecActive;
    bool compressImageDataActive;
    bool compressRecDataActive;
    bool compressGlyphDataActive;

    // Custom state variables (depend on development software)
    // NOTE: This variables should be added manually if required
    Texture2D texFont;

} GuiWindowFontAtlasState;

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
GuiWindowFontAtlasState InitGuiWindowFontAtlas(void);
void GuiWindowFontAtlas(GuiWindowFontAtlasState *state);

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
static Rectangle fontAtlasRec = { 0 };
static Vector2 fontAtlasPosition = { 0 };
static Vector2 prevFontAtlasPosition = { 0 };
static Vector2 fontAtlasOffset = { 0 };
static float fontAtlasScale = 1.0f;
static bool panningMode = false;

//----------------------------------------------------------------------------------
// Internal Module Functions Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
GuiWindowFontAtlasState InitGuiWindowFontAtlas(void)
{
    GuiWindowFontAtlasState state = { 0 };

    state.anchor = (Vector2){ 12, 48 };
    
    state.windowActive = false;

    state.btnLoadFontPressed = false;
    state.btnLoadCharsetPressed = false;
    state.fontGenSizeEditMode = false;
    state.fontGenSizeValue = 10;
    state.btnExportFontAtlasPressed = false;
    state.btnCropAtlasPressed = false;
    state.selectWhiteRecActive = false;

    state.compressImageDataActive = true;
    state.compressRecDataActive = true;
    state.compressGlyphDataActive = true;

    // Custom variables initialization

    return state;
}

void GuiWindowFontAtlas(GuiWindowFontAtlasState *state)
{
    if (state->windowActive)
    {
        state->windowActive = !GuiWindowBox((Rectangle){ state->anchor.x, state->anchor.y, 724, 532 }, "#30# Font Atlas Generation");
        
        state->btnLoadFontPressed = GuiButton((Rectangle){ state->anchor.x + 8, state->anchor.y + 32, 24, 24 }, "#30#"); 
        state->btnLoadCharsetPressed = GuiButton((Rectangle){ state->anchor.x + 36, state->anchor.y + 32, 24, 24 }, "#31#"); 
        state->btnExportFontAtlasPressed = GuiButton((Rectangle){ state->anchor.x + 64, state->anchor.y + 32, 24, 24 }, "#7#");

        if (GuiSpinner((Rectangle){ state->anchor.x + 168, state->anchor.y + 32, 96, 24 }, "Gen Size: ", &state->fontGenSizeValue, 0, 100, state->fontGenSizeEditMode)) state->fontGenSizeEditMode = !state->fontGenSizeEditMode;
        GuiToggle((Rectangle){ state->anchor.x + 284, state->anchor.y + 32, 24, 24 }, "#79#", &state->selectWhiteRecActive);

        state->btnCropAtlasPressed = GuiButton((Rectangle){ state->anchor.x + 312, state->anchor.y + 32, 24, 24 }, "#38#");

        GuiToggle((Rectangle){ state->anchor.x + 360, state->anchor.y + 32, 24, 24 }, "#178#", &state->compressImageDataActive);
        GuiToggle((Rectangle){ state->anchor.x + 360 + 24 + 4, state->anchor.y + 32, 24, 24 }, "#179#", &state->compressRecDataActive);
        GuiToggle((Rectangle){ state->anchor.x + 360 + 48 + 8, state->anchor.y + 32, 24, 24 }, "#180#", &state->compressGlyphDataActive);

        GuiLine((Rectangle){ state->anchor.x + 0, state->anchor.y + 24 + 40 - 2, 724, 2 }, NULL);

        GuiStatusBar((Rectangle){ state->anchor.x + 0, state->anchor.y + 531, 217, 24 }, "File: Mecha.ttf");
        GuiStatusBar((Rectangle){ state->anchor.x + 216, state->anchor.y + 531, 145, 24 }, "Codepoints: 222");
        GuiStatusBar((Rectangle){ state->anchor.x + 360, state->anchor.y + 531, 161, 24 }, "Atlas Size: 512x512");
        GuiStatusBar((Rectangle){ state->anchor.x + 520, state->anchor.y + 531, 204, 24 }, "White rec: [23, 34, 2, 4]");

        // Update and draw font texture
        //--------------------------------------------------------------------------------------------------
        fontAtlasScale += GetMouseWheelMove();
        if (fontAtlasScale < 1.0f) fontAtlasScale = 1.0f;
        else if (fontAtlasScale > 16.0f) fontAtlasScale = 16.0f;

        // Calculate font atlas rectangle (considering transformations)
        fontAtlasRec = (Rectangle){ fontAtlasPosition.x - state->texFont.width*fontAtlasScale/2, 
            fontAtlasPosition.y - state->texFont.height*fontAtlasScale/2,
            state->texFont.width*fontAtlasScale, state->texFont.height*fontAtlasScale };

        // Font atlas panning with mouse logic
        if (CheckCollisionPointRec(GetMousePosition(), fontAtlasRec))
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                panningMode = true;
                fontAtlasOffset = GetMousePosition();
                prevFontAtlasPosition = fontAtlasPosition;
            }
        }
        if (panningMode)
        {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                fontAtlasPosition.x = prevFontAtlasPosition.x + (GetMouseX() - fontAtlasOffset.x);
                fontAtlasPosition.y = prevFontAtlasPosition.y + (GetMouseY() - fontAtlasOffset.y);
            }

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) panningMode = false;
        }

        BeginScissorMode(state->anchor.x + 1, state->anchor.y + 24 + 40, 724 - 2, 532 - 65);
            DrawRectangleRec(fontAtlasRec, BLACK);
            DrawRectangleLinesEx(fontAtlasRec, 1.0f, RED);
            DrawTexturePro(state->texFont, (Rectangle){ 0, 0, state->texFont.width, state->texFont.height }, fontAtlasRec, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
        EndScissorMode();
        //--------------------------------------------------------------------------------------------------
    }
    else
    {
        fontAtlasScale = 1.0f;
        fontAtlasPosition.x = state->anchor.x + 724/2;
        fontAtlasPosition.y = state->anchor.y + 532/2;
        prevFontAtlasPosition = fontAtlasPosition;
    }
}

#endif // GUI_WINDOW_FONT_ATLAS_IMPLEMENTATION
