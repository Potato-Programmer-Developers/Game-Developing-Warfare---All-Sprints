/**
 * @file map.h
 * @brief Declarations for Tiled map loading and spatial collision queries.
 * 
 * Handles management of tilesets and interactions with the cute_tiled library.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef MAP_H
#define MAP_H

#include "cute_tiled.h"
#include "raylib.h"

#define MAX_TILESETS 64

typedef enum {
    EXTERIOR,
    INTERIOR,
    FARM,
    FOREST
} Location;

/**
 * @brief Representation of the game world layer.
 * 
 * Stores the parsed Tiled map structure and its corresponding GL textures.
 */
typedef struct Map{
    cute_tiled_map_t* tiled_map;              // Parsed Tiled JSON structure
    Texture2D textures[MAX_TILESETS];         // VRAM textures for tilesets
    int tileset_count;                        // Number of active textures
    Vector2 spawn_position;                   // Spawn position for the player
} Map;

/** @brief Loads Tiled map and tileset images. */
Map InitMap(const char* path);

/** @brief Renders the map layers using Raylib textures. */
void DrawMap(Map* map);

/** @brief Deallocates map textures and Tiled memory. */
void FreeMap(Map* map);

/** @brief Performs AABB collision check against map objects. */
bool CheckMapCollision(Map* map, Rectangle rect);

/** @brief Retrieves the visual bounds of a named object from Tiled Object Layers. */
Rectangle GetMapObjectBounds(Map* map, const char* name);

#endif