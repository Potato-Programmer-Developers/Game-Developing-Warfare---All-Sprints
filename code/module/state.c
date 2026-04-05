/**
 * @file state.c
 * @brief Implementation of the central game state machine, world update logic, and transition handling.
 * 
 * Update History:
 * - 2026-03-22: Foundation of the "Main Menu to Gameplay" state machine. (Goal: Create a stable 
 *                loop for switching between UI and active gameplay.)
 * - 2026-04-01: Added the "Pause and Settings" logic. (Goal: Allow the player to adjust the 
 *                experience mid-game without losing session progress.)
 * - 2026-04-05: Synchronized "Dialogue-to-Map" and "Phone-to-Gameplay" transitions. (Goal: Bridge 
 *                multiple systems so that narrative triggers correctly initiate camera fades and 
 *                map loads at the 'peak' of the transition.)
 * 
 * Revision Details:
 * - Implemented a global fade update hook in `UpdateGame` to manage asynchronous map transitions.
 * - Created logic to trigger `StartFadeTransition` based on `Dialogue` pending map targets.
 * - Integrated `UpdateStory` into the main loop to ensure objectives are polled every frame.
 * - Added safe state-restoration in the `PAUSE` and `SETTINGS` case blocks.
 * - Forced a 24 FPS target during settings menus for consistent UI performance.
 * 
 * Authors: Andrew Zhuo and Steven Kenneth Darwy
 */

#include "state.h"
#include "scene.h"
#include "phone.h"
#include "data.h"
#include "audio.h"
#include "interactive.h"
#include "assets.h"
#include "story.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Helper function to get color from name
 * 
 * @param name The name of the color
 * @return The color
 */
static Color GetColorFromName(const char* name){
    if (strcmp(name, "BLACK") == 0) return BLACK;
    if (strcmp(name, "WHITE") == 0) return WHITE;
    if (strcmp(name, "RED") == 0) return RED;
    if (strcmp(name, "BLANK") == 0) return BLANK;
    return BLACK; 
}

int UpdateGame(GameState* game_state, struct Interactive* game_interactive, Character* player, 
    Settings* game_settings, Map* game_map, GameContext* game_context, Audio* game_audio,
    Vector2 map_size, Scene* game_scene){
 
    static GameState last_state = MAINMENU;

    // Global fade update
    if (game_scene != NULL){
        UpdateFade(game_scene, GetFrameTime(), *game_state);
    }
    
    // Trigger deferred transitions when returning from dialogue to gameplay
    if (last_state == DIALOGUE_CUTSCENE && *game_state == GAMEPLAY){
        if (game_context->game_dialogue->pending_target_map[0] != '\0') {
            Color c = GetColorFromName(game_context->game_dialogue->pending_fade_color);
            StartFadeTransition(game_scene, c, game_context->game_dialogue->pending_target_map, game_context->game_dialogue->pending_target_loc);
            game_context->game_dialogue->pending_target_map[0] = '\0';
        }
    }
    last_state = *game_state;

    // Map transition trigger at peak of fade
    if (game_scene->fade_alpha >= 1.0f && game_scene->pending_map[0] != '\0'){
        Location targetLoc = INTERIOR; 
        if (strcmp(game_scene->pending_loc, "EXTERIOR") == 0) targetLoc = EXTERIOR;
        else if (strcmp(game_scene->pending_loc, "INTERIOR") == 0) targetLoc = INTERIOR;
        else if (strcmp(game_scene->pending_loc, "FARM") == 0) targetLoc = FARM;
        else if (strcmp(game_scene->pending_loc, "FOREST") == 0) targetLoc = FOREST;
        else if (strcmp(game_scene->pending_loc, "APARTMENT") == 0) targetLoc = APARTMENT;
        
        FreeMap(game_map);
        *game_map = InitMap(game_scene->pending_map);
        player->position = game_map->spawn_position;
        game_context->location = targetLoc;

        game_scene->pending_map[0] = '\0';
        game_scene->is_fading_in = true;
        game_scene->is_fading_out = false;
        game_scene->fade_alpha = 1.0f;
        
        StoryPhase* active = GetActivePhase(&game_context->story);
        if (active) LoadPhaseAssets(active, game_context);
        if (*game_state == DIALOGUE_CUTSCENE) *game_state = GAMEPLAY;
    }

    switch(*game_state){
        case MAINMENU: {
            static float menu_frame_timer = 0.0f;
            menu_frame_timer += GetFrameTime();
            bool save_exists = FileExists("../data/data.dat");
            if (menu_frame_timer >= 1.0f / 24.0f){
                if (game_scene->current_cutscene_frame < 1 || game_scene->current_cutscene_frame > 102) game_scene->current_cutscene_frame = 1;
                LoadMenuFrame(game_scene, game_scene->current_cutscene_frame, save_exists);
                game_scene->current_cutscene_frame++;
                if (game_scene->current_cutscene_frame > 102) game_scene->current_cutscene_frame = 1;
                menu_frame_timer = 0.0f;
            }
            UpdateInteractive(game_interactive, game_settings);
            if (game_interactive->is_new_game_clicked){
                ResetGameData(game_context, game_map->spawn_position);
                LoadStoryDay(&game_context->story, "../assets/text/day1/day1.txt");
                *game_state = GAMEPLAY;
                game_interactive->is_new_game_clicked = false;
                StopMusicStream(game_audio->bg_music);
                PlayMusicStream(game_audio->bg_music);
                SetTargetFPS(60);
            } else if (game_interactive->is_continue_clicked){
                HandleGameData(game_context, game_map, game_settings);
                *game_state = GAMEPLAY; 
                game_interactive->is_continue_clicked = false;
                StopMusicStream(game_audio->bg_music);
                PlayMusicStream(game_audio->bg_music);
                SetTargetFPS(60);
            } else if (game_interactive->is_settings_clicked){
                game_context->settings_previous_state = *game_state;
                *game_state = SETTINGS;
            } else if (game_interactive->is_quit_clicked) return 1;
            ShowCursor();
            break;
        }
        case GAMEPLAY:
            // Update character if phone is not opened
            if (game_context->phone.state != PHONE_OPENED && game_context->phone.state != PHONE_SHOWING_REPLY){
                UpdateCharacter(player, game_settings, map_size, game_map, game_audio, game_context->location, &game_context->story, game_context->worldItems, game_context->itemCount, game_context->worldNPCs, game_context->npcCount, game_context->worldDoors, game_context->doorCount, game_context->picked_up_registry, game_context->picked_up_count);
            }

            // Update game context and story
            UpdateGameContext(game_context, map_size);
            UpdateStory(game_context, GetFrameTime());

            static int last_phase = -1;
            static int last_set = -1;
            StoryPhase* active = GetActivePhase(&game_context->story);

            // Check if the story phase has changed
            if (active && (game_context->story.current_phase_idx != last_phase || game_context->story.current_set_idx != last_set)) {
                // Check if the story phase has changed location
                if (active->location != STORY_LOC_NONE && (Location)active->location != game_context->location){
                    if (game_scene->pending_map[0] == '\0'){
                        const char* targetMap = "";
                        const char* targetLocStr = "";
                        if (active->location == STORY_LOC_APARTMENT) {targetMap = "../assets/map/map_apart/APARTMENT_MAP.json"; targetLocStr = "APARTMENT";}
                        else if (active->location == STORY_LOC_EXTERIOR) {targetMap = "../assets/map/map_ext/MAINMAP.json"; targetLocStr = "EXTERIOR";}
                        else if (active->location == STORY_LOC_INTERIOR) {targetMap = "../assets/map/map_int/MAIN_MAP_INT.json"; targetLocStr = "INTERIOR";}
                        else if (active->location == STORY_LOC_FARM) {targetMap = "../assets/map/map_farm/FARM.json"; targetLocStr = "FARM";}
                        StartFadeTransition(game_scene, BLACK, targetMap, targetLocStr);
                    }
                } else{
                    LoadPhaseAssets(active, game_context);
                    last_phase = game_context->story.current_phase_idx;
                    last_set = game_context->story.current_set_idx;
                }

                // Check end conditions
                if (active->location != STORY_LOC_NONE && (Location)active->location == game_context->location){
                    for (int i = 0; i < active->condition_count; i++){
                        if (active->end_conditions[i].type == CONDITION_ENTER_LOCATION && (int)active->end_conditions[i].target_value == (int)active->location){
                            active->end_conditions[i].met = true;
                        }
                    }
                }
            }

            if (game_scene->fade_alpha > 0.9f && game_scene->is_fading_in){
                 last_phase = game_context->story.current_phase_idx;
                 last_set = game_context->story.current_set_idx;
            }
            
            // Check if phone needs to be triggered after map has been handled
            if (game_context->story.phone_pending && active && (active->location == STORY_LOC_NONE || (Location)active->location == game_context->location)){
                TriggerPhoneSequence(&game_context->phone, active->phone_sender, active->phone_messages, active->phone_message_count);
                game_context->story.phone_pending = false;
                PlaySound(game_audio->notif_sound);
            }
            HandlePhoneInput(&game_context->phone, game_context);
            if (game_context->phone.state == PHONE_OPENED || game_context->phone.state == PHONE_SHOWING_REPLY) ShowCursor();
            else HideCursor();

            // Check if player presses ESCAPE to pause the game
            if (IsKeyPressed(KEY_ESCAPE)){
                game_context->previous_state = *game_state;
                *game_state = PAUSE;
                UpdateInteractiveLayout(game_interactive, PAUSE, game_settings);
                ShowCursor();
            }
            break;
        case PAUSE:
            UpdateInteractive(game_interactive, game_settings);

            // Check if player clicks buttons in pause menu
            if (game_interactive->is_continue_clicked){
                if (game_context->previous_state == PHOTO_CUTSCENE) PauseMusicStream(game_audio->bg_music);
                *game_state = game_context->previous_state;
                game_interactive->is_continue_clicked = false;
            } else if (game_interactive->is_settings_clicked){
                game_context->settings_previous_state = *game_state;
                SetTargetFPS(24);
                *game_state = SETTINGS;
            } else if (game_interactive->is_main_menu_clicked){
                SaveData(game_context, game_settings);
                *game_state = MAINMENU;
                UpdateInteractiveLayout(game_interactive, MAINMENU, game_settings);
                game_interactive->is_main_menu_clicked = false;
            } else if (game_interactive->is_quit_clicked){
                SaveData(game_context, game_settings);
                return 1;
            }
            ShowCursor();
            break;
        case SETTINGS:
            // Reset cutscene frame if needed
            if (game_scene->current_cutscene_frame < 1 || game_scene->current_cutscene_frame > 102) game_scene->current_cutscene_frame = 1;

            // Load settings frame
            LoadSettingsFrame(game_scene, game_scene->current_cutscene_frame);
            LoadKnobFrame(game_scene, game_scene->current_cutscene_frame);
            game_scene->current_cutscene_frame++;
            if (game_scene->current_cutscene_frame > 102) game_scene->current_cutscene_frame = 1;
            UpdateInteractive(game_interactive, game_settings);

            // Check if player clicks buttons in settings menu
            if (game_interactive->is_settings_back_clicked){
                if (game_context->settings_previous_state == PAUSE) SetTargetFPS(60);
                *game_state = game_context->settings_previous_state;
                UpdateInteractiveLayout(game_interactive, *game_state, game_settings);
            }
            ShowCursor();
            break;
        case DIALOGUE_CUTSCENE:
            HideCursor();
            break;
        case NARRATION_CUTSCENE:
            // Keep story timer running for phone auto-advance during narration
            UpdateStory(game_context, GetFrameTime());
            HideCursor();
            break;
        case PHOTO_CUTSCENE:
            HideCursor(); game_scene->cutscene_timer += GetFrameTime();
            static float last_enter_press = -1.0f;

            // Check if player double-presses ENTER to skip the cutscene
            if (IsKeyPressed(KEY_ENTER)) {
                float current_time = GetTime();
                if (last_enter_press > 0 && (current_time - last_enter_press) < 0.3f) {
                    PlayMusicStream(game_audio->bg_music);
                    SetTargetFPS(60);
                    ClearCutscene(game_scene);
                    *game_state = GAMEPLAY; last_enter_press = -1.0f; break;
                }
                last_enter_press = current_time;
            }

            // Load cutscene frame if there are more frames to load
            if (game_scene->current_cutscene_frame < 717){
                game_scene->current_cutscene_frame++;
                LoadCutsceneFrame(game_scene, game_scene->current_cutscene_frame, game_settings);
            } else{
                PlayMusicStream(game_audio->bg_music);
                SetTargetFPS(60);
                ClearCutscene(game_scene);
                *game_state = GAMEPLAY; game_scene->current_cutscene_frame = 0;
            }
            break;
        default: break;
    }
    return 0;
}