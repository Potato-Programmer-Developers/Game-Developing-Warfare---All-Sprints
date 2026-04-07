/**
 * @file story.c
 * @brief Handles story progression, multi-day state management, narrative parsing, and objective tracking.
 * 
 * Update History:
 * - 2026-03-20: Foundational implementation of the "Phase-Set System." (Goal: Move from linear scripting to a 
 *                manageable structure where story 'sets' contain multiple 'phases' for organized development.)
 * - 2026-04-03: Implemented the "Sequential Narration Engine" and tag parser. (Goal: Support complex 
 *                narrative actions like `[PLAY]` sound triggers, `[MESSAGE]` phone alerts, and branching 
 *                logic directly within `narration.txt` and `dayX.txt` files.)
 * - 2026-04-04: Added cross-day progression and "Day Transition" memory safety. (Goal: Resolve 
 *                segmentation faults occurring during the transition from Day 1 to Day 2 by implementing 
 *                robust `NULL` checks and state-reset logic in `AdvanceStory`.)
 * - 2026-04-05: Integrated "Phone Pending" and "Narration Timing" logic. (Goal: Synchronize the 
 *                narrative system with the camera and map fade effects, ensuring dialogues and 
 *                phone notifications only appear after the world has fully settled.)
 * - 2026-04-07: Resolved "Premature Quest Completion" BUG. (Goal: Prevent 
 *                `CONDITION_NARRATION_COMPLETE` from triggering before narration has started.)
 * 
 * Revision Details:
 * - Refactored `AdvanceStory` to support loading `dayX.txt` files dynamically via `LoadStoryDay`.
 * - Refined the `CONDITION_PHONE_COMPLETE` state machine for bi-directional choice support.
 * - Restricted auto-narration to phases with zero interactables (pure cutscenes).
 * - Updated `UpdateStory` to handle deferred triggering via the new dialogue [PHONE] tag.
 * - Implemented a `ReplaceNewlines` helper function to handle `\n` character sequences globally.
 * - Added persistence for `met` flags in the `StoryCondition` struct to prevent phase-skipping.
 * - Created a dedicated `phone_pending` flag to defer phone sequence triggers until map loads complete.
 * - Implemented directory parsing logic in `LoadStoryDay` to extract the current `day_folder` from file paths.
 * 
 * Authors: Andrew Zhuo
 */

#include "story.h"
#include "scene.h"
#include "game_context.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ctype.h"
#include "game_context.h"
#include "data.h"
#include "audio.h"
#include "raylib.h"

/**
 * @brief Trims leading and trailing whitespace from a string.
 */
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

static void ReplaceNewlines(char* str) {
    char* src = str;
    char* dst = str;
    while (*src) {
        if (*src == '\\' && *(src + 1) == 'n') {
            *dst++ = '\n';
            src += 2;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
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

    // Load story day
    while (fgets(raw_line, sizeof(raw_line), file)){
        line_num++;
        char* line = trim(raw_line);
        if (strlen(line) == 0) continue;

        // Detect Set/Phase Headers
        if (strstr(line, "SET") && strstr(line, "PHASE")){
            int set_num = 0;
            if (sscanf(line, "SET%d", &set_num) == 1){
                // Update current set and phase
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

                    // Load narration if it exists in the phase folder: assets/text/day1/setX/phaseY/narration.txt
                    char narration_path[256];
                    snprintf(narration_path, sizeof(narration_path), "../assets/text/%s/set%d/phase%d/narration.txt", 
                             story->day_folder, current_set + 1, current_phase + 1);
                    
                    FILE* nfile = fopen(narration_path, "r");
                    if (nfile) {
                        char nline[256];
                        bool in_loop = false;
                        bool in_phone = false;
                        NarrationChoice* current_choice = NULL;
                        PhoneMessage* current_pmsg = NULL;
                        PhoneChoice* current_pchoice = NULL;
                        
                        while (fgets(nline, sizeof(nline), nfile)) {
                            char* trimmed = trim(nline);
                            if (strlen(trimmed) == 0) continue;
                            
                            if (strstr(trimmed, "[PHONE]") && !in_loop) {
                                char* sender = strstr(trimmed, "[PHONE]") + 7;
                                while (*sender == ' ') sender++;
                                strncpy(phase_ptr->phone_sender, sender, 63);
                                phase_ptr->phone_message_count = 0;
                                in_phone = true;
                                current_pmsg = NULL;
                                current_pchoice = NULL;
                                // Add a phone_start narration line marker
                                if (phase_ptr->narration_count < 20) {
                                    phase_ptr->narration_lines[phase_ptr->narration_count].type = 3; // phone_start
                                    strncpy(phase_ptr->narration_lines[phase_ptr->narration_count].text, "PHONE", 127);
                                    phase_ptr->narration_count++;
                                }
                            } else if (in_phone && strstr(trimmed, "[MESSAGE]")) {
                                char* msg = strstr(trimmed, "[MESSAGE]") + 9;
                                while (*msg == ' ') msg++;
                                if (phase_ptr->phone_message_count < 8) {
                                    current_pmsg = &phase_ptr->phone_messages[phase_ptr->phone_message_count];
                                    strncpy(current_pmsg->text, msg, 127);
                                    ReplaceNewlines(current_pmsg->text);
                                    current_pmsg->choice_count = 0;
                                    current_pchoice = NULL;
                                    phase_ptr->phone_message_count++;
                                } else {
                                    current_pmsg = NULL;
                                }
                            } else if (in_phone && current_pmsg && strstr(trimmed, "[CHOICE]")) {
                                char* choice_text = strstr(trimmed, "[CHOICE]") + 8;
                                while (*choice_text == ' ') choice_text++;
                                if (current_pmsg->choice_count < 4) {
                                    current_pchoice = &current_pmsg->choices[current_pmsg->choice_count];
                                    strncpy(current_pchoice->text, choice_text, 127);
                                    ReplaceNewlines(current_pchoice->text);
                                    current_pchoice->dream_count = 0;
                                    current_pmsg->choice_count++;
                                } else {
                                    current_pchoice = NULL;
                                }
                            } else if (in_phone && current_pchoice && strstr(trimmed, "[FULL TEXT]")) {
                                char* dtxt = strstr(trimmed, "[FULL TEXT]") + 11;
                                while (*dtxt == ' ') dtxt++;
                                if (current_pchoice->dream_count < 4) {
                                    // Optionally parsing 'BLACK' prefix if needed, but for now just copy the text (it might include 'BLACK You fall asleep')
                                    // Actually we can leave 'BLACK ' in or skip it. Let's just strncpy.
                                    strncpy(current_pchoice->dream_lines[current_pchoice->dream_count], dtxt, 127);
                                    ReplaceNewlines(current_pchoice->dream_lines[current_pchoice->dream_count]);
                                    current_pchoice->dream_count++;
                                }
                            } else if (in_phone && strstr(trimmed, "[SANITY]")) {
                                // Ignore sanity tags
                                continue;
                            } else if (strstr(trimmed, "[PLAY]")) {
                                in_phone = false; // Any non-phone tag ends phone block
                                if (phase_ptr->narration_count < 20) {
                                    char* sound_name = strstr(trimmed, "[PLAY]") + 6;
                                    while (*sound_name == ' ') sound_name++;
                                    strncpy(phase_ptr->narration_lines[phase_ptr->narration_count].text, sound_name, 127);
                                    phase_ptr->narration_lines[phase_ptr->narration_count].type = 1; // play_sound
                                    phase_ptr->narration_count++;
                                }
                            } else if (strstr(trimmed, "[LOOP]")) {
                                in_phone = false;
                                if (phase_ptr->narration_count < 20) {
                                    phase_ptr->narration_lines[phase_ptr->narration_count].type = 2; // loop_start
                                    strncpy(phase_ptr->narration_lines[phase_ptr->narration_count].text, "LOOP", 127);
                                    phase_ptr->narration_count++;
                                }
                                in_loop = true;
                            } else if (in_loop && strstr(trimmed, "[CHOICE]")) {
                                if (phase_ptr->narration_choice_count < 8) {
                                    current_choice = &phase_ptr->narration_choices[phase_ptr->narration_choice_count];
                                    char* label = strstr(trimmed, "[CHOICE]") + 8;
                                    while (*label == ' ') label++;
                                    strncpy(current_choice->label, label, 63);
                                    current_choice->completed = false;
                                    phase_ptr->narration_choice_count++;
                                }
                            } else if (in_loop && current_choice && strstr(trimmed, "[RESPONSE]")) {
                                char* resp = strstr(trimmed, "[RESPONSE]") + 10;
                                while (*resp == ' ') resp++;
                                strncpy(current_choice->response, resp, 127);
                            } else if (in_loop && current_choice && strstr(trimmed, "[STATE]")) {
                                char* state_str = strstr(trimmed, "[STATE]") + 7;
                                while (*state_str == ' ') state_str++;
                                char key[32]; char val_str[16];
                                if (sscanf(state_str, "%31s %15s", key, val_str) == 2) {
                                    strncpy(current_choice->state_key, key, 31);
                                    current_choice->state_value = (strcmp(val_str, "true") == 0);
                                }
                            } else if (!in_phone) {
                                // Plain text narration line (only if not inside phone block)
                                in_phone = false;
                                if (phase_ptr->narration_count < 20) {
                                    strncpy(phase_ptr->narration_lines[phase_ptr->narration_count].text, trimmed, 127);
                                    phase_ptr->narration_lines[phase_ptr->narration_count].type = 0; // text
                                    phase_ptr->narration_count++;
                                }
                            }
                        }
                        fclose(nfile);
                    }
                }
            }
            continue;
        }

        // Skip if current set or phase is invalid
        if (current_set < 0 || current_phase < 0 || 
            current_set >= MAX_SETS_PER_DAY || current_phase >= MAX_PHASES_PER_SET){
            continue;
        }

        StoryPhase* phase = &story->sets[current_set].phases[current_phase];

        // Parse story day for the location, interactable type, and end condition
        if (strstr(line, "[LOCATION]")){
            if (strstr(line, "APARTMENT")) phase->location = STORY_LOC_APARTMENT;
            else if (strstr(line, "INTERIOR")) phase->location = STORY_LOC_INTERIOR;
            else if (strstr(line, "EXTERIOR")) phase->location = STORY_LOC_EXTERIOR;
            else if (strstr(line, "FARM")) phase->location = STORY_LOC_FARM;
            else if (strstr(line, "FOREST")) phase->location = STORY_LOC_FOREST;
            else phase->location = STORY_LOC_NONE;
        } else if (strstr(line, "[FORCE_NARRATION]")){
            phase->force_narration = true;
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
            if (phase->condition_count < 5) {
                StoryCondition* cond = &phase->end_conditions[phase->condition_count];
                char type_str[32];
                if (sscanf(line, "[CONDITION] %s", type_str) == 1){
                    if (strcmp(type_str, "ALL_QUESTS_COMPLETE") == 0) cond->type = CONDITION_ALL_QUESTS_COMPLETE;
                    else if (strcmp(type_str, "INTERACT_OBJECT") == 0){
                        cond->type = CONDITION_INTERACT_OBJECT;
                        sscanf(line, "[CONDITION] INTERACT_OBJECT %s %f", cond->target_id, &cond->target_value);
                    } else if (strcmp(type_str, "TIME_PASS") == 0){
                        cond->type = CONDITION_TIME_PASS;
                        sscanf(line, "[CONDITION] TIME_PASS %f", &cond->target_value);
                    } else if (strcmp(type_str, "ENTER_LOCATION") == 0){
                        cond->type = CONDITION_ENTER_LOCATION;
                        char loc_str[32];
                        if (sscanf(line, "[CONDITION] ENTER_LOCATION %s", loc_str) == 1){
                            if (strcmp(loc_str, "INTERIOR") == 0) cond->target_value = (float)STORY_LOC_INTERIOR;
                            else if (strcmp(loc_str, "EXTERIOR") == 0) cond->target_value = (float)STORY_LOC_EXTERIOR;
                            else if (strcmp(loc_str, "FARM") == 0) cond->target_value = (float)STORY_LOC_FARM;
                            else if (strcmp(loc_str, "FOREST") == 0) cond->target_value = (float)STORY_LOC_FOREST;
                            else if (strcmp(loc_str, "APARTMENT") == 0) cond->target_value = (float)STORY_LOC_APARTMENT;
                        }
                    } else if (strcmp(type_str, "COLLECT_OBJECTS") == 0){
                        cond->type = CONDITION_COLLECT_OBJECTS;
                        sscanf(line, "[CONDITION] COLLECT_OBJECTS %s %f", cond->target_id, &cond->target_value);
                    } else if (strcmp(type_str, "NARRATION_COMPLETE") == 0){
                        cond->type = CONDITION_NARRATION_COMPLETE;
                    } else if (strcmp(type_str, "PHONE_COMPLETE") == 0){
                        cond->type = CONDITION_PHONE_COMPLETE;
                    } else if (strcmp(type_str, "DREAM_COMPLETE") == 0){
                        cond->type = CONDITION_DREAM_COMPLETE;
                    }
                    cond->met = false;
                    phase->condition_count++;
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
    story->current_phase_idx = 0;
    story->phase_timer = 0;
    story->narration_has_started = false;

    // Trigger narration if first phase has it (deferred via pending flag)
    // only auto-trigger if there are no interactables (indicating a pure cutscene phase)
    // or if FORCE_NARRATION is explicitly set
    StoryPhase* first = GetActivePhase(story);
    if (first && first->narration_count > 0 && (first->interactable_count == 0 || first->force_narration)) {
        story->narration_pending = true;
    }
}

static bool AllConditionsMet(StoryPhase* active, struct GameContext* game_context) {
    if (!active || active->condition_count == 0) return true;
    for (int i = 0; i < active->condition_count; i++) {
        StoryCondition* cond = &active->end_conditions[i];

        // Check conditions based on type
        switch (cond->type) {
            case CONDITION_ALL_QUESTS_COMPLETE: {
                bool all_done = true;
                for (int j = 0; j < active->quest_count; j++) {
                    if (!active->quests[j].completed) {
                        all_done = false;
                        break;
                    }
                }
                if (!all_done) return false;
            } break;
            
            case CONDITION_TIME_PASS:
                if (game_context->story.phase_timer < cond->target_value) return false;
                break;
            
            // For INTERACT and COLLECT, we rely on them being marked '.met = true' by interaction.c
            case CONDITION_INTERACT_OBJECT:
            case CONDITION_COLLECT_OBJECTS:
                if (!cond->met) return false;
                break;

            case CONDITION_ENTER_LOCATION:
                if (game_context->location == (int)cond->target_value) cond->met = true;
                if (!cond->met) return false;
                break;

            case CONDITION_NARRATION_COMPLETE: {
                // Check that narration is no longer active AND has actually started
                if (game_context->story.narration_active) return false;
                if (active->narration_count > 0 && !game_context->story.narration_has_started) return false;
                for (int j = 0; j < active->narration_choice_count; j++) {
                    if (!active->narration_choices[j].completed) return false;
                }
                cond->met = true;
            } break;

            case CONDITION_PHONE_COMPLETE:
                if (!cond->met) return false;
                break;
                
            case CONDITION_DREAM_COMPLETE:
                if (!cond->met) return false;
                break;
        }
    }
    return true;
}

void UpdateStory(struct GameContext* game_context, float delta){
    StorySystem* story = &game_context->story;
    Scene* game_scene = game_context->game_scene;

    // Guard: Do not update story logic while the screen is fading.
    // This prevents AdvanceStory from being called multiple times during the transition.
    if (game_scene && (game_scene->is_fading_out || game_scene->is_fading_in)) return;

    StoryPhase* active = GetActivePhase(story);
    if (!active) return;

    // Increment timer regardless, AllConditionsMet will check it if needed
    story->phase_timer += delta;

    // Handle phone message sequence playback (triggered inline by narration type=3)
    if (story->phone_sequence_active) {
        story->phone_message_timer += delta;
        if (story->phone_message_timer >= 2.0f) {
            story->phone_message_timer = 0;
            story->phone_current_index++;
            if (story->phone_current_index >= story->phone_active_count) {
                // Phone sequence done, advance narration to next line
                story->phone_sequence_active = false;
                story->narration_current_line++;
                // Check if next line exists
                if (story->narration_current_line >= active->narration_count) {
                    if (active->narration_choice_count == 0) {
                        story->narration_active = false;
                    }
                }
            }
        }
    }

    // Handle dream sequence automatically starting after a phone conversation yields dream lines
    if (!game_context->dream_active && game_context->dream_count > 0 && game_context->dream_current < game_context->dream_count && game_context->phone.state == PHONE_IDLE && game_context->phone.already_triggered) {
        game_context->dream_active = true;
        game_context->dream_timer = 0;
    }

    // Advance active dream sequence
    if (game_context->dream_active) {
        game_context->dream_timer += delta;
        if (game_context->dream_timer >= 2.0f) {
            game_context->dream_timer = 0;
            game_context->dream_current++;
            if (game_context->dream_current >= game_context->dream_count) {
                game_context->dream_active = false;
                // Mark DREAM_COMPLETE condition as met
                for (int i = 0; i < active->condition_count; i++) {
                    if (active->end_conditions[i].type == CONDITION_DREAM_COMPLETE) {
                        active->end_conditions[i].met = true;
                    }
                }
            }
        }
    }
    
    // Check for interactive phone completion
    if (game_context->phone.state == PHONE_IDLE && game_context->phone.already_triggered && !game_context->dream_active) {
         // If we don't have a dream queued, we can satisfy PHONE_COMPLETE
         // If a dream is queued, it will start next frame and PHONE_COMPLETE is implicitly handled by the next state
         for (int i = 0; i < active->condition_count; i++) {
             if (active->end_conditions[i].type == CONDITION_PHONE_COMPLETE) {
                 active->end_conditions[i].met = true;
             }
         }
    }

    if (AllConditionsMet(active, game_context)) {
        AdvanceStory(game_context);
    }
}

void AdvanceStory(struct GameContext* game_context){
    StorySystem* story = &game_context->story;
    if (story->current_set_idx < 0 || story->current_set_idx >= MAX_SETS_PER_DAY) return;
    
    StoryPhase* old = GetActivePhase(story);
    story->current_phase_idx++;
    story->phase_timer = 0;

    // Reset narration for new phase
    story->narration_active = false;
    story->narration_current_line = 0;
    story->narration_in_loop = false;
    story->narration_showing_response = false;
    story->narration_has_started = false;
    story->phone_sequence_active = false;
    story->phone_message_timer = 0.0f;

    // Reset phone for new phase
    game_context->phone.state = PHONE_IDLE;
    game_context->phone.already_triggered = false;
    
    // Reset dream state
    game_context->dream_active = false;
    game_context->dream_count = 0;
    game_context->dream_current = 0;
    game_context->dream_timer = 0;

    // If we've reached the end of the current set, move to the next set
    if (story->current_phase_idx >= story->sets[story->current_set_idx].phase_count){
        story->current_phase_idx = 0;
        story->current_set_idx++;
        
        // If we've reached the end of the day, move to the next day
        if (story->current_set_idx >= story->set_count){
            int current_day = 0;
            if (sscanf(story->day_folder, "day%d", &current_day) == 1) {
                char next_day_path[256];
                snprintf(next_day_path, sizeof(next_day_path), "../assets/text/day%d/day%d.txt", current_day + 1, current_day + 1);
                
                if (FileExists(next_day_path)) {
                    LoadStoryDay(story, next_day_path);
                    // LoadStoryDay resets current_set_idx and current_phase_idx to 0
                    // which is what we want for the new day
                } else {
                    // Stay at end of current day if next day doesn't exist
                    story->current_set_idx = story->set_count - 1;
                    story->current_phase_idx = story->sets[story->current_set_idx].phase_count - 1;
                }
            }
        }
    }
    
    StoryPhase* next = GetActivePhase(story);
    if (next) {
        // Reset met flags for the new phase
        for (int i = 0; i < next->condition_count; i++) {
            next->end_conditions[i].met = false;
        }

        // Automatic map transition if the new phase has a different location
        if (next->location != STORY_LOC_NONE && (int)next->location != (int)game_context->location) {
            if (game_context->game_scene) {
                game_context->game_scene->is_fading_out = true;
                game_context->game_scene->fade_alpha = 0.0f;
                game_context->game_scene->fade_color = BLACK;
                
                if (next->location == STORY_LOC_EXTERIOR) {
                    strncpy(game_context->game_scene->pending_map, "../assets/map/map_ext/MAINMAP.json", 127);
                    strncpy(game_context->game_scene->pending_loc, "EXTERIOR", 31);
                } else if (next->location == STORY_LOC_APARTMENT) {
                    strncpy(game_context->game_scene->pending_map, "../assets/map/map_int/MAINMAP.json", 127);
                    strncpy(game_context->game_scene->pending_loc, "APARTMENT", 31);
                }
            }
        }
    }

    bool wants_interactive_phone = false;
    if (next) {
        // Check if we need to trigger an interactive phone sequence
        for (int i = 0; i < next->condition_count; i++) {
            if (next->end_conditions[i].type == CONDITION_PHONE_COMPLETE || 
                next->end_conditions[i].type == CONDITION_DREAM_COMPLETE) {
                wants_interactive_phone = true;
                break;
            }
        }
    }

    story->phone_pending = wants_interactive_phone;
    if (!wants_interactive_phone && next && next->narration_count > 0 && (next->interactable_count == 0 || next->force_narration)) {
        // Mark narration as pending (will be activated after fade/camera settle)
        story->narration_pending = true;
    }
    // Note: Do NOT call LoadPhaseAssets here!
    // The main loop in state.c will detect the index change and call it after swapping the map.

    SaveData(game_context, NULL);
}

StoryPhase* GetActivePhase(StorySystem* story){
    // Return the active phase
    if (story->current_set_idx < 0 || story->current_set_idx >= MAX_SETS_PER_DAY) return NULL;
    if (story->current_phase_idx < 0 || story->current_phase_idx >= MAX_PHASES_PER_SET) return NULL;
    return &story->sets[story->current_set_idx].phases[story->current_phase_idx];
}

// Helper to apply [STATE] mutations to GameContext
static void ApplyStateMutation(struct GameContext* ctx, const char* key, bool value) {
    if (strcmp(key, "fireplace_on") == 0) ctx->fireplace_on = value;
    else if (strcmp(key, "main_door_locked") == 0) ctx->main_door_locked = value;
    else if (strcmp(key, "windows_locked") == 0) ctx->windows_locked = value;
    else if (strcmp(key, "has_room_keys") == 0) ctx->has_room_keys = value;
    else if (strcmp(key, "doors") == 0) ctx->doors = value;
}

void HandleNarrationInput(struct GameContext* game_context, int* game_state, struct Audio* game_audio) {
    StorySystem* story = &game_context->story;
    StoryPhase* active = GetActivePhase(story);
    if (!active || !story->narration_active) return;

    // Don't accept input during phone sequence (auto-advancing)
    if (story->phone_sequence_active) return;

    // Check if current line is a phone_start — trigger phone sequence
    if (story->narration_current_line < active->narration_count &&
        active->narration_lines[story->narration_current_line].type == 3) {
        // Copy phase phone data into active playback
        strncpy(story->phone_active_sender, active->phone_sender, 63);
        for (int i = 0; i < active->phone_message_count && i < 8; i++) {
            story->phone_active_messages[i] = active->phone_messages[i];
        }
        story->phone_active_count = active->phone_message_count;
        story->phone_current_index = 0;
        story->phone_message_timer = 0;
        story->phone_sequence_active = true;
        return;
    }

    // If showing a choice response, SPACE returns to loop
    if (story->narration_showing_response) {
        if (IsKeyPressed(KEY_SPACE)) {
            story->narration_showing_response = false;
            // Check if all choices are done
            bool all_done = true;
            for (int i = 0; i < active->narration_choice_count; i++) {
                if (!active->narration_choices[i].completed) { all_done = false; break; }
            }
            if (all_done) {
                // Narration loop complete, end narration
                story->narration_active = false;
                story->narration_in_loop = false;
                *game_state = GAMEPLAY;
            }
        }
        return;
    }

    // If in loop mode, handle choice selection
    if (story->narration_in_loop) {
        for (int i = 0; i < active->narration_choice_count; i++) {
            if (IsKeyPressed(KEY_ONE + i)) {
                if (!active->narration_choices[i].completed) {
                    active->narration_choices[i].completed = true;
                    strncpy(story->narration_response_text, active->narration_choices[i].response, 127);
                    story->narration_showing_response = true;
                    // Apply state mutation
                    if (active->narration_choices[i].state_key[0] != '\0') {
                        ApplyStateMutation(game_context, active->narration_choices[i].state_key, active->narration_choices[i].state_value);
                    }
                }
                break;
            }
        }
        return;
    }

    // Normal text line advancement
    if (IsKeyPressed(KEY_SPACE)) {
        // Advance to next narration line
        story->narration_current_line++;
        
        // Skip through any [PLAY] sound lines immediately
        while (story->narration_current_line < active->narration_count) {
            NarrationLine* line = &active->narration_lines[story->narration_current_line];
            if (line->type == 1) {
                // Play sound
                if (game_audio && strcmp(line->text, "FOOTSTEP") == 0) {
                    PlayStep(game_audio, game_context->location);
                }
                story->narration_current_line++;
            } else {
                break;
            }
        }
        
        // Check if we hit a LOOP line
        if (story->narration_current_line < active->narration_count) {
            if (active->narration_lines[story->narration_current_line].type == 2) {
                story->narration_in_loop = true;
                return;
            }
            // Check if we hit a phone_start line (type=3) — will be handled next frame
            if (active->narration_lines[story->narration_current_line].type == 3) {
                return;
            }
        }
        
        // Check if narration is exhausted
        if (story->narration_current_line >= active->narration_count) {
            if (active->narration_choice_count == 0) {
                // No loop choices, just end narration
                story->narration_active = false;
                *game_state = GAMEPLAY;
            }
        }
    }
}
