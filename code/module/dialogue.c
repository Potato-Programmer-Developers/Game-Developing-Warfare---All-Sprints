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
        // remove newline
        dialogue.lines[dialogue.line_count][strcspn(dialogue.lines[dialogue.line_count], "\n")] = 0;
        dialogue.line_count++;

        if (dialogue.line_count >= MAX_DIALOGUE_LINES)
            break;
    }

    fclose(file);

    return dialogue;
}

void NextDialogueLine(Dialogue* dialogue)
{
    if (dialogue->current_line < dialogue->line_count - 1)
    {
        dialogue->current_line++;
    }
}

void UnloadDialogue(Dialogue* dialogue) {
    dialogue->line_count = 0;
    dialogue->current_line = 0;
}