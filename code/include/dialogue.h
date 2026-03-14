#ifndef DIALOGUE_H
#define DIALOGUE_H

#define MAX_DIALOGUE_LINES 100
#define MAX_LINE_LENGTH 256

typedef struct {
    char lines[MAX_DIALOGUE_LINES][MAX_LINE_LENGTH];
    int line_count;
    int current_line;
} Dialogue;

Dialogue LoadDialogue(const char* filename);
void UnloadDialogue(Dialogue* dialogue);
void NextDialogueLine(Dialogue* dialogue);

#endif