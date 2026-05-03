/**
 * @file dialogue.c
 * @brief Handles dialogue system initialization, tree-structure parsing, and response picking.
 * 
 * Update History:
 * - 2026-03-22: Foundational parsing of `[RESPONSE]` and basic line buffers. (Goal: Support 
 *                static NPC text.)
 * - 2026-04-03: Implemented the "Indentation-based Tree Parser." (Goal: Support branching 
 *                conversations where player choices lead to child nodes.)
 * - 2026-04-05: Integrated state-based transitions and "Sanity/Karma" effects. (Goal: Bridge 
 *                conversations to game mechanics, allowing NPC interaction to alter player stats.)
 * - 2026-04-07: Implemented "Phone Notification" tags and "Sequential Follow-ups." (Goal: 
 *                Allow dialogues to trigger phase-based phone messages and merge choice-leaves 
 *                into sequential conversations.)
 * - 2026-05-02: Implemented inline `SANITY` threshold gating for dialogue choices. (Goal: Support
 *                `| SANITY > 50` syntax on `[CHOICE]` labels to dynamically hide choices when the
 *                player's sanity does not meet the threshold, enabling the Saul ending branch where
 *                certain options only appear if sanity is sufficiently high.)
 * - 2026-05-02: Added `[IF] KARMA`, `[IF] CORRECT PLANTED`, and `[IF] BOX_SCORE` conditional blocks.
 *                (Goal: Support conditional dialogue branching based on NPC karma values,
 *                correctly-planted pot counts, and box puzzle scores for Day 3 and Day 4 interactions.)
 * - 2026-05-02: Added `[TRIGGER_ENDING]` tag parsing in dialogue nodes. (Goal: Allow a dialogue
 *                choice to directly trigger an ending sequence by specifying the ending script filename,
 *                used for farmer and Saul endings.)
 * - 2026-05-02: Expanded response buffer from 10 to 32. (Goal: Support longer NPC conversations
 *                in Day 3 and Day 4 farmer dialogues that exceed the previous 10-line limit.)
 * - 2026-05-02: Changed implicit line parsing to auto-set `is_conversation = true`. (Goal: Lines
 *                without explicit tags are now treated as conversation responses automatically,
 *                simplifying dialogue file authoring.)
 * 
 * Revision Details:
 * - Added a recursive-like stack parser in `LoadInteraction` to handle multi-level indentation.
 * - Implemented `PickResponse` for randomized NPC variations.
 * - Created `PrepareNodeText` to format dialogue for the UI system.
 * - Added serialization for `pending_target_map` within the dialogue node structure.
 * - Added a leaf-node linking loop that connects the `next_node` of all previous choices 
 *    to the new conversation parent, ensuring a unified flow.
 * - Optimized character buffer handling for narrative checks.
 * - Added `[PHONE]` tag parsing to `LoadInteraction`, mapping it to `DialogueNode.triggers_phone`.
 * - Implemented root-level `[CONVERSATION]` linking to bridge different dialogue blocks.
 * - Added `| SANITY > N` / `| SANITY < N` parsing on `[CHOICE]` labels: extracts operator and
 *    threshold via `sscanf`, evaluates against `context->player->sanity`, and hides the choice
 *    (via `skip_block`) if not met. The tag text is stripped from the displayed label.
 * - Added `[IF] KARMA` block parsing using `GetAssetKarma(interactable_id)` with `>`, `<`, `==`.
 * - Added `[IF] CORRECT PLANTED` block parsing that counts correctly-planted pots.
 * - Added `[IF] BOX_SCORE` block parsing using `context->left_box_big + context->right_box_small`.
 * - Added `[TRIGGER_ENDING]` tag parsing on dialogue nodes, storing the filename in `trigger_ending_file`.
 * - Expanded `response_count` bounds check from `r_idx < 10` to `r_idx < 32` in both `[RESPONSE]`
 *    and implicit line parsing blocks.
 * - Changed implicit line parsing (`else if (line[0] != '[')`) to set `is_conversation = true`.
 * 
 * Authors: Andrew Zhuo and Cornelius Jabez Lim
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include "dialogue.h"
#include "game_context.h"
#include "assets.h"

// Helper: replace literal "\n" sequences with actual newline characters
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

bool IsResponseUsed(struct GameContext* context, const char* text){
    if (!context || !text) return false;
    for (int i = 0; i < context->used_lines_count; i++) {
        if (strncmp(context->dialogue_used_lines[i].text, text, 63) == 0) {
            return true;
        }
    }
    return false;
}

void MarkResponseUsed(struct GameContext* context, const char* text){
    if (!context || !text) return;
    if (context->used_lines_count < 256) {
        strncpy(context->dialogue_used_lines[context->used_lines_count].text, text, 63);
        context->used_lines_count++;
    }
}

// Helper to count leading spaces for indentation detection
static int GetIndentation(const char* line) {
    int count = 0;
    while (line[count] == ' ') count++;
    return count;
}

void LoadInteraction(const char* filename, Dialogue* dialogue, struct GameContext* context, const char* interactable_id) {
    if (!dialogue) return;
    memset(dialogue, 0, sizeof(Dialogue));
    for (int i = 0; i < MAX_DIALOGUE_LINES; i++) {
        for (int j = 0; j < 4; j++) dialogue->nodes[i].child_nodes[j] = -1;
        dialogue->nodes[i].next_node = -1;
    }

    FILE* file = fopen(filename, "r");
    if (!file) return;

    char raw_line[MAX_LINE_LENGTH];
    int node_stack[32];               // Stack of parent node indices
    int indent_stack[32];             // Indentation levels corresponding to the stack
    int stack_ptr = 0;

    // Start with a root node
    dialogue->node_count = 1;
    node_stack[stack_ptr] = 0;
    indent_stack[stack_ptr] = -1;     // Root is above any real indent
    stack_ptr++;

    // Track the current 'root' block
    int current_block_root = 0;
    bool skip_block = false;
    int skip_indent = -1;
    bool chain_met = false;

    // Parse dialogue file
    while (fgets(raw_line, sizeof(raw_line), file)) {
        int line_indent = GetIndentation(raw_line);
        char* line = raw_line + line_indent;
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;

        if (strstr(line, "[IF] PLANTED")) {
            bool is_planted = false;
            if (context && interactable_id) {
                for (int i = 0; i < 18; i++) {
                    if (strcmp(context->pot_registry[i].pot_id, interactable_id) == 0) {
                        is_planted = context->pot_registry[i].is_planted;
                        break;
                    }
                }
            }
            skip_block = !is_planted;
            skip_indent = line_indent;
            chain_met = is_planted;
            continue;
        } else if (strstr(line, "[IF] TIME")) {
            float limit = 0;
            char op[4] = {0};
            bool met = false;
            if (sscanf(line, "[IF] TIME %s %f", op, &limit) == 2) {
                if (strcmp(op, "<") == 0) met = (context->day3_mowing_timer < limit);
                else if (strcmp(op, ">") == 0) met = (context->day3_mowing_timer > limit);
            }
            skip_block = !met;
            skip_indent = line_indent;
            chain_met = met;
            continue;
        } else if (strstr(line, "[IF] CORRECT LOGS") || strstr(line, "[ELSE IF] CORRECT LOGS")) {
            int is_else = (strstr(line, "[ELSE") != NULL);
            if (is_else && skip_indent != -1 && line_indent == skip_indent) {
                if (chain_met) { skip_block = true; continue; }
            }
            
            int val = 0;
            if (context) val = context->left_box_big + context->right_box_small;
            bool met = false;
            
            int target = 0;
            if (strstr(line, "<")) {
                sscanf(strstr(line, "<") + 1, "%d", &target);
                met = (val < target);
            } else if (strstr(line, ">")) {
                sscanf(strstr(line, ">") + 1, "%d", &target);
                met = (val > target);
            } else if (strstr(line, "==")) {
                sscanf(strstr(line, "==") + 2, "%d", &target);
                met = (val == target);
            }
            
            skip_block = !met;
            if (!is_else) skip_indent = line_indent;
            chain_met = met;
            continue;
        } else if (strstr(line, "[IF] CORRECT PLANTED") || strstr(line, "[ELSE IF] CORRECT PLANTED")) {
            int is_else = (strstr(line, "[ELSE") != NULL);
            if (is_else && skip_indent != -1 && line_indent == skip_indent) {
                if (chain_met) { skip_block = true; continue; }
            }
            
            int val = 0;
            if (context) {
                for (int i = 0; i < 18; i++) {
                    if (context->pot_registry[i].is_planted) {
                        if (strstr(context->pot_registry[i].pot_id, "green_pot") && context->pot_registry[i].seed_type == 1) val++;
                        else if (strstr(context->pot_registry[i].pot_id, "red_pot") && context->pot_registry[i].seed_type == 2) val++;
                        else if (strstr(context->pot_registry[i].pot_id, "orange_pot") && context->pot_registry[i].seed_type == 3) val++;
                    }
                }
            }
            bool met = false;
            
            int target = 0;
            if (strstr(line, "<")) {
                sscanf(strstr(line, "<") + 1, "%d", &target);
                met = (val < target);
            } else if (strstr(line, ">")) {
                sscanf(strstr(line, ">") + 1, "%d", &target);
                met = (val > target);
            } else if (strstr(line, "==")) {
                sscanf(strstr(line, "==") + 2, "%d", &target);
                met = (val == target);
            }
            
            skip_block = !met;
            if (!is_else) skip_indent = line_indent;
            chain_met = met;
            continue;
        } else if (strstr(line, "[IF] KARMA") || strstr(line, "[ELSE IF] KARMA")) {
            int is_else = (strstr(line, "[ELSE") != NULL);
            if (is_else && skip_indent != -1 && line_indent == skip_indent) {
                if (chain_met) { skip_block = true; continue; }
            }
            
            int val = 0;
            if (interactable_id) val = GetAssetKarma(interactable_id);
            bool met = false;
            
            int target = 0;
            if (strstr(line, "<")) {
                sscanf(strstr(line, "<") + 1, "%d", &target);
                met = (val < target);
            } else if (strstr(line, ">")) {
                sscanf(strstr(line, ">") + 1, "%d", &target);
                met = (val > target);
            } else if (strstr(line, "==")) {
                sscanf(strstr(line, "==") + 2, "%d", &target);
                met = (val == target);
            }
            
            skip_block = !met;
            if (!is_else) skip_indent = line_indent;
            chain_met = met;
            continue;
        } else if (strstr(line, "_TALKED")) {
            char* if_start = strstr(line, "[IF] ");
            if (if_start) {
                char npc_name[64] = {0};
                char* talked_pos = strstr(if_start + 5, "_TALKED");
                if (talked_pos) {
                    int name_len = (int)(talked_pos - (if_start + 5));
                    if (name_len > 0 && name_len < 64) {
                        strncpy(npc_name, if_start + 5, name_len);
                        npc_name[name_len] = '\0';
                        for (int i = 0; npc_name[i]; i++) if (npc_name[i] >= 'A' && npc_name[i] <= 'Z') npc_name[i] += 32;
                    }
                }
                bool met = false;
                if (context && npc_name[0]) {
                    int current_day = 1;
                    if (sscanf(context->story.day_folder, "day%d", &current_day) != 1) current_day = 1;

                    for (int m = 0; m < context->met_npc_count; m++) {
                        if (strstr(context->met_npcs[m], npc_name)) {
                            if (context->met_npc_day[m] < current_day) {
                                met = true;
                            } else if (context->met_npc_day[m] == current_day) {
                                if (context->met_npc_set[m] < context->story.current_set_idx || 
                                   (context->met_npc_set[m] == context->story.current_set_idx && context->met_npc_phase[m] < context->story.current_phase_idx)) {
                                    met = true;
                                }
                            }
                            break;
                        }
                    }
                }
                bool expects_false = (strstr(line, "== FALSE") != NULL);
                bool condition_met = expects_false ? !met : met;
                skip_block = !condition_met;
                skip_indent = line_indent;
                chain_met = condition_met;
            }
            continue;
        } else if (strstr(line, "[ELSE IF] TIME")) {
            if (skip_indent != -1 && line_indent == skip_indent) {
                if (chain_met) skip_block = true;
                else {
                    float limit = 0; char op[4] = {0}; bool met = false;
                    if (sscanf(line, "[ELSE IF] TIME %s %f", op, &limit) == 2) {
                        if (strcmp(op, "<") == 0) met = (context->day3_mowing_timer < limit);
                        else if (strcmp(op, ">") == 0) met = (context->day3_mowing_timer > limit);
                    }
                    skip_block = !met;
                    chain_met = met;
                }
            }
            continue;
        } else if (strstr(line, "[ELSE]")) {
            if (skip_indent != -1 && line_indent == skip_indent) {
                skip_block = chain_met;
            }
            continue;
        }

        if (skip_block && line_indent > skip_indent) {
            continue;
        } else if (line_indent <= skip_indent) {
            skip_block = false;
            skip_indent = -1;
        }

        // Determine hierarchy based on indentation
        while (stack_ptr > 1 && line_indent <= indent_stack[stack_ptr - 1]) {
            stack_ptr--;
        }

        int current_parent_idx = node_stack[stack_ptr - 1];

        // 4. Phone Notification Tag
        if (strstr(line, "[PHONE]")) {
            dialogue->nodes[current_parent_idx].triggers_phone = true;
        }
        // 5. Trigger Ending Tag
        if (strstr(line, "[TRIGGER_ENDING]")) {
            char filename[64];
            if (sscanf(strstr(line, "[TRIGGER_ENDING]"), "[TRIGGER_ENDING] %s", filename) == 1) {
                strncpy(dialogue->nodes[current_parent_idx].trigger_ending_file, filename, 63);
            }
        }
        if (strstr(line, "[CONVERSATION]")) {
            // If we are at root level and have already defined a block (choices or sequence)
            if (stack_ptr == 1 && (dialogue->nodes[current_block_root].choice_count > 0 || dialogue->nodes[current_block_root].response_count > 0)) {
                int new_root = dialogue->node_count++;
                // Link "leaves" of the previous block (choices with response) to this new root
                for (int n = current_block_root; n < new_root; n++) {
                    // Leaves are nodes that don't have further child choices
                    if (dialogue->nodes[n].choice_count == 0 && n != new_root) {
                        dialogue->nodes[n].next_node = new_root;
                    }
                }
                current_block_root = new_root;
                node_stack[0] = new_root;
                current_parent_idx = new_root;
            }
            dialogue->nodes[current_parent_idx].is_conversation = true;
        } else if (strstr(line, "[RESPONSE]")) {
            char* text = strstr(line, "[RESPONSE]") + 10;
            while (*text == ' ') text++;
            
            bool is_once = false;
            char* once_tag = strstr(text, "| 1");
            if (once_tag) {
                is_once = true;
                *once_tag = '\0';
                // Trim trailing space before | 1
                char* end = once_tag - 1;
                while(end > text && *end == ' ') { *end = '\0'; end--; }
            }
            
            int r_idx = dialogue->nodes[current_parent_idx].response_count;
            if (r_idx < 32) {
                strncpy(dialogue->nodes[current_parent_idx].responses[r_idx], text, MAX_LINE_LENGTH - 1);
                ReplaceNewlines(dialogue->nodes[current_parent_idx].responses[r_idx]);
                dialogue->nodes[current_parent_idx].response_once[r_idx] = is_once;
                dialogue->nodes[current_parent_idx].response_count++;
            }
        }
        // Read simple sequential lines implicitly as a conversation block
        else if (line[0] != '[') {
            dialogue->nodes[current_parent_idx].is_conversation = true;
            bool is_once = false;
            char* once_tag = strstr(line, "| 1");
            if (once_tag) {
                is_once = true;
                *once_tag = '\0';
                // Trim trailing space before | 1
                char* end = once_tag - 1;
                while(end > line && *end == ' ') { *end = '\0'; end--; }
            }
            int r_idx = dialogue->nodes[current_parent_idx].response_count;
            if (r_idx < 32) {
                strncpy(dialogue->nodes[current_parent_idx].responses[r_idx], line, MAX_LINE_LENGTH - 1);
                ReplaceNewlines(dialogue->nodes[current_parent_idx].responses[r_idx]);
                dialogue->nodes[current_parent_idx].response_once[r_idx] = is_once;
                dialogue->nodes[current_parent_idx].response_count++;
            }
        }
        // 2. Player Choice Tag
        else if (strstr(line, "[CHOICE")) {
            int c_idx = -1;
            if (sscanf(line, "[CHOICE%d]", &c_idx) == 1 || sscanf(line, "[CHOICE %d]", &c_idx) == 1) {
                // If we are at root level (stack_ptr == 1) and we see CHOICE1 but our current block already has choices,
                // it MUST be a new block of root choices following a previous sequence.
                if (stack_ptr == 1 && c_idx == 1 && dialogue->nodes[current_block_root].choice_count > 0) {
                    int new_root = dialogue->node_count++;
                    // Link all "leaves" of the previous block to this new root
                    for (int n = current_block_root; n < new_root; n++) {
                        if (dialogue->nodes[n].choice_count == 0) {
                            dialogue->nodes[n].next_node = new_root;
                        }
                    }
                    current_block_root = new_root;
                    node_stack[0] = new_root;
                    current_parent_idx = new_root;
                }

                // Adjust for 0-indexing
                int choice_slot = dialogue->nodes[current_parent_idx].choice_count;
                if (choice_slot < 4) {
                    char* label = strchr(line, ']') + 1;
                    while (*label == ' ') label++;

                    // Check for deposit tags before displaying text
                    char* deposit_start = strstr(label, "[DEPOSIT:");
                    if (deposit_start) {
                        char deposit_item[64];
                        if (sscanf(deposit_start, "[DEPOSIT:%63[^]]]", deposit_item) == 1) {
                            strncpy(dialogue->nodes[current_parent_idx].deposit_tags[choice_slot], deposit_item, 63);
                        }
                        char* tag_end = strchr(deposit_start, ']');
                        if (tag_end) {
                            tag_end++;
                            while (*tag_end == ' ') tag_end++;
                            memmove(deposit_start, tag_end, strlen(tag_end) + 1);
                        }
                    }

                    // Check for visibility condition: (unable to choose this if the player didn’t talk to saul earlier)
                    if (strstr(label, "(unable to choose this if the player didn’t talk to")) {
                        char target_npc[64];
                        // Basic extraction: "talk to " -> npc name
                        char* npc_start = strstr(label, "talk to ") + 8;
                        sscanf(npc_start, "%s", target_npc);
                        
                        bool met = false;
                        if (context) {
                            int current_day = 1;
                            if (sscanf(context->story.day_folder, "day%d", &current_day) != 1) current_day = 1;

                            for (int m = 0; m < context->met_npc_count; m++) {
                                if (strstr(context->met_npcs[m], target_npc)) { 
                                    if (context->met_npc_day[m] < current_day) {
                                        met = true;
                                    } else if (context->met_npc_day[m] == current_day) {
                                        if (context->met_npc_set[m] < context->story.current_set_idx || 
                                           (context->met_npc_set[m] == context->story.current_set_idx && context->met_npc_phase[m] < context->story.current_phase_idx)) {
                                            met = true;
                                        }
                                    }
                                    break; 
                                }
                            }
                        }
                        if (!met) continue; // HIDDEN per user request
                    }

                    // Check for inline sanity condition
                    char* sanity_tag = strstr(label, "| SANITY");
                    if (sanity_tag) {
                        char op;
                        int target_val;
                        if (sscanf(sanity_tag, "| SANITY %c %d", &op, &target_val) == 2 && context) {
                            bool met = false;
                            if (op == '<') met = (context->player->sanity < target_val);
                            else if (op == '>') met = (context->player->sanity > target_val);
                            else if (op == '=') met = (context->player->sanity == target_val);
                            
                            if (!met) {
                                skip_block = true;
                                skip_indent = line_indent;
                                continue; // Skip this choice and its children
                            }
                        }
                        // Remove the tag from the label so it doesn't show in UI
                        *sanity_tag = '\0';
                        char* end = sanity_tag - 1;
                        while(end > label && *end == ' ') { *end = '\0'; end--; }
                    }

                    // Metadata tags
                    if (strstr(label, "(-Johnny)")) dialogue->nodes[current_parent_idx].karma_change -= 10;
                    if (strstr(label, "(+Johnny)")) dialogue->nodes[current_parent_idx].karma_change += 10;

                    strncpy(dialogue->nodes[current_parent_idx].choices[choice_slot], label, 63);
                    
                    // Create a new child node for this choice
                    int child_idx = dialogue->node_count++;

                    dialogue->nodes[current_parent_idx].child_nodes[choice_slot] = child_idx;
                    dialogue->nodes[current_parent_idx].choice_count++;

                    // Push this child to the stack - it will receive the [RESPONSE] at the NEXT level
                    node_stack[stack_ptr] = child_idx;
                    indent_stack[stack_ptr] = line_indent;
                    stack_ptr++;
                }
            }
        }
        // 3. Side-effects
        else if (strstr(line, "[SANITY]")) {
            int delta = 0;
            if (strstr(line, "--")) delta = -20;
            else if (strstr(line, "-")) delta = -10;
            else if (strstr(line, "++")) delta = 20;
            else if (strstr(line, "+")) delta = 10;
            dialogue->nodes[current_parent_idx].sanity_change = delta;
        } else if (strstr(line, "[KARMA]")) {
            int delta = 0;
            if (strstr(line, "++")) delta = 20;
            else if (strstr(line, "--")) delta = -20;
            else if (strstr(line, "+")) delta = 10;
            else if (strstr(line, "-")) delta = -10;
            dialogue->nodes[current_parent_idx].karma_change = delta;
        } else if (strstr(line, "[SET_PLANTED:")) {
            int seed = 0;
            if (sscanf(strstr(line, "[SET_PLANTED:"), "[SET_PLANTED: %d]", &seed) == 1) {
                dialogue->nodes[current_parent_idx].plant_seed_type = seed;
            }
        }
    }

    fclose(file);
    dialogue->current_node_idx = 0;
}

ResponseGroup LoadResponseGroup(const char* filename){
    ResponseGroup group = {0};
    FILE* file = fopen(filename, "r");
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file) && group.count < 16){
        // Remove newline characters
        line[strcspn(line, "\r\n")] = 0;

        // Parse dialogue file
        if (strstr(line, "[RESPONSE]")){
            char* text_start = strstr(line, "[RESPONSE]") + 10;
            while (*text_start == ' ') text_start++;
            char* once_tag = strstr(text_start, "| 1");

            // Check if the response is once-only
            if (once_tag){
                group.responses[group.count].once = true;
                *once_tag = '\0';
            }
            strncpy(group.responses[group.count].text, text_start, MAX_LINE_LENGTH - 1);
            group.count++;
        }
    }
    fclose(file);
    return group;
}

const char* PickResponse(ResponseGroup* group, const char* filename){
    // Pick a response based on the random-choice
    if (group->count == 0) return "No response.";
    static bool seeded = false;
    if (!seeded) {srand((unsigned int)time(NULL)); seeded = true;}
    int choice_idx = rand() % group->count;
    return group->responses[choice_idx].text;
}
