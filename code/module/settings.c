/*
This module stores the implementation of the variable and functions
contained in the settings.h header file.

Module made by Andrew Zhuo.
*/

#include "settings.h"

Settings InitSettings(){
    /* Initialize the settings. */
    Settings new_settings = {0};
    new_settings.window_width = 1200;
    new_settings.window_height = 800;
    new_settings.fps = 60;
    new_settings.game_volume = 50;
    new_settings.mc_speed = 2.5f;
    
    return new_settings;
}
