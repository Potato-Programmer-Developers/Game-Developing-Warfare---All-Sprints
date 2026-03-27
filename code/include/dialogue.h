/**
 * @file dialogue.h
 * @brief Function prototypes and definitions for the dialogue system.
 * 
 * Defines the structure for managing multi-line dialogue boxes and 
 * handles loading narrative text from external files.
 * 
 * Authors: Cornelius Jabez Lim
 */

#ifndef DIALOGUE_H
#define DIALOGUE_H

#define MAX_DIALOGUE_LINES 100
#define MAX_LINE_LENGTH 256

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
 * @brief Container for a dialogue sequence.
 * 
 * Stores lines of text and tracks the current reading progress.
 */
typedef struct {
    char lines[MAX_DIALOGUE_LINES][MAX_LINE_LENGTH];          // Buffer for dialogue strings
    int line_count;                                           // Total number of lines loaded
    int current_line;                                         // Index of the currently active line
    char choices[4][64];                                      // Branching choices
    char choice_responses[4][MAX_LINE_LENGTH];                // Follow-up text for each choice
    int choice_count;                                         // Number of available choices
    int selected_choice;                                      // Input from player
} Dialogue;

/**
 * @brief Parses a text file into a Dialogue object.
 * 
 * @param filename FS path to the .txt dialogue source.
 * @return Populated Dialogue struct. Empty struct if file load fails.
 */
Dialogue LoadDialogue(const char* filename);

/**
 * @brief Loads multiple randomized/once-only responses from a file.
 * 
 * @param filename FS path to the .txt response source.
 * @return Populated ResponseGroup struct. Empty struct if file load fails.
 */
ResponseGroup LoadResponseGroup(const char* filename);

/**
 * @brief Picks a response based on the random-choice and once-only rules.
 * 
 * @param group Pointer to the ResponseGroup object to pick a response from.
 * @param filename FS path to the .txt response source.
 * @return Returns a string pointer to the chosen response.
 */
const char* PickResponse(ResponseGroup* group, const char* filename);

/**
 * @brief Loads an interaction from a file.
 * 
 * @param filename FS path to the .txt interaction source.
 * @param dialogue Pointer to the Dialogue object to load the interaction into.
 */
void LoadInteraction(const char* filename, Dialogue* dialogue);

#endif