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

typedef enum {
    /* This enum contains the states of the game. */
    GAMEPLAY,      // Game is running.
    PAUSE,         // Game is paused.
    SETTINGS,      // Settings menu is open.
    GAMEOVER       // Game is over.
} GameState;

void InitGame(Settings* game_settings);
void RunGame(Character* player, Audio* game_audio, Settings* game_settings, Scene* game_scene, Interactive* game_interactive, Map* game_map);
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

    // Run the game.
    RunGame(&player, &game_audio, &game_settings, &game_scene, &game_interactive, &game_map);

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

void RunGame(Character* player, Audio* game_audio, Settings* game_settings, Scene* game_scene, Interactive* game_interactive, Map* game_map){
    /* Run the game */
    GameState game_state = GAMEPLAY;
    
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
            UpdateCharacter(player, game_settings);
        }
        
        // Draw game assets to the screen.
        BeginDrawing();
        ClearBackground(BLACK);
        if (game_state == GAMEPLAY){
            DrawMap(game_map);
            DrawCharacter(player); 
            DrawTexture(game_scene->vignette, 0, 0, WHITE);
        } else if (game_state == PAUSE){
            UpdateInteractive(game_interactive, game_settings);
            
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
            UpdateInteractive(game_interactive, game_settings);
            
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