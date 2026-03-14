/*
This file contains the implementation of the interactive module.

Module made by Andrew Zhuo.
*/

#include "interactive.h"
#include "raylib.h"
#include "raymath.h"
#include "state.h"

Interactive InitInteractive(Settings* game_settings){
    /* Initialize the interactive elements. */
    Interactive new_interactive = {0};

    // Load button textures.
    new_interactive.play_button = LoadTexture("../assets/images/buttons/play.jpg");
    new_interactive.settings_button = LoadTexture("../assets/images/buttons/settings.jpg");
    new_interactive.quit_button = LoadTexture("../assets/images/buttons/quit.jpg");

    // Initialize slider dimensions.
    new_interactive.bar_width = 400.0f;
    new_interactive.bar_height = 10.0f;
    new_interactive.knob_width = 20.0f;
    new_interactive.knob_height = 30.0f;
    
    return new_interactive;
}

void UpdateInteractive(Interactive* interactive, Settings* game_settings, GameState* state){ //Added state parameter to separate main menu and settings button bounds
    /* Update the interactive elements. */

    // Get mouse position
    Vector2 mouse_position = GetMousePosition();

    // Reset clicked flags
    interactive->is_play_clicked = false;
    interactive->is_settings_clicked = false;
    interactive->is_quit_clicked = false;
    interactive->is_mm_play_clicked = false;

    if(*state == PAUSE){
        // Calculate play button bounds
        interactive->play_bounds = (Rectangle){
            (float)game_settings->window_width / 2.0f - (float)interactive->play_button.width / 2.0f,
            (float)game_settings->window_height / 2.0f - (float)interactive->play_button.height / 2.0f - 2.0f * (float)interactive->play_button.height,
            (float)interactive->play_button.width,
            (float)interactive->play_button.height
        };

        // Calculate settings button bounds
        interactive->settings_bounds = (Rectangle){
            (float)game_settings->window_width / 2.0f - (float)interactive->settings_button.width / 2.0f,
            (float)game_settings->window_height / 2.0f - (float)interactive->settings_button.height / 2.0f,
            (float)interactive->settings_button.width,
            (float)interactive->settings_button.height
        };

        // Calculate quit button bounds
        interactive->quit_bounds = (Rectangle){
            (float)game_settings->window_width / 2.0f - (float)interactive->quit_button.width / 2.0f,
            (float)game_settings->window_height / 2.0f + (float)interactive->quit_button.height / 2.0f + (float)interactive->quit_button.height,
            (float)interactive->quit_button.width,
            (float)interactive->quit_button.height
        };

        // Check hover and clicks
        interactive->is_play_hovered = CheckCollisionPointRec(mouse_position, interactive->play_bounds);
        interactive->is_settings_hovered = CheckCollisionPointRec(mouse_position, interactive->settings_bounds);
        interactive->is_quit_hovered = CheckCollisionPointRec(mouse_position, interactive->quit_bounds);

        if (interactive->is_play_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) interactive->is_play_clicked = true;
        if (interactive->is_settings_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) interactive->is_settings_clicked = true;
        if (interactive->is_quit_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) interactive->is_quit_clicked = true;
    }

    if(*state == SETTINGS){
       // Slider logic
        interactive->volume_slider_bar = (Rectangle){
            (float)game_settings->window_width / 2.0f - interactive->bar_width / 2.0f,
            (float)game_settings->window_height / 2.0f - interactive->bar_height / 2.0f,
            interactive->bar_width,
            interactive->bar_height
        };

        // Current volume position
        float knob_x = interactive->volume_slider_bar.x + (game_settings->game_volume * interactive->bar_width / 100.0f);
        
        interactive->volume_slider_knob = (Rectangle){
            knob_x - interactive->knob_width / 2.0f,
            interactive->volume_slider_bar.y + interactive->bar_height / 2.0f - interactive->knob_height / 2.0f,
            interactive->knob_width,
            interactive->knob_height
        };

        // Check if the mouse is clicking the volume slider
        if (CheckCollisionPointRec(mouse_position, interactive->volume_slider_knob) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            interactive->is_volume_moving = true;
        }

        // Check if the mouse is releasing the volume slider
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
            interactive->is_volume_moving = false;
        }

        // Update volume if the mouse is moving the volume slider
        if (interactive->is_volume_moving){
            float new_knob_x = mouse_position.x;
            
            // Clamp knob to bar bounds
            new_knob_x = Clamp(new_knob_x, interactive->volume_slider_bar.x, interactive->volume_slider_bar.x + interactive->bar_width);

            // Update settings volume
            game_settings->game_volume = (new_knob_x - interactive->volume_slider_bar.x) / interactive->bar_width * 100.0f;
            
            // Update knob position
            interactive->volume_slider_knob.x = new_knob_x - interactive->knob_width / 2.0f;
        }
    }


    if(*state == MAINMENU){
        // Calculate main menu play button bounds
        interactive->mm_play_bounds = (Rectangle){
            (float)game_settings->window_width / 2.0f - (float)interactive->play_button.width / 2.0f,
            (float)game_settings->window_height / 2.0f - (float)interactive->play_button.height / 2.0f,
            (float)interactive->play_button.width,
            (float)interactive->play_button.height
        };

        interactive->is_mm_play_hovered = CheckCollisionPointRec(mouse_position, interactive->mm_play_bounds);

        if (interactive->is_mm_play_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) interactive->is_mm_play_clicked = true;
    }
}

void CloseInteractive(Interactive* interactive){
    /* Close the interactive elements. */
    UnloadTexture(interactive->play_button);
    UnloadTexture(interactive->settings_button);
    UnloadTexture(interactive->quit_button);
}