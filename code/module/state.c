/**
 * @file state.c
 * @brief Implementation of the central game state machine and world update logic.
 * 
 * Manages the transition between menus, gameplay, and cutscenes. Handles
 * high-level input polling and calls specific system updates (physics, phone, etc).
 * 
 * Authors: Steven Kenneth Darwy
 */

#include "state.h"
#include "scene.h"
#include "phone.h"
#include "data.h"
#include "audio.h"
#include "interactive.h"
#include "assets.h"
#include <stdio.h>

/**
 * @brief Logic orchestrator for state-based updates.
 * 
 * This function is called once per frame. It routes execution based on the 
 * current value of game_state and manages transitions like starting a new game,
 * loading data, or handling the intro cutscene.
 * 
 * @return 1 if the game should Quit, 0 otherwise.
 */
int UpdateGame(GameState* game_state, Interactive* game_interactive, Character* player, 
    Settings* game_settings, Map* game_map, 
    GameContext* game_context, Audio* game_audio, Vector2 map_size, Scene* game_scene){

    switch(*game_state){
        case MAINMENU: {
            // --- Main Menu Animation (Throttled for Performance) ---
            static float menu_frame_timer = 0.0f;
            menu_frame_timer += GetFrameTime();
            
            bool save_exists = FileExists("../data/data.dat");
            
            // Only load a new frame at 24 FPS to avoid IO bottlenecks
            if (menu_frame_timer >= 1.0f/24.0f) {
                if (game_scene->current_cutscene_frame < 1 || game_scene->current_cutscene_frame > 102) {
                    game_scene->current_cutscene_frame = 1;
                }
                LoadMenuFrame(game_scene, game_scene->current_cutscene_frame, save_exists);
                game_scene->current_cutscene_frame++;
                if (game_scene->current_cutscene_frame > 102) game_scene->current_cutscene_frame = 1;
                menu_frame_timer = 0.0f;
            }

            // --- Main Menu Interaction ---
            UpdateInteractive(game_interactive, game_settings);
            
            if (game_interactive->is_new_game_clicked){
                // 1. Reset state and start gameplay directly
                ResetGameData(player, game_context->worldItems, game_context->itemCount, game_map->spawn_position);
                *game_state = GAMEPLAY;
                game_interactive->is_new_game_clicked = false;
                
                // 2. Audio transition
                StopMusicStream(game_audio->bg_music);
                PlayMusicStream(game_audio->bg_music);
                
                // 3. Gameplay config (60 FPS)
                SetTargetFPS(60);
                
            } else if (game_interactive->is_continue_clicked){
                // 1. Load data and start gameplay directly
                HandleGameData(player, game_context->worldItems, game_context->itemCount, game_settings, game_map);
                *game_state = GAMEPLAY; 
                game_interactive->is_continue_clicked = false;
                
                StopMusicStream(game_audio->bg_music);
                PlayMusicStream(game_audio->bg_music);
                SetTargetFPS(60);
                
            } else if (game_interactive->is_settings_clicked){
                game_context->settings_previous_state = *game_state;
                *game_state = SETTINGS;
            } else if (game_interactive->is_quit_clicked){
                return 1;
            }
            ShowCursor();
            break;
        }

        case GAMEPLAY:
            // --- Active Gameplay ---
            // Input is suppressed while phone is active
            if (game_context->phone.state != PHONE_OPENED && game_context->phone.state != PHONE_SHOWING_REPLY){
                UpdateCharacter(player, game_settings, map_size, game_map, game_audio, game_context->location, &game_context->story);
            }
            
            UpdateGameContext(game_context, map_size);
            UpdateStory(&game_context->story, GetFrameTime());

            // --- Phase Change Detection & Dynamic Asset Loading ---
            static int last_phase = -1;
            static int last_set = -1;
            StoryPhase* active = GetActivePhase(&game_context->story);
            if (active && (game_context->story.current_phase_idx != last_phase || game_context->story.current_set_idx != last_set)) {
                LoadPhaseAssets(active, game_context);
                last_phase = game_context->story.current_phase_idx;
                last_set = game_context->story.current_set_idx;
                
                if (game_context->player_teleport_requested) {
                    // Swap Map based on new location
                    if (active->location == STORY_LOC_INTERIOR) {
                        FreeMap(game_map);
                        *game_map = InitMap("../assets/map/map_int/APARTMENT MAP.json");
                    } else if (active->location == STORY_LOC_EXTERIOR) {
                        FreeMap(game_map);
                        *game_map = InitMap("../assets/map/map_ext/MAINMAP.json");
                    }
                    player->position = game_map->spawn_position;
                    game_context->player_teleport_requested = false;
                    TraceLog(LOG_INFO, "PHASE TRANSITION: Teleported to %s spawn.", (active->location == STORY_LOC_INTERIOR) ? "Interior" : "Exterior");
                }
            }

            HandlePhoneInput(&game_context->phone);

            // Cursor visibility toggling
            if (game_context->phone.state == PHONE_OPENED || game_context->phone.state == PHONE_SHOWING_REPLY){
                ShowCursor();
            } else{
                HideCursor();
            }
            break;

        case PAUSE:
            // --- Pause Menu ---
            UpdateInteractive(game_interactive, game_settings);
            if (game_interactive->is_continue_clicked){
                if (game_context->previous_state == PHOTO_CUTSCENE) {
                    PauseMusicStream(game_audio->bg_music);
                    ResumeMusicStream(game_audio->cutscene_music);
                }
                *game_state = game_context->previous_state;
                game_interactive->is_continue_clicked = false;
            } else if (game_interactive->is_settings_clicked){
                game_context->settings_previous_state = *game_state;
                SetTargetFPS(24);
                *game_state = SETTINGS;
            } else if (game_interactive->is_main_menu_clicked){
                SaveData(player, game_context->worldItems, game_context->itemCount, game_settings);
                *game_state = MAINMENU;
                UpdateInteractiveLayout(game_interactive, MAINMENU, game_settings);
                game_interactive->is_main_menu_clicked = false;
            } else if (game_interactive->is_quit_clicked){
                SaveData(player, game_context->worldItems, game_context->itemCount, game_settings);
                return 1;
            }
            ShowCursor();
            break;

        case SETTINGS:
            // --- Settings Animation ---
            if (game_scene->current_cutscene_frame < 1 || game_scene->current_cutscene_frame > 102) {
                game_scene->current_cutscene_frame = 1;
            }
            LoadSettingsFrame(game_scene, game_scene->current_cutscene_frame);
            LoadKnobFrame(game_scene, game_scene->current_cutscene_frame);
            game_scene->current_cutscene_frame++;
            if (game_scene->current_cutscene_frame > 102) game_scene->current_cutscene_frame = 1;

            // --- Settings Adjustment ---
            UpdateInteractive(game_interactive, game_settings);
            if (game_interactive->is_settings_back_clicked){
                if (game_context->settings_previous_state == PAUSE) {
                    SetTargetFPS(60);
                }
                *game_state = game_context->settings_previous_state;
                UpdateInteractiveLayout(game_interactive, *game_state, game_settings);
            }
            ShowCursor();
            break;

        case DIALOGUE_CUTSCENE:
            HideCursor();
            break;

        case PHOTO_CUTSCENE:
            // --- Intro Cutscene Playback ---
            HideCursor();
            game_scene->cutscene_timer += GetFrameTime();
            
            // Allow skipping the cutscene with double-click ENTER
            static float last_enter_press = -1.0f;
            if (IsKeyPressed(KEY_ENTER)) {
                float current_time = GetTime();
                if (last_enter_press > 0 && (current_time - last_enter_press) < 0.3f) {
                    StopMusicStream(game_audio->cutscene_music);
                    PlayMusicStream(game_audio->bg_music);
                    SetTargetFPS(60);
                    ClearCutscene(game_scene);
                    *game_state = GAMEPLAY;
                    last_enter_press = -1.0f;
                    break;
                }
                last_enter_press = current_time;
            }

            // Frame-by-frame loading
            if (game_scene->current_cutscene_frame < 717){
                game_scene->current_cutscene_frame++;
                LoadCutsceneFrame(game_scene, game_scene->current_cutscene_frame, game_settings);
            } else{
                // Sequence finished
                StopMusicStream(game_audio->cutscene_music);
                PlayMusicStream(game_audio->bg_music);
                SetTargetFPS(60);
                ClearCutscene(game_scene);
                *game_state = GAMEPLAY;
                game_scene->current_cutscene_frame = 0;
            }
            break;

        default:
            break;
    }

    return 0;
}