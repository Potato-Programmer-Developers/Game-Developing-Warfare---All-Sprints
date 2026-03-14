/*
This file contains the logic for the map.

Made by Andrew Zhuo.
*/

#define CUTE_TILED_IMPLEMENTATION

#include "map.h"
#include <stdio.h>

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
                // Get the tile ID
                int gid = layer->data[i];

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

                    DrawTextureRec(
                        map->textures[tileset_idx],
                        (Rectangle){tx, ty, tileset->tilewidth, tileset->tileheight},
                        (Vector2){x * tileset->tilewidth, y * tileset->tileheight},
                        WHITE
                    );
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