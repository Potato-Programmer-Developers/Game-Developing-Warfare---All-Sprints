/**
 * @file dialogue.c
 * @brief Implementation of the dialogue loading and parsing system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "dialogue.h"

// Global registry for 'once-only' responses to persist across phase reloads if needed
typedef struct {
    char filename[128];
    int index;
} UsedRegistry;

static UsedRegistry used_responses[256];
static int used_count = 0;

static bool IsResponseUsed(const char* filename, int index) {
    for (int i = 0; i < used_count; i++) {
        if (strcmp(used_responses[i].filename, filename) == 0 && used_responses[i].index == index) {
            return true;
        }
    }
    return false;
}

static void MarkResponseUsed(const char* filename, int index) {
    if (used_count < 256) {
        strncpy(used_responses[used_count].filename, filename, 127);
        used_responses[used_count].index = index;
        used_count++;
    }
}

Dialogue LoadDialogue(const char* filename)
{
    Dialogue dialogue = {0};

    FILE* file = fopen(filename, "r");

    dialogue.selected_choice = -1;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, file)){
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;

        if (strstr(line, "[CHOICE]")){
            if (dialogue.choice_count < 4){
                const char* choiceText = strstr(line, "[CHOICE]") + 8;
                while(*choiceText == ' ') choiceText++;
                strcpy(dialogue.choices[dialogue.choice_count], choiceText);
                dialogue.choice_count++;
            }
        } else{
            strcpy(dialogue.lines[dialogue.line_count], line);
            dialogue.line_count++;
        }

        if (dialogue.line_count >= MAX_DIALOGUE_LINES)
            break;
    }

    fclose(file);
    dialogue.current_line = 0;
    return dialogue;
}

ResponseGroup LoadResponseGroup(const char* filename){
    ResponseGroup group = {0};
    FILE* file = fopen(filename, "r");

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) && group.count < 16){
        line[strcspn(line, "\r\n")] = 0;
        if (strstr(line, "[RESPONSE]")) {
            char* text_start = strstr(line, "[RESPONSE]") + 10;
            while (*text_start == ' ') text_start++;

            char* once_tag = strstr(text_start, "| 1");
            if (once_tag) {
                group.responses[group.count].once = true;
                *once_tag = '\0';
                char* end = once_tag - 1;
                while (end >= text_start && *end == ' '){*end = '\0'; end--;}
            }

            strncpy(group.responses[group.count].text, text_start, MAX_LINE_LENGTH - 1);
            if (group.responses[group.count].once){
                group.responses[group.count].used = IsResponseUsed(filename, group.count);
            }
            group.count++;
        }
    }
    fclose(file);
    return group;
}

const char* PickResponse(ResponseGroup* group, const char* filename){
    if (group->count == 0) return "No response.";

    int available[16];
    int avail_count = 0;
    for (int i = 0; i < group->count; i++){
        if (!group->responses[i].once || !IsResponseUsed(filename, i)){
            available[avail_count++] = i;
        }
    }

    if (avail_count == 0) return group->responses[group->count-1].text;

    static bool seeded = false;
    if (!seeded) {srand((unsigned int)time(NULL)); seeded = true;}
    
    int choice_idx = available[rand() % avail_count];
    if (group->responses[choice_idx].once) MarkResponseUsed(filename, choice_idx);
    
    return group->responses[choice_idx].text;
}

void LoadInteraction(const char* filename, Dialogue* dialogue){
    if (!dialogue) return;
    memset(dialogue, 0, sizeof(Dialogue));

    FILE* file = fopen(filename, "r");

    // Pools for choices and responses - static to avoid stack issues
    static char choice_pool[4][10][MAX_LINE_LENGTH];
    static char global_pool[MAX_DIALOGUE_LINES][MAX_LINE_LENGTH];
    memset(choice_pool, 0, sizeof(choice_pool));
    memset(global_pool, 0, sizeof(global_pool));

    // Counts for each choice pool and global pool
    int choice_pool_counts[4] = {0};
    int global_count = 0;
    
    // Index of the current choice being processed
    int current_choice_idx = -1;
    char raw_line[MAX_LINE_LENGTH];

    while (fgets(raw_line, sizeof(raw_line), file)) {
        char* line = raw_line;
        while (*line == ' ' || *line == '\t') line++;
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;

        if (strstr(line, "[CHOICE")){
            int idx = -1;
            if (sscanf(line, "[CHOICE%d]", &idx) == 1 || sscanf(line, "[CHOICE %d]", &idx) == 1) {
                if (idx >= 1 && idx <= 4){
                    current_choice_idx = idx - 1;
                    char* end_tag = strchr(line, ']');
                    if (end_tag){
                        char* label = end_tag + 1;
                        while (*label == ' ') label++;
                        strncpy(dialogue->choices[current_choice_idx], label, 63);
                        dialogue->choices[current_choice_idx][63] = '\0';
                    }
                    if (current_choice_idx >= dialogue->choice_count) dialogue->choice_count = current_choice_idx + 1;
                }
            }
        } else if (strstr(line, "[RESPONSE]")){
            char* text = strstr(line, "[RESPONSE]") + 10;
            while (*text == ' ') text++;
            if (current_choice_idx != -1){
                if (choice_pool_counts[current_choice_idx] < 10){
                    strncpy(choice_pool[current_choice_idx][choice_pool_counts[current_choice_idx]], text, MAX_LINE_LENGTH - 1);
                    choice_pool[current_choice_idx][choice_pool_counts[current_choice_idx]][MAX_LINE_LENGTH - 1] = '\0';
                    choice_pool_counts[current_choice_idx]++;
                }
            } else{
                if (global_count < MAX_DIALOGUE_LINES){
                    strncpy(global_pool[global_count], text, MAX_LINE_LENGTH - 1);
                    global_pool[global_count][MAX_LINE_LENGTH - 1] = '\0';
                    global_count++;
                }
            }
        } else if (line[0] != '['){
            if (current_choice_idx == -1 && global_count < MAX_DIALOGUE_LINES){
                strncpy(global_pool[global_count], line, MAX_LINE_LENGTH - 1);
                global_pool[global_count][MAX_LINE_LENGTH - 1] = '\0';
                global_count++;
            }
        }
    }
    fclose(file);

    static bool seeded = false;
    if (!seeded){srand((unsigned int)time(NULL)); seeded = true;}

    if (dialogue->choice_count > 0){
        for (int i = 0; i < global_count && i < MAX_DIALOGUE_LINES; i++){
            strncpy(dialogue->lines[dialogue->line_count], global_pool[i], MAX_LINE_LENGTH - 1);
            dialogue->lines[dialogue->line_count][MAX_LINE_LENGTH - 1] = '\0';
            dialogue->line_count++;
        }
        for (int i = 0; i < dialogue->choice_count; i++){
            if (choice_pool_counts[i] > 0){
                strncpy(dialogue->choice_responses[i], choice_pool[i][rand() % choice_pool_counts[i]], MAX_LINE_LENGTH - 1);
                dialogue->choice_responses[i][MAX_LINE_LENGTH - 1] = '\0';
            } else strcpy(dialogue->choice_responses[i], "...");
        }
    } else if (global_count > 0){
        int pick = rand() % global_count;
        strncpy(dialogue->lines[0], global_pool[pick], MAX_LINE_LENGTH - 1);
        dialogue->lines[0][MAX_LINE_LENGTH - 1] = '\0';
        dialogue->line_count = 1;
    }
}
