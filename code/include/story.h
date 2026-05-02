/**
 * @file story.h
 * @brief Defines the core data structures and API for story sets, phases, and conditions.
 * 
 * Update History:
 * - 2026-03-20: Initial definition of `StorySet` and `StoryPhase`. (Goal: Establish the 
 *                architectural foundation for a phase-based narrative system.)
 * - 2026-04-03: Expanded `StoryCondition` types to include `CONDITION_PHONE_COMPLETE`. (Goal: Support 
 *                bi-directional communication between the story engine and external UI modules.)
 * - 2026-04-05: Added `phone_pending` and `narration_pending` flags to the `StorySystem` struct. (Goal: 
 *                Prevent race conditions and visual overlapping by deferring narrative prompts until 
 *                camera/map transitions are complete.)
 * - 2026-04-05: Integrated persistent state monitoring to the `StorySystem`. (Goal: Enable 
 *                seamless cross-day state tracking and automated file loading.)
 * - 2026-04-07: Implemented `narration_has_started` Tracking. (Goal: Ensure that 
 *                story phases ending in `NARRATION_COMPLETE` do not auto-skip before 
 *                the narration has actually been triggered.)
 * - 2026-04-10: Implemented `narration_loop_broken` flag for Day 2 SET4-PHASE1. (Goal: Allow a
 *                `[BREAK]` choice to end the narration loop early and satisfy `CONDITION_NARRATION_COMPLETE`
 *                without requiring all loop choices to be completed, preserving un-selected choices as
 *                `false` to reflect the player's actual decisions for downstream branching logic.)
 * - 2026-04-10: Expanded `StoryPhase` phone data capacity from 8 to 32 messages. (Goal: Support the
 *                deeply nested branching conversation tree format introduced in Day 2 SET4-PHASE3,
 *                where indentation-driven message hierarchies create many message nodes in a flat array.)
 * - 2026-05-02: Added ending sequence and scrolling credits system. (Goal: Support multi-ending
 *                narrative conclusions with typed dialogue scenes, followed by a scrolling credits
 *                sequence loaded from `credit.txt` with a dedicated credit music track and full
 *                lifecycle management including save deletion and MAINMENU re-entry.)
 * - 2026-05-02: Extended `CONDITION_ENTER_LOCATION` to support dual-location conditions. (Goal: Allow
 *                a phase to be completed by entering one of two possible locations, enabling branching
 *                story paths in Day 4 where the player can choose between the Forest or Farm ending.)
 * - 2026-05-02: Added conditional quest loading via inline `| CONDITION` syntax. (Goal: Dynamically
 *                show or hide quests based on the player's met-NPC history, so that Day 4 SET1-PHASE1
 *                only shows the Saul quest if the player has met Saul on both Day 1 and a subsequent day.)
 * 
 * Revision Details:
 * - Added `StoryConditionType` enum entries for `CONDITION_DREAM_COMPLETE` and `CONDITION_AUTO_COMPLETE`.
 * - Expanded `StoryCondition` struct to include a `met` flag for reliable state polling.
 * - Updated `StorySystem` to store the `day_folder` string for relative asset pathing.
 * - Prototyped `ReplaceNewlines` to be shared across narration and phone modules.
 * - Added `bool narration_has_started` to the `StorySystem` struct.
 * - Updated story condition logic to require a "started" state before marking as "complete".
 * - Added `bool narration_loop_broken` to `StorySystem` to track when a narration loop has been
 *    intentionally terminated via a `[BREAK]` choice. This flag bypasses the all-choices-completed
 *    requirement in `CONDITION_NARRATION_COMPLETE` evaluation, enabling early phase advancement.
 * - Expanded `StoryPhase.phone_messages` from 8 to 32 entries to accommodate branching conversation
 *    trees parsed from deeply nested `narration.txt` files.
 * - Expanded `StorySystem.phone_active_messages` from 8 to 32 entries to match the increased
 *    phase capacity during runtime phone playback.
 * - Added `ending_file[128]` and `has_ending` fields to `StoryPhase` for ending sequence support.
 * - Added ending sequence state to `StorySystem`: `ending_active`, `ending_show_credits`,
 *    `ending_lines[80][256]`, `ending_line_count`, `ending_current_line`, `ending_typing_timer`,
 *    `ending_typing_index`.
 * - Added scrolling credits state to `StorySystem`: `ending_credits_lines[128][128]`,
 *    `ending_credits_line_count`, `ending_credits_y`.
 * - Added `target_value2` to `StoryCondition` for dual-location `ENTER_LOCATION` conditions.
 * - Added `HandleEndingInput` and `TriggerEnding` function prototypes.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef STORY_H
#define STORY_H

#include "quest.h"
#include "phone.h"

struct GameContext;
struct Audio;

#define MAX_QUESTS_PER_PHASE 5
#define MAX_PHASES_PER_SET 5
#define MAX_SETS_PER_DAY 10

/**
 * @brief Types of conditions that can end a story phase.
 */
typedef enum StoryConditionType {
    CONDITION_ALL_QUESTS_COMPLETE,    // Advance when all phase quests are done
    CONDITION_INTERACT_OBJECT,        // Advance when specific object interacted
    CONDITION_TIME_PASS,              // Advance when time passes
    CONDITION_ENTER_LOCATION,         // Advance when entering a specific location
    CONDITION_COLLECT_OBJECTS,        // Advance when a number of unique objects are interacted with
    CONDITION_COLLIDE_OBJECTS,        // Advance when a number of objects have been collided with (e.g. mowed grass)
    CONDITION_NARRATION_COMPLETE,     // Advance when all narration lines and choices are done
    CONDITION_PHONE_COMPLETE,         // Advance when the interactive phone dialogue finishes
    CONDITION_DREAM_COMPLETE,         // Advance when the dream text sequence finishes
} StoryConditionType;

/**
 * @brief Types of locations in the game.
 */
typedef enum {
    STORY_LOC_NONE = -1,             // No location
    STORY_LOC_APARTMENT = 0,         // Apartment location
    STORY_LOC_EXTERIOR = 1,          // Exterior location
    STORY_LOC_INTERIOR = 2,          // Interior location
    STORY_LOC_FARM = 3,              // Farm location
    STORY_LOC_FOREST = 4,            // Forest location
} StoryLocation;

/**
 * @brief Represents the requirement to exit the current phase.
 */
typedef struct StoryCondition {
    StoryConditionType type;         // Type of condition
    char target_id[64];              // e.g., "fridge" or "farmer"
    float target_value;              // e.g., timer duration or expected choice index
    float target_value2;             // Optional secondary value (e.g. for OR locations)
    bool met;                        // Whether this specific condition is satisfied
    int current_count;               // Tracker for COLLECTION conditions
} StoryCondition;

/**
 * @brief Represents an interactable object in the game.
 */
typedef struct {
    char id[64];       // ID of the interactable object
    char type[32];     // "ITEM" or "NPC" or "DOOR"
} PhaseInteractable;

/**
 * @brief A single narration line entry.
 */
typedef struct {
    char text[256];          // Display text or sound name
    int type;                // 0=text, 1=play_sound, 2=loop_start, 3=phone_start
    int sanity_change;       // Sanity change on this line
} NarrationLine;

/**
 * @brief A choice inside a narration loop block.
 */
typedef struct {
    char label[64];          // Choice text
    char response[256];      // Response after choosing
    char state_key[32];      // GameContext field name to mutate
    bool state_value;        // Value to set
    bool is_break;           // Does this choice break the loop?
    bool completed;          // Has this choice been picked?
    bool only_one;           // Part of an [ONLY_ONE] choice group
} NarrationChoice;

/**
 * @brief A single narrative step with its own objectives and exit condition.
 */
typedef struct StoryPhase {
    char name[64];                           // Name of the phase
    StoryLocation location;                  // Location of the phase
    Quest quests[MAX_QUESTS_PER_PHASE];      // Quests in the phase
    int quest_count;                         // Number of quests in the phase
    PhaseInteractable interactables[20];     // Interactable objects in the phase
    int interactable_count;                  // Number of interactable objects in the phase
    bool force_narration;                    // Force narration skip based on interactable count?
    StoryCondition end_conditions[5];        // Multiple conditions to end the phase
    int condition_count;                     // Number of active conditions
    NarrationLine narration_lines[40];       // Interactive narration lines
    int narration_count;                     // Number of narration lines
    NarrationChoice narration_choices[8];    // Choices inside LOOP blocks
    int narration_choice_count;              // Number of loop choices
    // Per-phase phone message data
    char phone_sender[64];                   // Phone sender for this phase
    PhoneMessage phone_messages[32];         // Phone messages for this phase
    int phone_message_count;                 // Number of phone messages
    // Ending data
    char ending_file[128];                   // Ending script filename (e.g. "mike_ending1.txt")
    bool has_ending;                         // Whether this phase triggers a game ending
} StoryPhase;

/**
 * @brief A collection of phases grouped by scene or narrative beat.
 */
typedef struct StorySet {
    char name[64];                             // Name of the set
    int phase_count;                           // Number of phases in the set
    StoryPhase phases[MAX_PHASES_PER_SET];     // Phases in the set
} StorySet;

/**
 * @brief The top-level state for the current day's progression.
 */
typedef struct StorySystem {
    StorySet sets[MAX_SETS_PER_DAY];        // Sets in the day
    int set_count;                          // Number of sets in the day
    int current_set_idx;                    // Index of the current set
    int current_phase_idx;                  // Index of the current phase
    float phase_timer;                      // Timer for TIME_PASS conditions
    char day_folder[32];                    // e.g., "day1"
    
    // Interactive narration playback state
    bool narration_active;                  // Flag for active narration
    int narration_current_line;             // Current narration line index
    bool narration_in_loop;                 // Are we in the LOOP choice block?
    bool narration_showing_response;        // Showing a choice response?
    char narration_response_text[256];      // Current response text being shown
    bool narration_pending;                 // Narration waiting for fade/camera to settle
    bool narration_has_started;             // Has the current phase's narration ever been activated?
    bool narration_loop_broken;             // Has the narration loop been intentionally broken early?
    float narration_typing_timer;           // Timer for progressive typing effect
    int narration_typing_index;             // Current character index for typing effect

    // Phone playback state
    int phone_current_index;                // Currently showing message index
    float phone_message_timer;              // Timer for auto-advance
    bool phone_sequence_active;             // Is a multi-message sequence playing?
    char phone_active_sender[64];           // Sender for current phone sequence
    PhoneMessage phone_active_messages[32];  // Messages for current phone sequence
    int phone_active_count;                 // Message count for current sequence

    // Scene overlay state
    float scene_timer;                      // Timer for SCENE overlays
    char current_scene[32];                 // Text for the current scene (e.g., "FLASHBACK")
    bool phone_pending;                     // Flag to start interactive phone sequence

    // Ending sequence state
    bool ending_active;                     // Is an ending sequence playing?
    bool ending_show_credits;               // Show credits screen after ending
    char ending_lines[80][256];             // Ending text lines (speaker: text)
    int ending_line_count;                  // Total ending lines
    int ending_current_line;                // Currently displayed line index
    float ending_typing_timer;              // Typing effect timer
    int ending_typing_index;                // Typing effect character index
    char ending_credits_lines[128][128];    // Parsed credit text lines
    int ending_credits_line_count;          // Total credit lines
    float ending_credits_y;                 // Current Y position for scrolling
} StorySystem;

/**
 * @brief Parses a Day text file and populates the StorySystem.
 *
 * @param story Pointer to the StorySystem to populate.
 * @param path Path to the Day text file.
 */
void LoadStoryDay(StorySystem* story, const char* path, struct GameContext* game_context);

/**
 * @brief Checks phase progression every frame.
 *
 * @param game_context Pointer to the GameContext.
 * @param delta Time elapsed since the last frame.
 */
void UpdateStory(struct GameContext* game_context, float delta);

/**
 * @brief Force-advances the story to the next phase.
 *
 * @param game_context Pointer to the GameContext.
 */
void AdvanceStory(struct GameContext* game_context);

/**
 * @brief Returns a pointer to the current active phase.
 *
 * @param story Pointer to the StorySystem.
 * @return StoryPhase* Pointer to the current active phase.
 */
StoryPhase* GetActivePhase(StorySystem* story);

/**
 * @brief Triggers an ending sequence from a file.
 *
 * @param story Pointer to the StorySystem.
 * @param ending_file Name of the ending file to load.
 */
void TriggerEnding(StorySystem* story, const char* ending_file);

/**
 * @brief Handles player input during NARRATION_CUTSCENE state.
 *
 * @param game_context Pointer to the GameContext.
 * @param game_state Pointer to the current game state.
 * @param game_audio Pointer to the audio system for sound effects.
 */
void HandleNarrationInput(struct GameContext* game_context, int* game_state, struct Audio* game_audio);

/**
 * @brief Dynamically loads narration for the specified phase, parsing [IF] conditions based on GameContext.
 *
 * @param phase Pointer to the StoryPhase.
 * @param game_context Pointer to the GameContext.
 */
void LoadPhaseNarration(StoryPhase* phase, struct GameContext* game_context);

/**
 * @brief Handles player input during ENDING_CUTSCENE state.
 *
 * @param game_context Pointer to the GameContext.
 * @param game_state Pointer to the current game state.
 * @param game_audio Pointer to the audio system for sound effects.
 */
void HandleEndingInput(struct GameContext* game_context, int* game_state, struct Audio* game_audio);

#endif
