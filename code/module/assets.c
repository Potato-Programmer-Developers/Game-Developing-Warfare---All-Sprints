/**
 * @file assets.c
 * @brief Utility functions for loading and unloading game resources.
 * 
 * Update History:
 * - 2026-03-24: Initial resource manager implementation. (Goal: Centralize 
 *                loading to prevent memory leaks.)
 * - 2026-04-03: Added directional sprite support. (Goal: Load all character 
 *                orientations simultaneously.)
 * - 2026-04-05: Integrated audio resource cleanup. (Goal: Ensure `UnloadWave` 
 *                and `UnloadSound` are called correctly during map transitions.)
 * 
 * Revision Details:
 * - Implemented `LoadCharacterTextures` to batch-load Kane's movement set.
 * - Added `UnloadPlayerTextures` helper for clean state resets.
 * - Fixed a resource leak where TMX tilemap textures were not being freed properly.
 * 
 * Authors: Andrew Zhuo
 */

#include "assets.h"
#include "interaction.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Master registry for all possible interactable objects in the game.
 * Maps IDs to their visual and physical properties.
 */
typedef struct {
    char id[64];
    char texturePath[128];
    Rectangle bounds;
    InteractableType type;
    char dialoguePath[128];
    int karma;                 // Persistent karma for this NPC / Object
} AssetMetadata;

static AssetMetadata ASSET_REGISTRY[] = {
    // --- Apartment Assets ---
    {"fridge", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/fridge.txt", 0},
    {"plant", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/plant.txt", 0},
    {"sofa", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/sofa.txt", 0},
    {"oven", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/oven.txt", 0},
    {"sink", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/sink.txt", 0},
    {"bathtub", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/bathtub.txt", 0},
    {"toilet", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/toilet.txt", 0},
    {"lamp", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/lamp.txt", 0},
    {"cabinet", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/cabinet.txt", 0},
    {"bed", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/bed.txt", 0},
    {"table", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/table.txt", 0},
    
    // --- Exterior Assets ---
    {"potato", "", {1200, 1500, 64, 64}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase2/potato.txt", 0},
    {"big tree", "", {1500, 1000, 192, 256}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase2/big tree.txt", 0},
    {"tree", "", {1000, 1100, 96, 128}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase2/tree.txt", 0},
    {"farmer", "../assets/images/character/jhonny/jhonny_idle.png", {1600, 1200, 64, 96}, INTERACTABLE_TYPE_NPC, "", 0},
    {"saul", "../assets/images/character/furina.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_NPC, "", 0},
    {"house", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"house_door", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_DOOR, "", 0},
    {"farm_road", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_DOOR, "../assets/text/day1/set4/phase1/farm_road.txt", 0},
    {"forest_road", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_DOOR, "../assets/text/day1/set4/phase1/forest_road.txt", 0},

    // --- Interior Assets ---
    {"fireplace", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"bed", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"table", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"fridge", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"bathtub", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"toilet", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"plant", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"cabinet", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"lamp", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"sofa", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"garbage1", "../assets/map/map_int/Garbage.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"garbage2", "../assets/map/map_int/Garbage.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"garbage3", "../assets/map/map_int/Garbage.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"garbage4", "../assets/map/map_int/Garbage.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"garbage5", "../assets/map/map_int/Garbage.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"garbage6", "../assets/map/map_int/Garbage.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"garbage7", "../assets/map/map_int/Garbage.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"garbage8", "../assets/map/map_int/Garbage.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},

    // --- Farm Assets ---
    {"scarecrow", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"small_logs1", "../assets/map/map_farm/small logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"small_logs2", "../assets/map/map_farm/small logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"small_logs3", "../assets/map/map_farm/small logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"small_logs4", "../assets/map/map_farm/small logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"small_logs5", "../assets/map/map_farm/small logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"small_logs6", "../assets/map/map_farm/small logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"small_logs7", "../assets/map/map_farm/small logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"small_logs8", "../assets/map/map_farm/small logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"small_logs9", "../assets/map/map_farm/small logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"small_logs10", "../assets/map/map_farm/small logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"big_logs1", "../assets/map/map_farm/big logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"big_logs2", "../assets/map/map_farm/big logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"big_logs3", "../assets/map/map_farm/big logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"big_logs4", "../assets/map/map_farm/big logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"big_logs5", "../assets/map/map_farm/big logs.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"left box", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"right box", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"red_pot1", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"red_pot2", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"red_pot3", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"red_pot4", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"red_pot5", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"red_pot6", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"orange_pot1", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"orange_pot2", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"orange_pot3", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"orange_pot4", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"orange_pot5", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"orange_pot6", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"green_pot1", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"green_pot2", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"green_pot3", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"green_pot4", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"green_pot5", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"green_pot6", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"footsteps", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"feathers", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"rosary", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
    {"pumpkin_piece", "", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "", 0},
};

static int REGISTRY_COUNT = sizeof(ASSET_REGISTRY) / sizeof(ASSET_REGISTRY[0]);

static AssetMetadata* FindInRegistry(const char* id){
    // Find the asset in the registry
    for (int i = 0; i < REGISTRY_COUNT; i++) {
        if (strcmp(ASSET_REGISTRY[i].id, id) == 0) return &ASSET_REGISTRY[i];
    }
    return NULL;
}

void UpdateAssetKarma(const char* id, int delta){
    // Update the karma of the asset
    AssetMetadata* meta = FindInRegistry(id);
    if (meta) {
        meta->karma += delta;
        if (meta->karma > 100) meta->karma = 100;
        if (meta->karma < -100) meta->karma = -100; 
    }
}

int GetRegistryKarma(int* dest, int max_size){
    // Get the karma of the asset
    int count = (REGISTRY_COUNT < max_size) ? REGISTRY_COUNT : max_size;
    for (int i = 0; i < count; i++) {
        dest[i] = ASSET_REGISTRY[i].karma;
    }
    return count;
}

void SetRegistryKarma(int* src, int count){
    // Set the karma of the asset
    int limit = (REGISTRY_COUNT < count) ? REGISTRY_COUNT : count;
    for (int i = 0; i < limit; i++) {
        ASSET_REGISTRY[i].karma = src[i];
    }
}

void LoadPhaseAssets(StoryPhase* phase, GameContext* context){
    if (!phase || !context) return;      // Check if the phase or context is NULL

    // 1. Handle Location Change
    if (phase->location != STORY_LOC_NONE && (Location)phase->location != context->location){
        context->player_teleport_requested = true; 
    }

    // 2. Clear current dynamic interactables
    UnloadLocationAssets(context);

    // 3. Load requested interactables
    context->worldNPCs = (NPC*)malloc(sizeof(NPC) * 20);
    context->worldItems = (Item*)malloc(sizeof(Item) * 20);
    context->worldDoors = (Door*)malloc(sizeof(Door) * 20);
    memset(context->worldNPCs, 0, sizeof(NPC) * 20);
    memset(context->worldItems, 0, sizeof(Item) * 20);
    memset(context->worldDoors, 0, sizeof(Door) * 20);

    for (int i = 0; i < phase->interactable_count; i++){
        // Find the asset in the registry
        AssetMetadata* meta = FindInRegistry(phase->interactables[i].id);
        if (!meta) continue;

        // Explicit gate for clues: Day 2 SET 1 only
        if (strcmp(meta->id, "rosary") == 0 || strcmp(meta->id, "feathers") == 0 || 
            strcmp(meta->id, "pumpkin_piece") == 0 || strcmp(meta->id, "footsteps") == 0) {
            bool day2_active = (strcmp(context->story.day_folder, "day2") == 0);
            if (!(day2_active && context->story.current_set_idx == 0)) continue;
        }

        // Try to get dynamic bounds from Tiled object layer
        Rectangle finalBounds = GetMapObjectBounds(context->map, meta->id);
        if (finalBounds.width == 0 || finalBounds.height == 0){
            finalBounds = meta->bounds;
        }

        if (meta->type == INTERACTABLE_TYPE_NPC && context->npcCount < 20){
            NPC* npc = &context->worldNPCs[context->npcCount++];
            npc->base.bounds = finalBounds;
            strncpy(npc->base.interactable_id, meta->id, 63);
            npc->base.type = INTERACTABLE_TYPE_NPC;
            strncpy(npc->base.texturePath, meta->texturePath, 127);
            
            // Construct dynamic dialogue path based on current story state
            sprintf(npc->base.dialoguePath, "../assets/text/%s/set%d/phase%d/%s.txt", 
                    context->story.day_folder, context->story.current_set_idx + 1, 
                    context->story.current_phase_idx + 1, meta->id);
        } else if (meta->type == INTERACTABLE_TYPE_ITEM && context->itemCount < 20){
            Item* item = &context->worldItems[context->itemCount++];
            item->base.bounds = finalBounds;
            strncpy(item->base.interactable_id, meta->id, 63);
            item->base.type = INTERACTABLE_TYPE_ITEM;
            strncpy(item->base.texturePath, meta->texturePath, 127);
            
            // Construct dynamic dialogue path based on current story state
            if (strstr(meta->id, "pot") != NULL) {
                sprintf(item->base.dialoguePath, "../assets/text/%s/set%d/phase%d/pot.txt", 
                        context->story.day_folder, context->story.current_set_idx + 1, 
                        context->story.current_phase_idx + 1);
            } else {
                sprintf(item->base.dialoguePath, "../assets/text/%s/set%d/phase%d/%s.txt", 
                        context->story.day_folder, context->story.current_set_idx + 1, 
                        context->story.current_phase_idx + 1, meta->id);
            }
            item->is_pickup = false; 
            item->no_collision = false;
            if (strstr(meta->id, "logs") || strstr(meta->id, "garbage")) item->is_pickup = true;
            
            // Clue items should never block the player
            if (strcmp(meta->id, "rosary") == 0 || strcmp(meta->id, "feathers") == 0 || 
                strcmp(meta->id, "pumpkin_piece") == 0 || strcmp(meta->id, "footsteps") == 0) {
                item->no_collision = true;
            }
            
            // Persistent state: Check if this item was already picked up in this session
            for (int j = 0; j < context->picked_up_count; j++) {
                if (strcmp(context->picked_up_registry[j], item->base.interactable_id) == 0) {
                    item->picked_up = true;
                    break;
                }
            }
        } else if (meta->type == INTERACTABLE_TYPE_DOOR && context->doorCount < 20){
            Door* door = &context->worldDoors[context->doorCount++];
            door->base.bounds = finalBounds;
            strncpy(door->base.interactable_id, meta->id, 63);
            door->base.type = INTERACTABLE_TYPE_DOOR;
            strncpy(door->base.texturePath, meta->texturePath, 127);
            
            // Hardcoded targets: each door always leads to the same map
            if (strcmp(meta->id, "house_door") == 0){
                strcpy(door->targetMapPath, "../assets/map/map_int/MAIN_MAP_INT.json");
                door->targetLocation = INTERIOR;
            } else if (strcmp(meta->id, "farm_road") == 0){
                strcpy(door->targetMapPath, "../assets/map/map_farm/FARM.json");
                door->targetLocation = FARM;
            } else if (strcmp(meta->id, "forest_road") == 0){
                strcpy(door->targetMapPath, "../assets/map/map_ext/MAINMAP.json");
                door->targetLocation = EXTERIOR;
            }

            sprintf(door->base.dialoguePath, "../assets/text/%s/set%d/phase%d/%s.txt", 
                    context->story.day_folder, context->story.current_set_idx + 1, 
                    context->story.current_phase_idx + 1, meta->id);
        }
    }

    LoadNPCs(context->worldNPCs, context->npcCount);
    LoadItems(context->worldItems, context->itemCount);
    
    // Dynamically load any branching narration for this phase
    LoadPhaseNarration(phase, context);
}

void UnloadLocationAssets(GameContext* context){
    if (context == NULL) return; 

    // Unload NPCs
    if (context->worldNPCs != NULL){
        UnloadNPCs(context->worldNPCs, context->npcCount);
        free(context->worldNPCs);
        context->worldNPCs = NULL;
    }
    context->npcCount = 0;

    // Unload Items
    if (context->worldItems != NULL){
        UnloadItems(context->worldItems, context->itemCount);
        free(context->worldItems);
        context->worldItems = NULL;
    }
    context->itemCount = 0;
    
    // Unload Doors
    if (context->worldDoors != NULL){
        free(context->worldDoors);
        context->worldDoors = NULL;
    }
    context->doorCount = 0;
}
