#include "story.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static char* trim(char* str){
    // Remove leading and trailing whitespace from a string
    char* end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void LoadStoryDay(StorySystem* story, const char* path) {
    FILE* file = fopen(path, "r");

    memset(story, 0, sizeof(StorySystem));
    
    // Extract day folder from path (e.g., "../assets/text/day1/day1.txt" -> "day1")
    const char* lastSlash = strrchr(path, '/');
    const char* secondLastSlash = lastSlash - 1;
    while (secondLastSlash > path && *secondLastSlash != '/') secondLastSlash--;
    if (*secondLastSlash == '/'){
        int len = lastSlash - secondLastSlash - 1;
        if (len > 31) len = 31;
        strncpy(story->day_folder, secondLastSlash + 1, len);
        story->day_folder[len] = '\0';
    }

    char raw_line[256];
    int current_set = -1;
    int current_phase = -1;
    int line_num = 0;

    while (fgets(raw_line, sizeof(raw_line), file)){
        line_num++;
        char* line = trim(raw_line);
        if (strlen(line) == 0) continue;

        // Detect Set/Phase Headers
        if (strstr(line, "SET") && strstr(line, "PHASE")){
            int set_num = 0;
            if (sscanf(line, "SET%d", &set_num) == 1){
                if (set_num - 1 > current_set){
                    current_set = set_num - 1;
                    current_phase = 0;
                } else{
                    current_phase++;
                }
                
                if (current_set >= 0 && current_set < MAX_SETS_PER_DAY && 
                    current_phase >= 0 && current_phase < MAX_PHASES_PER_SET){
                    
                    StoryPhase* phase_ptr = &story->sets[current_set].phases[current_phase];
                    strncpy(phase_ptr->name, line, 63);
                    story->set_count = (current_set + 1 > story->set_count) ? current_set + 1 : story->set_count;
                    story->sets[current_set].phase_count = current_phase + 1;
                }
            }
            continue;
        }

        if (current_set < 0 || current_phase < 0 || 
            current_set >= MAX_SETS_PER_DAY || current_phase >= MAX_PHASES_PER_SET){
            continue;
        }

        StoryPhase* phase = &story->sets[current_set].phases[current_phase];

        // Known Tags
        if (strstr(line, "[LOCATION]")){
            if (strstr(line, "INTERIOR")) phase->location = STORY_LOC_INTERIOR;
            else if (strstr(line, "EXTERIOR")) phase->location = STORY_LOC_EXTERIOR;
            else phase->location = STORY_LOC_NONE;
        } else if (strstr(line, "[INTERACTABLE_TYPE_ITEM]")){
            if (phase->interactable_count < 20){
                strcpy(phase->interactables[phase->interactable_count].type, "ITEM");
                const char* id_start = strstr(line, "] ");
                if (id_start) strncpy(phase->interactables[phase->interactable_count].id, id_start + 2, 63);
                phase->interactable_count++;
            }
        } else if (strstr(line, "[INTERACTABLE_TYPE_NPC]")){
            if (phase->interactable_count < 20){
                strcpy(phase->interactables[phase->interactable_count].type, "NPC");
                const char* id_start = strstr(line, "] ");
                if (id_start) strncpy(phase->interactables[phase->interactable_count].id, id_start + 2, 63);
                phase->interactable_count++;
            }
        } else if (strstr(line, "[CONDITION]")){
            char type_str[32];
            if (sscanf(line, "[CONDITION] %s", type_str) == 1){
                if (strcmp(type_str, "ALL_QUESTS_COMPLETE") == 0) phase->end_condition.type = CONDITION_ALL_QUESTS_COMPLETE;
                else if (strcmp(type_str, "INTERACT_OBJECT") == 0){
                    phase->end_condition.type = CONDITION_INTERACT_OBJECT;
                    sscanf(line, "[CONDITION] INTERACT_OBJECT %s %f", phase->end_condition.target_id, &phase->end_condition.target_value);
                } else if (strcmp(type_str, "TIME_PASS") == 0){
                    phase->end_condition.type = CONDITION_TIME_PASS;
                    sscanf(line, "[CONDITION] TIME_PASS %f", &phase->end_condition.target_value);
                }
            }
        } else if (strstr(line, "[END]")){
            // Ignore [END] tag for now
        } else if (line[0] != '[' || strstr(line, "[QUEST]")){
            // Treat as a quest description
            if (phase->quest_count < MAX_QUESTS_PER_PHASE){
                const char* desc = strstr(line, "[QUEST] ") ? (line + 8) : line;
                strncpy(phase->quests[phase->quest_count].description, desc, 63);
                phase->quests[phase->quest_count].completed = false;
                phase->quest_count++;
            }
        }
    }

    fclose(file);
    story->current_set_idx = 0;
    story->current_phase_idx = 0;
    story->phase_timer = 0;
}

void UpdateStory(StorySystem* story, float delta) {
    StoryPhase* active = GetActivePhase(story);
    if (!active) return;

    switch (active->end_condition.type) {
        case CONDITION_ALL_QUESTS_COMPLETE: {
            bool all_done = true;
            for (int i = 0; i < active->quest_count; i++) {
                if (!active->quests[i].completed) {
                    all_done = false;
                    break;
                }
            }
            // If all quests are done (or there are no quests), we can advance
            if (all_done) AdvanceStory(story);
        } break;

        case CONDITION_TIME_PASS:
            story->phase_timer += delta;
            if (story->phase_timer >= active->end_condition.target_value) {
                AdvanceStory(story);
            }
            break;

        default:
            break;
    }
}

void AdvanceStory(StorySystem* story) {
    if (story->current_set_idx < 0 || story->current_set_idx >= MAX_SETS_PER_DAY) return;
    
    StoryPhase* old = GetActivePhase(story);
    story->current_phase_idx++;
    story->phase_timer = 0;

    if (story->current_phase_idx >= story->sets[story->current_set_idx].phase_count){
        story->current_phase_idx = 0;
        story->current_set_idx++;
        
        if (story->current_set_idx >= story->set_count){
            story->current_set_idx = story->set_count - 1;
            story->current_phase_idx = story->sets[story->current_set_idx].phase_count - 1;
        }
    }
    StoryPhase* next = GetActivePhase(story);
}

StoryPhase* GetActivePhase(StorySystem* story) {
    if (story->current_set_idx < 0 || story->current_set_idx >= MAX_SETS_PER_DAY) return NULL;
    if (story->current_phase_idx < 0 || story->current_phase_idx >= MAX_PHASES_PER_SET) return NULL;
    return &story->sets[story->current_set_idx].phases[story->current_phase_idx];
}
