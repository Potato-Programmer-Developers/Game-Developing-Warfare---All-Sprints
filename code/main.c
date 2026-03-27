/**
 * @file main.c
 * @brief Entry point and main game loop for the Aisling game.
 * 
 * Authors: Andrew Zhuo, Cornelius Jabez Lim, Steven Kenneth Darwy
 */

#include "audio.h"
#include "character.h"
#include "game_context.h"
#include "dialogue.h"
#include "interaction.h"
#include "map.h"
#include "raylib.h"
#include "scene.h"
#include "settings.h"
#include "state.h"
#include "phone.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "assets.h"

void InitGame(Settings *game_settings);
void RunGame(Character *player, Audio *game_audio, Settings *game_settings,
             Scene *game_scene, Interactive *game_interactive,
             Dialogue *game_dialogue, Map *game_map,
             GameContext *game_context, GameState *game_state);
void EndGame(Audio *game_audio, Character *player, Scene *game_scene,
             Interactive *game_interactive, Map *game_map,
             Settings *game_settings, GameContext *game_context);

int main(void){
    Settings game_settings = InitSettings();
    InitGame(&game_settings);

    Data game_data = LoadData(&game_settings);
    Map game_map = InitMap("../assets/map/map_int/APARTMENT MAP.json");
    Character player = InitCharacter(&game_settings, &game_data, &game_map);
    Audio game_audio = InitAudio(&game_settings);
    Scene game_scene = InitScene(&game_settings);
    Interactive game_interactive = InitInteractive(&game_settings);
    
    Dialogue* game_dialogue = (Dialogue*)malloc(sizeof(Dialogue));
    memset(game_dialogue, 0, sizeof(Dialogue));
    
    GameContext* game_context = (GameContext*)malloc(sizeof(GameContext));
    InitGameContext(game_context, &game_map, &player, INTERIOR);
    
    LoadStoryDay(&game_context->story, "../assets/text/day1/day1.txt");
    StoryPhase* initial = GetActivePhase(&game_context->story);
    LoadPhaseAssets(initial, game_context);
    
    GameState game_state = MAINMENU;

    RunGame(&player, &game_audio, &game_settings, &game_scene, &game_interactive,
            game_dialogue, &game_map, game_context, &game_state);
    
    EndGame(&game_audio, &player, &game_scene, &game_interactive, &game_map, &game_settings, game_context);
    free(game_context);
    
    return 0;
}

void InitGame(Settings *game_settings){
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    SetTargetFPS(game_settings->fps);
    InitWindow(game_settings->window_width, game_settings->window_height, "Aisling");
    Image icon = LoadImage("../assets/images/icon/app_icon.png");
    SetWindowIcon(icon);
    UnloadImage(icon);
    SetExitKey(0);
}

void RunGame(Character *player, Audio *game_audio, Settings *game_settings,
        Scene *game_scene, Interactive *game_interactive,
        Dialogue *game_dialogue, Map *game_map, GameContext *game_context,
        GameState *game_state){
    
    Dialogue *current_dialogue = game_dialogue;
    Interactable *objectToInteractWith = NULL;

    while (!WindowShouldClose()){
        UpdateAudio(game_audio);

        // Interaction system: calculate proximity-based hitbox (expanded and centered)
        Rectangle playerHitbox = {player->position.x - 75, player->position.y - 75, 500, 500};
        CheckInteractable(game_context->worldNPCs, game_context->worldItems, game_context->worldDoors, 
            game_context->npcCount, game_context->itemCount, game_context->doorCount,
            playerHitbox, &objectToInteractWith);

        Vector2 map_size = {(float)game_map->tiled_map->width * game_map->tiled_map->tilewidth,
                            (float)game_map->tiled_map->height * game_map->tiled_map->tileheight};

        // Input Handling: Interaction Trigger (E)
        if (IsKeyPressed(KEY_E) && *game_state == GAMEPLAY) {
            InteractWithObject(objectToInteractWith, current_dialogue, game_state, player, game_map, game_context);
        }

        // Input Handling: Dialogue Progression and Choice Selection
        if (*game_state == DIALOGUE_CUTSCENE) {
            InteractWithNPC(NULL, current_dialogue, game_state, game_context);
        }

        UpdatePhone(&game_context->phone, GetFrameTime());

        if (UpdateGame(
            game_state, game_interactive, player, game_settings, game_map,
            game_context, game_audio, map_size, game_scene
        )) break;

        DrawGame(game_scene, game_settings, game_interactive, game_map, player,
                 game_dialogue, game_context, game_state, game_context->worldNPCs, game_context->worldItems);
    }
}

void EndGame(Audio *game_audio, Character *player, Scene *game_scene,
             Interactive *game_interactive, Map *game_map,
             Settings *game_settings, GameContext *game_context){
    CloseAudio(game_audio);
    CloseCharacter(player);
    CloseScene(game_scene);
    CloseInteractive(game_interactive);
    FreeMap(game_map);
    CloseWindow();
}