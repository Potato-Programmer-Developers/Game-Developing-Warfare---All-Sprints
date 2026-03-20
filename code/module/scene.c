/*
This is for scene managing, such as loading and drawing the background.

Module made by Andrew Zhuo, Cornelius Jabez Lim, and Steven Kenneth Darwy.
*/

#include "scene.h"
#include <stdio.h>

Scene InitScene(Settings* game_settings){
    /* Initialize the game scene. */
    Scene new_scene = {0};

    // Load scene backgrounds
    new_scene.mainmenu_background = LoadTexture("../assets/images/background/main_menu/main_menu.png");
    new_scene.pause_menu_background = LoadTexture("../assets/images/background/pause/pause.png");
    new_scene.settings_background = LoadTexture("../assets/images/background/settings/settings.png");
    new_scene.vignette = LoadTexture("../assets/images/background/vignette/vignette.png");
    
    // Initialize photo cutscene state
    new_scene.current_cutscene_frame_texture = (Texture2D){0};
    new_scene.cutscene_timer = 0.0f;
    new_scene.current_cutscene_frame = 0;

    return new_scene;
}

void DrawGame(Scene *game_scene, Settings *game_settings, 
              Interactive *game_interactive, Map *game_map, Character *player,
              Dialogue *game_dialogue, GameContext *game_context,
              GameState *game_state, NPC worldNPCs[], Item worldItems[]){
    /* Draw the game */
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (*game_state == MAINMENU) {
        DrawMainMenu(game_scene, game_interactive);
    } else if (*game_state == SETTINGS) {
        DrawSettings(game_scene, game_settings, game_interactive);
    } else if (*game_state == PAUSE) {
        DrawPauseMenu(game_scene, game_settings, game_interactive);
    } else if (*game_state == PHOTO_CUTSCENE) {
        DrawTexturePro(game_scene->current_cutscene_frame_texture,
            (Rectangle){0, 0, (float)game_scene->current_cutscene_frame_texture.width, (float)game_scene->current_cutscene_frame_texture.height},
            (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
            (Vector2){0, 0}, 0.0f, WHITE);
        if (game_scene->cutscene_timer < 5.0f) {
            DrawText("Double click 'Enter' to skip", GetScreenWidth() - 300, GetScreenHeight() - 40, 20, LIGHTGRAY);
        }
    } else {
        DrawGameplay(game_scene, game_settings, game_interactive, game_map, player, worldNPCs, worldItems, game_context);
        
        DrawTexturePro(game_scene->vignette,
            (Rectangle){0, 0, (float)game_scene->vignette.width, (float)game_scene->vignette.height},
            (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
            (Vector2){0, 0}, 0.0f, WHITE);
        
        // Draw darkening effect based on hallucination
        // Only start darkening after the bar is full (hallucination > max_hallucination)
        float darkness_alpha = (player->hallucination - player->max_hallucination) * 5.0f / player->max_hallucination;
        if (darkness_alpha > 0.0f){
            if (darkness_alpha > 1.0f) darkness_alpha = 1.0f;
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, darkness_alpha));
        }

        if (*game_state == DIALOGUE_CUTSCENE){
            DrawRectangle(0, GetScreenHeight() - 120, GetScreenWidth(), 120, Fade(BLACK, 0.8f));

            const char *line =
                game_dialogue->lines[game_dialogue->current_line];
            DrawText(line, GetScreenWidth() / 2 - MeasureText(line, 20) / 2,
                    GetScreenHeight() - 80, 20, WHITE);
            DrawText("Press 'SPACE' to continue", GetScreenWidth() - 150,
                    GetScreenHeight() - 30, 10, GRAY);
        }

        // Draw Phone
        DrawPhone(&game_context->phone);
    }
    EndDrawing();
}

void DrawMainMenu(Scene* scene, Interactive* game_interactive){
    /* Draw main menu scene. */
    DrawTexturePro(scene->mainmenu_background, 
        (Rectangle){0, 0, (float)scene->mainmenu_background.width, (float)scene->mainmenu_background.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0}, 0.0f, WHITE);
    
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

void DrawPauseMenu(Scene* scene, Settings* game_settings, Interactive* game_interactive){
    /* Draw pause menu scene. */

    // Draw pause menu background
    DrawTexturePro(scene->pause_menu_background, 
        (Rectangle){0, 0, (float)scene->pause_menu_background.width, (float)scene->pause_menu_background.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0}, 0.0f, WHITE);
    
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
    DrawTexturePro(scene->settings_background, 
        (Rectangle){0, 0, (float)scene->settings_background.width, (float)scene->settings_background.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0}, 0.0f, WHITE);

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

    DrawText("Press ESC to return", 10, GetScreenHeight() - 40, 20, WHITE);
}

void DrawGameplay(Scene* scene, Settings* game_settings, Interactive* game_interactive, Map* game_map,
                Character* player, NPC worldNPCs[], Item worldItems[], GameContext* game_context){
    BeginMode2D(game_context->camera);
        DrawMap(game_map);
        DrawCharacter(player);
        for (int i = 0; i < 2; i++){
            DrawTexturePro(worldNPCs[i].base.texture,
                (Rectangle){0, 0, (float)worldNPCs[i].base.texture.width, (float)worldNPCs[i].base.texture.height},
                worldNPCs[i].base.bounds,
                (Vector2){0, 0},
                0, WHITE);
            if (worldNPCs[i].base.isActive){
                DrawText("!", worldNPCs[i].base.bounds.x + worldNPCs[i].base.bounds.width / 2,
                    worldNPCs[i].base.bounds.y - 40, 50, RED);
            }
        }
        for (int i = 0; i < 1; i++){
            if (!worldItems[i].picked_up){
                DrawTexturePro(worldItems[i].base.texture,
                    (Rectangle){0, 0, (float)worldItems[i].base.texture.width, (float)worldItems[i].base.texture.height},
                    worldItems[i].base.bounds,
                    (Vector2){0, 0},
                    0, WHITE);
                if (worldItems[i].base.isActive){
                    DrawText("!", worldItems[i].base.bounds.x + 20,
                        worldItems[i].base.bounds.y - 30, 50, RED);
                }
            }
        }
        EndMode2D();
}

void LoadCutsceneFrame(Scene *scene, int frame_index, Settings *game_settings){
    /* Load a specific cutscene frame on-the-fly and unload the previous one. */

    // Unload the previous cutscene frame if it still occupies memory
    if (scene->current_cutscene_frame_texture.id != 0){
        UnloadTexture(scene->current_cutscene_frame_texture);
    }
    
    // Load the current cutscene frame
    char path[100];
    sprintf(path, "../assets/videos/cutscene/frame%04d.qoi", frame_index);
    scene->current_cutscene_frame_texture = LoadTexture(path);
}

void ClearCutscene(Scene* scene){
    /* Unload the current cutscene frame if it's still loaded. */
    if (scene->current_cutscene_frame_texture.id != 0){
        UnloadTexture(scene->current_cutscene_frame_texture);
        scene->current_cutscene_frame_texture.id = 0;
    }
}

void CloseScene(Scene* scene){
    /* Unload all scene textures. */
    UnloadTexture(scene->mainmenu_background);
    UnloadTexture(scene->pause_menu_background);
    UnloadTexture(scene->settings_background);
    UnloadTexture(scene->vignette);
    ClearCutscene(scene);
}
