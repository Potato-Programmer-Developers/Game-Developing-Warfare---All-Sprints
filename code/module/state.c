/*
This file contains the logic for the game state.

Made by Steven Kenneth Darwy
*/

#include "state.h"
#include "scene.h"
#include "phone.h"

int UpdateGame(GameState* game_state, Interactive* game_interactive, 
    Character* player, Settings* game_settings, Map* game_map, 
    GameContext* game_context, Audio* game_audio, Vector2 map_size, Scene* game_scene){
    /* Update the game state */
    switch(*game_state){
        case MAINMENU:
            UpdateInteractive(game_interactive, game_settings);
            if (game_interactive->is_play_clicked){
                *game_state = PHOTO_CUTSCENE;
                game_interactive->is_play_clicked = false;
                PauseMusicStream(game_audio->bg_music);
                PlayMusicStream(game_audio->cutscene_music);
                SetTargetFPS(30);
                game_scene->current_cutscene_frame = 0;
                game_scene->cutscene_timer = 0.0f;
            }
            if (game_interactive->is_settings_clicked){
                *game_state = SETTINGS;
            }
            if (game_interactive->is_quit_clicked){
                return 1;
            }
            ShowCursor();
            break;
        case GAMEPLAY:
            if (game_context->phone.state != PHONE_OPENED && game_context->phone.state != PHONE_SHOWING_REPLY){
                UpdateCharacter(player, game_settings, map_size, game_map, game_audio, game_context->is_outdoor);
                UpdateGameContext(game_context, game_settings, map_size);

                if (player->position.x == 200) PlayScream(game_audio);

                // Trigger notification when passing x = 900
                if ((int)player->position.x >= 900 && game_context->phone.state == PHONE_IDLE){
                    PlayNotif(game_audio);
                    TriggerPhoneNotification(&game_context->phone, "You have new message", "Help me!", "I'm busy");
                }
            }

            HandlePhoneInput(&game_context->phone);

            if (game_context->phone.state == PHONE_OPENED || game_context->phone.state == PHONE_SHOWING_REPLY){
                ShowCursor();
            } else{
                HideCursor();
            }
            break;
        case PAUSE:
            UpdateInteractive(game_interactive, game_settings);
            if (game_interactive->is_play_clicked){
                if (game_context->previous_state == PHOTO_CUTSCENE) {
                    PauseMusicStream(game_audio->bg_music);
                    ResumeMusicStream(game_audio->cutscene_music);
                }
                *game_state = game_context->previous_state;
                game_interactive->is_play_clicked = false;
            }
            if (game_interactive->is_settings_clicked){
                *game_state = SETTINGS;
            }
            if (game_interactive->is_quit_clicked){
                return 1;
            }
            ShowCursor();
            break;
        case SETTINGS:
            UpdateInteractive(game_interactive, game_settings);
            if (IsKeyPressed(KEY_ESCAPE)){
                *game_state = MAINMENU;
            }
            ShowCursor();
            break;
        case DIALOGUE_CUTSCENE:
            HideCursor();
            break;
        case PHOTO_CUTSCENE:
            HideCursor();
            
            // Update cutscene timer
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
                    last_enter_press = -1.0f; // Reset
                    break;
                }
                last_enter_press = current_time;
            }

            // Update cutscene frame
            if (game_scene->current_cutscene_frame < 896){
                game_scene->current_cutscene_frame++;
                LoadCutsceneFrame(game_scene, game_scene->current_cutscene_frame, game_settings);
            } else{
                StopMusicStream(game_audio->cutscene_music);
                PlayMusicStream(game_audio->bg_music);
                SetTargetFPS(60);
                ClearCutscene(game_scene);
                *game_state = GAMEPLAY;
                game_scene->current_cutscene_frame = 0;      // Reset for next time
            }
            break;
        default:
            break;
    }

    return 0;
}