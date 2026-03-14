/*
This file contains the implementation of the game camera and 
camera-related logic.

Made by Andrew Zhuo.
*/

#include "game_context.h"
#include "raymath.h"

GameContext InitGameContext(Map* map, Character* player, Settings* settings){
    /* Initialize the game context */
    GameContext game_context = {0};

    // Initialize the camera.
    game_context.camera.offset = (Vector2){settings->window_width / 2.0f, settings->window_height / 2.0f};
    game_context.camera.target = player->position;
    game_context.camera.rotation = 0.0f;
    game_context.camera.zoom = 1.0f;

    // Initialize the map.
    game_context.map = map;

    // Initialize the player.
    game_context.player = player;

    return game_context;
}

void UpdateGameContext(GameContext* game_context, Settings* settings, Vector2 map_size){
    /* Update the game context. */
    
    // Set camera target to follow player
    game_context->camera.target = game_context->player->position;

    // Clamping camera target to map boundaries.
    game_context->camera.target.x = Clamp(
        game_context->camera.target.x,
        game_context->camera.offset.x / game_context->camera.zoom,
        map_size.x - (settings->window_width - game_context->camera.offset.x) / game_context->camera.zoom
    );
    game_context->camera.target.y = Clamp(
        game_context->camera.target.y,
        game_context->camera.offset.y / game_context->camera.zoom,
        map_size.y - (settings->window_height - game_context->camera.offset.y) / game_context->camera.zoom
    );
    
    // If map is smaller than window, keep the camera centered
    if (map_size.x < settings->window_width / game_context->camera.zoom){
        game_context->camera.target.x = map_size.x / 2.0f;
    }
    if (map_size.y < settings->window_height / game_context->camera.zoom){
        game_context->camera.target.y = map_size.y / 2.0f;
    }
}
