/*
This file contains function prototypes for the dialogue module.

Module made by Cornelius Jabez Lim.
*/

#ifndef DIALOGUE_H
#define DIALOGUE_H

#define MAX_DIALOGUE_LINES 100
#define MAX_LINE_LENGTH 256

typedef struct {
    /* Struct for dialogue */
    char lines[MAX_DIALOGUE_LINES][MAX_LINE_LENGTH];          // Array of dialogue lines
    int line_count;                                           // Number of dialogue lines
    int current_line;                                         // Current dialogue line
} Dialogue;

Dialogue LoadDialogue(const char* filename);                  // Load dialogue from a file

#endif