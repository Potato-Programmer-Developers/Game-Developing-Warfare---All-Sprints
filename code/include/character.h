/*
This file contains variable declarations and function prototypes for the
mc module.

Module made by Andrew Zhuo.
*/

#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdbool.h>
#include "settings.h"

typedef struct Map Map;

typedef struct Character{
    Texture2D sprite_idle;
    Texture2D sprite_walk;
    Texture2D sprite_run;
    Texture2D sprite;
    Vector2 position;
    Vector2 size;
    float speed;
    int direction;

    Rectangle frame_rect;
    Rectangle bounds;
    int frame_number;
    int current_frame;
    int frame_counter;
    int frame_speed;
} Character;


Character InitCharacter(Settings* game_settings);                                                      // Initialize the character.
void UpdateCharacter(Character* character, Settings* game_settings, Vector2 map_size, Map* map);       // Update the character.
void CloseCharacter(Character* character);                                                             // Close the character.
void DrawCharacter(Character* character);                                                              // Draw the character.

#endif