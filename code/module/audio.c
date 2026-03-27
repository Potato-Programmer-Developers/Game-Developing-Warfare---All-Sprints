/**
 * @file audio.c
 * @brief Implementation of the audio management system.
 * 
 * Handles Raylib audio device lifecycle and specific sound playback logic.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef AUDIO_H
#include "audio.h"
#endif

/**
 * @brief Sets up the audio device and loads initial music/sound assets.
 * 
 * @param game_settings Configuration for master volume.
 * @return Fully initialized Audio structure.
 */
Audio InitAudio(Settings* game_settings){
    Audio new_audio = {0};
    
    // 1. Initialize the physical/logical audio device
    InitAudioDevice();

    // 2. Set the global volume based on user settings
    SetMasterVolume(game_settings->game_volume);

    // 3. Load music streams (streamed from disk to save memory)
    new_audio.bg_music = LoadMusicStream("../assets/audios/bg_music.ogg");
    new_audio.cutscene_music = LoadMusicStream("../assets/audios/cutscene.mp3");

    // 4. Load static sounds (loaded fully into RAM for low latency)
    new_audio.scream_sound = LoadSound("../assets/audios/ghost_scream.wav");
    new_audio.step_outdoor = LoadSound("../assets/audios/step_outdoor.mp3");
    new_audio.step_indoor = LoadSound("../assets/audios/step_indoor.mp3");
    new_audio.notif_sound = LoadSound("../assets/audios/notif.wav");

    // 5. Start the background music loop immediately
    PlayMusicStream(new_audio.bg_music);

    return new_audio;
}

/**
 * @brief Keeps music buffers filled. Must be called every frame.
 * @param audio Pointer to the audio system.
 */
void UpdateAudio(Audio* audio){
    UpdateMusicStream(audio->bg_music);
    UpdateMusicStream(audio->cutscene_music);
}

/**
 * @brief Cleans up all audio resources and closes the device.
 * @param audio Pointer to the audio system.
 */
void CloseAudio(Audio* audio){
    // Unload all loaded resources from memory
    UnloadMusicStream(audio->bg_music);
    UnloadMusicStream(audio->cutscene_music);
    UnloadSound(audio->scream_sound);
    UnloadSound(audio->step_outdoor);
    UnloadSound(audio->step_indoor);
    UnloadSound(audio->notif_sound);
    
    // Shutdown the audio driver
    CloseAudioDevice();
}

/**
 * @brief One-shot playback of the scream sound effect.
 * @param audio Pointer to the audio system.
 */
void PlayScream(Audio* audio){
    // Prevent overlapping instances of the same scream
    if (!IsSoundPlaying(audio->scream_sound)){
        PlaySound(audio->scream_sound);
    }
}

/**
 * @brief One-shot playback of the phone notification chime.
 * @param audio Pointer to the audio system.
 */
void PlayNotif(Audio* audio){
    if (!IsSoundPlaying(audio->notif_sound)){
        PlaySound(audio->notif_sound);
    }
}

/**
 * @brief Context-aware footstep sound playback.
 * 
 * @param audio Pointer to the audio system.
 * @param location Current character location for footstep sound effect.
 */
void PlayStep(Audio* audio, Location location){
    if (!IsSoundPlaying(audio->step_outdoor) && location == EXTERIOR){
        PlaySound(audio->step_outdoor);
    } else if (!IsSoundPlaying(audio->step_indoor) && location != EXTERIOR){
        PlaySound(audio->step_indoor);
    }
}