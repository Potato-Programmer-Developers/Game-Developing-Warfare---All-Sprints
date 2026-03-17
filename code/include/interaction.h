/*
This file contains function prototypes for the interaction module.

Module made by Andrew Zhuo andCornelius Jabez Lim.
*/

#ifndef INTERACTION_H
#define INTERACTION_H

#include <stdbool.h>
#include "dialogue.h"
#include "raylib.h"
#include "state.h"

typedef struct {
    /* Struct for interactable objects */
    Rectangle bounds;              // The bounds of the interactable object
    const char* dialoguePath;      // The path to the dialogue file
    bool isActive;                 // Whether the interactable object is active
} Interactable;

// Check if the player is colliding with any interactable objects
void CheckInteractable(
    Interactable interactable[], int interactableCount, Rectangle playerHitbox,
    Interactable** objectToInteractWith
);

// Interact with an interactable object
void InteractWithObject(
    Interactable* objectToInteractWith, Dialogue* game_dialogue,
    GameState* game_state
);

#endif