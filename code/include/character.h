/*
This file contains variable declarations and function prototypes for the
mc module.

Module made by Andrew Zhuo.
*/

#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdbool.h>
#include "settings.h"

typedef struct Character{
    /* This struct contains the information for character in the game. */
    Texture2D sprite_idle;   // Idle sprite of the character.
    Texture2D sprite_walk;   // Walking sprite of the character.
    Texture2D sprite_run;    // Running sprite of the character.
    Texture2D sprite;        // Current sprite of the character.

    Vector2 position;        // Position of the character.
    Vector2 size;            // Size of the character.
    float speed;             // Speed of the character.
    int direction;           // Direction of the character (1 = left, 2 = right, 3 = up, 4 = down).

    Rectangle frame_rect;    // Rectangle of the current frame of the animation.

    int frame_number;        // Number of frames in the animation.
    int current_frame;       // Current frame of the animation.
    int frame_counter;       // Counter for frame animation.
    int frame_speed;         // Speed of the animation.
} Character;


Character InitCharacter(Settings* game_settings);                                           // Initialize the character.
void UpdateCharacter(Character* character, Settings* game_settings, Vector2 map_size);      // Update the character.
void CloseCharacter(Character* character);                                                  // Close the character.
void DrawCharacter(Character* character);                                                   // Draw the character.

#endif