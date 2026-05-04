/**
 * @file main.c
 * @brief Entry point and central orchestration for the project.
 * 
 * Update History:
 * - 2026-03-21: Initial creation of the game loop and system initialization. (Goal: Establish 
 *                a stable 60 FPS loop that coordinates audio, input, and graphics.)
 * - 2026-04-01: Integrated session-loading and state management. (Goal: Support resuming 
 *                gameplay from `data.dat` and switching between Menu/Play states.)
 * - 2026-04-05: Real-time synchronization of Narration, Phone, and Fade states. (Goal: Create 
 *                a reactive main loop where narrative beats correctly trigger UI layers and map switches.)
 * - 2026-04-07: Added `narration_has_started` Logic. (Goal: Synchronize the narrative 
 *                engine with the main game loop to prevent premature quest completion in 
 *                interactive phases.)
 * - 2026-05-02: Added `ENDING_CUTSCENE` input handling in the main loop. (Goal: Route
 *                `HandleEndingInput` when the game state is `ENDING_CUTSCENE`, enabling typed
 *                dialogue progression and credits interaction.)
 * - 2026-05-02: Prevented erroneous save-on-exit after credits. (Goal: Ensure that quitting
 *                after reaching the end of the game does not re-save stale data, maintaining
 *                a fresh "New Game" state on next launch.)
 * - 2026-05-02: Updated `LoadStoryDay` call to pass `GameContext`. (Goal: Support conditional
 *                quest loading that requires access to persistent game state.)
 * - 2026-05-03: Synchronized global `UpdatePhone` and photo overlay timers. (Goal: Ensure 
 *                consistent UI timing across all narrative cutscenes.)
 * 
 * Revision Details:
 * - Refactored the main loop to support `NARRATION_CUTSCENE` as a blocking state.
 * - Expanded the `playerHitbox` logic to provide a 40px interaction buffer.
 * - Implemented a "Fade-Safe" trigger for narrative prompts to prevent UI overlapping during maps loads.
 * - Added comprehensive cleanup calls for `CloseScene`, `CloseAudio`, and `FreeMap` on exit.
 * - Updated the `narration_pending` processor to set `narration_has_started = true` 
 *    once the fade is settled and the UI is active.
 * - Optimized the call order of `UpdateStory` and `UpdatePhone` to ensure better 
 *    state transitions.
 * - Added `if (*game_state == ENDING_CUTSCENE) HandleEndingInput(...)` block in `RunGame`.
 * - Updated `EndGame` signature to accept `GameState game_state` and gated `SaveData` behind
 *    `if (game_state != MAINMENU)`.
 * - Updated all `LoadStoryDay` calls to pass the `game_context` pointer.
 * - Optimized the call order of `UpdateStory` and `UpdatePhone` to ensure better 
 *    state transitions and global timer synchronization.
 * 
 * Authors: Andrew Zhuo, Cornelius Jabez Lim, Steven Kenneth Darwy
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
             Settings *game_settings, GameContext *game_context, GameState game_state);

int main(void){
    Settings game_settings = InitSettings();
    InitGame(&game_settings);

    Data game_data = LoadData(&game_settings);
    Map game_map = InitMap("../assets/map/map_apart/APARTMENT_MAP.json", NULL);
    Character player = InitCharacter(&game_settings, &game_data, &game_map);
    Audio game_audio = InitAudio(&game_settings);
    Scene game_scene = InitScene(&game_settings);
    Interactive game_interactive = InitInteractive(&game_settings);
    
    Dialogue* game_dialogue = (Dialogue*)malloc(sizeof(Dialogue));
    memset(game_dialogue, 0, sizeof(Dialogue));
    
    GameContext* game_context = (GameContext*)malloc(sizeof(GameContext));
    InitGameContext(game_context, &game_map, &player, APARTMENT);
    game_context->game_scene = &game_scene;
    game_context->game_dialogue = game_dialogue;
    
    LoadStoryDay(&game_context->story, "../assets/text/day1/day1.txt", game_context);
    StoryPhase* initial = GetActivePhase(&game_context->story);
    LoadPhaseAssets(initial, game_context);
    
    GameState game_state = MAINMENU;

    RunGame(&player, &game_audio, &game_settings, &game_scene, &game_interactive,
            game_dialogue, &game_map, game_context, &game_state);
    
    EndGame(&game_audio, &player, &game_scene, &game_interactive, &game_map, &game_settings, game_context, game_state);
    free(game_context);
    
    return 0;
}

void InitGame(Settings *game_settings){
    // Set log level to warning
    SetTraceLogLevel(LOG_WARNING);
    // Set window configuration
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    // Set target FPS
    SetTargetFPS(game_settings->fps);
    // Initialize window
    InitWindow(game_settings->window_width, game_settings->window_height, "Aisling");
    // Load icon
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
        Rectangle playerHitbox = {player->position.x - 40, player->position.y - 30, player->size.x + 80, player->size.y + 60};
        CheckInteractable(game_context->worldNPCs, game_context->worldItems, game_context->worldDoors, 
            game_context->npcCount, game_context->itemCount, game_context->doorCount,
            playerHitbox, &objectToInteractWith);
            
        Vector2 map_size = {(float)game_map->tiled_map->width * game_map->tiled_map->tilewidth,
            (float)game_map->tiled_map->height * game_map->tiled_map->tileheight};
                
        // Input Handling: Interaction Trigger (E)
        if (IsKeyPressed(KEY_E) && *game_state == GAMEPLAY) {
            InteractWithObject(objectToInteractWith, current_dialogue, game_state, player, game_map, game_context, game_audio);
        }
                
        // Input Handling: Dialogue Progression and Choice Selection
        if (*game_state == DIALOGUE_CUTSCENE) {
            InteractWithNPC(NULL, current_dialogue, game_state, game_context, game_audio);
        }
                
        // Input Handling: Narration Progression and Choice Selection
        if (*game_state == NARRATION_CUTSCENE) {
            HandleNarrationInput(game_context, (int*)game_state, game_audio);
        }
                
        // Input Handling: Opening/Ending Sequence
        if (*game_state == OPENING_CUTSCENE) {
            HandleOpeningInput(game_context, (int*)game_state, game_audio);
        }
        if (*game_state == ENDING_CUTSCENE) {
            HandleEndingInput(game_context, (int*)game_state, game_audio);
        }
                
        // Trigger NARRATION_CUTSCENE when narration becomes active during GAMEPLAY
        if (*game_state == GAMEPLAY && game_context->story.narration_active) {
            // Only enter if fade is not in progress (camera/map are settled)
            if (!game_scene->is_fading_out && !game_scene->is_fading_in && game_scene->fade_alpha <= 0.01f) {
                *game_state = NARRATION_CUTSCENE;
            }
        }
                
        // Activate pending narration once fade is fully done
        if (*game_state == GAMEPLAY && game_context->story.narration_pending) {
            if (!game_scene->is_fading_out && !game_scene->is_fading_in && game_scene->fade_alpha <= 0.01f) {
                game_context->story.narration_pending = false;
                game_context->story.narration_active = true;
                game_context->story.narration_has_started = true;
                game_context->story.narration_current_line = 0;
                game_context->story.narration_in_loop = false;
                game_context->story.narration_showing_response = false;
                game_context->story.narration_typing_index = 0;
                game_context->story.narration_typing_timer = 0.0f;
            }
        }
                
        // Update phone
        UpdatePhone(&game_context->phone, GetFrameTime());
                
        // Update game
        if (UpdateGame(
            game_state, game_interactive, player, game_settings, game_map,
            game_context, game_audio, map_size, game_scene
        )) break;
                
        // Draw game
        DrawGame(game_scene, game_settings, game_interactive, game_map, player,
            game_dialogue, game_context, game_state, game_context->worldNPCs,
            game_context->worldItems);
    }
}

void EndGame(Audio *game_audio, Character *player, Scene *game_scene,
             Interactive *game_interactive, Map *game_map,
             Settings *game_settings, GameContext *game_context, GameState game_state){
    // Save data only if not in MAINMENU
    if (game_state != MAINMENU) {
        SaveData(game_context, game_settings);
    }
    // Close audio
    CloseAudio(game_audio);
    // Close character
    CloseCharacter(player);
    // Close scene
    CloseScene(game_scene);
    // Close map
    FreeMap(game_map);
    // Close window
    CloseWindow();
}