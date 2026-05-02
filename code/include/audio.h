/**
 * @file audio.h
 * @brief Function prototypes and structures for the audio management system.
 * 
 * This module handles the loading, playing, and updating of music and sound effects
 * used throughout the game, including background music, cutscene audio, and spatial sounds.
 * 
 * Update History:
 * - 2026-03-22: Foundational audio struct definition. (Goal: Establish a centralized
 *                container for all audio resources used across the game.)
 * - 2026-04-10: Integrated narrative-driven horror sound effects for Day 2 SET4-PHASE2. (Goal: Support
 *                dynamically triggered ambient horror sounds during the nightly interior narration sequence,
 *                including door banging, window scraping, and chimney rustling effects that are played
 *                inline via `[PLAY]` tags parsed from `narration.txt` files.)
 * - 2026-05-02: Added `Music credit_music` stream to the `Audio` struct. (Goal: Support a dedicated
 *                music track for the end-game credits sequence, separate from the main background
 *                music loop.)
 * 
 * Revision Details:
 * - Added `Sound door_banging` to the `Audio` struct for the heavy door impact effect triggered
 *    during the "locked doors" narration path in SET4-PHASE2.
 * - Added `Sound window_scraping` for the metallic scraping sound effect triggered during the
 *    "windows locked" narration path in SET4-PHASE2.
 * - Added `Sound chimney_rustling` for the low rustling/breathing effect triggered during the
 *    "fireplace on" narration path in SET4-PHASE2.
 * - Added `Music credit_music` field to the `Audio` struct for streaming `credit.mp3` during
 *    the scrolling credits screen.
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
    Sound door_banging;         // Door banging sound
    Sound window_scraping;      // Window scraping sound
    Sound chimney_rustling;     // Chimney rustling sound
    Sound typing_sound;         // Typing text effect sound
    Music credit_music;         // Special credit music
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
