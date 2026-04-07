/**
 * @file dialogue.h
 * @brief Function prototypes and definitions for the dialogue system.
 * 
 * Defines the structure for managing multi-line dialogue boxes and 
 * handles loading narrative text from external files.
 * 
 * Authors: Andrew Zhuo and Cornelius Jabez Lim
 */

#ifndef DIALOGUE_H
#define DIALOGUE_H

#define MAX_DIALOGUE_LINES 100
#define MAX_LINE_LENGTH 256

struct GameContext;

/**
 * @brief Container for a story response.
 * 
 * Stores response text and tracks if it has been used.
 */
typedef struct {
    char text[MAX_LINE_LENGTH];     // Response text
    bool once;                      // If '| 1' exists, execute only once
    bool used;                      // Tracker for 'once' responses
} StoryResponse;

/**
 * @brief Container for a group of story responses.
 * 
 * Stores multiple story responses and tracks the current count.
 */
typedef struct {
    StoryResponse responses[16];    // All available responses in a file
    int count;                      // Current count
} ResponseGroup;

/**
 * @brief A single node in a conversation tree.
 */
typedef struct {
    char responses[10][MAX_LINE_LENGTH]; // NPC can say one of these (or all if conversation)
    int response_count;                  // Number of available responses
    bool is_conversation;                // If true, play responses sequentially
    char choices[4][64];                 // Player can say...
    char deposit_tags[4][64];            // Item ID to deposit if this choice is made
    int child_nodes[4];                  // Index of node to go to for each choice (-1 for end)
    int next_node;                       // Default next node to proceed to if no choices exist (-1 for dialogue end)
    int choice_count;                    // Current choices in this node
    int sanity_change;                   // Sanity impact of reaching this node
    int karma_change;                    // Karma impact of reaching this node
    char fade_color[32];                 // Custom fade color (BLACK, WHITE, etc)
    char target_map[128];                // Optional fade target
    char target_loc[32];                 // Optional fade location
} DialogueNode;


/**
 * @brief Container for a dialogue tree.
 */
typedef struct Dialogue {
    DialogueNode nodes[MAX_DIALOGUE_LINES];                    // Pool of nodes (replaces lines buffer)
    int node_count;                                            // Total number of nodes loaded
    int current_node_idx;                                      // Level/Node the player is currently at
    
    // Header-level fade (original functionality)
    char root_fade_color[32];                                  // Fade color
    char root_fade_target[128];                                // Fade target
    char root_fade_loc[32];                                    // Fade location

    // For backwards compatibility and simple multi-line text
    char lines[MAX_DIALOGUE_LINES][MAX_LINE_LENGTH];          // Buffer for dialogue strings
    int line_count;                                           // Total number of lines loaded
    int current_line;                                         // Index of the currently active line
    int selected_choice;                                      // Input from player

    // Pending Transition (to be triggered when DIALOGUE ends)
    char pending_fade_color[32];                              // Fade color
    char pending_target_map[128];                             // Fade target
    char pending_target_loc[32];                              // Fade location
} Dialogue;

/**
 * @brief Loads multiple randomized/once-only responses from a file.
 * 
 * @param filename Path to the .txt response source.
 * @return Populated ResponseGroup struct. Empty struct if file load fails.
 */
ResponseGroup LoadResponseGroup(const char* filename);

/**
 * @brief Picks a response based on the random-choice and once-only rules.
 * 
 * @param group Pointer to the ResponseGroup object to pick a response from.
 * @param filename Path to the .txt response source.
 * @return Returns a string pointer to the chosen response.
 */
const char* PickResponse(ResponseGroup* group, const char* filename);

/**
 * @brief Loads an interaction from a file.
 * 
 * @param filename FS path to the .txt interaction source.
 * @param dialogue Pointer to the Dialogue object to load the interaction into.
 */
void LoadInteraction(const char* filename, Dialogue* dialogue, struct GameContext* context);

#endif