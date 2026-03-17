/*
This file contains function implementations for the interaction module.

Module made by Andrew Zhuo and Cornelius Jabez Lim.
*/

#include "interaction.h"
#include <string.h>

void CheckInteractable(Interactable interactable[], int interactableCount,
    Rectangle playerHitbox, Interactable** objectToInteractWith){
    *objectToInteractWith = NULL;
    for (int i = 0; i < interactableCount; i++){
        // Check collision
        if (CheckCollisionRecs(playerHitbox, interactable[i].bounds)){
            interactable[i].isActive = true;
            *objectToInteractWith = &interactable[i];
        } else{
            interactable[i].isActive = false;
        }
    }
}

void InteractWithObject(Interactable* objectToInteractWith,
    Dialogue* game_dialogue, GameState* game_state){
    if (*game_state == GAMEPLAY && objectToInteractWith != NULL){
        // Load dialogue from the object to interact with
        *game_dialogue = LoadDialogue(objectToInteractWith->dialoguePath);

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