/**
 * @file game_context.h
 * @brief Master context for managing global game systems and state.
 * 
 * Provides a central registry for the player, map, camera, and peripheral 
 * systems like the phone. Essential for cross-module accessibility.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef GAME_CONTEXT_H
#define GAME_CONTEXT_H

#include "map.h"
#include "character.h"
#include "settings.h"
#include "phone.h"

/**
 * @brief Global singleton-like container for the game instance.
 * 
 * Holds references to persistent world objects and manages UI-layer 
 * components like the phone.
 */
typedef struct GameContext {
    Map* map;                  // Reference to the active Tiled world
    Character* player;         // Reference to the player character state
    Camera2D camera;           // 2D camera viewport config
    bool is_outdoor;           // Environment flag for hallucination effects
    Phone phone;               // Mobile phone notification sub-system
    int previous_state;        // State cache for returning from Pause/Settings
    int settings_previous_state; // State cache explicitly for returning from Settings
} GameContext;

/** @brief Allocates and connects core systems into the context. */
GameContext InitGameContext(Map* map, Character* player, Settings* settings);

/** @brief Updates global systems (Phone, Camera) every frame. */
void UpdateGameContext(GameContext* context, Settings* settings, Vector2 map_size);

#endif