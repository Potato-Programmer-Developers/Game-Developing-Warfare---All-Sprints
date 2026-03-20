/*
This file contains function prototypes for the scene module.

Module made by Andrew Zhuo and Steven Kenneth Darwy.
*/

#ifndef SCENE_H
#define SCENE_H

#include "interaction.h"
#include "interactive.h"
#include "settings.h"
#include "map.h"
#include "character.h"
#include "game_context.h"

typedef struct Scene{
    /* This struct contains the information for the scene in the game. */
    Texture2D mainmenu_background;                  // The background texture for the main menu.
    Texture2D pause_menu_background;                // The background texture for the pause menu.
    Texture2D settings_background;                  // The background texture for the settings menu.
    Texture2D vignette;                             // The vignette texture for the game scene.
    Texture2D current_cutscene_frame_texture;       // Currently loaded cutscene frame (on-the-fly).
    float cutscene_timer;                           // The timer for the photo cutscene.
    int current_cutscene_frame;                     // The current frame for the photo cutscene.
} Scene;

Scene InitScene(Settings* game_settings);           // Initializes the game scene.

// Draws the game scene.
void DrawGame(
    Scene *game_scene, Settings *game_settings, 
    Interactive *game_interactive, Map *game_map, Character *player,
    Dialogue *game_dialogue, GameContext *game_context,
    GameState *game_state, NPC worldNPCs[], Item worldItems[]
);

void LoadCutsceneFrame(Scene *scene, int frame_index, Settings *game_settings);                 // Loads a specific cutscene frame on-the-fly.
void DrawMainMenu(Scene* scene, Interactive* game_interactive);                                 // Draws the main menu background.
void DrawPauseMenu(Scene* scene, Settings* game_settings, Interactive* game_interactive);       // Draws the pause menu background.
void DrawSettings(Scene* scene, Settings* game_settings, Interactive* game_interactive);        // Draws the settings menu background.

// Draws the gameplay scene.
void DrawGameplay(
    Scene* scene, Settings* game_settings, Interactive* game_interactive,
    Map* game_map, Character* player, NPC worldNPCs[], Item worldItems[], 
    GameContext* game_context
);

void ClearCutscene(Scene* scene);             // Unloads the cutscene textures.
void CloseScene(Scene* scene);                // Closes the game scene and unloads textures.

#endif