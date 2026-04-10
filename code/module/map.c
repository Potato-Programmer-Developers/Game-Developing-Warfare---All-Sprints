#define CUTE_TILED_IMPLEMENTATION
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "map.h"

Map InitMap(const char* path, const char* spawn_id){
    Map map = {0};
    strncpy(map.current_path, path, 127);
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
        snprintf(fullPath, sizeof(fullPath), "%s%s", dir, tileset->image.ptr);
        map.textures[map.tileset_count] = LoadTexture(fullPath);
        map.tileset_count++;
        tileset = tileset->next;
    }

    // Search for a specialized spawn object
    const char* target_spawn = (spawn_id && spawn_id[0] != '\0') ? spawn_id : "Spawn";
    cute_tiled_layer_t* layer = map.tiled_map->layers;
    while (layer){
        if (strcmp(layer->type.ptr, "objectgroup") == 0){
            cute_tiled_object_t* object = layer->objects;
            while (object){
                if (object->name.ptr && strcmp(object->name.ptr, target_spawn) == 0) {
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

void DrawMap(Map* map, bool fireplace_on, bool doors, bool day2_active, int set_idx){
    if (!map || !map->tiled_map) return;
    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer){
        if (strcmp(layer->type.ptr, "tilelayer") == 0){
            if (!fireplace_on && layer->name.ptr && strcmp(layer->name.ptr, "fireplace") == 0) {
                layer = layer->next;
                continue;
            }
            if (!doors && layer->name.ptr && strcmp(layer->name.ptr, "doors") == 0) {
                layer = layer->next;
                continue;
            }
            
            // Logic: Skip pot tiles in Day 1. Skip clues tile layer unless specifically in Day 2 SET 1.
            if (layer->name.ptr) {
                if (strcmp(layer->name.ptr, "pot") == 0 && !day2_active) {
                    layer = layer->next;
                    continue;
                }
                if (strcmp(layer->name.ptr, "clues") == 0) {
                    if (!(day2_active && set_idx == 0)) {
                        layer = layer->next;
                        continue;
                    }
                }
            }
            for (int i = 0; i < layer->data_count; i++){
                unsigned int raw_gid = (unsigned int)layer->data[i];
                int gid = cute_tiled_unset_flags(raw_gid);
                if (gid == 0) continue;

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

                    // Use the MAP's tile size for grid positioning
                    int map_tw = map->tiled_map->tilewidth;
                    int map_th = map->tiled_map->tileheight;

                    bool flip_h = (raw_gid & CUTE_TILED_FLIPPED_HORIZONTALLY_FLAG) != 0;
                    bool flip_v = (raw_gid & CUTE_TILED_FLIPPED_VERTICALLY_FLAG) != 0;
                    bool flip_d = (raw_gid & CUTE_TILED_FLIPPED_DIAGONALLY_FLAG) != 0;
                    float rotation = 0.0f;

                    if (flip_d){
                        if (flip_h && flip_v){ rotation = 90.0f; flip_h = true; flip_v = false; }
                        else if (flip_h){ rotation = 90.0f; flip_h = false; flip_v = false; }
                        else if (flip_v){ rotation = -90.0f; flip_h = false; flip_v = false; }
                        else{ rotation = 90.0f; flip_h = false; flip_v = true; }
                    }

                    Rectangle source = {
                        (float)tx, 
                        (float)ty, 
                        (float)tileset->tilewidth, 
                        (float)tileset->tileheight
                    };
                    if (flip_h) source.width = -source.width;
                    if (flip_v) source.height = -source.height;

                    // Position on map grid using map tile size, but draw at the tile's actual size
                    // Center the tile within the grid cell, aligned to the bottom
                    float dest_x = (float)(x * map_tw) + map_tw / 2.0f;
                    float dest_y = (float)(y * map_th) + map_th - tileset->tileheight / 2.0f;
                    
                    Rectangle dest = {
                        dest_x,
                        dest_y,
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

void FreeMap(Map* map){
    if (!map) return;
    for (int i = 0; i < map->tileset_count; i++){
        UnloadTexture(map->textures[i]);
    }
    if (map->tiled_map) cute_tiled_free_map(map->tiled_map);
}

bool CheckMapCollision(Map* map, Rectangle rect, char picked_up_registry[][64], int picked_up_count, bool day2_active){
    if (!map || !map->tiled_map) return false;
    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer){
        if (strcmp(layer->type.ptr, "objectgroup") == 0){
            // Logic: Clues layer NEVER has collision (interaction only). Pot has collision in Day 2+ only.
            if (layer->name.ptr) {
                if (strcmp(layer->name.ptr, "clues") == 0) {
                    layer = layer->next;
                    continue;
                }
                if (strcmp(layer->name.ptr, "pot") == 0 && !day2_active) {
                    layer = layer->next;
                    continue;
                }
            }
            
            cute_tiled_object_t* object = layer->objects;
            while (object){
                // If it's a named object, check if it's already picked up
                bool skip = false;
                if (object->name.ptr && strlen(object->name.ptr) > 0) {
                    for (int i = 0; i < picked_up_count; i++) {
                        if (strcmp(picked_up_registry[i], object->name.ptr) == 0) {
                            skip = true;
                            break;
                        }
                    }
                }
                
                if (!skip) {
                    Rectangle object_rect = { (float)object->x, (float)object->y, (float)object->width, (float)object->height };
                    if (CheckCollisionRecs(rect, object_rect)) return true;
                }
                object = object->next;
            }
        }
        layer = layer->next;
    }
    return false;
}

Rectangle GetMapObjectBounds(Map* map, const char* name){
    if (!map || !map->tiled_map || !name || strlen(name) == 0) return (Rectangle){0, 0, 0, 0};
    cute_tiled_layer_t* layer = map->tiled_map->layers;
    while (layer){
        // Get the bounds of the object in Tiled map data
        if (strcmp(layer->type.ptr, "objectgroup") == 0){
            cute_tiled_object_t* object = layer->objects;
            while (object){
                if (object->name.ptr && strcmp(object->name.ptr, name) == 0){
                    return (Rectangle){
                        (float)object->x, 
                        (float)object->y, 
                        (float)object->width, 
                        (float)object->height
                    };
                }
                object = object->next;
            }
        }
        layer = layer->next;
    }
    return (Rectangle){0, 0, 0, 0};
}