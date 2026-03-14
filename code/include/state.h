/*
Globally sets the game state. This is used to determine which scene to draw and which logic to run in the main game loop.
*/
#pragma once

typedef enum {
    /* This enum contains the states of the game. */
    MAINMENU,       // Main menu is open.
    GAMEPLAY,      // Game is running.
    PAUSE,         // Game is paused.
    SETTINGS,      // Settings menu is open.
    GAMEOVER       // Game is over.
} GameState;