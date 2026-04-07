/**
 * @file interactive.h
 * @brief UI elements and interactive menu system.
 * 
 * Update History:
 * - 2026-03-24: Foundation of the `Interactive` container for UI state polling. (Goal: 
 *                Separate UI logic from the main rendering loop.)
 * - 2026-04-05: Improved hover detection for multi-resolution support. (Goal: Ensure 
 *                button hitboxes scale correctly with the window size.)
 * - 2026-04-06: System-wide "Texture-to-Rect" refactor. (Goal: Modernize the UI 
 *                rendering pipeline by removing static button textures and using 
 *                centralized background animations with bounding-box logic.)
 * 
 * Revision Details:
 * - Removed `new_game_button`, `continue_button`, and other legacy `Texture2D` members 
 *    from the `Interactive` struct to save VRAM and reduce initialization time.
 * - Removed `is_main_menu_clicked` and its bounds, simplifying the navigation state machine.
 * - Optimized coordinate mapping for THE Main, Pause, and Settings menus into hardcoded, 
 *    high-reliability rectangular regions.
 * - Deleted the `CloseInteractive` prototype as texture management is now handled 
 *    globally by the `Scene` module.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include "raylib.h"
#include "settings.h"
#include "state.h"

/**
 * @brief Container for all UI elements and their interaction states.
 */
typedef struct Interactive {
    // --- Button Layout ---
    Rectangle new_game_bounds;         // Hitbox for 'New Game' button
    Rectangle continue_bounds;         // Hitbox for 'Continue' button
    Rectangle settings_bounds;         // Hitbox for 'Settings' button
    Rectangle quit_bounds;             // Hitbox for 'Quit' button
    Rectangle settings_back_bounds;    // Hitbox for 'Back' button in settings

    // --- Interaction State ---
    bool is_new_game_clicked;          // True if button was clicked this frame
    bool is_continue_clicked;          // True if button was clicked this frame
    bool is_settings_clicked;          // True if button was clicked this frame
    bool is_quit_clicked;              // True if button was clicked this frame
    bool is_settings_back_clicked;     // True if back was clicked

    bool is_new_game_hovered;          // True if mouse is over button
    bool is_continue_hovered;          // True if mouse is over button
    bool is_settings_hovered;          // True if mouse is over button
    bool is_quit_hovered;              // True if mouse is over button
    bool is_settings_back_hovered;     // True if back is hovered

    // --- Volume Slider ---
    Rectangle volume_slider_bar;       // Visual bar of the slider
    Rectangle volume_slider_knob;      // Draggable handle of the slider
    float bar_width;                   // Width of the slider bar
    float bar_height;                  // Height of the slider bar
    float knob_width;                  // Width of the slider knob
    float knob_height;                 // Height of the slider knob
    bool is_volume_moving;             // True if user is currently dragging the knob
} Interactive;

/**
 * @brief Loads UI textures and initializes default button positions.
 *
 * @param game_settings Pointer to settings for initial layout.
 * @return An initialized Interactive structure.
 */
Interactive InitInteractive(Settings* game_settings);

/**
 * @brief Updates hover and click states based on mouse position/input.
 *
 * @param interactive Pointer to the UI container.
 * @param game_settings Pointer to settings.
 */
void UpdateInteractive(Interactive* interactive, Settings* game_settings);

/**
 * @brief Adjusts UI layout based on current game state (e.g., resizing window).
 *
 * @param interactive Pointer to the UI container.
 * @param game_state Current state to determine which buttons to show.
 * @param game_settings Pointer to settings.
 */
void UpdateInteractiveLayout(Interactive* interactive, int game_state, Settings* game_settings);

#endif