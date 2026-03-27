#ifndef QUEST_H
#define QUEST_H

#include <stdbool.h>

/**
 * @brief Represents a single mission or tutorial objective.
 */
typedef struct Quest {
    char description[64];             // Text displayed in the UI
    bool completed;                   // Status flag
} Quest;

/**
 * @brief Manages a sequence of objectives for the player.
 */
typedef struct QuestSystem {
    Quest quests[3];                  // Fixed array for the 3 tutorial quests
    int active_quest_index;           // Current quest being tracked (-1 if none)
} QuestSystem;

#endif
