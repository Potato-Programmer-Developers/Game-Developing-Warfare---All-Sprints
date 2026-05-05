/**
 * @file game_context.c
 * @brief Implementation of global game state and camera orchestration.
 * 
 * Update History:
 * - 2026-03-20: Initial camera and context logic. (Goal: Support basic 2D camera tracking.)
 * - 2026-05-03: Added photo overlay state initialization and Mike cutscene camera logic. 
 *                (Goal: Support the Mike cinematic sequence with dedicated camera clamping.)
 * 
 * Revision Details:
 * - Updated `UpdateGameContext` to implement strict camera clamping during the Mike cutscene,
 *    preventing the camera from moving outside map boundaries while following Mike.
 * - Updated `UpdateGameContext` to handle camera offset logic (photo timer logic moved to `state.c`).
 * - Updated `InitGameContext` to zero-initialize the `photo_overlay` structure via `memset`.
 * 
 * Authors: Andrew Zhuo
 */

#include <string.h>
#include <stdlib.h>
#include "game_context.h"
#include "raymath.h"

void InitGameContext(GameContext* context, Map *map, Character *player, Location location){
    memset(context, 0, sizeof(GameContext));       // Reset the game context

	// Initialize the camera
	context->camera.offset = (Vector2){(float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f};
	context->camera.target = player->position;
	context->camera.rotation = 0.0f;
	context->camera.zoom = 1.0f;

	// Initialize the map, player, and location
	context->map = map;
	context->player = player;
	context->location = location;
	context->settings_previous_state = 0; 

	// Initialize the phone and player teleport request
	InitPhone(&context->phone);
	context->player_teleport_requested = false;
}

void UpdateGameContext(GameContext *game_context, Vector2 map_size){
	float screen_width = (float)GetScreenWidth();
	float screen_height = (float)GetScreenHeight();
	game_context->camera.offset = (Vector2){screen_width / 2.0f, screen_height / 2.0f};

	if (!game_context->mike_cutscene_active) {
		// Update the camera target to the player's position
		game_context->camera.target = game_context->player->position;
	}

	// Update the story system
	StoryPhase* active = GetActivePhase(&game_context->story);
	if (active && strcmp(active->name, "SET1-PHASE1") == 0) {
		if (game_context->phone.state == PHONE_OPENED) {
			if (active->quest_count > 2) active->quests[2].completed = true;
		}
	}

	// Mike Cutscene Logic
	if (game_context->location == EXTERIOR && strcmp(game_context->story.day_folder, "day3") == 0 && 
		game_context->story.current_set_idx == 0 && game_context->story.current_phase_idx == 0 && !game_context->mike_cutscene_played) {
		
		float dist = Vector2Distance(game_context->player->position, game_context->mike_pos);
		if (!game_context->mike_cutscene_active && dist < 800.0f) {
			game_context->mike_cutscene_active = true;
			game_context->mike_cutscene_stage = 1;
		}

		if (game_context->mike_cutscene_active) {
			// 1. Block player movement
			game_context->player->can_move = false;

			switch (game_context->mike_cutscene_stage) {
				case 1: // Focus Mike
					game_context->camera.target = Vector2Lerp(game_context->camera.target, game_context->mike_pos, 0.05f);
					if (Vector2Distance(game_context->camera.target, game_context->mike_pos) < 2.0f) {
						game_context->mike_cutscene_stage = 2;
					}
					break;
				case 2: // Mike move to Road
					{
						Vector2 road_center = { 2881.0f + 100.0f / 2.0f, 2688.0f };
						game_context->mike_pos = Vector2MoveTowards(game_context->mike_pos, road_center, 2.0f);
						game_context->camera.target = game_context->mike_pos;
						if (Vector2Distance(game_context->mike_pos, road_center) < 10.0f) {
							game_context->mike_cutscene_stage = 3;
						}
					}
					break;
				case 3: // Mike move Down
					game_context->mike_pos.y += 2.0f;
					game_context->camera.target = game_context->mike_pos;
					if (game_context->mike_pos.y > map_size.y) { // Disappear from map
						game_context->mike_cutscene_stage = 4;
					}
					break;
				case 4: // End Cutscene
					game_context->camera.target = Vector2Lerp(game_context->camera.target, game_context->player->position, 0.05f);
					if (Vector2Distance(game_context->camera.target, game_context->player->position) < 2.0f) {
						game_context->mike_cutscene_active = false;
						game_context->mike_cutscene_played = true;
						game_context->player->can_move = true;
					}
					break;
			}
		}
	}

	// ALWAYS Clamp the camera target to the map boundaries (including during cutscenes)
	game_context->camera.target.x = Clamp(game_context->camera.target.x,
			game_context->camera.offset.x / game_context->camera.zoom,
			map_size.x - (screen_width - game_context->camera.offset.x) / game_context->camera.zoom);
			
	game_context->camera.target.y = Clamp(game_context->camera.target.y,
			game_context->camera.offset.y / game_context->camera.zoom,
			map_size.y - (screen_height - game_context->camera.offset.y) / game_context->camera.zoom);

	if (map_size.x < screen_width / game_context->camera.zoom) game_context->camera.target.x = map_size.x / 2.0f;
	if (map_size.y < screen_height / game_context->camera.zoom) game_context->camera.target.y = map_size.y / 2.0f;
}
