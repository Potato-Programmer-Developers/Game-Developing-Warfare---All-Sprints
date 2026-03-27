/**
 * @file data.h
 * @brief Structures and functions for managing persistent game data.
 * 
 * This module handles saving and loading player progress (position, inventory, state)
 * to disc, as well as managing the initial state for new games.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef DATA_H
#define DATA_H

#include "raylib.h"
#include "settings.h"
#include "interaction.h"

/**
 * @brief Represents the serializable state of the game world and player.
 * 
 * This structure is used to package all relevant information that needs to be
 * persisted between game sessions.
 */
typedef struct Data {
    // --- Player State ---
    Vector2 position;                                                // Last recorded player coordinates
    int direction;                                                   // Direction the player was facing
    char inventory[MAX_INVENTORY_SIZE][MAX_ITEM_NAME_LENGTH];        // List of item names in inventory
    int item_count[MAX_INVENTORY_SIZE];                              // Quantities for each inventory item
    int inventory_count;                                             // Total number of unique items
    float player_sanity_level;                                       // Current hallucination intensity

    // --- World State ---
    bool picked_up_items[1];                                         // Tracking which items have been removed from the world

    // --- Settings ---
    float volume;                                                    // User-defined audio volume
} Data;

/**
 * @brief Reads game state from a save file on disk.
 * @param game_settings Pointer to settings (used for file paths).
 * @return A Data struct populated with saved information.
 */
Data LoadData(Settings* game_settings);

/**
 * @brief Applies a Data structure's values to the active game entities.
 * @param player Pointer to the player to update.
 * @param worldItems Array of items to sync.
 * @param itemCount Number of items in the array.
 * @param game_settings Pointer to settings.
 * @param data Pointer to the source data to apply.
 */
void ApplyData(Character* player, Item worldItems[], int itemCount, Settings* game_settings, Data* data);

/**
 * @brief Writes current game state to a save file on disk.
 * @param player Pointer to the player character.
 * @param worldItems Array of currently tracked items.
 * @param itemCount Number of items in the array.
 * @param game_settings Pointer to settings.
 */
void SaveData(Character* player, Item worldItems[], int itemCount, Settings* game_settings);

/**
 * @brief Clears all player and item state to default values (New Game).
 * @param player Pointer to the player character.
 * @param worldItems Array of world items.
 * @param itemCount Number of items in the array.
 */
void ResetGameData(Character* player, Item worldItems[], int itemCount, Vector2 default_spawn);

/**
 * @brief High-level wrapper to load data and immediately apply it to the game.
 * @param player Pointer to the player.
 * @param worldItems Array of world items.
 * @param itemCount Number of items.
 * @param game_settings Pointer to settings.
 */
void HandleGameData(Character* player, Item worldItems[], int itemCount, Settings* game_settings, Map* game_map);

#endif