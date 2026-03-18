/*
This file contains function implementations for the interaction module.

Module made by Andrew Zhuo and Cornelius Jabez Lim.
*/

#include "interaction.h"
#include <string.h>

void LoadNPCs(NPC npcs[], int count){
    /* Load NPCs */
    for (int i = 0; i < count; i++){
        npcs[i].base.texture = LoadTexture(npcs[i].base.texturePath);
    }
}

void LoadItems(Item items[], int count){
    /* Load items */
    for (int i = 0; i < count; i++){
        items[i].base.texture = LoadTexture(items[i].base.texturePath);
    }
}

void CheckInteractable(NPC worldNPCs[], Item worldItems[], int npcCount, int itemCount,
    Rectangle playerHitbox, Interactable** objectToInteractWith){
    /* Check if the player is colliding with any interactable objects. */
    *objectToInteractWith = NULL;

    for (int i = 0; i < npcCount; i++){
        // Check collision with NPCs
        if (CheckCollisionRecs(playerHitbox, worldNPCs[i].base.bounds)){
            worldNPCs[i].base.isActive = true;
            *objectToInteractWith = &worldNPCs[i].base;
        } else{
            worldNPCs[i].base.isActive = false;
        }
    }

    for (int i = 0; i < itemCount; i++){
        // Check collision with items
        if (!worldItems[i].picked_up && CheckCollisionRecs(playerHitbox, worldItems[i].base.bounds)){
            worldItems[i].base.isActive = true;
            *objectToInteractWith = &worldItems[i].base;
        } else{
            worldItems[i].base.isActive = false;
        }
    }
}

void InteractWithObject(Interactable* objectToInteractWith, Dialogue* game_dialogue, 
    GameState* game_state, Character *player){
    /* Handle interaction based on the type of the object. */
    if (objectToInteractWith == NULL) return;

    switch (objectToInteractWith->type){
        case INTERACTABLE_TYPE_NPC:
            InteractWithNPC((NPC*)objectToInteractWith, game_dialogue, game_state);
            break;
        case INTERACTABLE_TYPE_ITEM:
            InteractWithItem((Item*)objectToInteractWith, player);
            break;
    }
}

void InteractWithNPC(NPC *npc, Dialogue *game_dialogue, GameState *game_state){
    /* Handle interaction with NPCs. */
    if (*game_state == GAMEPLAY && npc != NULL){
        // Load dialogue from the object to interact with
        *game_dialogue = LoadDialogue(npc->dialoguePath);

        // Enter dialogue cutscene if there are lines to display
        if (game_dialogue->line_count > 0){
            *game_state = DIALOGUE_CUTSCENE;
        }
    } else if (*game_state == DIALOGUE_CUTSCENE){
        // Increment first
        game_dialogue->current_line++;

        // If it has reached or passed the line count, go back to gameplay
        if (game_dialogue->current_line >= game_dialogue->line_count){
            *game_state = GAMEPLAY;
        }
    }
}

void InteractWithItem(Item *item, Character *player){
    /* Handle interaction with items. */
    strcpy(player->inventory[player->inventory_count], item->base.texturePath);
    player->item_count[player->inventory_count]++;
    player->inventory_count++;
    item->picked_up = true;
}