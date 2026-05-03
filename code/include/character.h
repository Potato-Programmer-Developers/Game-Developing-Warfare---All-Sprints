/**
 * @file character.h
 * @brief Logic and data structures for player movement, stats, and animation.
 * 
 * Update History:
 * - 2026-03-21: Basic `Character` struct implementation for 2D movement. (Goal: Establish the 
 *                primary player-controlled entity.)
 * - 2026-04-03: Implemented directional movement and sprite switching. (Goal: Support up, down, 
 *                left, and right textures for more immersive navigation.)
 * - 2026-04-05: Integrated stamina and exhaustion mechanics. (Goal: Add tactical depth to 
 *                movement and exploration.)
 * 
 * Revision Details:
 * - Added `walk_up`, `walk_down`, etc., `Texture2D` members to the `Character` struct.
 * - Implemented `exhausted` and `needs_shift_reset` flags for improved stamina UX.
 * - Expanded the character to track `sanity` and `inventory` counts globally.
 * - Added `frame_rect` and `frame_speed` for flexible sprite-sheet animation control.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef CHARACTER_H
#define CHARACTER_H

#define MAX_INVENTORY_SIZE 100
#define MAX_ITEM_NAME_LENGTH 256

#include <stdbool.h>
#include "settings.h"
#include "audio.h"
#include "map.h"
#include "quest.h"

typedef struct Map Map;
typedef struct Data Data;
typedef struct StorySystem StorySystem;
typedef struct Item Item;
typedef struct NPC NPC;
typedef struct Door Door;

/**
 * @brief Represents the player character and all associated state.
 */
typedef struct Character {
    // --- Animation and Graphics ---
    Texture2D walk_down;                 // Texture for walking state
    Texture2D walk_up;                   // Texture for walking state
    Texture2D walk_left;                 // Texture for walking state
    Texture2D walk_right;                // Texture for walking state
    Texture2D lawnmower_mode;            // Texture for character in lawnmower mode
    Texture2D lawnmower_item;            // Texture for the animated lawnmower object itself
    Texture2D sprite;                    // Current active texture being drawn
    Vector2 position;                    // World coordinates of the player
    Vector2 size;                        // Visual size of the character
    float speed;                         // Current movement velocity
    int direction;                       // Facing direction (used for flipping sprites)

    // --- Animation Logic ---
    Rectangle frame_rect;                // Source rectangle for current frame in sprite sheet
    Rectangle bounds;                    // Collision/Hitbox rectangle
    int frame_number;                    // Total frames in current animation
    int current_frame;                   // Index of current animation frame
    int frame_counter;                   // Counter for frame speed logic
    int frame_speed;                     // Animation play speed

    // --- Gameplay Mechanics ---
    float stamina;                       // Current energy for running
    float max_stamina;                   // Maximum energy capacity
    float sanity;                        // Current sanity level (horror mechanic)
    float max_sanity;                    // Maximum sanity threshold
    bool exhausted;                      // Flag set when stamina reaches 0, preventing run
    bool needs_shift_reset;              // Mechanic requiring Shift key release after exhaustion

    // --- Inventory System ---
    char inventory[MAX_INVENTORY_SIZE][MAX_ITEM_NAME_LENGTH]; // Names of items currently held
    int item_count[MAX_INVENTORY_SIZE];                       // Quantities of each unique item
    int inventory_count;                                      // Total number of unique items in inventory
} Character;

/**
 * @brief Initializes character state and loads textures.
 *
 * @param game_settings Pointer to settings for initial spawn and speed.
 * @param game_data Pointer to loaded progress to restore state.
 * @param game_map Pointer to map for initial positioning.
 * @return An initialized Character structure.
 */
Character InitCharacter(Settings* game_settings, Data* game_data, Map* game_map);

/**
 * @brief Updates movement, animation, and stamina based on input.
 *
 * @param character Pointer to the player character.
 * @param game_settings Pointer to game settings.
 * @param map_size Total dimensions of the world for clamping.
 * @param map Pointer to map for collision detection.
 * @param audio Pointer to audio system for footstep sounds.
 * @param location Current character location for footstep sounds.
 * @param story Pointer to the story system for tutorial progress.
 */
void UpdateCharacter(Character *character, Settings *game_settings, Vector2 map_size, Map *map, Audio* audio, Location location, StorySystem* story, Item* items, int itemCount, NPC* npcs, int npcCount, Door* doors, int doorCount, char picked_up_registry[][64], int picked_up_count);

/**
 * @brief Unloads all character textures and frees resources.
 *
 * @param character Pointer to the character to clean up.
 */
void CloseCharacter(Character* character);

/**
 * @brief Draws the character at their current position/frame.
 *
 * @param character Pointer to the character to draw.
 */
void DrawCharacter(Character* character);

#endif