/*
This file contains the main logic for the game.

Made by Andrew Zhuo and Steven Kenneth Darwy
*/

#include <stdio.h>
#include "raylib.h"
#include "settings.h"
#include "character.h"
#include "audio.h"
#include "dialogue.h"
#include "scene.h"
#include "interaction.h"

typedef enum {
    GAMEPLAY,
    PAUSE,
    GAMEOVER,
    VIDEO_CUTSCENE,
    DIALOGUE_CUTSCENE
} GameState;

void InitGame(Settings* game_settings);
void RunGame(Character* player, Audio* game_audio, Settings* game_settings, Scene* game_scene, Dialogue* dialogue, Interactable* worldObjects);

int main(void){
    /* Initialize the game */
    
    // Initialize the settings and game.
    Settings game_settings = InitSettings();
    InitGame(&game_settings);
    Interactable worldObjects[2] = {
        {{ 150, 200, 50, 50 }, "../assets/text/signpost.txt", false},
        {{ 600, 300, 60, 60 }, "../assets/text/oldman.txt", false}
    };

    // Load game resources.
    Character player = InitCharacter(&game_settings);
    Audio game_audio = InitAudio(&game_settings);
    Scene game_scene = InitScene(&game_settings);
    Interactive game_interactive = InitInteractive(&game_settings);
    Map game_map = InitMap("../assets/map/map.json");
    GameContext game_context = InitGameContext(&game_map, &player, &game_settings);
    Dialogue intro_dialogue = LoadDialogue("../assets/text/dialogue1.txt");

    // Run the game.
    RunGame(&player, &game_audio, &game_settings, &game_scene, &game_interactive, &intro_dialogue, &game_map, worldObjects, &game_context);

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

void RunGame(Character* player, Audio* game_audio, Settings* game_settings, Scene* game_scene, Interactive* game_interactive, Dialogue* intro_dialogue, Interactable* worldObjects Map* game_map, GameContext* game_context){
    /* Run the game */
    GameState game_state = GAMEPLAY;
    Dialogue* current_dialogue = intro_dialogue; // Pointer to the currently active dialogue (if any)
    Interactable* objectToInteractWith = NULL; 

    while (!WindowShouldClose()){
        // Update audio stream.
        UpdateAudio(game_audio);
        Rectangle playerHitbox = { player->position.x+50, player->position.y+50, 80, 80 };
        
        objectToInteractWith = NULL; 
        for (int i = 0; i < 2; i++) {
            // 2. Check collision
            if (CheckCollisionRecs(playerHitbox, worldObjects[i].bounds)) {
                worldObjects[i].isActive = true;
                objectToInteractWith = &worldObjects[i]; // Remember this object!
            } else {
                worldObjects[i].isActive = false;
            }
        }

        // Toggle pause state
        if (IsKeyPressed(KEY_ESCAPE)){
            game_state = (game_state == PAUSE) ? GAMEPLAY : PAUSE;
        } 

        if (IsKeyPressed(KEY_ENTER)) {
            if (game_state == GAMEPLAY && objectToInteractWith != NULL) {
                // --- START DIALOGUE ---
                UnloadDialogue(current_dialogue);
                *current_dialogue = LoadDialogue(objectToInteractWith->dialoguePath);
                
                if (current_dialogue->line_count > 0) {
                    game_state = DIALOGUE_CUTSCENE;
                    current_dialogue->current_line = 0;
                }
            } 
            else if (game_state == DIALOGUE_CUTSCENE) {
                // --- ADVANCE OR CLOSE ---
                // Increment first
                current_dialogue->current_line++;
                
                // If we have reached or passed the line count, go back to gameplay
                if (current_dialogue->current_line >= current_dialogue->line_count) {
                    game_state = GAMEPLAY;
                    current_dialogue->current_line = 0; // Reset for next time
                }
            }
        }

        switch(game_state) {
            case GAMEPLAY:
                UpdateScene(game_scene, game_settings);
                UpdateCharacter(player, game_settings);

                if (player->position.x == 200) PlayScream(game_audio);
                HideCursor();
                break;
            case PAUSE:
                // No updates needed for pause state
                break;
            default: break;
        }
        
        // Draw game assets to the screen.
        BeginDrawing();
        ClearBackground(RAYWHITE);
        draw_mainmenu(game_scene);
        DrawCharacter(player);
        for (int i = 0; i < 2; i++) {
            // If active (touching), draw GREEN. Otherwise, draw GRAY.
            Color boxColor = worldObjects[i].isActive ? LIME : GRAY;
            
            DrawRectangleRec(worldObjects[i].bounds, boxColor);
            
            if (worldObjects[i].isActive) {
                DrawText("!", worldObjects[i].bounds.x + 20, worldObjects[i].bounds.y - 30, 20, RED);
            }
        }
        DrawRectangleLinesEx(playerHitbox, 1, RED);

        if (game_state == PAUSE) {
            DrawText("PAUSED", game_settings->window_width / 2 - MeasureText("PAUSED", 40) / 2, game_settings->window_height / 2 - 20, 40, LIGHTGRAY);
        }
        if (game_state == DIALOGUE_CUTSCENE) {
            DrawRectangle(0, GetScreenHeight() - 120, GetScreenWidth(), 120, Fade(BLACK, 0.8f));
            
            const char* line = current_dialogue->lines[current_dialogue->current_line];
            DrawText(line, GetScreenWidth()/2 - MeasureText(line, 20)/2, GetScreenHeight() - 80, 20, WHITE);
            DrawText("Press ENTER to continue", GetScreenWidth() - 150, GetScreenHeight() - 30, 10, GRAY);
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