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
} AssetMetadata;

static AssetMetadata ASSET_REGISTRY[] = {
    // --- Interior Assets ---
    {"fridge", "../assets/map/map_int/fridge.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/fridge.txt"},
    {"plant", "../assets/map/map_int/plant.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/plant.txt"},
    {"sofa", "../assets/map/map_int/couchff.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/sofa.txt"},
    {"oven", "../assets/map/map_int/oven.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/oven.txt"},
    {"sink", "../assets/map/map_int/sink cabinet.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/sink.txt"},
    {"bathtub", "../assets/map/map_int/bathtub.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/bathtub.txt"},
    {"toilet", "../assets/map/map_int/toilet.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/toilet.txt"},
    {"lamp", "../assets/map/map_int/lamp.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/lamp.txt"},
    {"cabinet", "../assets/map/map_int/tallcabinet.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/cabinet.txt"},
    {"bed", "../assets/map/map_int/bed.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/bed.txt"},
    {"table", "../assets/map/map_int/foodtable.png", {0, 0, 0, 0}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase1/table.txt"},

    // --- Exterior Assets ---
    {"potato", "../assets/images/item/potato.png", {1200, 1500, 64, 64}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase2/potato.txt"},
    {"big tree", "../assets/images/item/tree.png", {1500, 1000, 192, 256}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase2/big tree.txt"},
    {"tree", "../assets/images/npc/tree.png", {1000, 1100, 96, 128}, INTERACTABLE_TYPE_ITEM, "../assets/text/day1/phase2/tree.txt"},
    {"farmer", "../assets/images/npc/farmer.png", {1600, 1200, 64, 96}, INTERACTABLE_TYPE_NPC, "../assets/text/day1/phase2/farmer.txt"},
};

static int REGISTRY_COUNT = sizeof(ASSET_REGISTRY) / sizeof(ASSET_REGISTRY[0]);

static AssetMetadata* FindInRegistry(const char* id) {
    for (int i = 0; i < REGISTRY_COUNT; i++) {
        if (strcmp(ASSET_REGISTRY[i].id, id) == 0) return &ASSET_REGISTRY[i];
    }
    return NULL;
}

void LoadPhaseAssets(StoryPhase* phase, GameContext* context) {
    if (!phase || !context) return;

    // 1. Handle Location Change
    if (phase->location != STORY_LOC_NONE && (Location)phase->location != context->location) {
        Location targetLoc = (Location)phase->location;
        LoadLocationAssets(targetLoc, context);
        context->player_teleport_requested = true; 
    }

    // 2. Clear current dynamic interactables
    UnloadLocationAssets(context);

    // 3. Load requested interactables
    context->worldNPCs = (NPC*)malloc(sizeof(NPC) * 20);
    context->worldItems = (Item*)malloc(sizeof(Item) * 20);
    memset(context->worldNPCs, 0, sizeof(NPC) * 20);
    memset(context->worldItems, 0, sizeof(Item) * 20);

    for (int i = 0; i < phase->interactable_count; i++) {
        AssetMetadata* meta = FindInRegistry(phase->interactables[i].id);
        if (!meta) continue;

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
            sprintf(item->base.dialoguePath, "../assets/text/%s/set%d/phase%d/%s.txt", 
                    context->story.day_folder, context->story.current_set_idx + 1, 
                    context->story.current_phase_idx + 1, meta->id);
            item->is_pickup = false; 
        }
    }

    LoadNPCs(context->worldNPCs, context->npcCount);
    LoadItems(context->worldItems, context->itemCount);
}

void LoadLocationAssets(Location location, GameContext* context) {
    context->location = location;
}

void UnloadLocationAssets(GameContext* context) {
    if (context == NULL) return;

    if (context->worldNPCs != NULL) {
        UnloadNPCs(context->worldNPCs, context->npcCount);
        free(context->worldNPCs);
        context->worldNPCs = NULL;
    }
    context->npcCount = 0;

    if (context->worldItems != NULL) {
        UnloadItems(context->worldItems, context->itemCount);
        free(context->worldItems);
        context->worldItems = NULL;
    }
    context->itemCount = 0;
    
    if (context->worldDoors != NULL) {
        free(context->worldDoors);
        context->worldDoors = NULL;
    }
    context->doorCount = 0;
}
