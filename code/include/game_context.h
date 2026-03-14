/*
This file contains the game context struct and functions.

Made by Andrew Zhuo.
*/

#ifndef GAME_CONTEXT_H
#define GAME_CONTEXT_H

#include "map.h"
#include "character.h"
#include "settings.h"

typedef struct{
    /* Game context */
    Map* map;                  // The map of the game.
    Character* player;         // The player of the game.
    Camera2D camera;           // The camera of the game.
} GameContext;

GameContext InitGameContext(Map* map, Character* player, Settings* settings);             // Initialize the game context.
void UpdateGameContext(GameContext* context, Settings* settings, Vector2 map_size);       // Update the game context.

#endif