/**
 * @file game_context.c
 * @brief Implementation of global game state and camera orchestration.
 * 
 * Manages the transition between game states and handles complex camera
 * behaviors like resolution-independent zoom and boundary clamping.
 * 
 * Authors: Andrew Zhuo
 */

#include "game_context.h"
#include "raymath.h"

/**
 * @brief Initializes the global game context and sets up FOV-consistent camera.
 */
GameContext InitGameContext(Map *map, Character *player, Settings *settings) {
  GameContext game_context = {0};

  // --- Dynamic Base Zoom Calculation ---
  // To ensure the game looks the same on a 1080p screen vs a 4K screen,
  // we define a 'baseline visible width' in world units.
  float baseline_visible_width = 2200.0f;
  float initial_zoom = (float)GetScreenWidth() / baseline_visible_width;

  // Camera Setup
  game_context.camera.offset = (Vector2){(float)GetScreenWidth() / 2.0f,
                                         (float)GetScreenHeight() / 2.0f};
  game_context.camera.target = player->position;
  game_context.camera.rotation = 0.0f;
  game_context.camera.zoom = initial_zoom;

  // Link core systems
  game_context.map = map;
  game_context.player = player;
  game_context.is_outdoor = true;
  game_context.settings_previous_state = 0; // Default to MAINMENU

  // Sub-system initialization
  InitPhone(&game_context.phone);

  return game_context;
}

/**
 * @brief Per-frame update for global systems (Camera, Phone).
 * 
 * Handles camera following logic and applies hallucination-based FOV warping.
 */
void UpdateGameContext(GameContext *game_context, Settings *settings,
                       Vector2 map_size) {
  // 1. Follow the player's position
  game_context->camera.target = game_context->player->position;

  // 2. Handle Dynamic Window Scaling
  // Refresh screen dimensions in case of resize/fullscreen toggle
  float screen_width = (float)GetScreenWidth();
  float screen_height = (float)GetScreenHeight();

  game_context->camera.offset = (Vector2){screen_width / 2.0f, screen_height / 2.0f};

  // Maintain the 2200-unit visible width baseline
  float baseline_visible_width = 2200.0f;
  float base_zoom = screen_width / baseline_visible_width;
  game_context->camera.zoom = base_zoom;

  // 3. Hallucination Camera Warp
  // When hallucination is critically high (>90%), zoom in to increase claustrophobia
  if (game_context->player->hallucination > game_context->player->max_hallucination * 0.9f) {
    float zoom_increase = (game_context->player->hallucination -
                           game_context->player->max_hallucination * 0.9f) /
                          (game_context->player->max_hallucination * 0.1f) * 0.5f;
    game_context->camera.zoom = base_zoom + zoom_increase;

    // Safety clamp (cap at +1.5 units) to prevent looking inside the character sprite
    if (game_context->camera.zoom > base_zoom + 1.5f)
      game_context->camera.zoom = base_zoom + 1.5f;
  }

  // 4. Map Boundary Clamping
  // Prevents the camera from showing the black void outside the tiled map
  game_context->camera.target.x = Clamp(game_context->camera.target.x,
            game_context->camera.offset.x / game_context->camera.zoom,
            map_size.x - (screen_width - game_context->camera.offset.x) / game_context->camera.zoom);
            
  game_context->camera.target.y = Clamp(game_context->camera.target.y,
            game_context->camera.offset.y / game_context->camera.zoom,
            map_size.y - (screen_height - game_context->camera.offset.y) / game_context->camera.zoom);

  // 5. Centering Logic (Small Maps)
  // If the map is smaller than the window, lock the camera to the center of the world
  if (map_size.x < screen_width / game_context->camera.zoom) {
    game_context->camera.target.x = map_size.x / 2.0f;
  }
  if (map_size.y < screen_height / game_context->camera.zoom) {
    game_context->camera.target.y = map_size.y / 2.0f;
  }
}
