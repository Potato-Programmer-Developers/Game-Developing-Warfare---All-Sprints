/*
This file contains the declaration of the variable and functions
contained in the map.c file.

Module made by Andrew Zhuo.
*/

#ifndef MAP_H
#define MAP_H

#include "cute_tiled.h"
#include "raylib.h"

#define MAX_TILESETS 10

typedef struct {
    /* The tiled map. */
    cute_tiled_map_t* tiled_map;              // The textures of the map
    Texture2D textures[MAX_TILESETS];         // The number of tilesets
    int tileset_count;                        // The number of tilesets
} Map;

Map InitMap(const char* path);                // Initialize the map
void DrawMap(Map* map);                       // Draw the map
void FreeMap(Map* map);                       // Free the map

#endif