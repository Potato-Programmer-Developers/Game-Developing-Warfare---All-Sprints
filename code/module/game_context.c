/**
 * @file game_context.c
 * @brief Implementation of global game state and camera orchestration.
 */

#include <string.h>
#include <stdlib.h>
#include "game_context.h"
#include "raymath.h"

void InitGameContext(GameContext* context, Map *map, Character *player, Location location) {
    memset(context, 0, sizeof(GameContext));

	context->camera.offset = (Vector2){(float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f};
	context->camera.target = player->position;
	context->camera.rotation = 0.0f;
	context->camera.zoom = 1.0f;

	context->map = map;
	context->player = player;
	context->location = location;
	context->settings_previous_state = 0; 

	InitPhone(&context->phone);
	context->player_teleport_requested = false;
}

void UpdateGameContext(GameContext *game_context, Vector2 map_size) {
	game_context->camera.target = game_context->player->position;

	float screen_width = (float)GetScreenWidth();
	float screen_height = (float)GetScreenHeight();
	game_context->camera.offset = (Vector2){screen_width / 2.0f, screen_height / 2.0f};

	game_context->camera.target.x = Clamp(game_context->camera.target.x,
			game_context->camera.offset.x / game_context->camera.zoom,
			map_size.x - (screen_width - game_context->camera.offset.x) / game_context->camera.zoom);
			
	game_context->camera.target.y = Clamp(game_context->camera.target.y,
			game_context->camera.offset.y / game_context->camera.zoom,
			map_size.y - (screen_height - game_context->camera.offset.y) / game_context->camera.zoom);

	if (map_size.x < screen_width / game_context->camera.zoom) game_context->camera.target.x = map_size.x / 2.0f;
	if (map_size.y < screen_height / game_context->camera.zoom) game_context->camera.target.y = map_size.y / 2.0f;

	StoryPhase* active = GetActivePhase(&game_context->story);
	if (active && strcmp(active->name, "SET1-PHASE1") == 0) {
		if (game_context->phone.state == PHONE_OPENED) {
			if (active->quest_count > 2) active->quests[2].completed = true;
		}
	}
}
