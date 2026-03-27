#include "interaction.h"
#include <string.h>
#include <stdio.h>
#include "raymath.h"
#include "assets.h"

void LoadNPCs(NPC npcs[], int count){
    for (int i = 0; i < count; i++){
        npcs[i].base.texture = LoadTexture(npcs[i].base.texturePath);
    }
}

void LoadItems(Item items[], int count){
    for (int i = 0; i < count; i++){
        items[i].base.texture = LoadTexture(items[i].base.texturePath);
    }
}

void UnloadNPCs(NPC npcs[], int count){
    for (int i = 0; i < count; i++){
        UnloadTexture(npcs[i].base.texture);
    }
}

void UnloadItems(Item items[], int count){
    for (int i = 0; i < count; i++){
        UnloadTexture(items[i].base.texture);
    }
}

void CheckInteractable(NPC worldNPCs[], Item worldItems[], Door worldDoors[], 
    int npcCount, int itemCount, int doorCount,
    Rectangle playerHitbox, Interactable** objectToInteractWith){
    
    *objectToInteractWith = NULL;
    float min_dist = 1e6; 
    Vector2 playerPos = {playerHitbox.x + playerHitbox.width / 2.0f, playerHitbox.y + playerHitbox.height / 2.0f};

    for (int i = 0; i < npcCount; i++){
        if (CheckCollisionRecs(playerHitbox, worldNPCs[i].base.bounds)){
            worldNPCs[i].base.isActive = true;
            Vector2 npcPos = {worldNPCs[i].base.bounds.x + worldNPCs[i].base.bounds.width / 2.0f, 
                               worldNPCs[i].base.bounds.y + worldNPCs[i].base.bounds.height / 2.0f};
            float dist = Vector2Distance(playerPos, npcPos);
            if (dist < min_dist){
                min_dist = dist;
                *objectToInteractWith = (Interactable*)&worldNPCs[i];
            }
        } else { worldNPCs[i].base.isActive = false; }
    }

    for (int i = 0; i < itemCount; i++){
        if (!worldItems[i].picked_up && CheckCollisionRecs(playerHitbox, worldItems[i].base.bounds)){
            worldItems[i].base.isActive = true;
            Vector2 itemPos = {worldItems[i].base.bounds.x + worldItems[i].base.bounds.width / 2.0f, 
                                worldItems[i].base.bounds.y + worldItems[i].base.bounds.height / 2.0f};
            float dist = Vector2Distance(playerPos, itemPos);
            if (dist < min_dist){
                min_dist = dist;
                *objectToInteractWith = (Interactable*)&worldItems[i];
            }
        } else { worldItems[i].base.isActive = false; }
    }

    for (int i = 0; i < doorCount; i++){
        if (CheckCollisionRecs(playerHitbox, worldDoors[i].base.bounds)){
            worldDoors[i].base.isActive = true;
            Vector2 doorPos = {worldDoors[i].base.bounds.x + worldDoors[i].base.bounds.width / 2.0f, 
                                worldDoors[i].base.bounds.y + worldDoors[i].base.bounds.height / 2.0f};
            float dist = Vector2Distance(playerPos, doorPos);
            if (dist < min_dist){
                min_dist = dist;
                *objectToInteractWith = (Interactable*)&worldDoors[i];
            }
        } else { worldDoors[i].base.isActive = false; }
    }
}

void InteractWithObject(Interactable* objectToInteractWith, Dialogue* game_dialogue, 
    GameState* game_state, Character *player, Map *map, struct GameContext *game_context){
    
    if (*game_state == DIALOGUE_CUTSCENE) {
        InteractWithNPC(NULL, game_dialogue, game_state, game_context);
        return;
    }

    if (objectToInteractWith == NULL) {
        return;
    }

    switch (objectToInteractWith->type){
        case INTERACTABLE_TYPE_NPC:
            InteractWithNPC((NPC*)objectToInteractWith, game_dialogue, game_state, game_context);
            break;
        case INTERACTABLE_TYPE_ITEM:
            InteractWithItem((Item*)objectToInteractWith, game_dialogue, game_state, player, game_context);
            break;
        case INTERACTABLE_TYPE_DOOR:
            InteractWithDoor((Door*)objectToInteractWith, map, player, game_context);
            break;
    }

    // Story Advance Check
    StoryPhase* active = GetActivePhase(&game_context->story);
    if (active && strcmp(active->name, "SET1-PHASE1") == 0) {
        if (active->quest_count > 1) active->quests[1].completed = true;
    }
}

void InteractWithNPC(NPC *npc, Dialogue *game_dialogue, GameState *game_state, struct GameContext *game_context){
    if (*game_state == GAMEPLAY && npc != NULL){
        LoadInteraction(npc->base.dialoguePath, game_dialogue);
        if (game_dialogue->line_count > 0 || game_dialogue->choice_count > 0) {
            *game_state = DIALOGUE_CUTSCENE;
        }
    } else if (*game_state == DIALOGUE_CUTSCENE){
        // 1. Choice selection (1-4 keys)
        if (game_dialogue->choice_count > 0 && game_dialogue->current_line >= game_dialogue->line_count - 1) {
            int key = 0;
            if (IsKeyPressed(KEY_ONE)) key = 1;
            else if (IsKeyPressed(KEY_TWO)) key = 2;
            else if (IsKeyPressed(KEY_THREE)) key = 3;
            else if (IsKeyPressed(KEY_FOUR)) key = 4;

            if (key > 0 && key <= game_dialogue->choice_count) {
                int idx = key - 1;
                memset(game_dialogue->lines, 0, sizeof(game_dialogue->lines));
                strncpy(game_dialogue->lines[0], game_dialogue->choice_responses[idx], MAX_LINE_LENGTH-1);
                game_dialogue->line_count = 1;
                game_dialogue->current_line = 0;
                game_dialogue->choice_count = 0;
                
                if (strstr(game_dialogue->lines[0], "END SET")) {
                    AdvanceStory(&game_context->story);
                }
            }
            return; // BLOCKS standard SPACE progression during choice selection
        }

        // 2. Standard dialogue progression (only on SPACE)
        if (IsKeyPressed(KEY_SPACE)) {
            game_dialogue->current_line++;
            if (game_dialogue->current_line >= game_dialogue->line_count) {
                *game_state = GAMEPLAY;
                game_dialogue->current_line = 0;
            }
        }
    }
}

void InteractWithItem(Item *item, Dialogue *game_dialogue, GameState *game_state, Character *player, struct GameContext *game_context){
    if (item->is_pickup) {
        strcpy(player->inventory[player->inventory_count], item->base.texturePath);
        player->item_count[player->inventory_count]++;
        player->inventory_count++;
        item->picked_up = true;
    } else {
        LoadInteraction(item->base.dialoguePath, game_dialogue);
        if (game_dialogue->line_count > 0 || game_dialogue->choice_count > 0) *game_state = DIALOGUE_CUTSCENE;
    }
}

void InteractWithDoor(Door *door, Map *map, Character *player, struct GameContext *game_context) {
    if (door == NULL || map == NULL || player == NULL || game_context == NULL) return;

    char targetMap[128];
    strncpy(targetMap, door->targetMapPath, 127);
    Location targetLoc = door->targetLocation;

    FreeMap(map);
    *map = InitMap(targetMap);
    player->position = map->spawn_position;

    // Load static assets for this map
    LoadLocationAssets(targetLoc, game_context);
    
    // Also check if this map reload satisfies a phase change requirement
    game_context->location = targetLoc;
}