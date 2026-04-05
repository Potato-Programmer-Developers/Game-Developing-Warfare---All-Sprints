/**
 * @file scene.h
 * @brief Interfaces for high-level rendering, UI overlays, and cutscene management.
 * 
 * Update History:
 * - 2026-03-24: Initial definition of the `Scene` struct for camera and menu management. (Goal: 
 *                Establish a global rendering state.)
 * - 2026-04-03: Added `cutscene` and `fade` parameters. (Goal: Support cinematic transitions 
 *                and structured map-loading sequences.)
 * - 2026-04-05: Expanded for `Dream Sequence` and `Narration` UI. (Goal: Provide a way for 
 *                non-player entities to draw to the screen during cutscenes.)
 * 
 * Revision Details:
 * - Defined `Scene` to include `vignette`, `mainmenu`, and `pause` background textures.
 * - Added `fade_alpha` and `pending_map` for asynchronous map transition coordination.
 * - Implemented `DrawFade` and `UpdateFade` prototypes for project-wide use.
 * - Created `ClearCutscene` and `CloseScene` for memory management.
 * 
 * Authors: Andrew Zhuo and Steven Kenneth Darwy
 */

#ifndef SCENE_H
#define SCENE_H

#include "interaction.h"
#include "interactive.h"
#include "settings.h"
#include "map.h"
#include "character.h"
#include "game_context.h"

/**
 * @brief Holds shared textures and timers for non-world rendering (Menus, Cutscenes).
 */
typedef struct Scene {
    Texture2D mainmenu_background;                  // Visual for the start screen
    Texture2D pause_menu_background;                // Dimmed background for pause
    Texture2D vignette;                             // Horror-theming overlay effect
    Texture2D current_cutscene_frame_texture;       // Single frame currently being shown in a cutscene
    Texture2D current_knob_frame_texture;           // Single frame for the volume slider knob
    int current_cutscene_frame;                     // Index of the active cutscene image
    float cutscene_timer;                           // Progress tracker for cutscene timing
    
    // --- Fading System ---
    float fade_alpha;                               // Current alpha of the fade overlay (0 to 1)
    Color fade_color;                               // Color of the fade overlay
    bool is_fading_out;                             // True if fading to black
    bool is_fading_in;                              // True if fading from black
    char pending_map[128];                          // Map to load when black
    char pending_loc[32];                           // Location string to set when black
} Scene;

/**
 * @brief Loads common menu and overlay textures.
 *
 * @param game_settings Pointer to settings for resolution-specific loading.
 * @return An initialized Scene structure.
 */
Scene InitScene(Settings* game_settings);

/**
 * @brief Top-level drawing dispatcher. 
 * Calls appropriate sub-draw functions based on GaneState.
 *
 * @param game_scene Pointer to the scene.
 * @param game_settings Pointer to the settings.
 * @param game_interactive Pointer to the interactive.
 * @param game_map Pointer to the map.
 * @param player Pointer to the player.
 * @param game_dialogue Pointer to the dialogue.
 * @param game_context Pointer to the game context.
 * @param game_state Pointer to the game state.
 * @param worldNPCs Array of NPCs.
 * @param worldItems Array of items.
 */
void DrawGame(
    Scene *game_scene, Settings *game_settings, 
    Interactive *game_interactive, Map *game_map, Character *player,
    Dialogue *game_dialogue, GameContext *game_context,
    GameState *game_state, NPC worldNPCs[], Item worldItems[]
);

/**
 * @brief Loads a specific image from disk into the current_cutscene_frame_texture.
 * This is used for high-frame-count cutscenes to save memory.
 *
 * @param scene Pointer to the scene.
 * @param frame_index Index of the frame to load.
 * @param game_settings Pointer to the settings.
 */
void LoadCutsceneFrame(Scene *scene, int frame_index, Settings *game_settings);

/**
 * @brief Loads a specific image from disk into the current_menu_frame_texture.
 *
 * @param scene Pointer to the scene.
 * @param frame_index Index of the frame to load.
 * @param is_save_available Whether a save is available.
 */
void LoadMenuFrame(Scene *scene, int frame_index, bool is_save_available);

/**
 * @brief Loads a specific image from disk into the current_settings_frame_texture.
 *
 * @param scene Pointer to the scene.
 * @param frame_index Index of the frame to load.
 */
void LoadSettingsFrame(Scene *scene, int frame_index);

/**
 * @brief Loads a specific image from disk into the current_knob_frame_texture.
 *
 * @param scene Pointer to the scene.
 * @param frame_index Index of the frame to load.
 */
void LoadKnobFrame(Scene *scene, int frame_index);

/**
 * @brief Draws the main menu UI and background.
 *
 * @param scene Pointer to the scene.
 * @param game_interactive Pointer to the interactive.
 */
void DrawMainMenu(Scene* scene, Interactive* game_interactive);

/**
 * @brief Draws the pause menu overlay.
 *
 * @param scene Pointer to the scene.
 * @param game_settings Pointer to the settings.
 * @param game_interactive Pointer to the interactive.
 */
void DrawPauseMenu(Scene* scene, Settings* game_settings, Interactive* game_interactive);

/**
 * @brief Draws the settings configuration interface.
 *
 * @param scene Pointer to the scene.
 * @param game_settings Pointer to the settings.
 * @param game_interactive Pointer to the interactive.
 */
void DrawSettings(Scene* scene, Settings* game_settings, Interactive* game_interactive);

/**
 * @brief Core world rendering function (Map, Player, NPCs, Items, UI).
 *
 * @param scene Pointer to the scene.
 * @param game_settings Pointer to the settings.
 * @param game_interactive Pointer to the interactive.
 * @param game_map Pointer to the map.
 * @param player Pointer to the player.
 * @param worldNPCs Array of NPCs.
 * @param worldItems Array of items.
 * @param game_context Pointer to the game context.
 */
void DrawGameplay(
    Scene* scene, Settings* game_settings, Interactive* game_interactive,
    Map* game_map, Character* player, NPC worldNPCs[], Item worldItems[], 
    GameContext* game_context
);

/**
 * @brief Starts a screen fade effect with a specific color and optional target.
 * 
 * @param scene Pointer to the scene.
 * @param color Color of the fade overlay.
 * @param map Map to load when black.
 * @param loc Location to set when black.
 */
void StartFadeTransition(Scene* scene, Color color, const char* map, const char* loc);

/**
 * @brief Updates the fade animation state.
 * 
 * @param scene Pointer to the scene.
 * @param delta Delta time.
 * @param state Game state.
 */
void UpdateFade(Scene* scene, float delta, GameState state);

/**
 * @brief Frees textures used during cutscenes.
 * 
 * @param scene Pointer to the scene.
 */
void ClearCutscene(Scene* scene);

/**
 * @brief Unloads all scene-managed textures and clean up.
 * 
 * @param scene Pointer to the scene.
 */
void CloseScene(Scene* scene);

#endif