/**
 * @file audio.h
 * @brief Function prototypes and structures for the audio management system.
 * 
 * This module handles the loading, playing, and updating of music and sound effects
 * used throughout the game, including background music, cutscene audio, and spatial sounds.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"
#include "settings.h"
#include "map.h"

/**
 * @brief Container for all audio resources used in the game.
 */
typedef struct Audio {
    Music bg_music;             // Main background music stream
    Sound step_outdoor;         // Walking sound on outdoor surfaces
    Sound step_indoor;          // Walking sound on indoor surfaces
    Sound notif_sound;          // Phone notification alert sound
} Audio;

/**
 * @brief Initializes the audio device and loads all sound assets.
 *
 * @param game_settings Pointer to settings containing volume and paths.
 * @return An initialized Audio structure with loaded assets.
 */
Audio InitAudio(Settings* game_settings);

/**
 * @brief Updates music stream buffers to keep them playing smoothly.
 *
 * @param audio Pointer to the audio container.
 */
void UpdateAudio(Audio* audio);

/**
 * @brief Unloads all audio assets and closes the audio device.
 *
 * @param audio Pointer to the audio container to clean up.
 */
void CloseAudio(Audio* audio);

/**
 * @brief Plays a footstep sound based on the player's environment.
 *
 * @param audio Pointer to the audio container.
 * @param location Current character location.
 */
void PlayStep(Audio* audio, Location location);

/**
 * @brief Plays the notification sound effect.
 *
 * @param audio Pointer to the audio container.
 */
void PlayNotif(Audio* audio);

#endif
