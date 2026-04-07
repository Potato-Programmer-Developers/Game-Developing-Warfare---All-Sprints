/**
 * @file assets.h
 * @brief Header file for assets management.
 * 
 * This module handles loading and unloading assets for the game.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef ASSETS_H
#define ASSETS_H

#include "game_context.h"
#include "interaction.h"
#include "story.h"

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

/**
 * @brief Updates the persistent karma for an asset in the registry.
 * 
 * @param id The unique ID of the asset.
 * @param delta The amount to change karma by.
 */
void UpdateAssetKarma(const char* id, int delta);

/**
 * @brief Populates an array with all persistent karma values from the registry.
 * 
 * @param dest The array to populate.
 * @param max_size The maximum size of the array.
 * @return The number of values copied.
 */
int GetRegistryKarma(int* dest, int max_size);

/**
 * @brief Updates the registry with new karma values.
 * 
 * @param src The array to copy from.
 * @param count The number of values to copy.
 */
void SetRegistryKarma(int* src, int count);

#endif
