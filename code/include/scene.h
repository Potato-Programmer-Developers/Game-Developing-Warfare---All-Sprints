/*
This file contains function prototypes for the scene module.

Module made by Andrew Zhuo and Steven Kenneth Darwy.
*/

#ifndef SCENE_H
#define SCENE_H

#include "interactive.h"
#include "settings.h"

typedef struct Scene{
    /* This struct contains the information for the scene in the game. */
    Texture2D mainmenu_background;       // The background texture for the main menu.
    Texture2D game_background;           // The background texture for the game scene.
    Texture2D pause_menu_background;     // The background texture for the pause menu.
    Texture2D settings_background;       // The background texture for the settings menu.
    Texture2D vignette;                  // The vignette texture for the game scene.
} Scene;

Scene InitScene(Settings* game_settings);                                                       // Initializes the game scene.
Texture2D LoadBackground(const char* path, Settings* game_settings);                            // Loads the background texture.
void DrawMainMenu(Scene* scene, Interactive* interactive, Settings* game_settings);                                                                // Draws the main menu background.
void DrawPauseMenu(Scene* scene, Settings* game_settings, Interactive* game_interactive);       // Draws the pause menu background.
void DrawSettings(Scene* scene, Settings* game_settings, Interactive* game_interactive);        // Draws the settings menu background.
void CloseScene(Scene* scene);                                                                  // Closes the game scene and unloads textures.

#endif