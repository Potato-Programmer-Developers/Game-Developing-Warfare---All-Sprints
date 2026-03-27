/**
 * @file data.c
 * @brief Implementation of the game's persistence (Save/Load) system.
 * 
 * Uses binary file I/O to store and retrieve the player's state,
 * inventory, world progress, and user settings.
 * 
 * Authors: Andrew Zhuo
 */

#include <string.h>
#include "data.h"
#include "raylib.h"
#include "settings.h"

/**
 * @brief Loads the Data struct from the filesystem.
 * 
 * Checks for the existence of 'data.dat'. If found, reads the binary 
 * content directly into a Data structure. If not, returns a default struct.
 * 
 * @param game_settings Used for default volume if no save file exists.
 * @return Populated Data structure.
 */
Data LoadData(Settings* game_settings){
    Data data = {0};
    int file_size = 0;

    if (FileExists("../data/data.dat")){
        // Load raw binary data from disk
        unsigned char* file_data = LoadFileData("../data/data.dat", &file_size);

        // Safety check: ensure the file matches our expected structure size
        if (file_data != NULL && file_size == sizeof(Data)){
            memcpy(&data, file_data, sizeof(Data));
        }

        UnloadFileData(file_data); // Free the temporary buffer allocated by Raylib
    } else {
        // Sentinel value for "no save found"
        data.position = (Vector2){ -1.0f, -1.0f };
        data.volume = game_settings->game_volume;
    }

    return data;
}

/**
 * @brief Maps values from a Data struct onto active game objects.
 * 
 * Called after LoadData to synchronize the live game state with saved values.
 */
void ApplyData(Character* player, Item worldItems[], int itemCount, Settings* game_settings, Data* data){
    // 1. Sync Player State
    if (data->position.x != -1.0f && data->position.y != -1.0f) {
        player->position = data->position;
    }
    player->direction = data->direction;
    
    // Restore inventory strings and counts
    for (int i = 0; i < data->inventory_count; i++){
        strcpy(player->inventory[i], data->inventory[i]);
        player->item_count[i] = data->item_count[i];
    }
    player->inventory_count = data->inventory_count;
    player->sanity = data->player_sanity_level;

    // 2. Sync World State (which items are gone)
    for (int i = 0; i < itemCount; i++){
        worldItems[i].picked_up = data->picked_up_items[i];
    }

    // 3. Sync Engine Settings
    game_settings->game_volume = data->volume;
    SetMasterVolume(game_settings->game_volume);
}

/**
 * @brief Serializes current game state and writes it to a binary file.
 */
void SaveData(Character* player, Item worldItems[], int itemCount, Settings* game_settings){
    Data data = {0};

    // 1. Harvest Player State
    data.position = player->position;
    data.direction = player->direction;
    for (int i = 0; i < player->inventory_count; i++){
        strcpy(data.inventory[i], player->inventory[i]);
        data.item_count[i] = player->item_count[i];
    }
    data.inventory_count = player->inventory_count;
    data.player_sanity_level = player->sanity;

    // 2. Harvest World State
    for (int i = 0; i < itemCount; i++){
        data.picked_up_items[i] = worldItems[i].picked_up;
    }

    // 3. Harvest Settings
    data.volume = game_settings->game_volume;

    // 4. Persistence Logic
    // Ensure the destination folder exists before writing
    if (!DirectoryExists("../data")){
        MakeDirectory("../data");
    }

    // Write the entire struct as a single binary block
    SaveFileData("../data/data.dat", &data, sizeof(Data));
}

/**
 * @brief Hard reset of local object state (New Game logic).
 */
void ResetGameData(Character* player, Item worldItems[], int itemCount, Vector2 default_spawn){
    player->position = default_spawn;
    player->inventory_count = 0;
    player->sanity = 0.0f;
    player->direction = 0;
    
    for (int i = 0; i < itemCount; i++){
        worldItems[i].picked_up = false;
    }
}

/**
 * @brief High-level orchestrator for data handling during startup.
 */
void HandleGameData(Character* player, Item worldItems[], int itemCount, Settings* game_settings, Map* game_map){
    Data data = LoadData(game_settings);
    
    // If no position is found, we assume it's a fresh start or corrupted file
    if (data.position.x == -1.0f){
        Vector2 spawn = game_map->spawn_position;
        ResetGameData(player, worldItems, itemCount, spawn);
    } else {
        ApplyData(player, worldItems, itemCount, game_settings, &data);
    }
}