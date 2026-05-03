/**
 * @file state.h
 * @brief Core game state machine and state transition logic.
 * 
 * Defines the various operational modes of the game (Menu, Play, Pause)
 * and the primary update loop dispatcher.
 * 
 * Update History:
 * - 2026-05-02: Added `ENDING_CUTSCENE` to the `GameState` enum. (Goal: Distinguish ending
 *                sequences as a unique blocking state separate from narration or photo cutscenes.)
 * 
 * Revision Details:
 * - Added `ENDING_CUTSCENE` entry to the `GameState` enum for multi-ending support.
 * 
 * Authors: Andrew Zhuo and Steven Kenneth Darwy
 */

#ifndef STATE_H
#define STATE_H

#include "character.h"
#include "game_context.h"
#include "map.h"
#include "settings.h"

struct Interactive;                  // Forward declaration
struct Item;                         // Forward declaration
typedef struct Scene Scene;          // Forward declaration

/**
 * @brief Defines all possible major logical states of the application.
 */
typedef enum {
	MAINMENU,          // Start screen with New Game/Options/Quit
	GAMEPLAY,          // Active player-controlled world exploration
	PAUSE,             // Interrupted gameplay with menu overlay
	SETTINGS,          // Configuration sub-menu
	DIALOGUE_CUTSCENE, // Locked state focused on text interaction
	PHOTO_CUTSCENE,    // Non-interactive animated sequence
	NARRATION_CUTSCENE,// Locked state for interactive narration sequences
	OPENING_CUTSCENE,  // Game opening sequence (black screen + dialogue)
	ENDING_CUTSCENE    // Game ending sequence (black screen + dialogue)
} GameState;

/**
 * @brief The primary update dispatcher for the entire game.
 * 
 * Based on the current GameState, this function calls specific module
 * updates (Character, Phone, Interactive, etc.).
 * 
 * @param game_state Pointer to the game state.
 * @param game_interactive Pointer to the interactive.
 * @param player Pointer to the player.
 * @param game_settings Pointer to the settings.
 * @param game_map Pointer to the map.
 * @param game_context Pointer to the game context.
 * @param game_audio Pointer to the audio.
 * @param map_size Size of the map.
 * @param game_scene Pointer to the scene.
 * 
 * @return 1 if the game should exit (e.g., Quit clicked), 0 to continue.
 */
int UpdateGame(
	GameState *game_state, struct Interactive *game_interactive, Character *player,
	Settings *game_settings, Map *game_map, GameContext *game_context, Audio *game_audio,
	Vector2 map_size, Scene *game_scene
);

#endif