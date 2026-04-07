/**
 * @file character.c
 * @brief Handles player character initialization, movement physics, animation, and stamina management.
 * 
 * Update History:
 * - 2026-03-21: Initial 2D movement and collision logic. (Goal: Create the base for exploration.)
 * - 2026-04-03: Added directional sprite loading. (Goal: Support different textures for Kane 
 *                walking in all four cardinal directions.)
 * - 2026-04-05: Implemented stamina-based sprinting and recovery. (Goal: Balance movement 
 *                speed with resource management.)
 * 
 * Revision Details:
 * - Refactored `UpdateCharacter` to handle the `needs_shift_reset` flag for smoother sprinting UX.
 * - Expanded hitbox collision detection to be more forgiving for objects like the fridge.
 * - Integrated `PlayStep` sound triggers based on movement state and location (Interior vs Exterior).
 * - Fixed a frame-pacing bug in the `Character` animation counter.
 * 
 * Authors: Andrew Zhuo
 */
#include <stdio.h>
#include <string.h>
#include "character.h"
#include "raylib.h"
#include "raymath.h"
#include "map.h"
#include "data.h"
#include "story.h"
#include "interaction.h"

Character InitCharacter(Settings* game_settings, Data* game_data, Map* game_map){
    Character character = {0};

    // Load and initialize character sprites
    character.walk_down = LoadTexture("../assets/images/character/kane/kane_down.png");
    character.walk_up = LoadTexture("../assets/images/character/kane/kane_up.png");
    character.walk_left = LoadTexture("../assets/images/character/kane/kane_left.png");
    character.walk_right = LoadTexture("../assets/images/character/kane/kane_right.png");
    character.sprite = character.walk_down;

    // Initialize character properties
    character.size = (Vector2){80.0f, 175.0f};
    character.speed = game_settings->mc_speed;
    character.direction = 0; 

    // Initialize character animation properties
    character.frame_number = 4; 
    character.frame_speed = 8;
    character.current_frame = 0;
    character.frame_counter = 0;
    character.frame_rect = (Rectangle){
        0.0f, 0.0f,
        (float)character.sprite.width / character.frame_number - 0.25, 
        (float)character.sprite.height
    };

    // Initialize character stats
    character.stamina = 100.0f;
    character.max_stamina = 100.0f;
    character.sanity = 0.0f;
    character.max_sanity = 100.0f;
    character.exhausted = false;
    character.needs_shift_reset = false;

    if (game_data->position.x != -1.0f){
        character.position = game_data->position;
        character.direction = game_data->direction;
        character.inventory_count = game_data->inventory_count;
        for (int i = 0; i < game_data->inventory_count; i++){
            strcpy(character.inventory[i], game_data->inventory[i]);
            character.item_count[i] = game_data->item_count[i];
        }
        character.sanity = game_data->sanity;
    } else {
        character.position = game_map->spawn_position;
    }
    return character;
}

void UpdateCharacter(Character *character, Settings *game_settings, Vector2 map_size,
    Map *map, Audio* audio, Location location, StorySystem* story, Item* items, int itemCount,
    NPC* npcs, int npcCount, Door* doors, int doorCount, char picked_up_registry[][64],
    int picked_up_count){

    // Handle movement input
    Vector2 movement = {0, 0};
    if (IsKeyDown(KEY_W)){movement.y -= 1; character->direction = 3;}
    if (IsKeyDown(KEY_S)){movement.y += 1; character->direction = 0;}
    if (IsKeyDown(KEY_A)){movement.x -= 1; character->direction = 1;}
    if (IsKeyDown(KEY_D)){movement.x += 1; character->direction = 2;}

    // Handle story-specific movement logic
    StoryPhase* active = GetActivePhase(story);
    if (active && strcmp(active->name, "SET1-PHASE1") == 0){
        static bool w = false, a = false, s = false, d = false;
        if (IsKeyDown(KEY_W)) w = true;
        if (IsKeyDown(KEY_A)) a = true;
        if (IsKeyDown(KEY_S)) s = true;
        if (IsKeyDown(KEY_D)) d = true;
        if (w && a && s && d) active->quests[0].completed = true;
    }

    // Determine movement state
    bool is_moving = (movement.x != 0 || movement.y != 0);
    bool is_running = IsKeyDown(KEY_LEFT_SHIFT) && !character->exhausted && is_moving;

    // Update sprite and animation based on movement state
    if (is_running){
        if (character->direction == 0) character->sprite = character->walk_down;
        else if (character->direction == 1) character->sprite = character->walk_left;
        else if (character->direction == 2) character->sprite = character->walk_right;
        else if (character->direction == 3) character->sprite = character->walk_up;
    } else if (is_moving){
        if (character->direction == 0) character->sprite = character->walk_down;
        else if (character->direction == 1) character->sprite = character->walk_left;
        else if (character->direction == 2) character->sprite = character->walk_right;
        else if (character->direction == 3) character->sprite = character->walk_up;
    } else{
        if (character->direction == 0) character->sprite = character->walk_down;
        else if (character->direction == 1) character->sprite = character->walk_left;
        else if (character->direction == 2) character->sprite = character->walk_right;
        else if (character->direction == 3) character->sprite = character->walk_up;
    }

    // Update stamina and speed
    if (is_running){
        character->stamina -= game_settings->stamina_depletion_rate * GetFrameTime();
        character->speed = game_settings->mc_speed * 1.5f;
        if (character->stamina <= 0){
            character->stamina = 0;
            character->exhausted = true;
            character->needs_shift_reset = true;
        }
    } else{
        character->stamina += game_settings->stamina_recovery_rate * GetFrameTime();
        if (character->stamina > character->max_stamina) character->stamina = character->max_stamina;
        character->speed = game_settings->mc_speed;
        if (character->exhausted){
             if (!IsKeyDown(KEY_LEFT_SHIFT)) character->needs_shift_reset = false;
             if (!character->needs_shift_reset && character->stamina >= 20.0f) character->exhausted = false;
        }
    }

    // Handle movement and collision
    if (is_moving){
        movement = Vector2Normalize(movement);
        float next_x = character->position.x + movement.x * character->speed * GetFrameTime();
        float next_y = character->position.y + movement.y * character->speed * GetFrameTime();

        // X collision check
        bool collision_x = false;
        Rectangle collision_rect_x = {next_x, character->position.y, character->size.x, character->size.y};
        if (CheckMapCollision(map, collision_rect_x, picked_up_registry, picked_up_count)) collision_x = true;
        for (int i = 0; i < itemCount; i++) if (!items[i].picked_up && CheckCollisionRecs(collision_rect_x, items[i].base.bounds)) { collision_x = true; break; }
        for (int i = 0; i < npcCount; i++) if (CheckCollisionRecs(collision_rect_x, npcs[i].base.bounds)) { collision_x = true; break; }
        for (int i = 0; i < doorCount; i++) if (CheckCollisionRecs(collision_rect_x, doors[i].base.bounds)) { collision_x = true; break; }

        if (next_x >= 0 && next_x + character->size.x <= map_size.x && !collision_x) character->position.x = next_x;
        
        // Y collision check
        bool collision_y = false;
        Rectangle collision_rect_y = {character->position.x, next_y, character->size.x, character->size.y};
        if (CheckMapCollision(map, collision_rect_y, picked_up_registry, picked_up_count)) collision_y = true;
        for (int i = 0; i < itemCount; i++) if (!items[i].picked_up && CheckCollisionRecs(collision_rect_y, items[i].base.bounds)) { collision_y = true; break; }
        for (int i = 0; i < npcCount; i++) if (CheckCollisionRecs(collision_rect_y, npcs[i].base.bounds)) { collision_y = true; break; }
        for (int i = 0; i < doorCount; i++) if (CheckCollisionRecs(collision_rect_y, doors[i].base.bounds)) { collision_y = true; break; }

        if (next_y >= 0 && next_y + character->size.y <= map_size.y && !collision_y) character->position.y = next_y;
    }

    // Update animation frame
    character->frame_counter++;
    if (character->frame_counter >= (60 / character->frame_speed)){
        character->frame_counter = 0;
        character->current_frame++;
        if (character->current_frame >= character->frame_number) character->current_frame = 0;
    }

    // Calculate frame rectangle for sprite animation
    character->frame_rect.x = (float)character->current_frame * character->frame_rect.width;

    // Play footstep sound
    if (is_moving){
        float step_interval = is_running ? 0.3f : 0.5f;     // Faster when running
        static float step_timer = 0;
        step_timer += GetFrameTime();
        if (step_timer >= step_interval){
            if (location == EXTERIOR || location == FARM) PlaySound(audio->step_outdoor);
            else PlaySound(audio->step_indoor);
            step_timer = 0;
        }
    }
}

void DrawCharacter(Character *character){
    Rectangle dest_rect = {character->position.x, character->position.y,
        character->size.x, character->size.y};
    DrawTexturePro(character->sprite, character->frame_rect, dest_rect,
        (Vector2){0, 0}, 0, WHITE);
}

void CloseCharacter(Character *character){
    UnloadTexture(character->walk_down);
    UnloadTexture(character->walk_up);
    UnloadTexture(character->walk_left);
    UnloadTexture(character->walk_right);
}