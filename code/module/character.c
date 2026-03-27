/**
 * @file character.c
 * @brief Implementation of the player character and movement mechanics.
 * 
 * Authors: Andrew Zhuo
 */

#include <stdio.h>
#include <string.h>
#include "character.h"
#include "raymath.h"
#include "map.h"
#include "data.h"
#include "story.h"

Character InitCharacter(Settings* game_settings, Data* game_data, Map* game_map){
    Character character = {0};

    character.sprite_idle = LoadTexture("../assets/images/character/idle.png");
    character.sprite_walk = LoadTexture("../assets/images/character/walk.png");
    character.sprite_run = LoadTexture("../assets/images/character/run.png");
    character.sprite = character.sprite_idle;

    character.size = (Vector2){350.0f, 350.0f};
    character.speed = game_settings->mc_speed;
    character.direction = 0; // 0=Down, 1=Left, 2=Right, 3=Up

    // Initialize default animation state
    character.frame_number = 12; 
    character.frame_speed = 8;
    character.current_frame = 0;
    character.frame_counter = 0;
    character.frame_rect = (Rectangle){0.0f, 0.0f, (float)character.sprite.width / character.frame_number, (float)character.sprite.height / 4.0f};

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
        character.sanity = game_data->player_sanity_level;
    } else {
        character.position = game_map->spawn_position;
    }

    return character;
}

void UpdateCharacter(Character *character, Settings *game_settings, Vector2 map_size, Map *map, Audio* audio, Location location, StorySystem* story){
    Vector2 movement = {0, 0};
    
    // --- Phase 1: Input Processing ---
    if (IsKeyDown(KEY_W)){movement.y -= 1; character->direction = 3;}
    if (IsKeyDown(KEY_S)){movement.y += 1; character->direction = 0;}
    if (IsKeyDown(KEY_A)){movement.x -= 1; character->direction = 1;}
    if (IsKeyDown(KEY_D)){movement.x += 1; character->direction = 2;}

    // --- Story Support: Detect WASD usage ---
    StoryPhase* active = GetActivePhase(story);
    if (active && strcmp(active->name, "SET1-PHASE1") == 0){
        static bool w = false, a = false, s = false, d = false;
        if (IsKeyDown(KEY_W)) w = true;
        if (IsKeyDown(KEY_A)) a = true;
        if (IsKeyDown(KEY_S)) s = true;
        if (IsKeyDown(KEY_D)) d = true;

        if (w && a && s && d){
            active->quests[0].completed = true;
        }
    }

    bool is_moving = (movement.x != 0 || movement.y != 0);
    bool is_running = IsKeyDown(KEY_LEFT_SHIFT) && !character->exhausted && is_moving;

    // --- Update Animation State Rules ---
    if (is_running){
        character->sprite = character->sprite_run;
        character->frame_speed = 12;
        character->frame_number = 8;
    } else if (is_moving){
        character->sprite = character->sprite_walk;
        character->frame_speed = 8;
        character->frame_number = 6;
    } else{
        character->sprite = character->sprite_idle;
        if (character->direction == 3){
            character->frame_speed = 4;
            character->frame_number = 4;
        } else{
            character->frame_speed = 8;
            character->frame_number = 12;
        }
    }

    // --- Phase 2: Stamina and Exhaustion ---
    if (is_running){
        character->stamina -= 20.0f * GetFrameTime();
        character->speed = game_settings->mc_speed * 1.5f;
        if (character->stamina <= 0){
            character->stamina = 0;
            character->exhausted = true;
            character->needs_shift_reset = true;
        }
    } else{
        character->stamina += 10.0f * GetFrameTime();
        if (character->stamina > character->max_stamina) character->stamina = character->max_stamina;
        character->speed = game_settings->mc_speed;
        
        if (character->exhausted){
             if (!IsKeyDown(KEY_LEFT_SHIFT)) character->needs_shift_reset = false;
             if (!character->needs_shift_reset && character->stamina >= 20.0f) character->exhausted = false;
        }
    }

    // --- Phase 3: Collision and Transformation ---
    if (is_moving){
        movement = Vector2Normalize(movement);
        float next_x = character->position.x + movement.x * character->speed * GetFrameTime();
        float next_y = character->position.y + movement.y * character->speed * GetFrameTime();

        // Check map boundaries and collisions (using centered hitbox)
        Rectangle collision_rect_x = {next_x + 125, character->position.y + 100, character->size.x - 250, character->size.y - 200};
        if (next_x >= 0 && next_x + character->size.x <= map_size.x && !CheckMapCollision(map, collision_rect_x)){
            character->position.x = next_x;
        }
        
        Rectangle collision_rect_y = {character->position.x + 125, next_y + 100, character->size.x - 250, character->size.y - 200};
        if (next_y >= 0 && next_y + character->size.y <= map_size.y && !CheckMapCollision(map, collision_rect_y)){
            character->position.y = next_y;
        }
    }

    // --- Phase 4: Animation Management ---
    character->frame_counter++;
    if (character->frame_counter >= (60 / character->frame_speed)){
        character->frame_counter = 0;
        character->current_frame++;
        if (character->current_frame >= character->frame_number) character->current_frame = 0;
    }

    float frame_width = (float)character->sprite.width / character->frame_number;
    character->frame_rect.x = (float)character->current_frame * frame_width;

    float frame_height = (float)character->sprite.height / 4.0f;
    character->frame_rect.y = (float)character->direction * frame_height;
    
    character->frame_rect.width = frame_width;
    character->frame_rect.height = frame_height;

    // --- Phase 5: Audio Feedback ---
    if (is_moving){
        float step_interval = is_running ? 0.3f : 0.5f;
        static float step_timer = 0;
        step_timer += GetFrameTime();
        if (step_timer >= step_interval){
            if (location == EXTERIOR) PlaySound(audio->step_outdoor);
            else PlaySound(audio->step_indoor);
            step_timer = 0;
        }
    }
}

void DrawCharacter(Character *character){
    Rectangle dest_rect = {character->position.x, character->position.y, character->size.x, character->size.y};
    DrawTexturePro(character->sprite, character->frame_rect, dest_rect, (Vector2){0, 0}, 0, WHITE);
}

void CloseCharacter(Character *character){
    UnloadTexture(character->sprite_idle);
    UnloadTexture(character->sprite_walk);
    UnloadTexture(character->sprite_run);
}