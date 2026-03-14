/*
This module stores the implementation of the functions contained in the
audio.h header file.

Module made by Andrew Zhuo.
*/

#include "audio.h"

Audio InitAudio(Settings* game_settings){
    /* Initialize audio device and loading game audio. */

    // Initialize audio device
    Audio new_audio = {0};
    InitAudioDevice();

    // Set master volume
    SetMasterVolume(game_settings->game_volume);

    // Load game audio
    new_audio.bg_music = LoadMusicStream("../assets/audios/bg_music.ogg");
    new_audio.scream_sound = LoadSound("../assets/audios/ghost_scream.wav");

    // Play background music
    PlayMusicStream(new_audio.bg_music);

    return new_audio;
}

void UpdateAudio(Audio* audio){
    /* Keep updating the background music buffer. */
    UpdateMusicStream(audio->bg_music);
}

void CloseAudio(Audio* audio){
    /* Unload audio and close audio device. */
    UnloadMusicStream(audio->bg_music);
    UnloadSound(audio->scream_sound);
    CloseAudioDevice();
}

void PlayScream(Audio* audio){
    /* Play scream sound. */
    PlaySound(audio->scream_sound);
}