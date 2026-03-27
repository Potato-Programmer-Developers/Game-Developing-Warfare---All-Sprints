#ifndef GAME_CONTEXT_H
#define GAME_CONTEXT_H

#include "raylib.h"
#include "character.h"
#include "map.h"
#include "phone.h"
#include "story.h"

// Forward declarations to break circular dependencies
typedef struct NPC NPC;
typedef struct Item Item;
typedef struct Door Door;

/**
 * @brief Global container for session-persistent data.
 * 
 * This struct holds all the data that needs to be passed between different
 * systems in the game.
 */
typedef struct GameContext {
    Camera2D camera;           // Camera for rendering
    Map *map;                  // Current map
    Character *player;         // Player character
    Location location;         // Defined in map.h
    
    Phone phone;               // Phone system
    StorySystem story;         // Story system
    
    // Dynamic asset storage
    NPC* worldNPCs;            // Array of NPCs
    int npcCount;              // Number of NPCs
    Item* worldItems;          // Array of items
    int itemCount;             // Number of items
    Door* worldDoors;          // Array of doors
    int doorCount;             // Number of doors

    int settings_previous_state;        // Store state before pause/settings
    int previous_state;                 // Store state before pause/settings
    
    bool player_teleport_requested;     // Flag to request player teleport
} GameContext;

/** @brief In-place initialization of GameContext.
 * 
 * @param context Pointer to the GameContext to initialize
 * @param map Pointer to the Map to use
 * @param player Pointer to the Character to use
 * @param location Location to start at
 */
void InitGameContext(GameContext* context, Map *map, Character *player, Location location);

/** @brief Updates camera, phone, and other global logic.
 * 
 * @param game_context Pointer to the GameContext to update
 * @param map_size Size of the map
 */
void UpdateGameContext(GameContext *game_context, Vector2 map_size);

#endif