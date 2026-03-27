/**
 * @file map.c
 * @brief Implementation of the Tiled map system and collision physics.
 * 
 * Integrates cute_tiled to parse .json exports from Tiled. Handles 
 * multi-layered rendering and object-layer collision checks.
 * 
 * Authors: Andrew Zhuo
 */

#define CUTE_TILED_IMPLEMENTATION

#include <stdbool.h>
#include <stdio.h>
#include "map.h"

/**
 * @brief Loads a Tiled map JSON file and its associated tileset textures.
 * 
 * @param path FS path to the .json map file.
 * @return Populated Map struct with VRAM textures and cute_tiled handle.
 */
Map InitMap(const char* path){
    Map map = {0};
    map.tiled_map = cute_tiled_load_map_from_file(path, NULL);

    // Extract the directory from the map path
    char dir[256] = {0};
    const char* lastSlash = strrchr(path, '/');
    if (lastSlash) {
        int len = lastSlash - path + 1;
        strncpy(dir, path, len);
        dir[len] = '\0';
    }

    // Load each tileset referenced by the map
    cute_tiled_tileset_t* tileset = map.tiled_map->tilesets;
    while (tileset && map.tileset_count < MAX_TILESETS){
        char fullPath[256];
        // Note: cute_tiled provides relative paths; we prefix with our directory path
        snprintf(fullPath, sizeof(fullPath), "%s%s", dir, tileset->image.ptr);
        map.textures[map.tileset_count] = LoadTexture(fullPath);
        map.tileset_count++;
        tileset = tileset->next;
    }

    // Search for a specialized "Spawn" object in any object layer
    cute_tiled_layer_t* layer = map.tiled_map->layers;
    while (layer){
        if (strcmp(layer->type.ptr, "objectgroup") == 0){
            cute_tiled_object_t* object = layer->objects;
            while (object){
                if (object->name.ptr && strcmp(object->name.ptr, "Spawn") == 0) {
                    map.spawn_position = (Vector2){(float)object->x, (float)object->y};
                    return map;
                }
                object = object->next;
            }
        }
        layer = layer->next;
    }

    return map;
}

/**
 * @brief Renders the tile layers of the map.
 * 
 * Iterates through layers, identifies tiles by Global ID (GID), and 
 * handles Tiled's bit-flagged transformations (horiz/vert/diag flips).
 */
void DrawMap(Map* map){
    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer){
        // We only render 'tilelayer' types here; 'objectgroup' is used for physics
        if (strcmp(layer->type.ptr, "tilelayer") == 0){
            for (int i = 0; i < layer->data_count; i++){
                unsigned int raw_gid = (unsigned int)layer->data[i];
                int gid = cute_tiled_unset_flags(raw_gid); // Extract ID from flags

                if (gid == 0) continue; // 0 is an empty tile

                // 1. Find the parent tileset for this GID
                cute_tiled_tileset_t* tileset = map->tiled_map->tilesets;
                int tileset_idx = 0;
                while (tileset) {
                    if (gid >= tileset->firstgid && (tileset->next == NULL || gid < tileset->next->firstgid)){
                        break;
                    }
                    tileset = tileset->next;
                    tileset_idx++;
                }

                // 2. Calculate source/dest geometry
                if (tileset && tileset_idx < map->tileset_count){
                    int id = gid - tileset->firstgid;
                    int tx = (id % tileset->columns) * tileset->tilewidth;
                    int ty = (id / tileset->columns) * tileset->tileheight;

                    int x = i % layer->width;
                    int y = i / layer->width;

                    // 3. Handle Bitwise Flip Flags
                    bool flip_h = (raw_gid & CUTE_TILED_FLIPPED_HORIZONTALLY_FLAG) != 0;
                    bool flip_v = (raw_gid & CUTE_TILED_FLIPPED_VERTICALLY_FLAG) != 0;
                    bool flip_d = (raw_gid & CUTE_TILED_FLIPPED_DIAGONALLY_FLAG) != 0;

                    float rotation = 0.0f;

                    // Diagonal flip in Tiled indicates rotation + flipping
                    if (flip_d){
                        if (flip_h && flip_v){
                            rotation = 90.0f;
                            flip_h = true; flip_v = false;
                        } else if (flip_h){
                            rotation = 90.0f;
                            flip_h = false; flip_v = false;
                        } else if (flip_v){
                            rotation = -90.0f;
                            flip_h = false; flip_v = false;
                        } else{
                            rotation = 90.0f;
                            flip_h = false; flip_v = true;
                        }
                    }

                    Rectangle source = {(float)tx, (float)ty, (float)tileset->tilewidth, (float)tileset->tileheight};
                    if (flip_h) source.width = -source.width;
                    if (flip_v) source.height = -source.height;

                    // Destination centered for rotation support
                    Rectangle dest = {
                        (float)(x * tileset->tilewidth) + tileset->tilewidth / 2.0f,
                        (float)(y * tileset->tileheight) + tileset->tileheight / 2.0f,
                        (float)tileset->tilewidth, (float)tileset->tileheight
                    };

                    Vector2 origin = {tileset->tilewidth / 2.0f, tileset->tileheight / 2.0f};

                    DrawTexturePro(map->textures[tileset_idx], source, dest, origin, rotation, WHITE);
                }
            }
        }
        layer = layer->next;
    }
}

/** @brief Deallocates map textures and the parsed Tiled map object. */
void FreeMap(Map* map){
    for (int i = 0; i < map->tileset_count; i++){
        UnloadTexture(map->textures[i]);
    }
    cute_tiled_free_map(map->tiled_map);
}

/**
 * @brief Spatial query: checks if a world-space rectangle hits a collision object.
 * 
 * Scans Tiled 'objectgroup' layers for rectangles. 
 * Essential for player-vs-wall collisions.
 */
bool CheckMapCollision(Map* map, Rectangle rect){
    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer){
        if (strcmp(layer->type.ptr, "objectgroup") == 0){
            cute_tiled_object_t* object = layer->objects;
            while (object){
                Rectangle object_rect = { (float)object->x, (float)object->y, (float)object->width, (float)object->height };

                if (CheckCollisionRecs(rect, object_rect)){
                    return true;
                }
                object = object->next;
            }
        }
        layer = layer->next;
    }

    return false;
}

/**
 * @brief Retrieves the visual bounds of a named object from Tiled Object Layers.
 * 
 * Scans all 'objectgroup' layers for an object with a matching 'name' field.
 * Returns a Rectangle{0,0,0,0} if not found.
 */
Rectangle GetMapObjectBounds(Map* map, const char* name) {
    if (!map || !map->tiled_map || !name || strlen(name) == 0) return (Rectangle){0, 0, 0, 0};
    
    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer) {
        if (strcmp(layer->type.ptr, "objectgroup") == 0) {
            cute_tiled_object_t* object = layer->objects;
            while (object) {
                if (object->name.ptr && strcmp(object->name.ptr, name) == 0) {
                    return (Rectangle){ (float)object->x, (float)object->y, (float)object->width, (float)object->height };
                }
                object = object->next;
            }
        }
        layer = layer->next;
    }
    return (Rectangle){0, 0, 0, 0};
}