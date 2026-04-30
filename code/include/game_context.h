/**
 * @file game_context.h
 * @brief Global container for session-persistent data, world objects, and active systems.
 * 
 * Update History:
 * - 2026-03-20: Initial creation of the `GameContext` struct. (Goal: Provide a single point of 
 *                access for player and map data across the project.)
 * - 2026-04-03: Added `picked_up_registry` and `met_npcs`. (Goal: Support session-long 
 *                persistence for item collection and NPC encounters.)
 * - 2026-04-05: Integrated the `StorySystem` and `Phone` modules into the context. (Goal: 
 *                Enable cross-module communication for narrative-driven triggers.)
 * 
 * Revision Details:
 * - Added `previous_state` and `settings_previous_state` for robust pause-menu navigation.
 * - Expanded the context to hold a pointer to the global `Scene` for camera/fade controls.
 * - Implemented `dream_lines` and `dream_count` for nocturnal event string storage.
 * - Added `left_box_big`, `left_box_small`, etc., for specific interaction mechanics in SET5.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef GAME_CONTEXT_H
#define GAME_CONTEXT_H

#include "raylib.h"
#include "character.h"
#include "map.h"
#include "phone.h"
#include "story.h"

typedef struct NPC NPC;
typedef struct Item Item;
typedef struct Door Door;
typedef struct Scene Scene;
struct Dialogue;
struct Settings; // Forward Declaration

typedef struct {
    char text[64];
} UsedRegistry;

typedef struct {
    char pot_id[64];
    bool is_planted;
    int seed_type;              // 1: Tomato, 2: Lettuce, 3: Potato
} PotStatus;

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
    
    Phone phone;                      // Phone system
    StorySystem story;                // Story system
    Scene *game_scene;                // Pointer to the game scene for fading/transitions
    struct Dialogue *game_dialogue;   // Pointer to the dialogue system
    
    // Dynamic asset storage
    NPC* worldNPCs;            // Array of NPCs
    int npcCount;              // Number of NPCs
    Item* worldItems;          // Array of items
    int itemCount;             // Number of items
    Door* worldDoors;          // Array of doors
    int doorCount;             // Number of doors

    int settings_previous_state;        // Store state before pause/settings
    int previous_state;                 // Store state before pause/settings
    
    // SET5 Farm Mechanics
    int left_box_big;                   // Number of big boxes on the left
    int left_box_small;                 // Number of small boxes on the left
    int right_box_big;                  // Number of big boxes on the right
    int right_box_small;                // Number of small boxes on the right
    float day3_mowing_timer;            // Timer for Day 3 mowing performance

    bool fireplace_on;                  // Tracks if the fireplace is burning (SET7)
    bool doors;                         // Tracks if INTERIOR doors are active/drawn
    bool main_door_locked;              // Nightly: main door locked state
    bool windows_locked;                // Nightly: windows locked state
    bool has_room_keys;                 // Nightly: player has room keys
    bool look_outside;                  // Nightly: looked outside before sleep
    bool bear_trap_inside;              // Nightly: bear trap placed inside
    bool bear_trap_outside;             // Nightly: bear trap placed outside
    char last_narration_action[32];     // Nightly: last choice made before bed

    // Dream Sequence State
    bool dream_active;                  // Flag for active dream sequence
    char dream_lines[4][128];           // Text lines to show in dream
    int dream_count;                    // Total lines to show
    int dream_current;                  // Current line being shown
    float dream_timer;                  // Timer for auto-advancing dream text

    bool player_teleport_requested;     // Flag to request player teleport
    
    // Persistent World State
    char picked_up_registry[512][64];   // Registry of unique interactable IDs picked up
    int picked_up_count;                // Number of unique items picked up
    
    char met_npcs[64][64];            // Registry of unique NPC IDs spoken to
    int met_npc_day[64];              // Day index when met
    int met_npc_set[64];              // Set index when met
    int met_npc_phase[64];            // Phase index when met
    int met_npc_count;                // Number of NPCs spoken to

    PotStatus pot_registry[18];       // Tracking for Day 2 planting mechanic
    UsedRegistry dialogue_used_lines[256]; // Persisting once-only responses
    int used_lines_count;             // Number of lines that have been used
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