/*
This is for scene managing, such as loading and drawing the background.

Module made by Andrew Zhuo and Steven Kenneth Darwy.
*/

#include "scene.h"

Scene InitScene(Settings* game_settings){
    /* Initialize the game scene. */
    Scene new_scene = {0};

    // Load scene backgrounds
    new_scene.mainmenu_background = LoadBackground("../assets/images/background/Mountain/parallax-mountain-bg.png", game_settings);
    new_scene.game_background = LoadBackground("../assets/images/background/Mountain/parallax-mountain-bg.png", game_settings);
    new_scene.pause_menu_background = LoadBackground("../assets/images/background/Mountain/parallax-mountain-bg.png", game_settings);
    new_scene.settings_background = LoadBackground("../assets/images/background/Mountain/parallax-mountain-bg.png", game_settings);
    new_scene.vignette = LoadBackground("../assets/images/background/vignette/vig.png", game_settings);

    return new_scene;
}

Texture2D LoadBackground(const char* path, Settings* game_settings){
    /* Load background texture */
    Image background = LoadImage(path);
    
    ImageResize(&background, game_settings->window_width, game_settings->window_height);
    Texture2D background_texture = LoadTextureFromImage(background);
    UnloadImage(background);

    return background_texture;
}

void DrawMainMenu(Scene* scene, Interactive* game_interactive, Settings* game_settings){
    /* Draw main menu scene. */
    DrawTexture(scene->mainmenu_background, 0, 0, WHITE);
    DrawTexture(
        game_interactive->play_button,
        game_interactive->mm_play_bounds.x, 
        game_interactive->mm_play_bounds.y,
        game_interactive->is_mm_play_hovered ? GRAY : WHITE
    );
}

void DrawPauseMenu(Scene* scene, Settings* game_settings, Interactive* game_interactive){
    /* Draw pause menu scene. */

    // Draw pause menu background
    DrawTexture(scene->pause_menu_background, 0, 0, WHITE);
    
    // Draw Settings button
    DrawTexture(
        game_interactive->settings_button,
        game_interactive->settings_bounds.x, 
        game_interactive->settings_bounds.y, 
        game_interactive->is_settings_hovered ? GRAY : WHITE
    );
    
    // Draw Play button
    DrawTexture(
        game_interactive->play_button,
        game_interactive->play_bounds.x, 
        game_interactive->play_bounds.y, 
        game_interactive->is_play_hovered ? GRAY : WHITE
    );
    
    // Draw Quit button
    DrawTexture(
        game_interactive->quit_button,
        game_interactive->quit_bounds.x, 
        game_interactive->quit_bounds.y, 
        game_interactive->is_quit_hovered ? GRAY : WHITE
    );
}

void DrawSettings(Scene* scene, Settings* game_settings, Interactive* game_interactive){
    /* Draw settings scene. */
    DrawTexture(scene->settings_background, 0, 0, WHITE);

    // Draw Volume Slider Bar
    DrawRectangleRec(game_interactive->volume_slider_bar, LIGHTGRAY);
    
    // Draw Volume Slider Knob
    DrawRectangleRec(game_interactive->volume_slider_knob, GRAY);

    // Draw Volume Text
    int volume_percentage = (int)(game_settings->game_volume * 1.0f);
    DrawText(TextFormat("Volume: %d%%", volume_percentage), 
             game_interactive->volume_slider_bar.x, 
             game_interactive->volume_slider_bar.y - 40, 
             30, WHITE);

    DrawText("Press ESC to return", 10, game_settings->window_height - 40, 20, WHITE);
}

void CloseScene(Scene* scene){
    /* Unload all scene textures. */
    UnloadTexture(scene->mainmenu_background);
    UnloadTexture(scene->game_background);
    UnloadTexture(scene->pause_menu_background);
    UnloadTexture(scene->settings_background);
    UnloadTexture(scene->vignette);
}
