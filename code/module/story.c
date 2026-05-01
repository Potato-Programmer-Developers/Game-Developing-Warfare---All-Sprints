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
 * - 2026-04-10: Implemented "Conditional Narration Branching" for Day 2 SET4-PHASE2. (Goal: Support
 *                `[IF]` conditional blocks within `narration.txt` that evaluate `GameContext` state
 *                variables (e.g., `main_door_locked`, `fireplace_on`, `has_room_keys`) to dynamically
 *                include or exclude narration lines, sound effects, and choice branches based on the
 *                player's decisions in SET4-PHASE1. This enables unique narrative outcomes per playthrough.)
 * - 2026-04-10: Implemented `[BREAK]` loop termination and `narration_loop_broken` flag. (Goal: Allow
 *                the "go to bed" choice in SET4-PHASE1 to prematurely exit the narration choice loop,
 *                setting all remaining un-selected choices to `false` and satisfying
 *                `CONDITION_NARRATION_COMPLETE` without requiring all six choices to be completed.)
 * - 2026-04-10: Implemented "Indentation-Aware Phone Tree Parser" for Day 2 SET4-PHASE3. (Goal: Replace
 *                the flat linear phone message parser with a hierarchical, indentation-driven tree builder
 *                that constructs branching conversation graphs from nested `[MESSAGE]`/`[CHOICE]` tags.
 *                The parser uses leading whitespace to determine parent-child relationships and builds a
 *                flat array of `PhoneMessage` nodes connected via `next_msg_idx` links, with a two-pass
 *                post-processing algorithm that correctly resolves both branching child paths and linear
 *                sequential siblings while preventing cross-branch contamination.)
 * - 2026-04-10: Integrated `[PLAY]` sound tag processing into narration playback. (Goal: Trigger
 *                ambient horror sound effects inline during narration by matching `[PLAY]` tag values
 *                to named `Sound` handles in the `Audio` struct, enabling audio cues like door banging,
 *                window scraping, and chimney rustling at precise narrative moments.)
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
 * - Introduced `LoadPhaseNarration` function that dynamically parses `narration.txt` with `[IF]` condition
 *    evaluation against `GameContext` state fields, allowing `[ANY_MISSED]` and compound boolean logic
 *    to gate sub-blocks of narration text, sound cues, and choices at load time.
 * - Added `narration_loop_broken` flag handling in `HandleNarrationInput`: when a `[BREAK]` choice is
 *    selected, all remaining loop choices are marked `completed = false` and the loop exits immediately
 *    instead of continuing to display the choice menu.
 * - Modified `EvaluateCondition` to treat `narration_loop_broken == true` as equivalent to
 *    `CONDITION_NARRATION_COMPLETE` being met, bypassing the standard all-choices-completed check.
 * - Rewrote the `in_phone` parsing block in `LoadStorySets` to use `msg_levels[]` (indent level per
 *    message) and `msg_at_level[]` (latest message index at each indent level) for correct parent-child
 *    assignment. `[CHOICE]` lines now look up their parent message via indent level instead of relying
 *    on `current_pmsg`, which previously pointed to the most recently parsed message regardless of depth.
 * - Implemented a two-pass post-processing algorithm after phone tree parsing:
 *    Pass 1: Links each `PhoneChoice.next_msg_idx` to its correct child message by counting subtree
 *    sizes of earlier sibling choices and computing candidate indices, with fallback to same-level
 *    sibling search bounded by indent-level subtree boundaries.
 *    Pass 2: Computes `PhoneMessage.next_auto_idx` for choiceless messages by finding the next
 *    same-level sibling that is NOT a branch entry point (not targeted by any choice's `next_msg_idx`),
 *    preventing terminal dead-end messages from incorrectly auto-advancing into unrelated branches.
 * - Built a `is_choice_target[]` boolean lookup array during post-processing to distinguish between
 *    true sequential siblings (auto-advance targets) and branch entry points (reached only via choice).
 * - Expanded the phone data copy loop in `HandleNarrationInput` from 8 to 32 to match the increased
 *    `StoryPhase.phone_messages` and `StorySystem.phone_active_messages` array capacities.
 * 
 * Authors: Andrew Zhuo
 */

#include "story.h"
#include "scene.h"
#include "game_context.h"
#include "data.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ctype.h"
#include "game_context.h"
#include "data.h"
#include "audio.h"
#include "raylib.h"

// Forward declarations
static void LoadEndingSequence(StorySystem* story, StoryPhase* phase);

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
                        int msg_levels[32] = {0};  // Track indent level per phone message
                        int msg_at_level[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}; // msg index at each indent level
                        
                        while (fgets(nline, sizeof(nline), nfile)) {
                            char* trimmed = trim(nline);
                            if (strlen(trimmed) == 0) continue;
                            
                            // Count leading spaces for indentation
                            int indent = 0;
                            { char* raw = nline; while (*raw == ' ') { indent++; raw++; } }
                            int level = indent / 4;
                            
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
                                    strncpy(phase_ptr->narration_lines[phase_ptr->narration_count].text, "PHONE", 255);
                                    phase_ptr->narration_count++;
                                }
                            } else if (in_phone && strstr(trimmed, "[MESSAGE]")) {
                                char* msg = strstr(trimmed, "[MESSAGE]") + 9;
                                while (*msg == ' ') msg++;
                                if (phase_ptr->phone_message_count < 32) {
                                    int new_idx = phase_ptr->phone_message_count;
                                    current_pmsg = &phase_ptr->phone_messages[new_idx];
                                    memset(current_pmsg, 0, sizeof(PhoneMessage));
                                    strncpy(current_pmsg->text, msg, 127);
                                    ReplaceNewlines(current_pmsg->text);
                                    current_pmsg->choice_count = 0;
                                    current_pchoice = NULL;
                                    msg_levels[new_idx] = level;
                                    if (level < 10) msg_at_level[level] = new_idx;
                                    phase_ptr->phone_message_count++;
                                } else {
                                    current_pmsg = NULL;
                                }
                            } else if (in_phone && strstr(trimmed, "[CHOICE]")) {
                                // Find the correct parent message for this choice based on indent level.
                                // A choice at level L belongs to the message at level L (same level)
                                // or at level L-1 (one level above), whichever was most recently defined.
                                PhoneMessage* parent_msg = NULL;
                                if (level < 10 && msg_at_level[level] >= 0) {
                                    parent_msg = &phase_ptr->phone_messages[msg_at_level[level]];
                                }
                                if (!parent_msg && level > 0 && msg_at_level[level - 1] >= 0) {
                                    parent_msg = &phase_ptr->phone_messages[msg_at_level[level - 1]];
                                }
                                if (!parent_msg) parent_msg = current_pmsg; // fallback
                                
                                char* choice_text = strstr(trimmed, "[CHOICE]") + 8;
                                while (*choice_text == ' ') choice_text++;
                                if (parent_msg && parent_msg->choice_count < 4) {
                                    current_pchoice = &parent_msg->choices[parent_msg->choice_count];
                                    memset(current_pchoice, 0, sizeof(PhoneChoice));
                                    strncpy(current_pchoice->text, choice_text, 127);
                                    ReplaceNewlines(current_pchoice->text);
                                    current_pchoice->dream_count = 0;
                                    current_pchoice->next_msg_idx = -1; // Default: end conversation
                                    parent_msg->choice_count++;
                                } else {
                                    current_pchoice = NULL;
                                }
                            } else if (in_phone && current_pchoice && strstr(trimmed, "[FULL TEXT]")) {
                                char* dtxt = strstr(trimmed, "[FULL TEXT]") + 11;
                                while (*dtxt == ' ') dtxt++;
                                if (current_pchoice->dream_count < 4) {
                                    strncpy(current_pchoice->dream_lines[current_pchoice->dream_count], dtxt, 127);
                                    ReplaceNewlines(current_pchoice->dream_lines[current_pchoice->dream_count]);
                                    current_pchoice->dream_count++;
                                }
                            } else if (in_phone && strstr(trimmed, "[SANITY]") && current_pchoice) {
                                if (strstr(trimmed, "++")) current_pchoice->sanity_change = 20;
                                else if (strstr(trimmed, "--")) current_pchoice->sanity_change = -20;
                                else if (strstr(trimmed, "+")) current_pchoice->sanity_change = 10;
                                else if (strstr(trimmed, "-")) current_pchoice->sanity_change = -10;
                            } else if (in_phone && strstr(trimmed, "[SCENE]") && current_pchoice) {
                                char* scene_txt = strstr(trimmed, "[SCENE]") + 7;
                                while (*scene_txt == ' ') scene_txt++;
                                strncpy(current_pchoice->scene_trigger, scene_txt, 31);
                            } else if (strstr(trimmed, "[PLAY]")) {
                                in_phone = false; // Any non-phone tag ends phone block
                                if (phase_ptr->narration_count < 20) {
                                    char* sound_name = strstr(trimmed, "[PLAY]") + 6;
                                    while (*sound_name == ' ') sound_name++;
                                    strncpy(phase_ptr->narration_lines[phase_ptr->narration_count].text, sound_name, 255);
                                    phase_ptr->narration_lines[phase_ptr->narration_count].type = 1; // play_sound
                                    phase_ptr->narration_count++;
                                }
                            } else if (strstr(trimmed, "[LOOP]")) {
                                in_phone = false;
                                if (phase_ptr->narration_count < 20) {
                                    phase_ptr->narration_lines[phase_ptr->narration_count].type = 2; // loop_start
                                    strncpy(phase_ptr->narration_lines[phase_ptr->narration_count].text, "LOOP", 255);
                                    phase_ptr->narration_count++;
                                }
                                in_loop = true;
                            } else if (in_loop && strstr(trimmed, "[CHOICE]")) {
                                if (phase_ptr->narration_choice_count < 8) {
                                    current_choice = &phase_ptr->narration_choices[phase_ptr->narration_choice_count];
                                    char* label = strstr(trimmed, "[CHOICE]") + 8;
                                    while (*label == ' ') label++;
                                    
                                    current_choice->only_one = false;
                                    if (strncmp(label, "[ONLY_ONE]", 10) == 0) {
                                        current_choice->only_one = true;
                                        label += 10;
                                        while (*label == ' ') label++;
                                    }
                                    
                                    strncpy(current_choice->label, label, 63);
                                    current_choice->completed = false;
                                    phase_ptr->narration_choice_count++;
                                }
                            } else if (in_loop && current_choice && strstr(trimmed, "[RESPONSE]")) {
                                char* resp = strstr(trimmed, "[RESPONSE]") + 10;
                                while (*resp == ' ') resp++;
                                strncpy(current_choice->response, resp, 255);
                                ReplaceNewlines(current_choice->response);
                            } else if (in_loop && current_choice && strstr(trimmed, "[STATE]")) {
                                char* state_str = strstr(trimmed, "[STATE]") + 7;
                                while (*state_str == ' ') state_str++;
                                char key[32]; char val_str[16];
                                if (sscanf(state_str, "%31s %15s", key, val_str) == 2) {
                                    strncpy(current_choice->state_key, key, 31);
                                    current_choice->state_value = (strcmp(val_str, "true") == 0);
                                }
                            } else if (in_loop && current_choice && strstr(trimmed, "[BREAK]")) {
                                current_choice->is_break = true;
                            } else if (!in_phone) {
                                // Plain text narration line (only if not inside phone block)
                                in_phone = false;
                                if (phase_ptr->narration_count < 20) {
                                    strncpy(phase_ptr->narration_lines[phase_ptr->narration_count].text, trimmed, 255);
                                    ReplaceNewlines(phase_ptr->narration_lines[phase_ptr->narration_count].text);
                                    phase_ptr->narration_lines[phase_ptr->narration_count].type = 0; // text
                                    phase_ptr->narration_count++;
                                }
                            }
                        }
                        fclose(nfile);
                        
                        // Post-processing: link choices and compute auto-advance for choiceless messages
                        int mc = phase_ptr->phone_message_count;
                        
                        // Pass 1: Link choices to their next messages
                        for (int mi = 0; mi < mc; mi++) {
                            PhoneMessage* pm = &phase_ptr->phone_messages[mi];
                            int pm_level = msg_levels[mi];
                            
                            for (int ci = 0; ci < pm->choice_count; ci++) {
                                PhoneChoice* pc = &pm->choices[ci];
                                if (pc->next_msg_idx != -1) continue; // Already linked
                                if (pc->dream_count > 0 || pc->scene_trigger[0] != '\0') continue; // Terminal
                                
                                // Count messages in subtrees of earlier sibling choices
                                int children_to_skip = 0;
                                for (int prev_ci = 0; prev_ci < ci; prev_ci++) {
                                    if (pm->choices[prev_ci].next_msg_idx != -1) {
                                        int start = pm->choices[prev_ci].next_msg_idx;
                                        int subtree_level = msg_levels[start];
                                        children_to_skip++; // Count the root msg itself
                                        for (int k = start + 1; k < mc; k++) {
                                            if (msg_levels[k] <= subtree_level) break; // Sibling or shallower
                                            children_to_skip++;
                                        }
                                    }
                                }
                                
                                int candidate = mi + 1 + children_to_skip;
                                
                                if (candidate < mc && msg_levels[candidate] > pm_level) {
                                    pc->next_msg_idx = candidate;
                                } else {
                                    // Look for the next sibling message at EXACTLY the same level.
                                    // Stop if we hit a shallower level (different subtree).
                                    for (int k = mi + 1; k < mc; k++) {
                                        if (msg_levels[k] < pm_level) break;
                                        if (msg_levels[k] == pm_level) {
                                            pc->next_msg_idx = k;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        
                        // First, mark which messages are branch entry points
                        // (i.e., directly targeted by a choice's next_msg_idx)
                        bool is_choice_target[32] = {false};
                        for (int mi = 0; mi < mc; mi++) {
                            PhoneMessage* pm = &phase_ptr->phone_messages[mi];
                            for (int ci = 0; ci < pm->choice_count; ci++) {
                                int target = pm->choices[ci].next_msg_idx;
                                if (target >= 0 && target < 32) {
                                    is_choice_target[target] = true;
                                }
                            }
                        }
                        
                        // Pass 2: Compute next_auto_idx for choiceless messages
                        // Only auto-advance to the next same-level sibling if it
                        // is NOT a branch entry point (would mean jumping branches).
                        for (int mi = 0; mi < mc; mi++) {
                            PhoneMessage* pm = &phase_ptr->phone_messages[mi];
                            pm->next_auto_idx = -1; // Default: end conversation
                            if (pm->choice_count == 0) {
                                int pm_level = msg_levels[mi];
                                for (int k = mi + 1; k < mc; k++) {
                                    if (msg_levels[k] < pm_level) break;
                                    if (msg_levels[k] == pm_level) {
                                        if (!is_choice_target[k]) {
                                            pm->next_auto_idx = k;
                                        }
                                        break;
                                    }
                                }
                            }
                        }
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
                    } else if (strcmp(type_str, "COLLECT_OBJECTS") == 0 || strcmp(type_str, "COLLECT_OBJECT") == 0){
                        cond->type = CONDITION_COLLECT_OBJECTS;
                        if (sscanf(line, "[CONDITION] COLLECT_OBJECTS %s %f", cond->target_id, &cond->target_value) != 2) {
                            sscanf(line, "[CONDITION] COLLECT_OBJECT %s %f", cond->target_id, &cond->target_value);
                        }
                    } else if (strcmp(type_str, "COLLIDE_OBJECTS") == 0 || strncmp(type_str, "COLLIDE_OBJECT", 14) == 0){
                        cond->type = CONDITION_COLLIDE_OBJECTS;
                        if (sscanf(line, "[CONDITION] COLLIDE_OBJECTS %s %f", cond->target_id, &cond->target_value) != 2) {
                            sscanf(line, "[CONDITION] COLLIDE_OBJECT %s %f", cond->target_id, &cond->target_value);
                        }
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
            case CONDITION_COLLIDE_OBJECTS:
                if (!cond->met) return false;
                break;

            case CONDITION_ENTER_LOCATION:
                if (game_context->location == (int)cond->target_value) cond->met = true;
                if (!cond->met) return false;
                break;

            case CONDITION_NARRATION_COMPLETE: {
                // Check that narration is no longer active AND has actually started
                if (game_context->story.narration_active) return false;
                if (game_context->story.ending_active) return false;  // Ending blocks advancement
                if (active->narration_count > 0 && !game_context->story.narration_has_started) return false;
                if (!game_context->story.narration_loop_broken) {
                    for (int j = 0; j < active->narration_choice_count; j++) {
                        if (!active->narration_choices[j].completed) return false;
                    }
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

        // Quest completion is now handled explicitly via UpdateStoryConditions in interaction.c
        // to support many-to-one condition-to-quest mappings.
    }
    
    // Continuous quest marking for location/time-based conditions
    for (int i = 0; i < active->condition_count && i < active->quest_count; i++) {
        StoryCondition* cond = &active->end_conditions[i];
        if (cond->met && (cond->type == CONDITION_ENTER_LOCATION || cond->type == CONDITION_TIME_PASS)) {
            active->quests[i].completed = true;
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

    if (story->scene_timer > 0) {
        story->scene_timer -= delta;
        if (story->scene_timer <= 0) {
            story->scene_timer = 0;
            story->current_scene[0] = '\0';
            // Mark condition complete if needed?
            // Actually, we force-advance the story upon scene completion
            AdvanceStory(game_context);
        }
        return; // Do not update other story logic while scene is active
    }

    StoryPhase* active = GetActivePhase(story);
    if (!active) return;

    // Day 3 Mowing Timer
    if (strcmp(active->name, "SET2-PHASE2") == 0) {
        game_context->day3_mowing_timer += delta;
    }

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
                        TraceLog(LOG_INFO, "ENDING CHECK (UpdateStory): has_ending=%d, ending_file=%s, narr_count=%d",
                                 active->has_ending, active->ending_file, active->narration_count);
                        if (active->has_ending) {
                            LoadEndingSequence(story, active);
                        }
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

    StoryPhase* next = GetActivePhase(story);
    if (next && strcmp(next->name, "SET2-PHASE2") == 0) {
        game_context->day3_mowing_timer = 0;
    }

    // Reset narration for new phase
    story->narration_active = false;
    story->narration_current_line = 0;
    story->narration_in_loop = false;
    story->narration_showing_response = false;
    story->narration_has_started = false;
    story->narration_loop_broken = false;
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
    
    next = GetActivePhase(story);
    if (next) {
        // Reset met flags for the new phase
        for (int i = 0; i < next->condition_count; i++) {
            next->end_conditions[i].met = false;
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
    if (next) LoadPhaseNarration(next, game_context);
    
    if (!wants_interactive_phone && next && (next->narration_count > 0 || strcmp(next->name, "SET4-PHASE2") == 0) && (next->interactable_count == 0 || next->force_narration)) {
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
    if (strcmp(key, "fireplace_on") == 0) { ctx->fireplace_on = value; strcpy(ctx->last_narration_action, "FIREPLACE"); }
    else if (strcmp(key, "main_door_locked") == 0) { ctx->main_door_locked = value; strcpy(ctx->last_narration_action, "DOOR"); }
    else if (strcmp(key, "windows_locked") == 0) { ctx->windows_locked = value; strcpy(ctx->last_narration_action, "WINDOW"); }
    else if (strcmp(key, "has_room_keys") == 0) { ctx->has_room_keys = value; strcpy(ctx->last_narration_action, "KEYS"); }
    else if (strcmp(key, "look_outside") == 0) { ctx->look_outside = value; strcpy(ctx->last_narration_action, "OUTSIDE"); }
    else if (strcmp(key, "bear_trap_inside") == 0) { ctx->bear_trap_inside = value; strcpy(ctx->last_narration_action, "TRAP_INSIDE"); }
    else if (strcmp(key, "bear_trap_outside") == 0) { ctx->bear_trap_outside = value; strcpy(ctx->last_narration_action, "TRAP_OUTSIDE"); }
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
        for (int i = 0; i < active->phone_message_count && i < 32; i++) {
            story->phone_active_messages[i] = active->phone_messages[i];
        }
        story->phone_active_count = active->phone_message_count;
        story->phone_current_index = 0;
        story->phone_message_timer = 0;
        story->phone_sequence_active = true;
        return;
    }

    // Typing effect logic
    const char* current_text = NULL;
    if (story->narration_showing_response) {
        current_text = story->narration_response_text;
    } else if (!story->narration_in_loop && story->narration_current_line < active->narration_count) {
        if (active->narration_lines[story->narration_current_line].type == 0) {
            current_text = active->narration_lines[story->narration_current_line].text;
        }
    }

    int text_len = current_text ? strlen(current_text) : 0;
    if (current_text && story->narration_typing_index < text_len) {
        story->narration_typing_timer += GetFrameTime();
        if (story->narration_typing_timer >= 0.03f) {
            story->narration_typing_timer = 0.0f;
            story->narration_typing_index++;
            if (story->narration_typing_index >= text_len) {
                if (game_audio && IsSoundPlaying(game_audio->typing_sound)) StopSound(game_audio->typing_sound);
            } else if (game_audio && !IsSoundPlaying(game_audio->typing_sound)) {
                PlaySound(game_audio->typing_sound);
            }
        }
    }

    // If showing a choice response, SPACE returns to loop
    if (story->narration_showing_response) {
        if (IsKeyPressed(KEY_SPACE)) {
            if (story->narration_typing_index < text_len) {
                story->narration_typing_index = text_len;
                if (game_audio && IsSoundPlaying(game_audio->typing_sound)) StopSound(game_audio->typing_sound);
            } else {
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
        }
        return;
    }

    // If in loop mode, handle choice selection
    if (story->narration_in_loop) {
        for (int i = 0; i < active->narration_choice_count; i++) {
            if (IsKeyPressed(KEY_ONE + i)) {
                if (!active->narration_choices[i].completed) {
                    active->narration_choices[i].completed = true;
                    
                    // Handle [ONLY_ONE] exclusion
                    if (active->narration_choices[i].only_one) {
                        for (int j = 0; j < active->narration_choice_count; j++) {
                            if (active->narration_choices[j].only_one && i != j) {
                                active->narration_choices[j].completed = true;
                            }
                        }
                    }
                    
                    if (active->narration_choices[i].is_break) {
                        story->narration_loop_broken = true;
                        story->narration_active = false;
                        story->narration_in_loop = false;
                        *game_state = GAMEPLAY;
                        return;
                    }
                    
                    strncpy(story->narration_response_text, active->narration_choices[i].response, 255);
                    story->narration_showing_response = true;
                    story->narration_typing_index = 0;
                    story->narration_typing_timer = 0.0f;
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
        if (story->narration_typing_index < text_len) {
            story->narration_typing_index = text_len;
            if (game_audio && IsSoundPlaying(game_audio->typing_sound)) StopSound(game_audio->typing_sound);
        } else {
            // Advance to next narration line
            story->narration_current_line++;
            story->narration_typing_index = 0;
            story->narration_typing_timer = 0.0f;
        
        // Skip through any [PLAY] sound lines immediately
        while (story->narration_current_line < active->narration_count) {
            NarrationLine* line = &active->narration_lines[story->narration_current_line];
            
            // Apply sanity changes upon moving past or to the line
            if (line->sanity_change != 0 && game_context->player) {
                game_context->player->sanity += line->sanity_change;
                line->sanity_change = 0; // Prevent applying multiple times
            }

            if (line->type == 1) {
                // Play sound
                if (game_audio && strcmp(line->text, "FOOTSTEP") == 0) {
                    PlayStep(game_audio, game_context->location);
                } else if (game_audio) {
                    if (strcmp(line->text, "DOOR_BANGING") == 0) PlaySound(game_audio->door_banging);
                    else if (strcmp(line->text, "WINDOW_SCRAPING") == 0) PlaySound(game_audio->window_scraping);
                    else if (strcmp(line->text, "CHIMNEY_RUSTLING") == 0) PlaySound(game_audio->chimney_rustling);
                    else TraceLog(LOG_INFO, "PLAYING NARRATION SOUND: %s", line->text);
                }
                story->narration_current_line++;
            } else if (line->type == 4) {
                // Invisible modifier line, just skip past it
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
        
        if (story->narration_current_line >= active->narration_count) {
            if (active->narration_choice_count == 0) {
                // No loop choices, end narration
                story->narration_active = false;
                if (active->has_ending) {
                    // Trigger ending sequence
                    LoadEndingSequence(story, active);
                    *game_state = ENDING_CUTSCENE;
                } else {
                    *game_state = GAMEPLAY;
                }
            }
        }
        }
    }
}

static bool EvaluateCondition(struct GameContext* ctx, const char* cond) {
    if (strcmp(cond, "ANY_MISSED") == 0) return !ctx->main_door_locked || !ctx->windows_locked || !ctx->fireplace_on || !ctx->has_room_keys;
    if (strcmp(cond, "DOOR_MISSED") == 0) return !ctx->main_door_locked;
    if (strcmp(cond, "WINDOW_MISSED") == 0) return !ctx->windows_locked;
    if (strcmp(cond, "NOT FIREPLACE_ON AND HAS_ROOM_KEYS") == 0) return !ctx->fireplace_on && ctx->has_room_keys;
    if (strcmp(cond, "FIREPLACE_ON AND NOT HAS_ROOM_KEYS") == 0) return ctx->fireplace_on && !ctx->has_room_keys;
    if (strcmp(cond, "NOT FIREPLACE_ON AND NOT HAS_ROOM_KEYS") == 0) return !ctx->fireplace_on && !ctx->has_room_keys;
    if (strcmp(cond, "FIREPLACE_ON AND HAS_ROOM_KEYS") == 0) return ctx->fireplace_on && ctx->has_room_keys;
    if (strcmp(cond, "DOOR_LAST") == 0) return strcmp(ctx->last_narration_action, "DOOR") == 0;
    if (strcmp(cond, "WINDOW_LAST") == 0) return strcmp(ctx->last_narration_action, "WINDOW") == 0;
    if (strcmp(cond, "FIREPLACE_LAST") == 0) return strcmp(ctx->last_narration_action, "FIREPLACE") == 0;
    if (strcmp(cond, "KEYS_LAST") == 0) return strcmp(ctx->last_narration_action, "KEYS") == 0;

    // Day 3 SET4-PHASE2 Simplified Conditions
    if (strcmp(cond, "MAIN_DOOR_LOCKED AND WINDOWS_LOCKED AND FIREPLACE_ON AND BEAR_TRAP_OUTSIDE") == 0)
        return ctx->main_door_locked && ctx->windows_locked && ctx->fireplace_on && ctx->bear_trap_outside;
    if (strcmp(cond, "MAIN_DOOR_LOCKED AND WINDOWS_LOCKED AND FIREPLACE_ON AND BEAR_TRAP_INSIDE") == 0)
        return ctx->main_door_locked && ctx->windows_locked && ctx->fireplace_on && ctx->bear_trap_inside;
    if (strcmp(cond, "BEAR_TRAP_INSIDE") == 0) return ctx->bear_trap_inside;
    if (strcmp(cond, "BEAR_TRAP_OUTSIDE") == 0) return ctx->bear_trap_outside;

    // Sanity comparison: "SANITY > 50", "SANITY < 101", etc.
    if (strncmp(cond, "SANITY", 6) == 0) {
        char op;
        int threshold;
        if (sscanf(cond, "SANITY %c %d", &op, &threshold) == 2 && ctx->player) {
            if (op == '>') return ctx->player->sanity > threshold;
            if (op == '<') return ctx->player->sanity < threshold;
        }
        return false;
    }

    return false;
}

void LoadPhaseNarration(StoryPhase* phase, struct GameContext* game_context) {
    if (!phase || !game_context) return;
    
    // Only apply dynamic branching parsing for SET4-PHASE2
    if (strcmp(phase->name, "SET4-PHASE2") != 0) return;
    
    StorySystem* story = &game_context->story;
    char path[256];
    snprintf(path, sizeof(path), "../assets/text/%s/set%d/phase%d/narration.txt", 
             story->day_folder, story->current_set_idx + 1, story->current_phase_idx + 1);
    
    FILE* file = fopen(path, "r");
    if (!file) return;

    phase->narration_count = 0;
    phase->phone_message_count = 0;
    phase->has_ending = false;
    phase->ending_file[0] = '\0';
    
    char line[256];
    
    bool execute_branch[10];
    bool branch_satisfied[10];
    for (int i = 0; i < 10; i++) {
        execute_branch[i] = true;
        branch_satisfied[i] = false;
    }
    
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        
        int indent = 0;
        while (line[indent] == ' ') indent++;
        int level = indent / 4;
        if (level > 9) level = 9;
        
        // Ensure we are active at all parent levels
        bool parent_active = true;
        for (int i = 0; i < level; i++) {
            if (!execute_branch[i]) {
                parent_active = false;
                break;
            }
        }
        
        char trimmed[256];
        char* start = line + indent;
        char* end = start + strlen(start) - 1;
        while (end > start && (*end == '\n' || *end == '\r')) { *end = '\0'; end--; }
        strncpy(trimmed, start, 255);
        
        if (strncmp(trimmed, "[IF]", 4) == 0) {
            char* cond_str = trimmed + 5;
            while (*cond_str == ' ') cond_str++;
            
            if (!parent_active) {
                execute_branch[level] = false;
                branch_satisfied[level] = true;
            } else {
                bool cond_eval = EvaluateCondition(game_context, cond_str);
                execute_branch[level] = cond_eval;
                branch_satisfied[level] = cond_eval;
            }
            continue;
        } else if (strncmp(trimmed, "[ELSE IF]", 9) == 0) {
            char* cond_str = trimmed + 10;
            while (*cond_str == ' ') cond_str++;
            
            if (!parent_active || branch_satisfied[level]) {
                execute_branch[level] = false;
            } else {
                bool cond_eval = EvaluateCondition(game_context, cond_str);
                execute_branch[level] = cond_eval;
                if (cond_eval) branch_satisfied[level] = true;
            }
            continue;
        } else if (strncmp(trimmed, "[ELSE]", 6) == 0) {
            if (!parent_active || branch_satisfied[level]) {
                execute_branch[level] = false;
            } else {
                execute_branch[level] = true;
                branch_satisfied[level] = true;
            }
            continue;
        }
        
        if (!parent_active) continue;

        // Valid block: parsed narration lines (we only parse what's needed for phase2)
        if (strncmp(trimmed, "[RESPONSE]", 10) == 0) {
            char* resp = trimmed + 10;
            while (*resp == ' ') resp++;
            if (phase->narration_count < 40) {
                strncpy(phase->narration_lines[phase->narration_count].text, resp, 255);
                ReplaceNewlines(phase->narration_lines[phase->narration_count].text);
                phase->narration_lines[phase->narration_count].type = 0; // text
                phase->narration_lines[phase->narration_count].sanity_change = 0;
                phase->narration_count++;
            }
        } else if (strncmp(trimmed, "[PLAY]", 6) == 0) {
            char* sound = trimmed + 6;
            while (*sound == ' ') sound++;
            if (phase->narration_count < 40) {
                strncpy(phase->narration_lines[phase->narration_count].text, sound, 255);
                phase->narration_lines[phase->narration_count].type = 1; // play_sound
                phase->narration_lines[phase->narration_count].sanity_change = 0;
                phase->narration_count++;
            }
        } else if (strncmp(trimmed, "[SANITY]", 8) == 0) {
            int points = 0;
            if (strstr(trimmed, "--")) points = -20;
            else if (strstr(trimmed, "++")) points = 20;
            else if (strstr(trimmed, "-")) points = -10;
            else if (strstr(trimmed, "+")) points = 10;
            
            if (phase->narration_count > 0 && phase->narration_lines[phase->narration_count-1].type == 0) {
                phase->narration_lines[phase->narration_count-1].sanity_change += points;
            } else {
                // Prepend to next line
                if (phase->narration_count < 40) {
                    phase->narration_lines[phase->narration_count].text[0] = '\0';
                    phase->narration_lines[phase->narration_count].type = 4; // invisible modifiers
                    phase->narration_lines[phase->narration_count].sanity_change = points;
                    phase->narration_count++;
                }
            }
        } else if (strncmp(trimmed, "[PHONE]", 7) == 0) {
            char* sender = trimmed + 7;
            while (*sender == ' ') sender++;
            strncpy(phase->phone_sender, sender, 63);
            if (phase->narration_count < 40) {
                strncpy(phase->narration_lines[phase->narration_count].text, "PHONE_START", 255);
                phase->narration_lines[phase->narration_count].type = 3; // phone_start
                phase->narration_lines[phase->narration_count].sanity_change = 0;
                phase->narration_count++;
            }
        } else if (strncmp(trimmed, "[MESSAGE]", 9) == 0) {
            char* msg = trimmed + 9;
            while (*msg == ' ') msg++;
            if (phase->phone_message_count < 32) {
                strncpy(phase->phone_messages[phase->phone_message_count].text, msg, 127);
                ReplaceNewlines(phase->phone_messages[phase->phone_message_count].text);
                phase->phone_messages[phase->phone_message_count].choice_count = 0;
                phase->phone_message_count++;
            }
        } else if (strncmp(trimmed, "[TRIGGER_ENDING]", 16) == 0) {
            char* filename = trimmed + 16;
            while (*filename == ' ') filename++;
            strncpy(phase->ending_file, filename, 127);
            phase->has_ending = true;
            TraceLog(LOG_INFO, "PARSER: [TRIGGER_ENDING] parsed -> %s (has_ending=true)", filename);
        }
    }
    
    TraceLog(LOG_INFO, "PARSER DONE: narr_count=%d, phone_count=%d, has_ending=%d, ending_file=%s, sanity=%.0f",
             phase->narration_count, phase->phone_message_count, phase->has_ending, phase->ending_file,
             game_context->player ? game_context->player->sanity : -1);
    fclose(file);
}

static void LoadEndingSequence(StorySystem* story, StoryPhase* phase) {
    if (!story || !phase || !phase->has_ending) return;
    
    char path[256];
    snprintf(path, sizeof(path), "../assets/text/%s/set%d/phase%d/%s",
             story->day_folder, story->current_set_idx + 1, story->current_phase_idx + 1,
             phase->ending_file);
    
    FILE* file = fopen(path, "r");
    if (!file) {
        TraceLog(LOG_WARNING, "ENDING: Could not open ending file: %s", path);
        return;
    }
    
    story->ending_line_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), file) && story->ending_line_count < 80) {
        // Trim trailing newlines/carriage returns
        char* end = line + strlen(line) - 1;
        while (end >= line && (*end == '\n' || *end == '\r')) { *end = '\0'; end--; }
        
        // Skip empty lines
        if (strlen(line) == 0) continue;
        
        strncpy(story->ending_lines[story->ending_line_count], line, 255);
        story->ending_line_count++;
    }
    fclose(file);
    
    story->ending_active = true;
    story->ending_show_credits = false;
    story->ending_current_line = 0;
    story->ending_typing_timer = 0.0f;
    story->ending_typing_index = 0;
    
    TraceLog(LOG_INFO, "ENDING: Loaded %d lines from %s", story->ending_line_count, phase->ending_file);
}

void HandleEndingInput(struct GameContext* game_context, int* game_state, struct Audio* game_audio) {
    StorySystem* story = &game_context->story;
    
    if (!story->ending_active) return;
    
    // Credits screen — SPACE returns to main menu
    if (story->ending_show_credits) {
        if (IsKeyPressed(KEY_SPACE)) {
            story->ending_active = false;
            story->ending_show_credits = false;
            DeleteSaveData();
            *game_state = MAINMENU;
        }
        return;
    }
    
    // Typing effect logic
    const char* current_text = NULL;
    if (story->ending_current_line < story->ending_line_count) {
        current_text = story->ending_lines[story->ending_current_line];
    }
    
    int text_len = current_text ? strlen(current_text) : 0;
    if (current_text && story->ending_typing_index < text_len) {
        story->ending_typing_timer += GetFrameTime();
        if (story->ending_typing_timer >= 0.03f) {
            story->ending_typing_timer = 0.0f;
            story->ending_typing_index++;
            if (story->ending_typing_index >= text_len) {
                if (game_audio && IsSoundPlaying(game_audio->typing_sound)) StopSound(game_audio->typing_sound);
            } else if (game_audio && !IsSoundPlaying(game_audio->typing_sound)) {
                PlaySound(game_audio->typing_sound);
            }
        }
    }
    
    // SPACE key handling
    if (IsKeyPressed(KEY_SPACE)) {
        if (story->ending_typing_index < text_len) {
            // Skip typing effect
            story->ending_typing_index = text_len;
            if (game_audio && IsSoundPlaying(game_audio->typing_sound)) StopSound(game_audio->typing_sound);
        } else {
            // Advance to next line
            story->ending_current_line++;
            story->ending_typing_index = 0;
            story->ending_typing_timer = 0.0f;
            
            // If past last line, show credits
            if (story->ending_current_line >= story->ending_line_count) {
                story->ending_show_credits = true;
            }
        }
    }
}
