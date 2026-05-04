/**
 * @file settings.h
 * @brief Global game configuration and difficulty parameters.
 * 
 * Provides a centralized place to manage window dimensions, gameplay
 * balancer (speed, stamina), and other user-configurable settings.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "raylib.h"

/**
 * @brief Stores configuration values for the engine and gameplay logic.
 */
typedef struct Settings {
    int window_width;                    // Initial width of the Raylib window
    int window_height;                   // Initial height of the Raylib window
    int fps;                             // Target frames per second
    float game_volume;                   // Global audio volume multiplier (0.0 - 1.0)
    float mc_speed;                      // Base movement speed for the main character
    float max_stamina;                   // Total stamina capacity
    float stamina_depletion_rate;        // Amount of stamina lost per second of running
    float stamina_recovery_rate;         // Amount of stamina gained per second of idling
} Settings;

/**
 * @brief Loads default settings (or potentially from a config file).
 *
 * @return A Settings structure populated with default values.
 */
Settings InitSettings();

#endif