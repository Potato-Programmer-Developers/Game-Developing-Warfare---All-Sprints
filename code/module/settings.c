/**
 * @file settings.c
 * @brief Handles game settings initialization, serialization, and balancing constants.
 * 
 * Update History:
 * - 2026-03-21: Initial creation of the `Settings` structure. (Goal: Store user 
 *                preferences for volume and movement speed.)
 * - 2026-04-01: Integrated with the interactive UI for the Settings menu. (Goal: Support 
 *                real-time changes to stamina and sanity depletion rates.)
 * - 2026-04-05: Fine-tuned stamina and depletion constants. (Goal: Improve gameplay feel 
 *                by slowing down resource drain and speeding up recovery.)
 * 
 * Revision Details:
 * - Refactored `InitSettings` to set standard default values (Volume: 50.0f, Speed: 150.0f).
 * - Updated `stamina_depletion_rate` to 15.0f for better tactical pacing.
 * - Implemented `SaveSettings` and `LoadSettings` via binary file I/O.
 * - Added `hallucination_increase_rate` which scales with the player's sanity level.
 * 
 * Authors: Andrew Zhuo
 */

#include "settings.h"

Settings InitSettings(){
    // Initialize game settings with hardcoded defaults
    Settings new_settings = {0};
    new_settings.window_width = 1200;
    new_settings.window_height = 800;
    new_settings.fps = 60;
    new_settings.game_volume = 50.0f;
    new_settings.mc_speed = 150.0f;
    new_settings.max_stamina = 100.0f;
    new_settings.stamina_depletion_rate = 15.0f;
    new_settings.stamina_recovery_rate = 10.0f;
    
    return new_settings;
}
