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
#include <stdio.h>
#include "data.h"

void InitGame(Settings *game_settings);
void RunGame(Character *player, Audio *game_audio, Settings *game_settings,
             Scene *game_scene, Interactive *game_interactive,
             Dialogue *game_dialogue, Map *game_map,
             NPC worldNPCs[], Item worldItems[], GameContext *game_context,
             GameState *game_state);
void DrawGame(Scene *game_scene, Settings *game_settings,
              Interactive *game_interactive, Map *game_map, Character *player,
              Dialogue *game_dialogue, GameContext *game_context,
              GameState *game_state, NPC worldNPCs[], Item worldItems[]);
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
    Character player = InitCharacter(&game_settings, &game_data);
    Audio game_audio = InitAudio(&game_settings);
    Scene game_scene = InitScene(&game_settings);
    Interactive game_interactive = InitInteractive(&game_settings);
    Dialogue game_dialogue = LoadDialogue("../assets/text/dialogue1.txt");
    Map game_map = InitMap("../assets/map/map.json");
    NPC worldNPCs[2] = {
        {{{0}, "../assets/images/character/furina.png", {150, 200, 100, 100}, false, INTERACTABLE_TYPE_NPC}, "../assets/text/signpost.txt"},
        {{{0}, "../assets/images/character/oldman.png", {600, 300, 80, 80}, false, INTERACTABLE_TYPE_NPC}, "../assets/text/oldman.txt"},
    };
    Item worldItems[1] = {
        {{{0}, "../assets/images/items/potato.png", {450, 450, 20, 20}, false, INTERACTABLE_TYPE_ITEM}, false}
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

        // Calculate player hitbox and map size.
        Rectangle playerHitbox = {player->position.x + 75, player->position.y + 50, 60, 80};
        
        Vector2 map_size = {(float)game_map->tiled_map->width * game_map->tiled_map->tilewidth,
                            (float)game_map->tiled_map->height * game_map->tiled_map->tileheight};

        CheckInteractable(worldNPCs, worldItems, 2, 1, playerHitbox, &objectToInteractWith);

        // Toggle pause state
        if (IsKeyPressed(KEY_ESCAPE)) {
            *game_state = (*game_state == PAUSE) ? GAMEPLAY : PAUSE;
        }

        // Handle dialogue
        if (IsKeyPressed(KEY_ENTER)) {
            InteractWithObject(objectToInteractWith, current_dialogue, game_state, player);
        }

        // Update game state
        if (UpdateGame(
            game_state, game_interactive, player, game_settings, game_map,
            game_context, game_audio, map_size
        )){
            break;
        }

        // Draw game assets to the screen
        DrawGame(game_scene, game_settings, game_interactive, game_map, player,
                current_dialogue, game_context, game_state, worldNPCs, worldItems);
    }
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
    } else {
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
                    worldNPCs[i].base.bounds.y - 40, 20, RED);
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
                        worldItems[i].base.bounds.y - 30, 20, RED);
                }
            }
        }
        EndMode2D();
        
        DrawTexture(game_scene->vignette, 0, 0, WHITE);

        if (*game_state == DIALOGUE_CUTSCENE){
            DrawRectangle(0, GetScreenHeight() - 120, GetScreenWidth(), 120,
                        Fade(BLACK, 0.8f));

            const char *line =
                game_dialogue->lines[game_dialogue->current_line];
            DrawText(line, GetScreenWidth() / 2 - MeasureText(line, 20) / 2,
                    GetScreenHeight() - 80, 20, WHITE);
            DrawText("Press ENTER to continue", GetScreenWidth() - 150,
                    GetScreenHeight() - 30, 10, GRAY);
        }
    }
    EndDrawing();
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