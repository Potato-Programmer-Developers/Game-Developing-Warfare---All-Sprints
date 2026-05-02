/**
 * @file data.h
 * @brief Interfaces for game persistence and world-state serialization.
 * 
 * Update History:
 * - 2026-05-02: Added `DeleteSaveData` function prototype. (Goal: Expose save file deletion
 *                for post-credits cleanup to ensure a fresh "New Game" state on next launch.)
 * 
 * Revision Details:
 * - Added `void DeleteSaveData(void)` prototype for removing `data.dat` after credits.
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
    float sanity;                                                    // Current hallucination intensity

    // --- Story State ---
    char day_folder[32];                                             // e.g. "day1"
    int set_idx;                                                     // current set index
    int phase_idx;                                                   // current phase index
    int location;                                                    // current location index
    char map_path[128];                                              // current map file path

    // --- NPC Karma ---
    int npc_karma[64];                                               // Persistent karma for all assets

    // --- Quest Status ---
    bool quest_completion[10];                                       // Completion flags for active phase quests

    // --- World State ---
    char picked_up_registry[512][64];                                 // Tracking which items have been removed from the world
    int picked_up_count;                                             // Total number of items collected
    
    char met_npcs[64][64];                                           // Registry of unique NPC IDs spoken to
    int met_npc_day[64];                                             // Day index when met
    int met_npc_set[64];                                             // Set index when met
    int met_npc_phase[64];                                           // Phase index when met
    int met_npc_count;                                               // Number of NPCs spoken to

    PotStatus pot_registry[18];                                      // Planted tracking for Day 2
    UsedRegistry dialogue_used_lines[256];                           // Once-only lines memory
    int used_lines_count;                                            // Count of used lines

    // --- Settings ---
    float volume;                                                    // User-defined audio volume
} Data;

/**
 * @brief Reads game state from a save file on disk.
 *
 * @param game_settings Pointer to settings (used for file paths).
 * @return A Data struct populated with saved information.
 */
Data LoadData(Settings* game_settings);

/**
 * @brief Applies a Data structure's values to the active game entities.
 *
 * @param context Pointer to the game context to update.
 * @param game_settings Pointer to settings.
 * @param data Pointer to the source data to apply.
 */
void ApplyData(struct GameContext* context, Settings* game_settings, Data* data);

/**
 * @brief Writes current game state to a save file on disk.
 *
 * @param context Pointer to the game context.
 * @param game_settings Pointer to settings.
 */
void SaveData(struct GameContext* context, Settings* game_settings);

/**
 * @brief Clears all player and item state to default values (New Game).
 *
 * @param context Pointer to the game context.
 * @param default_spawn The default spawn position.
 */
void ResetGameData(struct GameContext* context, Vector2 default_spawn);

/**
 * @brief High-level wrapper to load data and immediately apply it to the game.
 *
 * @param context Pointer to the game context.
 * @param game_map Pointer to the map.
 * @param game_settings Pointer to settings.
 */
void HandleGameData(struct GameContext* context, Map* game_map, Settings* game_settings);

/**
 * @brief Deletes the save file from disk.
 */
void DeleteSaveData(void);

#endif