/*
This file contains functions for the dialogue module.

Module made by Cornelius Jabez Lim.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dialogue.h"

Dialogue LoadDialogue(const char* filename)
{
    Dialogue dialogue = {0};

    FILE* file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("Failed to load dialogue file: %s\n", filename);
        return dialogue;
    }

    while (fgets(dialogue.lines[dialogue.line_count], MAX_LINE_LENGTH, file))
    {
        // Remove newline character
        dialogue.lines[dialogue.line_count][strcspn(dialogue.lines[dialogue.line_count], "\n")] = 0;
        dialogue.line_count++;

        // Stop if we reach the maximum number of lines
        if (dialogue.line_count >= MAX_DIALOGUE_LINES)
            break;
    }

    fclose(file);

    // Start at the first line
    dialogue.current_line = 0;

    return dialogue;
}