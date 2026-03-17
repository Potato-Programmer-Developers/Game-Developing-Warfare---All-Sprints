/*
This file contains the logic for the game state.

Made by Steven Kenneth Darwy
*/

#include "state.h"

int UpdateGameState(GameState* game_state, Interactive* game_interactive, 
    Character* player, Settings* game_settings, Map* game_map, 
    GameContext* game_context, Audio* game_audio, Vector2 map_size){
    /* Update the game state */
    switch(*game_state){
        case MAINMENU:
            UpdateInteractive(game_interactive, game_settings);
            if (game_interactive->is_play_clicked){
                *game_state = GAMEPLAY;
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
            UpdateCharacter(player, game_settings, map_size, game_map);
            UpdateGameContext(game_context, game_settings, map_size);

            if (player->position.x == 200)
                PlayScream(game_audio);
            HideCursor();
            break;
        case PAUSE:
            UpdateInteractive(game_interactive, game_settings);
            if (game_interactive->is_play_clicked){
                *game_state = GAMEPLAY;
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
        default:
            break;
    }

    return 0;
}