/**
 * @file data.c
 * @brief Handles file serialization, world restoration, and map persistence.
 * 
 * Update History:
 * - 2026-03-23: Initial implementation of `SaveData` and `LoadData`. (Goal: Provide 
 *                persistent storage for player stats and map paths.)
 * - 2026-04-02: Added NPC and Pickup history tracking. (Goal: Ensure that world changes, 
 *                like talking to the farmer, are remembered across sessions.)
 * - 2026-04-05: Integrated the `StorySystem` save hook. (Goal: Store current Set/Phase 
 *                indices to allow accurate story resumption.)
 * 
 * Revision Details:
 * - Refactored `LoadData` to include a path validation check for missing assets.
 * - Implemented binary file writing in `SaveData` to improve read/write speed.
 * - Added a restoration hook for `Location` enums to ensure the player spawns in the 
 *    correct map type (Interior/Exterior).
 * - Fixed a data-truncation bug when saving long NPC ID strings.
 * 
 * Authors: Andrew Zhuo
 */

#include <string.h>
#include "data.h"
#include "raylib.h"
#include "settings.h"
#include "game_context.h"
#include "assets.h"
#include "story.h"
#include <stdio.h>

Data LoadData(Settings* game_settings){
    Data data = {0};
    int file_size = 0;

    // Check if the data file exists
    if (FileExists("../data/data.dat")){
        unsigned char* file_data = LoadFileData("../data/data.dat", &file_size);
        // Check if the data file is valid
        if (file_data != NULL && file_size == sizeof(Data)){
            memcpy(&data, file_data, sizeof(Data));       // Copy the data from the file to the data struct
        }
        UnloadFileData(file_data);                      // Unload the data from the file
    } else {
        data.position = (Vector2){-1.0f, -1.0f};        // Set the position to -1, -1 to indicate that the data has not been loaded
        data.volume = game_settings->game_volume;       // Set the volume to the game settings
    }
    return data;
}

void ApplyData(struct GameContext* context, Settings* game_settings, Data* data){
    Character* player = context->player;
    
    // 1. Restore Player State
    if (data->position.x != -1.0f) player->position = data->position;
    player->direction = data->direction;
    player->sanity = data->sanity;
    for (int i = 0; i < data->inventory_count; i++){
        strcpy(player->inventory[i], data->inventory[i]);
        player->item_count[i] = data->item_count[i];
    }
    player->inventory_count = data->inventory_count;

    // 2. Restore Story State
    if (strlen(data->day_folder) > 0) {
        char path[128];
        sprintf(path, "../assets/text/%s/%s.txt", data->day_folder, data->day_folder);
        LoadStoryDay(&context->story, path);
        context->story.current_set_idx = data->set_idx;
        context->story.current_phase_idx = data->phase_idx;
        context->location = (Location)data->location;

        // Map & Location Restoration
        if (data->map_path[0] != '\0' && strcmp(data->map_path, context->map->current_path) != 0) {
            FreeMap(context->map);
            *(context->map) = InitMap(data->map_path, NULL);
        }
        
        // Restore quest completion for active phase
        StoryPhase* active = GetActivePhase(&context->story);
        if (active) {
            for (int i = 0; i < active->quest_count && i < 10; i++) {
                active->quests[i].completed = data->quest_completion[i];
            }
            // Synchronize world assets for the restored map/phase
            LoadPhaseAssets(active, context);
        }
    }

    // 3. Restore World State
    context->picked_up_count = data->picked_up_count;
    for (int i = 0; i < data->picked_up_count && i < 512; i++){
        strncpy(context->picked_up_registry[i], data->picked_up_registry[i], 63);
    }
    for (int i = 0; i < 18; i++) context->pot_registry[i] = data->pot_registry[i];
    for (int i = 0; i < data->used_lines_count && i < 256; i++) {
        context->dialogue_used_lines[i] = data->dialogue_used_lines[i];
    }
    context->met_npc_count = data->met_npc_count;
    for (int i = 0; i < data->met_npc_count && i < 64; i++) {
        strncpy(context->met_npcs[i], data->met_npcs[i], 63);
        context->met_npc_day[i] = data->met_npc_day[i];
        context->met_npc_set[i] = data->met_npc_set[i];
        context->met_npc_phase[i] = data->met_npc_phase[i];
    }

    // 4. Restore Karma
    SetRegistryKarma(data->npc_karma, 64);

    // 5. Restore Settings
    game_settings->game_volume = data->volume;
    SetMasterVolume(game_settings->game_volume / 100.0f);
}

void SaveData(struct GameContext* context, Settings* game_settings){
    Data data = {0};
    Character* player = context->player;
    StorySystem* story = &context->story;

    // 1. Harvest Player State
    data.position = player->position;
    data.direction = player->direction;
    data.sanity = player->sanity;
    for (int i = 0; i < player->inventory_count; i++){
        strcpy(data.inventory[i], player->inventory[i]);
        data.item_count[i] = player->item_count[i];
    }
    data.inventory_count = player->inventory_count;

    // 2. Harvest Story State
    strncpy(data.day_folder, story->day_folder, 31);
    data.set_idx = story->current_set_idx;
    data.phase_idx = story->current_phase_idx;
    data.location = (int)context->location;
    strncpy(data.map_path, context->map->current_path, 127);

    StoryPhase* active = GetActivePhase(story);
    if (active) {
        for (int i = 0; i < active->quest_count && i < 10; i++) {
            data.quest_completion[i] = active->quests[i].completed;
        }
    }

    // 3. Harvest World State
    data.picked_up_count = context->picked_up_count;
    for (int i = 0; i < context->picked_up_count && i < 512; i++){
        strncpy(data.picked_up_registry[i], context->picked_up_registry[i], 63);
    }
    for (int i = 0; i < 18; i++) data.pot_registry[i] = context->pot_registry[i];
    for (int i = 0; i < context->used_lines_count && i < 256; i++) {
        data.dialogue_used_lines[i] = context->dialogue_used_lines[i];
    }
    data.met_npc_count = context->met_npc_count;
    for (int i = 0; i < context->met_npc_count && i < 64; i++) {
        strncpy(data.met_npcs[i], context->met_npcs[i], 63);
        data.met_npc_day[i] = context->met_npc_day[i];
        data.met_npc_set[i] = context->met_npc_set[i];
        data.met_npc_phase[i] = context->met_npc_phase[i];
    }

    // 4. Harvest Karma
    GetRegistryKarma(data.npc_karma, 64);

    // 5. Harvest Settings
    if (game_settings) data.volume = game_settings->game_volume;
    else data.volume = 100.0f; // Default if NULL

    // Create the data directory if it doesn't exist
    if (!DirectoryExists("../data")) MakeDirectory("../data");
    // Save the data to the data file
    SaveFileData("../data/data.dat", &data, sizeof(Data));
    TraceLog(LOG_INFO, "GAME AUTO-SAVED: S%d P%d at Loc %d", data.set_idx, data.phase_idx, data.location);
}

void ResetGameData(struct GameContext* context, Vector2 default_spawn){
    Character* player = context->player;
    
    // Reset Player
    player->position = default_spawn;
    player->inventory_count = 0;
    player->sanity = 0.0f;
    player->direction = 0;
    
    // Reset World
    context->picked_up_count = 0;
    for (int i = 0; i < 512; i++){
        memset(context->picked_up_registry[i], 0, 64);
    }
    memset(context->pot_registry, 0, sizeof(context->pot_registry));
    memset(context->dialogue_used_lines, 0, sizeof(context->dialogue_used_lines));
    context->used_lines_count = 0;
    context->met_npc_count = 0;
    for (int i = 0; i < 64; i++) {
        memset(context->met_npcs[i], 0, 64);
        context->met_npc_day[i] = 0;
        context->met_npc_set[i] = 0;
        context->met_npc_phase[i] = 0;
    }
    
    // Reset Story
    context->story.current_set_idx = 0;
    context->story.current_phase_idx = 0;
    
    // Reset Karma
    int zero_karma[64] = {0};
    SetRegistryKarma(zero_karma, 64);
}

void HandleGameData(struct GameContext* context, Map* game_map, Settings* game_settings){
    Data data = LoadData(game_settings);
    
    if (data.position.x == -1.0f){
        ResetGameData(context, game_map->spawn_position);
    } else {
        ApplyData(context, game_settings, &data); 
    }
}