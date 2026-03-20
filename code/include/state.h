/*
This file contains the enum for the states of the game.

Made by Steven Kenneth Darwy.
*/

#ifndef STATE_H
#define STATE_H

#include "character.h"
#include "game_context.h"
#include "interactive.h"
#include "map.h"
#include "settings.h"

typedef struct Scene Scene;

typedef enum {
  /* This enum contains the states of the game. */
  MAINMENU,          // Main menu state
  GAMEPLAY,          // Gameplay state
  PAUSE,             // Pause state
  SETTINGS,          // Settings state
  DIALOGUE_CUTSCENE, // Dialogue cutscene state
  PHOTO_CUTSCENE     // Photo cutscene state
} GameState;

// Update the game state
int UpdateGame(GameState *game_state, Interactive *game_interactive,
               Character *player, Settings *game_settings, Map *game_map,
               GameContext *game_context, Audio *game_audio, Vector2 map_size,
               Scene *game_scene);

#endif