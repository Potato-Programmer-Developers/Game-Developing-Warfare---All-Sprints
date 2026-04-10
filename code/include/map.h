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

#define MAX_TILESETS 128

/**
 * @brief Enum for different locations in the game.
 */
typedef enum {
    APARTMENT,
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
    char current_path[128];                   // Source file path for reloading
} Map;

/**
 * @brief Loads Tiled map and tileset images.
 *
 * @param path Path to the Tiled map file.
 * @param spawn_id Specific map object ID to spawn the player on, or NULL.
 * @return Initialized Map struct.
 */
Map InitMap(const char* path, const char* spawn_id);

/**
 * @brief Renders the map layers using Raylib textures.
 *
 * @param map Pointer to the Map struct.
 * @param fireplace_on Flag to determine if fireplace layer should be drawn.
 * @param doors Flag to determine if doors layer should be drawn.
 */
void DrawMap(Map* map, bool fireplace_on, bool doors, bool day2_active, int set_idx);

/**
 * @brief Deallocates map textures and Tiled memory.
 *
 * @param map Pointer to the Map struct to clean up.
 */
void FreeMap(Map* map);

/**
 * @brief Performs AABB collision check against map objects.
 *
 * @param map Pointer to the Map struct.
 * @param rect Rectangle to check for collision.
 * @return True if collision occurs, false otherwise.
 */
bool CheckMapCollision(Map* map, Rectangle rect, char picked_up_registry[][64], int picked_up_count, bool day2_active);

/**
 * @brief Retrieves the visual bounds of a named object from Tiled Object Layers.
 *
 * @param map Pointer to the Map struct.
 * @param name Name of the object to retrieve.
 * @return Rectangle containing the bounds of the object.
 */
Rectangle GetMapObjectBounds(Map* map, const char* name);

#endif