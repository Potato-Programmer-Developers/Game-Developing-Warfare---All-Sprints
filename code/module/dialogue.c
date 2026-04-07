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

// Global registry for 'once-only' responses
typedef struct {
    char filename[128];
    int index;
} UsedRegistry;

static UsedRegistry used_responses[256];
static int used_count = 0;

static bool IsResponseUsed(const char* filename, int index){
    for (int i = 0; i < used_count; i++) {
        if (strcmp(used_responses[i].filename, filename) == 0 && used_responses[i].index == index) {
            return true;
        }
    }
    return false;
}

static void MarkResponseUsed(const char* filename, int index){
    if (used_count < 256) {
        strncpy(used_responses[used_count].filename, filename, 127);
        used_responses[used_count].index = index;
        used_count++;
    }
}

// Helper to count leading spaces for indentation detection
static int GetIndentation(const char* line) {
    int count = 0;
    while (line[count] == ' ') count++;
    return count;
}

void LoadInteraction(const char* filename, Dialogue* dialogue, struct GameContext* context) {
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

    // Parse dialogue file
    while (fgets(raw_line, sizeof(raw_line), file)) {
        int line_indent = GetIndentation(raw_line);
        char* line = raw_line + line_indent;
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;

        // Determine hierarchy based on indentation
        while (stack_ptr > 1 && line_indent <= indent_stack[stack_ptr - 1]) {
            stack_ptr--;
        }

        int current_parent_idx = node_stack[stack_ptr - 1];

        // 4. Fade Tag
        if (strstr(line, "[FADE]")) {
            char color[32], path[128], loc[32];
            if (sscanf(strstr(line, "[FADE]"), "[FADE] %s %s %s", color, path, loc) == 3) {
                strncpy(dialogue->nodes[current_parent_idx].fade_color, color, 31);
                strncpy(dialogue->nodes[current_parent_idx].target_map, path, 127);
                strncpy(dialogue->nodes[current_parent_idx].target_loc, loc, 31);
            }
        }
        // 5. Phone Notification Tag
        if (strstr(line, "[PHONE]")) {
            dialogue->nodes[current_parent_idx].triggers_phone = true;
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
            
            int r_idx = dialogue->nodes[current_parent_idx].response_count;
            if (r_idx < 10) {
                strncpy(dialogue->nodes[current_parent_idx].responses[r_idx], text, MAX_LINE_LENGTH - 1);
                dialogue->nodes[current_parent_idx].response_count++;
            }
        }
        // Read simple sequential lines if it's a conversation block
        else if (dialogue->nodes[current_parent_idx].is_conversation && line[0] != '[') {
            int r_idx = dialogue->nodes[current_parent_idx].response_count;
            if (r_idx < 10) {
                strncpy(dialogue->nodes[current_parent_idx].responses[r_idx], line, MAX_LINE_LENGTH - 1);
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
                            for (int m = 0; m < context->met_npc_count; m++) {
                                if (strstr(context->met_npcs[m], target_npc)) { 
                                    if (context->met_npc_set[m] < context->story.current_set_idx || 
                                       (context->met_npc_set[m] == context->story.current_set_idx && context->met_npc_phase[m] < context->story.current_phase_idx)) {
                                        met = true;
                                    }
                                    break; 
                                }
                            }
                        }
                        if (!met) continue; // HIDDEN per user request
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
