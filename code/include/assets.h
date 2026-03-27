#ifndef ASSETS_H
#define ASSETS_H

#include "game_context.h"
#include "interaction.h"
#include "story.h"

/**
 * @brief Loads assets for a specific location.
 * 
 * @param location The location to load assets for.
 * @param context The game context to load assets into.
 */
void LoadLocationAssets(Location location, GameContext* context);

/**
 * @brief Loads assets for a specific phase.
 * 
 * @param phase The phase to load assets for.
 * @param context The game context to load assets into.
 */
void LoadPhaseAssets(StoryPhase* phase, GameContext* context);

/**
 * @brief Unloads assets for a specific location.
 * 
 * @param context The game context to unload assets from.
 */
void UnloadLocationAssets(GameContext* context);

#endif
