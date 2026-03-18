/*
This module stores the implementation of the variable and functions
contained in the mc.h header file.

Module made by Andrew Zhuo.
*/

#include "character.h"
#include "raylib.h"
#include "raymath.h"
#include "settings.h"
#include "map.h"
#include "data.h"

Character InitCharacter(Settings *game_settings, Data *game_data){
  /* Initialize the character. */
  Character new_character = {0};

  // Load character textures
  new_character.sprite_idle =
      LoadTexture("../assets/images/character/idle.png");
  new_character.sprite_walk =
      LoadTexture("../assets/images/character/walk.png");
  new_character.sprite_run = LoadTexture("../assets/images/character/run.png");
  new_character.sprite = new_character.sprite_idle;

  // Initialize character position, size, speed, and direction
  new_character.position = (Vector2){
    game_settings->window_width / 2.0f,
    game_settings->window_height / 2.0f
  };
  new_character.size = (Vector2){200.0f, 200.0f};
  new_character.speed = game_settings->mc_speed;
  new_character.direction = 0;

  // Initialize character animation frames
  new_character.frame_number = 12;
  new_character.frame_speed = 8;
  new_character.current_frame = 0;
  new_character.frame_counter = 0;

  // Initialize stamina
  new_character.max_stamina = game_settings->max_stamina;
  new_character.stamina = new_character.max_stamina;
  new_character.exhausted = false;
  new_character.needs_shift_reset = false;

  float frame_width =
      (float)new_character.sprite.width / new_character.frame_number;
  float frame_height = (float)new_character.sprite.height / 4;

  new_character.frame_rect = (Rectangle){0.0f, 0.0f, frame_width, frame_height};

  return new_character;
}

void UpdateCharacter(Character *character, Settings *game_settings, Vector2 map_size, Map *map){
    /* Update character movement and animation. */

    // Calculate movement vector first to determine if actually moving
    Vector2 movement = {0, 0};
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) movement.x -= 1;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) movement.x += 1;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) movement.y -= 1;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) movement.y += 1;

    // is_moving is true only if there's actual net movement
    bool is_moving = (movement.x != 0 || movement.y != 0);

    // Only update direction if actually moving
    if (is_moving) {
        if (movement.x < 0) character->direction = 1;
        else if (movement.x > 0) character->direction = 2;
        else if (movement.y < 0) character->direction = 3;
        else if (movement.y > 0) character->direction = 0;
    }

    // Check for shift reset (must release shift after exhaustion)
    if (character->needs_shift_reset){
        if (!character->exhausted && ((!IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) || !is_moving)){
            character->needs_shift_reset = false;
        }
    }

    // Can only run if character has stamina, is not exhausted, and doesn't need a shift reset
    bool is_running =
	((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && 
	is_moving && 
	character->stamina > 0 && 
	!character->exhausted && 
	!character->needs_shift_reset);

    // Update stamina and exhausted state
    float frame_time = GetFrameTime();  
	
    if (is_running){
        character->stamina -= game_settings->stamina_depletion_rate * frame_time;
        if (character->stamina <= 0){
            character->stamina = 0;
            character->exhausted = true;
            character->needs_shift_reset = true; 
        }
    } else{
        character->stamina += game_settings->stamina_recovery_rate * frame_time;
        if (character->stamina > character->max_stamina){
            character->stamina = character->max_stamina;
        }
        
        // Recover from exhaustion when stamina is at least 10%
        if (character->exhausted && character->stamina >= character->max_stamina * 0.1f){
            character->exhausted = false;
        }
    }

    // Update character sprite and animation based on movement
    int sprite_columns;
    if (is_running){
        character->sprite = character->sprite_run;
        character->speed = game_settings->mc_speed * 2.0f;
        character->frame_speed = 12;
        character->frame_number = 8;
        sprite_columns = 8;
    } else if (is_moving){
        character->sprite = character->sprite_walk;
        character->speed = game_settings->mc_speed;
        character->frame_speed = 8;
        character->frame_number = 6;
        sprite_columns = 6;
    } else{
        character->sprite = character->sprite_idle;
        character->speed = game_settings->mc_speed;
        character->frame_speed = (character->direction == 3) ? 4 : 8;
        character->frame_number = (character->direction == 3) ? 4 : 12;
        sprite_columns = 12; // The idle sprite sheet ALWAYS has 12 columns
    }

    character->frame_rect.width = (float)character->sprite.width / sprite_columns;

    // Normalize and scale movement
    if (is_moving) {
        movement = Vector2Normalize(movement);
        movement.x *= character->speed * frame_time;
        movement.y *= character->speed * frame_time;
    }

    // Try moving in X axis
    float next_x = character->position.x + movement.x;
    next_x = Clamp(next_x, 0.0f, map_size.x - character->size.x);
  
    Rectangle collision_rect_x = {next_x + 75, character->position.y + 100, character->size.x - 150, character->size.y - 150};
  
    if (!CheckMapCollision(map, collision_rect_x)){
        character->position.x = next_x;
    }

    // Try moving in Y axis
    float next_y = character->position.y + movement.y;
    next_y = Clamp(next_y, 0.0f, map_size.y - character->size.y);
  
    Rectangle collision_rect_y = {character->position.x + 75, next_y + 100, character->size.x - 150, character->size.y - 150};
  
    if (!CheckMapCollision(map, collision_rect_y)){
        character->position.y = next_y;
    }

    // Update character animation frame
    character->frame_counter++;
    if (character->frame_counter >= (60 / character->frame_speed)){
        character->frame_counter = 0;
        character->current_frame++;
      
        if (character->current_frame >= character->frame_number){
            character->current_frame = 0;
        }
  
        // Update character animation frame rectangle
        character->frame_rect.x = character->current_frame * character->frame_rect.width;
        character->frame_rect.y = character->direction * character->frame_rect.height;
    }
}

void CloseCharacter(Character *character){
    /* Unload character textures to free VRAM. */
    UnloadTexture(character->sprite_idle);
    UnloadTexture(character->sprite_walk);
    UnloadTexture(character->sprite_run);
}

void DrawCharacter(Character *character){
    /* Draw the character to the screen. */
    Rectangle dest_rect = {character->position.x, character->position.y,
                            character->size.x, character->size.y};
    DrawTexturePro(character->sprite, character->frame_rect, dest_rect,
                    (Vector2){0, 0}, 0.0f, WHITE);

    // Draw vertical stamina bar on the right side if not at max stamina
    if (character->stamina < character->max_stamina){
        float bar_width = 10.0f;
        float bar_height = character->size.y * 0.6f;
        float posX = character->position.x + character->size.x + 5.0f;
        float posY = character->position.y + (character->size.y - bar_height) / 2.0f;
        
        // Background (empty bar)
        DrawRectangle(posX, posY, bar_width, bar_height, BROWN);
        
        float current_bar_height = character->stamina / character->max_stamina * bar_height;
        // Draw from bottom up
        DrawRectangle(posX, posY + (bar_height - current_bar_height), bar_width, current_bar_height, ORANGE);
    }
}