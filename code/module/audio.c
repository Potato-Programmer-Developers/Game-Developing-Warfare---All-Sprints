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
 * - 2026-04-10: Loaded Day 2 horror ambiance sound effects. (Goal: Provide three new audio
 *                resources — door banging, window scraping, and chimney rustling — that are
 *                dynamically triggered by `[PLAY]` tags during SET4-PHASE2 narration sequences.
 *                These sounds create an escalating atmosphere of dread during the nightly interior
 *                scene, where the player's earlier choices determine which sounds are heard.)
 * 
 * Revision Details:
 * - Refactored `PlayStep` to switch sound profiles based on the `Location` context.
 * - Added `step_outdoor`, `step_indoor`, and `notif_sound` to the global `Audio` state.
 * - Implemented a volume scaling hook tied to the global `Settings` struct.
 * - Created `StopAllAudio` for clean state transitions between maps.
 * - Added `LoadSound` calls in `LoadAudio` for `door_banging.mp3`, `window_scraping.mp3`, and
 *    `chimney_rustling.mp3` from the `assets/audios/` directory.
 * - Added corresponding `UnloadSound` calls in `UnloadAudio` for the three new sound handles to
 *    prevent memory leaks during shutdown.
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
    new_audio.door_banging = LoadSound("../assets/audios/door_banging.mp3");
    new_audio.window_scraping = LoadSound("../assets/audios/window_scraping.mp3");
    new_audio.chimney_rustling = LoadSound("../assets/audios/chimney_rustling.mp3");
    new_audio.typing_sound = LoadSound("../assets/audios/typing_sound.mp3");

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
    UnloadSound(audio->door_banging);
    UnloadSound(audio->window_scraping);
    UnloadSound(audio->chimney_rustling);
    UnloadSound(audio->typing_sound);
    
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