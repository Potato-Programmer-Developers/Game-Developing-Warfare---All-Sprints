/*
This file contains the main logic for the game.

Made by Andrew Zhuo, Cornelius Jabez Lim, and Steven Kenneth Darwy
*/

#include "audio.h"
#include "character.h"
#include "dialogue.h"
#include "game_context.h"
#include "interaction.h"
#include "map.h"
#include "raylib.h"
#include "scene.h"
#include "settings.h"
#include "state.h"
#include "phone.h"
#include <stdio.h>
#include "data.h"

void InitGame(Settings *game_settings);
void RunGame(Character *player, Audio *game_audio, Settings *game_settings,
             Scene *game_scene, Interactive *game_interactive,
             Dialogue *game_dialogue, Map *game_map,
             NPC worldNPCs[], Item worldItems[], GameContext *game_context,
             GameState *game_state);
void EndGame(Audio *game_audio, Character *player, Item worldItems[], int itemCount, Scene *game_scene,
             Interactive *game_interactive, Map *game_map,
             Settings *game_settings);

int main(void){
    /* Initialize the game */

    // Initialize the settings and game.
    Settings game_settings = InitSettings();
    InitGame(&game_settings);

    // Load game data.
    Data game_data = LoadData(&game_settings);

    // Load game resources.
    Map game_map = InitMap("../assets/map/MAINMAP.json");
    Character player = InitCharacter(&game_settings, &game_data, &game_map);
    Audio game_audio = InitAudio(&game_settings);
    Scene game_scene = InitScene(&game_settings);
    Interactive game_interactive = InitInteractive(&game_settings);
    Dialogue game_dialogue = LoadDialogue("../assets/text/dialogue1.txt");
    NPC worldNPCs[2] = {
        {{{0}, "../assets/images/character/furina.png", {800, 600, 200, 200}, false, INTERACTABLE_TYPE_NPC}, "../assets/text/signpost.txt"},
        {{{0}, "../assets/images/character/oldman.png", {600, 300, 150, 150}, false, INTERACTABLE_TYPE_NPC}, "../assets/text/oldman.txt"},
    };
    Item worldItems[1] = {
        {{{0}, "../assets/images/items/potato.png", {500, 500, 50, 50}, false, INTERACTABLE_TYPE_ITEM}, false}
    };
    GameContext game_context = InitGameContext(&game_map, &player, &game_settings);
    GameState game_state = MAINMENU;
    ApplyData(&player, worldItems, 1, &game_settings, &game_data);

    // Initialize NPC and Item textures.
    LoadNPCs(worldNPCs, 2);
    LoadItems(worldItems, 1);

    // Run the game.
    RunGame(&player, &game_audio, &game_settings, &game_scene, &game_interactive,
            &game_dialogue, &game_map, worldNPCs, worldItems, &game_context, &game_state);
    
    // End the game.
    EndGame(&game_audio, &player, worldItems, 1, &game_scene, &game_interactive, &game_map, &game_settings);
    
    return 0;
}

void InitGame(Settings *game_settings){
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

void RunGame(Character *player, Audio *game_audio, Settings *game_settings,
        Scene *game_scene, Interactive *game_interactive,
        Dialogue *game_dialogue, Map *game_map,
        NPC worldNPCs[], Item worldItems[], GameContext *game_context,
        GameState *game_state){
    /* Run the game */
    Dialogue *current_dialogue = game_dialogue;
    Interactable *objectToInteractWith = NULL;

    while (!WindowShouldClose()){
        // Update audio stream.
        UpdateAudio(game_audio);

        // Calculate player hitbox for interaction (larger than physical hitbox)
        Rectangle playerHitbox = {player->position.x + 50, player->position.y, 300, 300};
        
        Vector2 map_size = {(float)game_map->tiled_map->width * game_map->tiled_map->tilewidth,
                            (float)game_map->tiled_map->height * game_map->tiled_map->tileheight};

        CheckInteractable(worldNPCs, worldItems, 2, 1, playerHitbox, &objectToInteractWith);

        // Toggle pause state
        if (IsKeyPressed(KEY_ESCAPE)){
            if (*game_state == PAUSE){
                if (game_context->previous_state == PHOTO_CUTSCENE) {
                    PauseMusicStream(game_audio->bg_music);
                    ResumeMusicStream(game_audio->cutscene_music);
                }
                *game_state = game_context->previous_state;
            } else if (*game_state == GAMEPLAY || *game_state == PHOTO_CUTSCENE){
                if (*game_state == PHOTO_CUTSCENE) {
                    PauseMusicStream(game_audio->cutscene_music);
                    ResumeMusicStream(game_audio->bg_music);
                }
                game_context->previous_state = *game_state;
                *game_state = PAUSE;
            }
        }

        // Handle interaction
        if (IsKeyPressed(KEY_E) && *game_state == GAMEPLAY) {
            InteractWithObject(objectToInteractWith, current_dialogue, game_state, player);
        }

        // Handle dialogue (advance with SPACE)
        if (IsKeyPressed(KEY_SPACE) && *game_state == DIALOGUE_CUTSCENE) {
            InteractWithObject(objectToInteractWith, current_dialogue, game_state, player);
        }

        // Update phone
        UpdatePhone(&game_context->phone, GetFrameTime());

        // Update game state
        if (UpdateGame(
            game_state, game_interactive, player, game_settings, game_map,
            game_context, game_audio, map_size, game_scene
        )){
            break;
        }

        // Handle window resize
        if (IsWindowResized()){
            UpdateInteractiveLayout(game_interactive);
        }

        // Draw game assets to the screen
        DrawGame(game_scene, game_settings, game_interactive, game_map, player,
                current_dialogue, game_context, game_state, worldNPCs, worldItems);
    }
}

void EndGame(Audio *game_audio, Character *player, Item worldItems[], int itemCount, Scene *game_scene,
            Interactive *game_interactive, Map *game_map, Settings *game_settings){
    /* End the game */

    // Save the game data.
    SaveData(player, worldItems, itemCount, game_settings);

    // Prepare to stop the game.
    CloseAudio(game_audio);
    CloseCharacter(player);
    CloseScene(game_scene);
    CloseInteractive(game_interactive);
    FreeMap(game_map);

    // Close the game window.
    CloseWindow();
}