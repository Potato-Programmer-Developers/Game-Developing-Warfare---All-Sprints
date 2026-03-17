/*
This file contains function prototypes for the interaction module.

Module made by Andrew Zhuo and Cornelius Jabez Lim.
*/

#ifndef INTERACTION_H
#define INTERACTION_H

#include <stdbool.h>
#include "dialogue.h"
#include "raylib.h"
#include "state.h"
#include "character.h"

typedef enum {
    /* Enum for interactable types */
    INTERACTABLE_TYPE_NPC,
    INTERACTABLE_TYPE_ITEM,
} InteractableType;

typedef struct {
    /* Struct for interactable objects */
    Texture2D texture;             // The texture of the interactable object
    const char* texturePath;       // The path to the texture of the interactable object
    Rectangle bounds;              // The bounds of the interactable object
    bool isActive;                 // Whether the interactable object is active
    InteractableType type;         // The type of the interactable object
} Interactable;

typedef struct {
    /* Struct for NPCs */
    Interactable base;             // The base interactable object
    const char* dialoguePath;      // The path to the dialogue file
} NPC;

typedef struct Item {
    /* Struct for items */
    Interactable base;             // The base interactable object
    bool picked_up;                // Whether the item has been picked up
} Item;

// Load textures for NPCs and items
void LoadNPCs(NPC npcs[], int count);
void LoadItems(Item items[], int count);

// Check if the player is colliding with any interactable objects
void CheckInteractable(
    NPC worldNPCs[], Item worldItems[], int npcCount, int itemCount,
    Rectangle playerHitbox, Interactable** objectToInteractWith
);

// Interact with an interactable object
void InteractWithObject(
    Interactable* objectToInteractWith, Dialogue* game_dialogue,
    GameState* game_state, Character *player
);

// Helper functions for specific interaction logic
void InteractWithNPC(NPC *npc, Dialogue *game_dialogue, GameState *game_state);
void InteractWithItem(Item *item, Character *player);

#endif