/**
 * @file character.h
 * @brief Player character definitions and movement logic.
 * 
 * This module defines the main character's state (stamina, hallucination),
 * animation frames, inventory, and movement functions.
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

/**
 * @brief Represents the player character and all associated state.
 */
typedef struct Character {
    // --- Animation and Graphics ---
    Texture2D sprite_idle;               // Texture for idle state
    Texture2D sprite_walk;               // Texture for walking state
    Texture2D sprite_run;                // Texture for running state
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
 * @param game_settings Pointer to settings for initial spawn and speed.
 * @param game_data Pointer to loaded progress to restore state.
 * @param game_map Pointer to map for initial positioning.
 * @return An initialized Character structure.
 */
Character InitCharacter(Settings* game_settings, Data* game_data, Map* game_map);

/**
 * @brief Updates movement, animation, and stamina based on input.
 * @param character Pointer to the player character.
 * @param game_settings Pointer to game settings.
 * @param map_size Total dimensions of the world for clamping.
 * @param map Pointer to map for collision detection.
 * @param audio Pointer to audio system for footstep sounds.
 * @param location Current character location for footstep sounds.
 * @param quest_system Pointer to the quest system for tutorial progress.
 */
void UpdateCharacter(Character* character, Settings* game_settings, Vector2 map_size, Map* map, Audio* audio, Location location, StorySystem* story);

/**
 * @brief Unloads all character textures and frees resources.
 * @param character Pointer to the character to clean up.
 */
void CloseCharacter(Character* character);

/**
 * @brief Draws the character at their current position/frame.
 * @param character Pointer to the character to draw.
 */
void DrawCharacter(Character* character);

#endif