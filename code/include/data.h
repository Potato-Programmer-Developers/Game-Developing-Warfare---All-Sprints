/*
This file contains the data structures for the game.

Made by Andrew Zhuo.
*/

#ifndef DATA_H
#define DATA_H

#include "raylib.h"
#include "character.h"
#include "settings.h"

typedef struct Data{
    /* Struct for game data */
    Vector2 position;                                                // Position of the player
    int direction;                                                   // Direction the player is facing
    char inventory[MAX_INVENTORY_SIZE][MAX_ITEM_NAME_LENGTH];        // Inventory of the player
    int inventory_count;                                             // Number of items in the inventory

    int volume;                                                      // Volume of the game
} Data;

Data LoadData(Settings* game_settings);                                          // Load game data
void ApplyData(Character* player, Settings* game_settings, Data* data);          // Apply game data
void SaveData(Character* player, Settings* game_settings);                       // Save game data

#endif