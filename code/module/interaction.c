/**
 * @file interaction.c
 * @brief Orchestrates player interaction, quest triggers, and world-state persistence.
 * 
 * Update History:
 * - 2026-03-25: Foundational implementation of the "Interactable" registry. (Goal: Establish a 
 *                centralized way for the player to query the world for NPCs, items, and doors.)
 * - 2026-04-02: Introduced the "Quest Guard" system. (Goal: Prevent the player from prematurely 
 *                triggering endgame triggers, ensuring the narrative flows in the intended sequence.)
 * - 2026-04-05: Significant refactor of the "Interaction Hitbox" and "Guard Logic." (Goal: Improve 
 *                player UX by moving from a rigid, center-based hitbox to a generous, sprite-wrapping 
 *                box and allowing non-critical interactions to occur even if movement quests are pending.)
 * - 2026-04-05: Implemented "Met NPC" and "Pickup Registry" persistence. (Goal: Ensure that session 
 *                progress is correctly recorded in the GameContext for cross-day consistency.)
 * 
 * Revision Details:
 * - Refactored `CheckInteractable` to use a dynamic `playerHitbox` with a 40px radius around the player sprite.
 * - Removed the hardcoded early-return in `InteractWithObject` that blocked terminal triggers 
 *    (like the Fridge) when earlier quests were incomplete.
 * - Fixed a string-comparison bug in `IsOtherQuestsPending` for "Explore" objectives.
 * - Added comprehensive Doxygen-style function headers for all internal static helpers.
 * 
 * Authors: Andrew Zhuo and Cornelius Jabez Lim
 */
#include "interaction.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "raymath.h"
#include "assets.h"
#include "scene.h"
#include "data.h"
#include "game_context.h"
#include "story.h"

static char current_interactable_id[64] = "";

/**
 * @brief Registers a pickup event in the game context.
 * 
 * @param game_context Pointer to the game context.
 * @param id Interactable ID to register.
 */
static void RegisterPickup(struct GameContext* game_context, const char* id){
    if (game_context->picked_up_count < 512) {
        strncpy(game_context->picked_up_registry[game_context->picked_up_count++], id, 63);
    }
}

/**
 * @brief Registers a met NPC event in the game context.
 * 
 * @param game_context Pointer to the game context.
 * @param id NPC ID to register.
 */
static void RegisterMeetNPC(struct GameContext* game_context, const char* id){
    for (int i = 0; i < game_context->met_npc_count; i++) {
        if (strcmp(game_context->met_npcs[i], id) == 0) return;
    }
    if (game_context->met_npc_count < 64) {
        strncpy(game_context->met_npcs[game_context->met_npc_count], id, 63);
        game_context->met_npc_set[game_context->met_npc_count] = game_context->story.current_set_idx;
        game_context->met_npc_phase[game_context->met_npc_count] = game_context->story.current_phase_idx;
        game_context->met_npc_count++;
    }
}

/**
 * @brief Checks if all quests in a phase are completed.
 * 
 * @param phase Pointer to the story phase.
 * @return true if all quests are completed, false otherwise.
 */
static bool IsAllQuestsCompleted(StoryPhase* phase){
    if (!phase) return true;
    for (int i = 0; i < phase->quest_count; i++){
        if (!phase->quests[i].completed && strstr(phase->quests[i].description, "Explore") == NULL) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Checks if any quest other than the target quest is pending.
 * 
 * @param phase Pointer to the story phase.
 * @param target_id Target interactable ID.
 * @return true if any other quest is pending, false otherwise.
 */
static bool IsOtherQuestsPending(StoryPhase* phase, const char* target_id){
    if (!phase) return false;
    for (int i = 0; i < phase->condition_count; i++){
        // If this condition is NOT about our current target, and its associated quest is NOT done
        if (strcmp(phase->end_conditions[i].target_id, target_id) != 0){
            if (i < phase->quest_count && !phase->quests[i].completed && strstr(phase->quests[i].description, "Explore") == NULL) {
                return true; // We found another quest that must be finished first
            }
        }
    }
    return false;
}

/**
 * @brief Updates story conditions based on interaction.
 * 
 * @param game_context Pointer to the game context.
 * @param game_dialogue Pointer to the dialogue system.
 * @param interactable_id ID of the interactable object.
 */
static void UpdateStoryConditions(struct GameContext* game_context, Dialogue* game_dialogue, const char* interactable_id) {
    StoryPhase* active = GetActivePhase(&game_context->story);
    if (!active) return;

    if (strcmp(active->name, "SET1-PHASE1") == 0){
        if (interactable_id && strlen(interactable_id) > 0){
            if (active->quest_count > 1) active->quests[1].completed = true;
        }
    }

    for (int i = 0; i < active->condition_count; i++){
        StoryCondition* cond = &active->end_conditions[i];
        if (cond->type == CONDITION_INTERACT_OBJECT){
            if (strcmp(cond->target_id, interactable_id) == 0){
                // Special check for phase-enders: only allow if other quests are done
                if (strcmp(interactable_id, "house_door") == 0 || strcmp(interactable_id, "fridge") == 0) {
                    if (IsOtherQuestsPending(active, interactable_id)) continue; 
                }
                if ((int)cond->target_value == 0) cond->met = true;
                else if (game_dialogue && game_dialogue->selected_choice == (int)cond->target_value - 1) cond->met = true;
            }
        } else if (cond->type == CONDITION_COLLECT_OBJECTS){
            int collected = 0;
            for (int j = 0; j < game_context->picked_up_count; j++){
                if (strstr(game_context->picked_up_registry[j], cond->target_id) != NULL){
                    collected++;
                }
            }
            cond->current_count = collected;
            if (collected >= (int)cond->target_value){
                cond->met = true;
            }
        }
    }

    // Special case for SET5-PHASE2
    if (strcmp(active->name, "SET5-PHASE2") == 0){
        int small_collected = 0, big_collected = 0;
        int total_in_boxes = game_context->left_box_big + game_context->left_box_small + 
                             game_context->right_box_big + game_context->right_box_small;

        // Count collected logs
        for (int j = 0; j < game_context->picked_up_count; j++){
            if (strstr(game_context->picked_up_registry[j], "small_logs")) small_collected++;
            else if (strstr(game_context->picked_up_registry[j], "big_logs")) big_collected++;
        }

        // Update conditions
        if (active->condition_count >= 2){
            active->end_conditions[0].current_count = small_collected;
            if (small_collected >= (int)active->end_conditions[0].target_value) active->end_conditions[0].met = true;
            active->end_conditions[1].current_count = big_collected;
            if (big_collected >= (int)active->end_conditions[1].target_value) active->end_conditions[1].met = true;
        }

        // Update quests
        if (active->quest_count > 0) active->quests[0].completed = (small_collected + big_collected >= 15);
        if (active->quest_count > 1) active->quests[1].completed = (total_in_boxes >= 15);

        // Update conditions
        if (total_in_boxes >= 15 && active->condition_count >= 4){
             active->end_conditions[2].met = true;
             active->end_conditions[3].met = true;
        }
    } else{
        for (int i = 0; i < active->condition_count; i++){
            // Update quests
            if (active->end_conditions[i].met && i < active->quest_count) active->quests[i].completed = true;
        }
    }
}

/**
 * @brief Prepares dialogue nodes for display.
 * 
 * @param dialogue Pointer to the dialogue system.
 * @param node_idx Index of the dialogue node to prepare.
 */
static void PrepareNodeText(Dialogue* dialogue, int node_idx){
    if (node_idx < 0 || node_idx >= MAX_DIALOGUE_LINES) return;
    DialogueNode* node = &dialogue->nodes[node_idx];
    if (node->response_count == 0){
        dialogue->line_count = 0;
        return;
    }
    // If the node is a conversation, copy all responses to the lines array
    if (node->is_conversation){
        for (int i = 0; i < node->response_count; i++){
            strncpy(dialogue->lines[i], node->responses[i], MAX_LINE_LENGTH - 1);
        }
        dialogue->line_count = node->response_count;
        dialogue->current_line = 0;
    } else{
        // If the node is not a conversation, copy a random response to the lines array
        int r_idx = 0;
        if (node->response_count > 1) r_idx = rand() % node->response_count;
        strncpy(dialogue->lines[0], node->responses[r_idx], MAX_LINE_LENGTH - 1);
        dialogue->line_count = 1;
        dialogue->current_line = 0;
    }
}

/**
 * @brief Starts a dialogue.
 * 
 * @param path Path to the dialogue file.
 * @param dialogue Pointer to the dialogue system.
 * @param state Pointer to the game state.
 * @param ctx Pointer to the game context.
 * @param interactable_id ID of the interactable object.
 */
static void StartDialogue(const char* path, Dialogue* dialogue, GameState* state, struct GameContext* ctx, const char* interactable_id){
    if (!path || !dialogue || !state || !ctx) return;
    LoadInteraction(path, dialogue, ctx);
    // If the dialogue has nodes, start the dialogue
    if (dialogue->node_count > 0){
        dialogue->current_node_idx = 0;
        PrepareNodeText(dialogue, 0);
        dialogue->selected_choice = -1;
        dialogue->pending_target_map[0] = '\0';
        *state = DIALOGUE_CUTSCENE;
        // If the interactable ID is not NULL, register the NPC as met
        if (interactable_id){
            strncpy(current_interactable_id, interactable_id, 63);
            RegisterMeetNPC(ctx, interactable_id);
        }
        // If the node has a target map, set the pending target map
        if (dialogue->nodes[0].target_map[0] != '\0'){
            strncpy(dialogue->pending_fade_color, dialogue->nodes[0].fade_color, 31);
            strncpy(dialogue->pending_target_map, dialogue->nodes[0].target_map, 127);
            strncpy(dialogue->pending_target_loc, dialogue->nodes[0].target_loc, 31);
        }
    }
}

void InteractWithNPC(NPC *npc, Dialogue* game_dialogue, GameState* game_state, struct GameContext* game_context) {
    if (game_dialogue == NULL || game_state == NULL || game_context == NULL) return;
    // If the game state is gameplay and the NPC is not NULL, start the dialogue
    if (*game_state == GAMEPLAY && npc != NULL){
        StartDialogue(npc->base.dialoguePath, game_dialogue, game_state, game_context, npc->base.interactable_id);
    } 
    // If the game state is dialogue cutscene, handle the dialogue
    else if (*game_state == DIALOGUE_CUTSCENE){
        DialogueNode* current_node = &game_dialogue->nodes[game_dialogue->current_node_idx];
        if (current_node->choice_count > 0 && game_dialogue->current_line >= game_dialogue->line_count - 1){
            int key = 0;

            // Check for player input
            if (IsKeyPressed(KEY_ONE)) key = 1;
            else if (IsKeyPressed(KEY_TWO)) key = 2;
            else if (IsKeyPressed(KEY_THREE)) key = 3;
            else if (IsKeyPressed(KEY_FOUR)) key = 4;
            
            // If the player has selected a choice and choice is valid
            if (key > 0 && key <= current_node->choice_count) {
                int next_node_idx = current_node->child_nodes[key - 1];
                game_dialogue->selected_choice = key - 1; 

                // Execute deposit logically bound to this choice if defined
                if (current_node->deposit_tags[key - 1][0] != '\0'){
                    char* target_tag = current_node->deposit_tags[key - 1];
                    int deposited = 0;
                    for (int i = 0; i < game_context->player->inventory_count; i++){
                        if (strstr(game_context->player->inventory[i], target_tag)){
                            // Check if the item that is being deposited
                            if (strstr(target_tag, "small_logs")){
                                if (strstr(current_interactable_id, "left")) game_context->left_box_small += game_context->player->item_count[i];
                                else game_context->right_box_small += game_context->player->item_count[i];
                            } else if (strstr(target_tag, "big_logs")){
                                if (strstr(current_interactable_id, "left")) game_context->left_box_big += game_context->player->item_count[i];
                                else game_context->right_box_big += game_context->player->item_count[i];
                            }
                            deposited += game_context->player->item_count[i];
                            game_context->player->item_count[i] = 0;
                            game_context->player->inventory[i][0] = '\0';
                        }
                    }
                    if (deposited > 0){
                        int new_count = 0;
                        for (int i = 0; i < game_context->player->inventory_count; i++){
                            if (game_context->player->item_count[i] > 0){
                                if (new_count != i){
                                    strcpy(game_context->player->inventory[new_count], game_context->player->inventory[i]);
                                    game_context->player->item_count[new_count] = game_context->player->item_count[i];
                                }
                                new_count++;
                            }
                        }
                        game_context->player->inventory_count = new_count;
                    }
                }

                if (next_node_idx != -1){
                    game_dialogue->current_node_idx = next_node_idx;
                    PrepareNodeText(game_dialogue, next_node_idx);
                    DialogueNode* next_node = &game_dialogue->nodes[next_node_idx];
                    if (next_node->sanity_change != 0){
                        game_context->player->sanity += next_node->sanity_change;
                        if (game_context->player->sanity < 0) game_context->player->sanity = 0;
                        if (game_context->player->sanity > 100) game_context->player->sanity = 100;
                    }
                    if (next_node->karma_change != 0) UpdateAssetKarma(current_interactable_id, next_node->karma_change);
                    UpdateStoryConditions(game_context, game_dialogue, current_interactable_id);
                    if (next_node->target_map[0] != '\0'){
                        strncpy(game_dialogue->pending_fade_color, next_node->fade_color, 31);
                        strncpy(game_dialogue->pending_target_map, next_node->target_map, 127);
                        strncpy(game_dialogue->pending_target_loc, next_node->target_loc, 31);
                    }
                } else { 
                    if (current_node->child_nodes[key - 1] != -1) {
                         DialogueNode* child = &game_dialogue->nodes[current_node->child_nodes[key - 1]];
                         if (child->triggers_phone) game_context->story.narration_pending = true;
                    }
                    *game_state = GAMEPLAY; 
                    UpdateStoryConditions(game_context, game_dialogue, current_interactable_id); 
                }
            }
        }
        if (IsKeyPressed(KEY_SPACE)) {
            if (current_node->choice_count > 0 && game_dialogue->current_line >= game_dialogue->line_count - 1){
                // Do nothing, force player to pick a choice.
            } else{
                game_dialogue->current_line++;
                if (game_dialogue->current_line >= game_dialogue->line_count){
                    if (current_node->choice_count == 0){
                        if (current_node->next_node != -1){
                            game_dialogue->current_node_idx = current_node->next_node;
                            PrepareNodeText(game_dialogue, current_node->next_node);
                        } else{
                            if (current_node->triggers_phone) game_context->story.narration_pending = true;
                            UpdateStoryConditions(game_context, game_dialogue, current_interactable_id);
                            *game_state = GAMEPLAY;
                        }
                    } else{ 
                        game_dialogue->current_line = game_dialogue->line_count - 1; 
                    }
                }
            }
        }
    }
}

void InteractWithItem(Item *item, Dialogue *game_dialogue, GameState *game_state, Character *player, struct GameContext *game_context){
    // If the item is a pickup item
    if (item->is_pickup){
        // If the item is logs
        if (strstr(item->base.interactable_id, "logs")){
            int current_logs = 0;
            for (int i = 0; i < player->inventory_count; i++) if (strstr(player->inventory[i], "logs")) current_logs += player->item_count[i];
            if (current_logs >= 5){
                memset(game_dialogue, 0, sizeof(Dialogue));
                strncpy(game_dialogue->lines[0], "My hands are full, I should put these in the boxes first.", MAX_LINE_LENGTH - 1);
                game_dialogue->line_count = 1; game_dialogue->current_line = 0; 
                game_dialogue->nodes[0].next_node = -1;
                *game_state = DIALOGUE_CUTSCENE;
                return;
            }
        }
    }
    // If the item is a pickup item
    if (item->is_pickup){
        strcpy(player->inventory[player->inventory_count], item->base.interactable_id);
        player->item_count[player->inventory_count] = 1; player->inventory_count++;
        item->picked_up = true; RegisterPickup(game_context, item->base.interactable_id);
        UpdateStoryConditions(game_context, NULL, item->base.interactable_id);
        SaveData(game_context, NULL);
        return;
    } 
    // If the item is fireplace
    if (strcmp(item->base.interactable_id, "fireplace") == 0 && 
        game_context->story.current_set_idx == 6 && 
        game_context->story.current_phase_idx == 0){
        game_context->fireplace_on = true;
    }
    // Start the dialogue if the dialogue path is valid
    if (FileExists(item->base.dialoguePath)) StartDialogue(item->base.dialoguePath, game_dialogue, game_state, game_context, item->base.interactable_id);
}

void InteractWithObject(Interactable* obj, Dialogue* dialogue, GameState* state, Character* player, Map* map, struct GameContext* ctx){
    // If the game state is dialogue cutscene, interact with the NPC to continue the dialogue
    if (*state == DIALOGUE_CUTSCENE){
        InteractWithNPC(NULL, dialogue, state, ctx); 
        return;
    }
    if (obj == NULL || ctx == NULL) return;

    // Dispatch the interaction to the appropriate function based on the object type
    switch (obj->type){
        case INTERACTABLE_TYPE_NPC: InteractWithNPC((NPC*)obj, dialogue, state, ctx); break;
        case INTERACTABLE_TYPE_ITEM: InteractWithItem((Item*)obj, dialogue, state, player, ctx); break;
        case INTERACTABLE_TYPE_DOOR: InteractWithDoor((Door*)obj, map, player, dialogue, state, ctx); break;
    }

    // Update the story conditions
    UpdateStoryConditions(ctx, dialogue, obj->interactable_id);
}

void InteractWithDoor(Door *door, Map *map, Character *player, Dialogue *game_dialogue, GameState *game_state, struct GameContext* game_context){
    if (door == NULL || map == NULL || player == NULL || game_context == NULL) return;
    StoryPhase* active = GetActivePhase(&game_context->story);
    if (IsOtherQuestsPending(active, door->base.interactable_id)) return;

    // If the door has a dialogue path, start the dialogue
    if (FileExists(door->base.dialoguePath)){
        StartDialogue(door->base.dialoguePath, game_dialogue, game_state, game_context, door->base.interactable_id);
    } else{
        StartFadeTransition(game_context->game_scene, BLACK, door->targetMapPath, "DEFAULT");
    }
}

void LoadNPCs(NPC npcs[], int count){
    for (int i = 0; i < count; i++){
        npcs[i].base.texture = LoadTexture(npcs[i].base.texturePath); 
    }
}

void UnloadNPCs(NPC npcs[], int count){
    for (int i = 0; i < count; i++){
        UnloadTexture(npcs[i].base.texture); 
    }
}

void LoadItems(Item items[], int count){ 
    for (int i = 0; i < count; i++){
        if (!items[i].picked_up && items[i].base.texturePath[0] != '\0'){
            items[i].base.texture = LoadTexture(items[i].base.texturePath); 
        }
    }
}

void UnloadItems(Item items[], int count){
    for (int i = 0; i < count; i++){
        if (items[i].base.texture.id != 0) UnloadTexture(items[i].base.texture); 
    }
}

void CheckInteractable(NPC worldNPCs[], Item worldItems[], Door worldDoors[], int npcCount, int itemCount, int doorCount, Rectangle playerHitbox, Interactable** objectToInteractWith) {
    if (objectToInteractWith == NULL) return;
    *objectToInteractWith = NULL;
    float min_dist = 1e6;
    Vector2 playerPos = {
        playerHitbox.x + playerHitbox.width/2.0f, 
        playerHitbox.y + playerHitbox.height/2.0f
    };
    // Check for NPCs
    for (int i = 0; i < npcCount; i++){
        if (CheckCollisionRecs(playerHitbox, worldNPCs[i].base.bounds)){
            worldNPCs[i].base.isActive = true;
            float dist = Vector2Distance(playerPos, (Vector2){worldNPCs[i].base.bounds.x, worldNPCs[i].base.bounds.y});
            if (dist < min_dist){
                min_dist = dist; 
                *objectToInteractWith = (Interactable*)&worldNPCs[i]; 
            }
        } else worldNPCs[i].base.isActive = false;
    }
    // Check for Items
    for (int i = 0; i < itemCount; i++) {
        if (!worldItems[i].picked_up && CheckCollisionRecs(playerHitbox, worldItems[i].base.bounds)) {
            worldItems[i].base.isActive = true;
            float dist = Vector2Distance(playerPos, (Vector2){worldItems[i].base.bounds.x, worldItems[i].base.bounds.y});
            if (dist < min_dist) { min_dist = dist; *objectToInteractWith = (Interactable*)&worldItems[i]; }
        } else worldItems[i].base.isActive = false;
    }
    // Check for Doors
    for (int i = 0; i < doorCount; i++) {
        if (CheckCollisionRecs(playerHitbox, worldDoors[i].base.bounds)) {
            worldDoors[i].base.isActive = true;
            float dist = Vector2Distance(playerPos, (Vector2){worldDoors[i].base.bounds.x, worldDoors[i].base.bounds.y});
            if (dist < min_dist) { min_dist = dist; *objectToInteractWith = (Interactable*)&worldDoors[i]; }
        } else worldDoors[i].base.isActive = false;
    }
}