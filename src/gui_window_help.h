/*******************************************************************************************
*
*   Window Help
*
*   MODULE USAGE:
*       #define GUI_WINDOW_HELP_IMPLEMENTATION
*       #include "gui_window_help.h"
*
*   On game init call:  GuiWindowHelpState state = InitGuiWindowHelp();
*   On game draw call:  GuiWindowHelp(&state);
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2022-2024 raylib technologies (@raylibtech) / Ramon Santamaria (@raysan5)
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

// WARNING: raygui implementation is expected to be defined before including this header

#ifndef GUI_WINDOW_HELP_H
#define GUI_WINDOW_HELP_H

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Gui window structure declaration
typedef struct {
    bool windowActive;

    Rectangle windowBounds;
    Vector2 panOffset;
    bool dragMode;
    bool supportDrag;
    
    int contentHeight;
    Vector2 scrollPanelOffset;

} GuiWindowHelpState;

#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
GuiWindowHelpState InitGuiWindowHelp(void);
void GuiWindowHelp(GuiWindowHelpState *state);

#ifdef __cplusplus
}
#endif

#endif // GUI_WINDOW_HELP_H

/***********************************************************************************
*
*   GUI_WINDOW_HELP IMPLEMENTATION
*
************************************************************************************/

#if defined(GUI_WINDOW_HELP_IMPLEMENTATION)

#include "raygui.h"

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define GUIHELPWINDOW_MAX_LINES            128

#define GUIHELPWINDOW_LINE_HEIGHT           24
#define GUIHELPWINDOW_LINE_EMPTY_HEIGHT     12

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
// Tool help info
static const char *helpLines[] = {
    "F1 - Show Help window",
    "F2 - Show About window",
    "F3 - Show Issue Report window",
    //"F4 - Show User window",
    "-File Controls",
    "LCTRL + N - New style file (.rgs)",
    "LCTRL + O - Open style file (.rgs)",
    "LCTRL + S - Save style file (.rgs)",
    "LCTRL + E - Export style file",
    "-Tool Controls",
    "F5 - Show Style table",
    "F6 - Show Font atlas",
    "LCTRL + R - Reload style template",
    "-",
    "ESCAPE - Close Window/Exit",
    NULL
};

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Init window Help
GuiWindowHelpState InitGuiWindowHelp(void)
{
    GuiWindowHelpState state = { 0 };

    state.windowActive = false;
    state.supportDrag = false;

    state.windowBounds = (Rectangle){ (float)GetScreenWidth()/2 - 360/2, 0, 360, 0 };
    state.panOffset = (Vector2){ 0, 0 };
    state.dragMode = false;

    // Calculate content height
    state.contentHeight = 0;
    for (int i = 0; i < GUIHELPWINDOW_MAX_LINES; i++)
    {
        if (helpLines[i] == NULL) break;
        else if ((helpLines[i][0] == '-') && (helpLines[i][1] == '\0'))  state.contentHeight += GUIHELPWINDOW_LINE_EMPTY_HEIGHT;
        else state.contentHeight += GUIHELPWINDOW_LINE_HEIGHT;
    }
    state.contentHeight += 8;    // Marging at the end

    // Calculate window height if not specifically defined
    if (state.windowBounds.height == 0) state.windowBounds.height = (float)(state.contentHeight + 24 + 4);
    state.windowBounds.y = GetScreenHeight()/2 - state.windowBounds.height/2;

    // Review size if it does not fit on the screen
    if (state.windowBounds.height > (GetScreenHeight() - 80))
    {
        state.windowBounds.height = (float)GetScreenHeight() - 80;
        state.windowBounds.y = GetScreenHeight()/2 - state.windowBounds.height/2;
    }
   
    // Init scroll offset
    state.scrollPanelOffset = (Vector2){ 0, 0 };

    return state;
}

// Gui window help
void GuiWindowHelp(GuiWindowHelpState *state)
{
    if (state->windowActive)
    {
        // Update window dragging
        //----------------------------------------------------------------------------------------
        if (state->supportDrag)
        {
            Vector2 mousePosition = GetMousePosition();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                // Window can be dragged from the top window bar
                if (CheckCollisionPointRec(mousePosition, (Rectangle){ state->windowBounds.x, state->windowBounds.y, state->windowBounds.width, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT }))
                {
                    state->dragMode = true;
                    state->panOffset.x = mousePosition.x - state->windowBounds.x;
                    state->panOffset.y = mousePosition.y - state->windowBounds.y;
                }
            }

            if (state->dragMode)
            {
                state->windowBounds.x = (mousePosition.x - state->panOffset.x);
                state->windowBounds.y = (mousePosition.y - state->panOffset.y);

                // Check screen limits to avoid moving out of screen
                if (state->windowBounds.x < 0) state->windowBounds.x = 0;
                else if (state->windowBounds.x > (GetScreenWidth() - state->windowBounds.width)) state->windowBounds.x = GetScreenWidth() - state->windowBounds.width;

                if (state->windowBounds.y < 40) state->windowBounds.y = 40;
                else if (state->windowBounds.y > (GetScreenHeight() - state->windowBounds.height - 24)) state->windowBounds.y = GetScreenHeight() - state->windowBounds.height - 24;

                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) state->dragMode = false;
            }
        }
        //----------------------------------------------------------------------------------------

        // Update window bounds and size: center to screen and allow scrolling
        //----------------------------------------------------------------------------------------
        state->windowBounds.x = GetScreenWidth()/2 - state->windowBounds.width/2;
        state->windowBounds.height = (float)(state->contentHeight + 24 + 4);
        state->windowBounds.y = GetScreenHeight()/2 - state->windowBounds.height/2;

        // Review size if it does not fit on the screen
        if (state->windowBounds.height > (GetScreenHeight() - 80))
        {
            state->windowBounds.height = (float)GetScreenHeight() - 80;
            state->windowBounds.y = GetScreenHeight()/2 - state->windowBounds.height/2;
        }
        //----------------------------------------------------------------------------------------

        // Draw window and controls
        //----------------------------------------------------------------------------------------
        state->windowActive = !GuiWindowBox(state->windowBounds, TextFormat("#193#%s Help", TOOL_NAME));

        int nextLineY = (GUIHELPWINDOW_LINE_HEIGHT + 4);

        // Draw scroll panel considering window bounds and content size (only if required)
        Rectangle scissor = { 0 };
        if (state->contentHeight > (state->windowBounds.height - 24))
        {
            GuiScrollPanel((Rectangle){ state->windowBounds.x, state->windowBounds.y + GUIHELPWINDOW_LINE_HEIGHT - 1, state->windowBounds.width, state->windowBounds.height - 24 + 1 }, NULL,
                       (Rectangle){ state->windowBounds.x, state->windowBounds.y + GUIHELPWINDOW_LINE_HEIGHT, state->windowBounds.width - 16, (float)state->contentHeight }, &state->scrollPanelOffset, &scissor);
        }

        // Draw help info and separation lines
        // WARNING: We only scissor if scrolling is required, scissor mode forces a new draw call
        if (state->contentHeight > (state->windowBounds.height - 24)) BeginScissorMode((int)scissor.x, (int)scissor.y, (int)scissor.width + 2, (int)scissor.height);

            for (int i = 0; i < GUIHELPWINDOW_MAX_LINES; i++)
            {
                if (helpLines[i] == NULL) break;
                else if ((helpLines[i][0] == '-') && (helpLines[i][1] == '\0')) GuiLine((Rectangle){ state->windowBounds.x, state->windowBounds.y + nextLineY + state->scrollPanelOffset.y, state->windowBounds.width, GUIHELPWINDOW_LINE_EMPTY_HEIGHT }, NULL);
                else if (helpLines[i][0] == '-') GuiLine((Rectangle){ state->windowBounds.x, state->windowBounds.y + nextLineY + state->scrollPanelOffset.y, state->windowBounds.width, GUIHELPWINDOW_LINE_HEIGHT }, helpLines[i] + 1);
                else GuiLabel((Rectangle){ state->windowBounds.x + 12, state->windowBounds.y + nextLineY + state->scrollPanelOffset.y, state->windowBounds.width, GUIHELPWINDOW_LINE_HEIGHT }, helpLines[i]);

                if ((helpLines[i][0] == '-') && (helpLines[i][1] == '\0')) nextLineY += GUIHELPWINDOW_LINE_EMPTY_HEIGHT;
                else nextLineY += GUIHELPWINDOW_LINE_HEIGHT;
            }

        if (state->contentHeight > (state->windowBounds.height - 24)) EndScissorMode();
    }
}

#endif // GUI_WINDOW_HELP_IMPLEMENTATION