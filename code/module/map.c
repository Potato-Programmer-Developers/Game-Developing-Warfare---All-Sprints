/*
This file contains the logic for the map.

Made by Andrew Zhuo.
*/

#define CUTE_TILED_IMPLEMENTATION

#include <stdbool.h>
#include <stdio.h>
#include "map.h"

Map InitMap(const char* path){
    /* Initialize the map. */

    // Initialize and load the map.
    Map map = {0};
    map.tiled_map = cute_tiled_load_map_from_file(path, NULL);

    // Load the tilesets.
    cute_tiled_tileset_t* tileset = map.tiled_map->tilesets;
    while (tileset && map.tileset_count < MAX_TILESETS){
        char fullPath[256];
        snprintf(fullPath, sizeof(fullPath), "../assets/map/%s", tileset->image.ptr);
        map.textures[map.tileset_count] = LoadTexture(fullPath);
        map.tileset_count++;
        tileset = tileset->next;
    }

    return map;
}

void DrawMap(Map* map){
    /* Draw the map. */

    // Loop through the layers
    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer){
        // Only draw the tile layer
        if (strcmp(layer->type.ptr, "tilelayer") == 0){
            // For each tile in the layer
            for (int i = 0; i < layer->data_count; i++){
                // Get the raw tile ID and split the flags
                unsigned int raw_gid = (unsigned int)layer->data[i];
                int gid = cute_tiled_unset_flags(raw_gid);

                // Skip if the tile ID is 0
                if (gid == 0) continue;

                // Find the correct tileset and texture
                cute_tiled_tileset_t* tileset = map->tiled_map->tilesets;
                int tileset_idx = 0;
                while (tileset) {
                    if (gid >= tileset->firstgid && (tileset->next == NULL || gid < tileset->next->firstgid)){
                        break;
                    }
                    tileset = tileset->next;
                    tileset_idx++;
                }

                if (tileset && tileset_idx < map->tileset_count){
                    int id = gid - tileset->firstgid;
                    int tx = (id % tileset->columns) * tileset->tilewidth;
                    int ty = (id / tileset->columns) * tileset->tileheight;

                    int x = i % layer->width;
                    int y = i / layer->width;

                    bool flip_h = (raw_gid & CUTE_TILED_FLIPPED_HORIZONTALLY_FLAG) != 0;
                    bool flip_v = (raw_gid & CUTE_TILED_FLIPPED_VERTICALLY_FLAG) != 0;
                    bool flip_d = (raw_gid & CUTE_TILED_FLIPPED_DIAGONALLY_FLAG) != 0;

                    float rotation = 0.0f;

                    if (flip_d){
                        if (flip_h && flip_v){
                            rotation = 90.0f;
                            flip_h = true;
                            flip_v = false;
                        } else if (flip_h){
                            rotation = 90.0f;
                            flip_h = false;
                            flip_v = false;
                        } else if (flip_v){
                            rotation = -90.0f;
                            flip_h = false;
                            flip_v = false;
                        } else{
                            rotation = 90.0f;
                            flip_h = false;
                            flip_v = true;
                        }
                    }

                    Rectangle source = {(float)tx, (float)ty, (float)tileset->tilewidth, (float)tileset->tileheight};
                    if (flip_h) source.width = -source.width;
                    if (flip_v) source.height = -source.height;

                    Rectangle dest = {
                        (float)(x * tileset->tilewidth) + tileset->tilewidth / 2.0f,
                        (float)(y * tileset->tileheight) + tileset->tileheight / 2.0f,
                        (float)tileset->tilewidth,
                        (float)tileset->tileheight
                    };

                    Vector2 origin = {tileset->tilewidth / 2.0f, tileset->tileheight / 2.0f};

                    DrawTexturePro(map->textures[tileset_idx], source, dest, origin, rotation, WHITE);
                }
            }
        }
        layer = layer->next;
    }
}

void FreeMap(Map* map){
    /* Free the map. */
    for (int i = 0; i < map->tileset_count; i++){
        UnloadTexture(map->textures[i]);
    }
    cute_tiled_free_map(map->tiled_map);
}

bool CheckMapCollision(Map* map, Rectangle rect){
    /* Check if a rectangle collides with map objects. */
    
    // Loop through the layers
    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer){
        // Only check the object layer
        if (strcmp(layer->type.ptr, "objectgroup") == 0){
            // For each object in the layer
            cute_tiled_object_t* object = layer->objects;
            while (object){
                // Tiled objects have x, y, width, height.
                Rectangle object_rect = {
                    object->x, 
                    object->y, 
                    object->width, 
                    object->height
                };

                // Check for collision
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