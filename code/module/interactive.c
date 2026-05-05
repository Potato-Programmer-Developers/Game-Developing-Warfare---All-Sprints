/**
 * @file interactive.c
 * @brief Implementation of the UI system (Buttons and Sliders).
 * 
 * Update History:
 * - 2026-03-24: Initial creation of the UI system using static PNG buttons. (Goal: 
 *                Provide basic navigation for the Main Menu.)
 * - 2026-04-05: Finalized the Volume Slider logic. (Goal: Connect the settings UI 
 *                directly to the global Audio system.)
 * - 2026-04-06: Comprehensive "Asset-Light" & "Dynamic Scaling" Refactor. (Goal: Eliminate 
 *                static textures and implement resolution-independent UI layout logic.)
 * - 2026-05-02: Fixed ghost "Continue" button after credits delete save data. (Goal: Ensure
 *                the `continue_bounds` are explicitly reset when no save file exists, preventing
 *                stale hover hitboxes on the Main Menu.)
 * 
 * Revision Details:
 * - Cleaned up `InitInteractive` to remove all `LoadTexture` calls for menu logic.
 * - Refactored `UpdateInteractiveLayout` to use absolute pixel coordinates (370, 290, etc.) 
 *    for menu buttons, ensuring pixel-perfect alignment with the new `.qoi` animations.
 * - Updated `UpdateInteractive` to handle collision detection for the new layout 
 *    without relying on texture dimensions.
 * - Removed the `CloseInteractive` implementation and all associated texture unloads.
 * - Refactored `UpdateInteractiveLayout` to scale button `x`, `y`, `width`, and `height` 
 *    dynamically based on the current `GetScreenWidth()` and `GetScreenHeight()`.
 * - Replaced hardcoded button coordinates with a scaling engine using `REF_WIDTH` (1200) 
 *    and `REF_HEIGHT` (800) as a baseline.
 * - Added `interactive->continue_bounds = (Rectangle){0, 0, 0, 0}` in `UpdateInteractiveLayout`
 *    when `data.dat` is missing to eliminate "ghost" hitboxes.
 * 
 * Authors: Andrew Zhuo
 */

#include "interactive.h"
#include "raylib.h"
#include "raymath.h"

#define REF_WIDTH 1200.0f
#define REF_HEIGHT 800.0f

Interactive InitInteractive(Settings* game_settings){
    Interactive new_interactive = {0};

    // Geometry Setup (Constants)
    new_interactive.bar_width = 770.0f;
    new_interactive.bar_height = 5.0f;
    new_interactive.knob_width = 100.0f;
    new_interactive.knob_height = 120.0f;

    // Initial Layout pass
    UpdateInteractiveLayout(&new_interactive, MAINMENU, game_settings);
    
    return new_interactive;
}

void UpdateInteractiveLayout(Interactive* interactive, int game_state, Settings* game_settings){
    // Get current screen dimensions
    float screen_width = (float)GetScreenWidth();
    float screen_height = (float)GetScreenHeight();
    
    // Calculate scale factors relative to the original reference resolution (1200x800)
    float scale_x = screen_width / REF_WIDTH;
    float scale_y = screen_height / REF_HEIGHT;

    if (game_state == MAINMENU){
        if (FileExists("../data/data.dat")){
            // Reference layout for New/Continue buttons
            interactive->continue_bounds = (Rectangle){140 * scale_x, 290 * scale_y, 460 * scale_x, 120 * scale_y};
            interactive->new_game_bounds = (Rectangle){630 * scale_x, 290 * scale_y, 460 * scale_x, 120 * scale_y};
        } else{
            // Centered New Game button reference (370 x, 290 y, 460 w, 120 h)
            interactive->new_game_bounds = (Rectangle){370 * scale_x, 290 * scale_y, 460 * scale_x, 120 * scale_y};
            // Clear continue bounds so it cannot be hovered or clicked
            interactive->continue_bounds = (Rectangle){0, 0, 0, 0};
        }
        interactive->settings_bounds = (Rectangle){370 * scale_x, 440 * scale_y, 460 * scale_x, 120 * scale_y};
        interactive->quit_bounds = (Rectangle){370 * scale_x, 590 * scale_y, 460 * scale_x, 120 * scale_y};
    } else if (game_state == PAUSE){
        // Reference layout for Pause menu buttons (centered, 115 height)
        interactive->continue_bounds = (Rectangle){370 * scale_x, 290 * scale_y, 460 * scale_x, 115 * scale_y};
        interactive->settings_bounds = (Rectangle){370 * scale_x, 450 * scale_y, 460 * scale_x, 115 * scale_y};
        interactive->quit_bounds = (Rectangle){370 * scale_x, 595 * scale_y, 460 * scale_x, 115 * scale_y};
    }

    // Volume Slider Bar
    float bar_w = interactive->bar_width * scale_x;
    float bar_h = interactive->bar_height * scale_y;
    interactive->volume_slider_bar = (Rectangle){ 
        screen_width / 2.0f - bar_w / 2.0f, 
        screen_height / 2.0f + bar_h * 14.0f, 
        bar_w, 
        bar_h 
    };

    // Reference Knob dimensions (100x120)
    interactive->knob_width = interactive->knob_width * scale_x;
    interactive->knob_height = interactive->knob_height * scale_y;

    // Settings Back Button (460x115 reference)
    float btn_w = 460.0f * scale_x;
    float btn_h = 115.0f * scale_y;
    interactive->settings_back_bounds = (Rectangle){
        screen_width / 2.0f - btn_w / 2.0f,
        screen_height / 2.0f + btn_h * 1.75f,
        btn_w,
        btn_h
    };
}

void UpdateInteractive(Interactive* interactive, Settings* game_settings){
    Vector2 mouse_position = GetMousePosition();

    // Reset clicked triggers (latches)
    interactive->is_new_game_clicked = false;
    interactive->is_continue_clicked = false;
    interactive->is_settings_clicked = false;
    interactive->is_quit_clicked = false;
    interactive->is_settings_back_clicked = false;

    // --- Phase 1: Button Interaction ---
    interactive->is_new_game_hovered = CheckCollisionPointRec(mouse_position, interactive->new_game_bounds);
    interactive->is_continue_hovered = CheckCollisionPointRec(mouse_position, interactive->continue_bounds);
    interactive->is_settings_hovered = CheckCollisionPointRec(mouse_position, interactive->settings_bounds);
    interactive->is_quit_hovered = CheckCollisionPointRec(mouse_position, interactive->quit_bounds);
    interactive->is_settings_back_hovered = CheckCollisionPointRec(mouse_position, interactive->settings_back_bounds);

    if (interactive->is_new_game_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) interactive->is_new_game_clicked = true;
    if (interactive->is_continue_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) interactive->is_continue_clicked = true;
    if (interactive->is_settings_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) interactive->is_settings_clicked = true;
    if (interactive->is_quit_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) interactive->is_quit_clicked = true;
    if (interactive->is_settings_back_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) interactive->is_settings_back_clicked = true;

    // --- Phase 2: Slider Interaction ---
    // 1. Calculate knob position based on current settings (percentage mapped to width)
    float knob_x = interactive->volume_slider_bar.x + (game_settings->game_volume * interactive->volume_slider_bar.width / 100.0f);
    
    interactive->volume_slider_knob = (Rectangle){
        knob_x - interactive->knob_width / 2.0f,
        interactive->volume_slider_bar.y + interactive->volume_slider_bar.height / 2.0f - interactive->knob_height / 2.0f,
        interactive->knob_width,
        interactive->knob_height
    };

    // 2. State transition: Drag Start
    if (CheckCollisionPointRec(mouse_position, interactive->volume_slider_knob) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
        interactive->is_volume_moving = true;
    }

    // 3. State transition: Drag Stop
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) interactive->is_volume_moving = false;

    // 4. Transform Logic: Dragging
    if (interactive->is_volume_moving){
        float new_knob_x = mouse_position.x;
        new_knob_x = Clamp(new_knob_x, interactive->volume_slider_bar.x, interactive->volume_slider_bar.x + interactive->volume_slider_bar.width);

        // Map x-position back to 0-100 range
        game_settings->game_volume = (new_knob_x - interactive->volume_slider_bar.x) / interactive->volume_slider_bar.width * 100.0f;
        
        // Immediate Feedback: update Raylib master volume
        SetMasterVolume(game_settings->game_volume / 100.0f);
        
        // Visual sync
        interactive->volume_slider_knob.x = new_knob_x - interactive->knob_width / 2.0f;
    }
}