/**
 * @file interaction.h
 * @brief Interfaces for player interaction, world objects, and story-driven triggers.
 * 
 * Update History:
 * - 2026-03-25: Initial definition of `Interactable`, `NPC`, and `Item` types. (Goal: Establish 
 *                a unified interface for world interaction.)
 * - 2026-04-03: Added `door` interaction structures for map transitions. (Goal: Support 
 *                moving between EXERTERIOR and INTERIOR maps.)
 * - 2026-04-05: Integrated session persistence for pickups and quest completion. (Goal: Allow 
 *                interaction results to influence the story phase and save-state.)
 * - 2026-04-07: Added `triggers_phone` logic to NPC interaction handler. (Goal: Bridges 
 *                dialogue results to the story-phase phone-sequence system.)
 * 
 * Revision Details:
 * - Defined `InteractableType` for `NPC`, `ITEM`, and `DOOR` classification.
 * - Expanded `Interactable` struct to include an `interactable_id` for story string matching.
 * - Added `Item` struct with `is_pickup` and `picked_up` status tracking.
 * - Created `CheckInteractable` to take a generic `playerHitbox` for flexible collision detection.
 * 
 * 
 * Authors: Andrew Zhuo and Cornelius Jabez Lim
 */

#ifndef INTERACTION_H
#define INTERACTION_H

#include <stdbool.h>
#include "dialogue.h"
#include "raylib.h"
#include "character.h"
#include "state.h"
#include "map.h"

// Forward declaration of GameContext to avoid circular dependency
struct GameContext;

/**
 * @brief Categorizes the type of interactable object.
 */
typedef enum {
    INTERACTABLE_TYPE_NPC,   // Non-player characters with dialogue
    INTERACTABLE_TYPE_ITEM,  // Collectible or inspectable world items
    INTERACTABLE_TYPE_DOOR,  // Map transition triggers
} InteractableType;

/**
 * @brief Base structure for all objects the player can interact with.
 */
typedef struct {
    Texture2D texture;             // Visual representation of the object (loaded at runtime)
    char texturePath[128];         // FS Path to the texture
    char interactable_id[64];      // Unique ID for story triggers (e.g. "fridge")
    Rectangle bounds;              // World boundaries for interaction checks
    bool isActive;                 // Whether the object is within collision range
    InteractableType type;         // Discriminator for NPC vs Item logic
    char dialoguePath[128];        // Path to the script or response file
} Interactable;

/**
 * @brief Specialized interactable for talking to characters.
 */
typedef struct NPC {
    Interactable base;             // Base interactable properties
} NPC;

/**
 * @brief Specialized interactable for world items.
 */
typedef struct Item {
    Interactable base;             // Base interactable properties
    bool picked_up;                // Persistence flag for collectibles
    bool is_pickup;                // If true, adds to inventory. If false, just shows text.
} Item;

/**
 * @brief Specialized interactable for moving between maps.
 */
typedef struct Door {
    Interactable base;             // Base interactable properties
    char targetMapPath[128];       // FS path to the destination .json map
    Location targetLocation;       // Destination location type
} Door;

/**
 * @brief Loads visual assets for a collection of NPCs.
 */
void LoadNPCs(NPC npcs[], int count);

/**
 * @brief Loads visual assets for a collection of world items.
 */
void LoadItems(Item items[], int count);

/**
 * @brief Unloads visual assets for an array of NPCs.
 * 
 * @param npcs Array of NPCs to unload assets for.
 * @param count Number of NPCs to unload assets for.
 */
void UnloadNPCs(NPC npcs[], int count);

/**
 * @brief Unloads visual assets for an array of items.
 * 
 * @param items Array of items to unload assets for.
 * @param count Number of items to unload assets for.
 */
void UnloadItems(Item items[], int count);

/**
 * @brief Detects if the player is within interaction range of any world object.
 * 
 * @param worldNPCs Array of NPCs to check for interaction.
 * @param worldItems Array of items to check for interaction.
 * @param worldDoors Array of doors to check for interaction.
 * @param npcCount Number of NPCs to check for interaction.
 * @param itemCount Number of items to check for interaction.
 * @param doorCount Number of doors to check for interaction.
 * @param playerHitbox Player's hitbox.
 * @param objectToInteractWith Pointer to the object to interact with.
 */
void CheckInteractable(
    NPC worldNPCs[], Item worldItems[], Door worldDoors[],
    int npcCount, int itemCount, int doorCount,
    Rectangle playerHitbox, Interactable** objectToInteractWith
);

/**
 * @brief High-level dispatcher for triggering an interaction.
 * 
 * @param objectToInteractWith Pointer to the object to interact with.
 * @param game_dialogue Pointer to the dialogue system.
 * @param game_state Pointer to the game state.
 * @param player Pointer to the player.
 * @param map Pointer to the map.
 * @param game_context Pointer to the game context.
 */
void InteractWithObject(
    Interactable* objectToInteractWith, Dialogue* game_dialogue,
    GameState* game_state, Character *player, Map *map, struct GameContext *game_context
);

/**
 * @brief Internal handler for Door interaction (swaps the active map).
 * 
 * @param door Pointer to the door to interact with.
 * @param map Pointer to the map.
 * @param player Pointer to the player.
 * @param game_dialogue Pointer to the dialogue system.
 * @param game_state Pointer to the game state.
 * @param game_context Pointer to the game context. 
 */
void InteractWithDoor(
    Door *door, Map *map, Character *player, Dialogue *game_dialogue,
    GameState *game_state, struct GameContext *game_context
);

/**
 * @brief Internal handler for NPC interaction (starts dialogue).
 * 
 * @param npc Pointer to the NPC to interact with.
 * @param game_dialogue Pointer to the dialogue system.
 * @param game_state Pointer to the game state.
 * @param game_context Pointer to the game context.
 */
void InteractWithNPC(
    NPC *npc, Dialogue *game_dialogue, GameState *game_state,
    struct GameContext *game_context
);

/**
 * @brief Internal handler for Item interaction (adds to inventory or shows text).
 * 
 * @param item Pointer to the item to interact with.
 * @param game_dialogue Pointer to the dialogue system.
 * @param game_state Pointer to the game state.
 * @param player Pointer to the player.
 * @param game_context Pointer to the game context.
 */
void InteractWithItem(
    Item *item, Dialogue *game_dialogue, GameState *game_state, Character *player,
    struct GameContext *game_context
);

#endif