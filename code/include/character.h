/*
This file contains variable declarations and function prototypes for the
mc module.

Module made by Andrew Zhuo.
*/

#ifndef CHARACTER_H
#define CHARACTER_H

#define MAX_INVENTORY_SIZE 6
#define MAX_ITEM_NAME_LENGTH 10

#include <stdbool.h>
#include "settings.h"

typedef struct Map Map;
typedef struct Data Data;

typedef struct Character{
    /* Struct for character */
    Texture2D sprite_idle;               // Idle sprite of the character
    Texture2D sprite_walk;               // Walk sprite of the character
    Texture2D sprite_run;                // Run sprite of the character
    Texture2D sprite;                    // Current sprite of the character
    Vector2 position;                    // Position of the character
    Vector2 size;                        // Size of the character
    float speed;                         // Speed of the character
    int direction;                       // Direction the character is facing

    Rectangle frame_rect;                // Rectangle for the current frame
    Rectangle bounds;                    // Rectangle for the character's bounds
    int frame_number;                    // Number of frames in the sprite
    int current_frame;                   // Current frame of the sprite
    int frame_counter;                   // Counter for the current frame
    int frame_speed;                     // Speed of the sprite animation

    char inventory[MAX_INVENTORY_SIZE][MAX_ITEM_NAME_LENGTH];            // Inventory of the character
    int inventory_count;                                                 // Number of items in the inventory
} Character;


Character InitCharacter(Settings* game_settings, Data* game_data);                                           // Initialize the character.
void UpdateCharacter(Character* character, Settings* game_settings, Vector2 map_size, Map* map);             // Update the character.
void CloseCharacter(Character* character);                                                                   // Close the character.
void DrawCharacter(Character* character);                                                                    // Draw the character.

#endif