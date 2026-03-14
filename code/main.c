/*
This file contains the main logic for the game.

Made by Andrew Zhuo and Steven Kenneth Darwy
*/

#include "raylib.h"
#include "settings.h"
#include "character.h"
#include "audio.h"
#include "interactive.h"
#include "scene.h"
#include "map.h"
#include "game_context.h"
#include "state.h"

void InitGame(Settings* game_settings);
void RunGame(Character* player, Audio* game_audio, Settings* game_settings, Scene* game_scene, Interactive* game_interactive, Map* game_map, GameContext* game_context);
void EndGame(Audio* game_audio, Character* player, Scene* game_scene, Interactive* game_interactive, Map* game_map);

int main(void){
    /* Initialize the game */
    
    // Initialize the settings and game.
    Settings game_settings = InitSettings();
    InitGame(&game_settings);

    // Load game resources.
    Character player = InitCharacter(&game_settings);
    Audio game_audio = InitAudio(&game_settings);
    Scene game_scene = InitScene(&game_settings);
    Interactive game_interactive = InitInteractive(&game_settings);
    Map game_map = InitMap("../assets/map/map.json");
    GameContext game_context = InitGameContext(&game_map, &player, &game_settings);

    // Run the game.
    RunGame(&player, &game_audio, &game_settings, &game_scene, &game_interactive, &game_map, &game_context);

    // End the game.
    EndGame(&game_audio, &player, &game_scene, &game_interactive, &game_map);

    return 0;
}

void InitGame(Settings* game_settings){
    /* Initialize the game */
    
    // Prepare and initialize the game windows.
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    SetTargetFPS(game_settings->fps);
    InitWindow(game_settings->window_width, game_settings->window_height, "Aisling");

    // Load game icon.
    Image icon = LoadImage("../assets/images/icon/app_icon.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

    // Prevent closing the window with ESC automatically so it can be used for pause.
    SetExitKey(0);
}

void RunGame(Character* player, Audio* game_audio, Settings* game_settings, Scene* game_scene, Interactive* game_interactive, Map* game_map, GameContext* game_context){
    /* Run the game */
    GameState game_state = MAINMENU;
    
    while (!WindowShouldClose()){
        // Update audio stream.
        UpdateAudio(game_audio);
          
        // Toggle pause state
        if (IsKeyPressed(KEY_ESCAPE)){
            if (game_state == GAMEPLAY) {
                game_state = PAUSE;
                ShowCursor();
            } else if (game_state == PAUSE) {
                game_state = GAMEPLAY;
                HideCursor();
            }
        }
        
        // Update game state.
        if (game_state == GAMEPLAY){
            if (player->position.x == 200){
                PlayScream(game_audio);
            }
            Vector2 map_size = {(float)game_context->map->tiled_map->width * game_context->map->tiled_map->tilewidth, 
                                (float)game_context->map->tiled_map->height * game_context->map->tiled_map->tileheight};
            UpdateCharacter(player, game_settings, map_size, game_map);
            UpdateGameContext(game_context, game_settings, map_size);
        }
        
        // Draw game assets to the screen.
        BeginDrawing();
        ClearBackground(BLACK);
        if (game_state == MAINMENU){
            ShowCursor();
            UpdateInteractive(game_interactive, game_settings, &game_state);
            DrawMainMenu(game_scene, game_interactive, game_settings);
            if (game_interactive->is_mm_play_clicked) {
                game_state = GAMEPLAY;
                HideCursor();
            }
        } else if (game_state == GAMEPLAY){
            DrawCharacter(player); 
            DrawTexture(game_scene->vignette, 0, 0, WHITE);
            EndMode2D();
        } else if (game_state == PAUSE){
            UpdateInteractive(game_interactive, game_settings, &game_state);
            
            if (game_interactive->is_play_clicked) {
                game_state = GAMEPLAY;
                HideCursor();
            } else if (game_interactive->is_settings_clicked) {
                game_state = SETTINGS;
            } else if (game_interactive->is_quit_clicked) {
                break;    // Exit the game.
            }
            } else if (game_state == PAUSE){
                UpdateInteractive(game_interactive, game_settings, &game_state);
                
                if (game_interactive->is_play_clicked) {
                    game_state = GAMEPLAY;
                    HideCursor();
                } else if (game_interactive->is_settings_clicked) {
                    game_state = SETTINGS;
                } else if (game_interactive->is_quit_clicked) {
                    break;    // Exit the game.
                }
            
            DrawPauseMenu(game_scene, game_settings, game_interactive);
        } else if (game_state == SETTINGS){
            UpdateInteractive(game_interactive, game_settings, &game_state);
            
            // Set master volume
            SetMasterVolume(game_settings->game_volume);
            
            if (IsKeyPressed(KEY_ESCAPE)){
                game_state = PAUSE;
            }
            
            DrawSettings(game_scene, game_settings, game_interactive);
        }

        EndDrawing();
    }
}

void EndGame(Audio* game_audio, Character* player, Scene* game_scene, Interactive* game_interactive, Map* game_map){
    /* End the game */
    
    // Prepare to stop the game.
    CloseAudio(game_audio);
    CloseCharacter(player);
    CloseScene(game_scene);
    CloseInteractive(game_interactive);
    FreeMap(game_map);

    // Close the game window.
    CloseWindow();
}