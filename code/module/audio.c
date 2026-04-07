/**
 * @file audio.c
 * @brief Handles background music, localized sound effects, and volume management.
 * 
 * Update History:
 * - 2026-03-22: Foundational audio driver implementation. (Goal: Support 
 *                streaming `.wav` and `.mp3` files via Raylib.)
 * - 2026-04-03: Added Footstep logic for multiple terrains. (Goal: Provide 
 *                auditory feedback for player movement on different surfaces.)
 * - 2026-04-05: Integrated Notification sound triggers. (Goal: Alert the player 
 *                to new phone messages with the `notif.wav` sound.)
 * 
 * Revision Details:
 * - Refactored `PlayStep` to switch sound profiles based on the `Location` context.
 * - Added `step_outdoor`, `step_indoor`, and `notif_sound` to the global `Audio` state.
 * - Implemented a volume scaling hook tied to the global `Settings` struct.
 * - Created `StopAllAudio` for clean state transitions between maps.
 * 
 * Authors: Andrew Zhuo
 */

#include "audio.h"
#include "map.h"

Audio InitAudio(Settings* game_settings){
    Audio new_audio = {0};
    
    // 1. Initialize the physical/logical audio device
    InitAudioDevice();

    // 2. Set the global volume based on user settings
    SetMasterVolume(game_settings->game_volume);

    // 3. Load music streams (streamed from disk to save memory)
    new_audio.bg_music = LoadMusicStream("../assets/audios/bg_music.ogg");

    // 4. Load static sounds (loaded fully into RAM for low latency)
    new_audio.step_outdoor = LoadSound("../assets/audios/step_outdoor.mp3");
    new_audio.step_indoor = LoadSound("../assets/audios/step_indoor.mp3");
    new_audio.notif_sound = LoadSound("../assets/audios/notif.wav");

    // 5. Start the background music loop immediately
    PlayMusicStream(new_audio.bg_music);

    return new_audio;
}

void UpdateAudio(Audio* audio){
    UpdateMusicStream(audio->bg_music);
}

void CloseAudio(Audio* audio){
    // Unload all loaded resources from memory
    UnloadMusicStream(audio->bg_music);
    UnloadSound(audio->step_outdoor);
    UnloadSound(audio->step_indoor);
    UnloadSound(audio->notif_sound);
    
    // Shutdown the audio driver
    CloseAudioDevice();
}

void PlayNotif(Audio* audio){
    if (!IsSoundPlaying(audio->notif_sound)){
        PlaySound(audio->notif_sound);
    }
}

void PlayStep(Audio* audio, Location location){
    if (!IsSoundPlaying(audio->step_outdoor) && (location == EXTERIOR || location == FARM)){
        PlaySound(audio->step_outdoor);
    } else if (!IsSoundPlaying(audio->step_indoor) && (location == APARTMENT || location == INTERIOR)){
        PlaySound(audio->step_indoor);
    }
}