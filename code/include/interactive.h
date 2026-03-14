/*
This file contains function prototypes for the interactive elements in
the game.

Module made by Andrew Zhuo.
*/

#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include "settings.h"

typedef struct Interactive{
    /* This struct contains the information for interactive elements in the game. */
    
    // Textures for the interactive elements
    Texture2D play_button;       // Play button texture.
    Texture2D settings_button;   // Settings button texture.
    Texture2D quit_button;       // Quit button texture.

    // Button bounds
    Rectangle play_bounds;       // Play button bounds.
    Rectangle settings_bounds;   // Settings button bounds.
    Rectangle quit_bounds;       // Quit button bounds.

    // Interaction flags
    bool is_play_clicked;        // Whether the play button is clicked.
    bool is_settings_clicked;    // Whether the settings button is clicked.
    bool is_quit_clicked;        // Whether the quit button is clicked.

    bool is_play_hovered;        // Whether the play button is hovered.
    bool is_settings_hovered;    // Whether the settings button is hovered.
    bool is_quit_hovered;        // Whether the quit button is hovered.

    // Slider bounds, size, and state
    Rectangle volume_slider_bar;   // Volume slider bar bounds.
    Rectangle volume_slider_knob;  // Volume slider knob bounds.
    float bar_width;               // Volume slider bar width.
    float bar_height;              // Volume slider bar height.
    float knob_width;              // Volume slider knob width.
    float knob_height;             // Volume slider knob height.
    bool is_volume_moving;         // Whether the volume slider is moving.
} Interactive;

Interactive InitInteractive(Settings* game_settings);                             // Initializes the interactive elements.
void UpdateInteractive(Interactive* interactive, Settings* game_settings);        // Updates the interactive elements.
void CloseInteractive(Interactive* interactive);                                  // Closes the interactive elements.

#endif