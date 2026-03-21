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
#include "interaction.h"
#include "data.h"
#include "audio.h"
#include "interactive.h"

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
    struct Item *worldItems, int worldItemsCount, Settings* game_settings, Map* game_map, 
    GameContext* game_context, Audio* game_audio, Vector2 map_size, Scene* game_scene){

    switch(*game_state){
        case MAINMENU:
            // --- Main Menu Interaction ---
            UpdateInteractive(game_interactive, game_settings);
            
            if (game_interactive->is_new_game_clicked){
                // 1. Reset state and start cutscene
                ResetGameData(player, worldItems, worldItemsCount);
                *game_state = PHOTO_CUTSCENE;
                game_interactive->is_new_game_clicked = false;
                
                // 2. Audio transition
                PauseMusicStream(game_audio->bg_music);
                PlayMusicStream(game_audio->cutscene_music);
                
                // 3. Cutscene config (30 FPS)
                SetTargetFPS(30);
                game_scene->current_cutscene_frame = 0;
                game_scene->cutscene_timer = 0.0f;
                
            } else if (game_interactive->is_continue_clicked){
                // 1. Load data and start cutscene
                HandleGameData(player, worldItems, worldItemsCount, game_settings);
                *game_state = PHOTO_CUTSCENE; 
                game_interactive->is_continue_clicked = false;
                
                PauseMusicStream(game_audio->bg_music);
                PlayMusicStream(game_audio->cutscene_music);
                SetTargetFPS(30);
                game_scene->current_cutscene_frame = 0;
                game_scene->cutscene_timer = 0.0f;
                
            } else if (game_interactive->is_settings_clicked){
                game_context->settings_previous_state = *game_state;
                *game_state = SETTINGS;
            } else if (game_interactive->is_quit_clicked){
                return 1;
            }
            ShowCursor();
            break;

        case GAMEPLAY:
            // --- Active Gameplay ---
            // Input is suppressed while phone is active
            if (game_context->phone.state != PHONE_OPENED && game_context->phone.state != PHONE_SHOWING_REPLY){
                UpdateCharacter(player, game_settings, map_size, game_map, game_audio, game_context->is_outdoor);
                UpdateGameContext(game_context, game_settings, map_size);

                // Audio cue at specific progress
                if (player->position.x == 200) PlayScream(game_audio);

                // Trigger phone notification when passing x = 900
                if ((int)player->position.x >= 900 && game_context->phone.state == PHONE_IDLE){
                    PlayNotif(game_audio);
                    TriggerPhoneNotification(&game_context->phone, "You have new message", "Help me!", "I'm busy");
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
                *game_state = SETTINGS;
            } else if (game_interactive->is_main_menu_clicked){
                SaveData(player, worldItems, worldItemsCount, game_settings);
                *game_state = MAINMENU;
                UpdateInteractiveLayout(game_interactive, MAINMENU);
                game_interactive->is_main_menu_clicked = false;
            } else if (game_interactive->is_quit_clicked){
                SaveData(player, worldItems, worldItemsCount, game_settings);
                return 1;
            }
            ShowCursor();
            break;

        case SETTINGS:
            // --- Settings Adjustment ---
            UpdateInteractive(game_interactive, game_settings);
            if (IsKeyPressed(KEY_ESCAPE)){
                *game_state = game_context->settings_previous_state;
                UpdateInteractiveLayout(game_interactive, *game_state);
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
            if (game_scene->current_cutscene_frame < 896){
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