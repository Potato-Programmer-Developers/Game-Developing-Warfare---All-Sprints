/**
 * @file scene.c
 * @brief Implementation of high-level rendering, UI overlays, and screen fade management.
 * 
 * Update History:
 * - 2026-03-24: Foundation of the Raylib rendering pipeline for the project. (Goal: Establish 
 *                a consistent 2D camera behavior and layered rendering for maps and sprites.)
 * - 2026-04-04: Implemented the "Objective Overlay" and Tutorial Tooltips. (Goal: Provide 
 *                visual feedback for quest progression through a semi-transparent UI box.)
 * - 2026-04-05: Integrated the "Dream Sequence" renderer and Fade synchronizer. (Goal: Support 
 *                the display of narrative-only 'dream' text on a black background during transitions.)
 * - 2026-04-06: Expansion of the "UI Animation Engine." (Goal: Modularize frame loading logic 
 *                to support diverse menu animations—Main, Pause, and Settings—while maintaining 
 *                low VRAM usage through on-demand loading.)
 * - 2026-05-02: Implemented the `ENDING_CUTSCENE` renderer with typed dialogue and scrolling credits.
 *                (Goal: Draw ending sequences as typed `Speaker: dialogue` text on a black screen,
 *                then render a scrolling credits sequence with paragraph-aware role/name coloring,
 *                special title formatting, and a gated SPACE prompt.)
 * - 2026-05-02: Fixed tutorial tooltip rendering to only display on Day 1. (Goal: Prevent
 *                tooltips from appearing on every `SET1-PHASE1` across all days by adding a
 *                `day_folder == "day1"` guard.)
 * - 2026-05-02: Added dialogue box typing effect with speaker-aware coloring. (Goal: Display
 *                dialogue text with a typewriter effect that correctly colors the speaker name in
 *                GOLD and dialogue text in WHITE.)
 * 
 * Revision Details:
 * - Refactored `DrawGame` to include conditional rendering for `NARRATION_CUTSCENE` and `PHONE` overlays.
 * - Implemented `DrawFade` to handle project-wide color transitions using a global alpha value.
 * - Added dynamic quest list rendering in `scene.c` that scales based on `active_phase->quest_count`.
 * - Fixed a rendering bug where UI elements were being drawn inside the camera transform.
 * - Implemented the `LoadPauseFrame` helper function to handle frame sequences from the 
 *    `../assets/videos/esc_option/` directory.
 * - Refactored `DrawPauseMenu` to render `current_cutscene_frame_texture` as its background, 
 *    removing the requirement for a static background texture.
 * - Added `ENDING_CUTSCENE` rendering block in `DrawGame` with two sub-states: dialogue display
 *    (typed `Speaker: text` with GOLD/WHITE coloring) and scrolling credits.
 * - Implemented paragraph-aware color state machine for credits: empty lines reset `next_is_role`,
 *    first non-empty line uses GOLD, subsequent lines use WHITE. `"AISLING"`, `"THE END"`, and
 *    `"THANK YOU FOR PLAYING"` are special-cased.
 * - Added `strcmp(game_context->story.day_folder, "day1") == 0` guard to the tutorial tooltip rendering.
 * - Updated `DrawMap` call in `DrawGameplay` to pass `game_context->bear_trap_inside`.
 * - Added speaker-aware typing effect in the dialogue box rendering.
 * 
 * Authors: Andrew Zhuo and Steven Kenneth Darwy
 */

#include "scene.h"
#include <stdio.h>
#include <string.h>
#include "interactive.h"
#include "story.h"

Scene InitScene(Settings* game_settings){
    Scene new_scene = {0};

    // Load all textures for the scene
    new_scene.mainmenu_background = LoadTexture("../assets/images/background/main_menu/main_menu.png");
    new_scene.pause_menu_background = LoadTexture("../assets/images/background/pause/pause.png");
    new_scene.vignette = LoadTexture("../assets/images/background/vignette/vignette.png");

    // Initialize cutscene variables
    new_scene.current_cutscene_frame_texture = (Texture2D){0};
    new_scene.current_knob_frame_texture = (Texture2D){0};
    new_scene.cutscene_timer = 0.0f;
    new_scene.current_cutscene_frame = 0;
    new_scene.fade_alpha = 0.0f;
    new_scene.is_fading_in = false;
    new_scene.is_fading_out = false;
    new_scene.pending_map[0] = '\0';
    new_scene.pending_loc[0] = '\0';

    return new_scene;
}

void StartFadeTransition(Scene* scene, Color color, const char* map, const char* loc, const char* spawn_id) {
    if (!scene) return;
    scene->fade_color = color;
    scene->fade_alpha = 0.0f;
    scene->is_fading_out = true;
    scene->is_fading_in = false;
    if (map) strncpy(scene->pending_map, map, 127);
    else scene->pending_map[0] = '\0';
    if (loc) strncpy(scene->pending_loc, loc, 31);
    else scene->pending_loc[0] = '\0';
    if (spawn_id) strncpy(scene->pending_spawn_id, spawn_id, 63);
    else scene->pending_spawn_id[0] = '\0';
}

void UpdateFade(Scene* scene, float delta, GameState state){
    if (scene == NULL) return;
    if (scene->is_fading_out){
        // Only block auto-fading if we are not in a map transition
        // This allows dialogue-triggered transitions to complete.
        if (scene->pending_map[0] == '\0' && (state == DIALOGUE_CUTSCENE || state == PHOTO_CUTSCENE)) return;

        scene->fade_alpha += delta * 1.5f;
        if (scene->fade_alpha >= 1.0f){
            scene->fade_alpha = 1.0f;
            scene->is_fading_out = false;
        }
    } else if (scene->is_fading_in){
        scene->fade_alpha -= delta * 1.5f;
        if (scene->fade_alpha <= 0.0f){
            scene->fade_alpha = 0.0f;
            scene->is_fading_in = false;
        }
    }
}

void DrawFade(Scene* scene){
    if (scene->fade_alpha > 0.0f){
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(scene->fade_color, scene->fade_alpha));
    }
}

void DrawGame(Scene *game_scene, Settings *game_settings, Interactive *game_interactive,
              Map *game_map, Character *player, Dialogue *game_dialogue, GameContext *game_context,
              GameState *game_state, NPC worldNPCs[], Item worldItems[]){
    
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw game based on the current state
    if (*game_state == MAINMENU){
        DrawMainMenu(game_scene, game_interactive);
    } else if (*game_state == SETTINGS){
        DrawSettings(game_scene, game_settings, game_interactive);
    } else if (*game_state == PAUSE){
        DrawPauseMenu(game_scene, game_settings, game_interactive);
    } else if (*game_state == PHOTO_CUTSCENE){
        DrawTexturePro(game_scene->current_cutscene_frame_texture,
            (Rectangle){0, 0, (float)game_scene->current_cutscene_frame_texture.width, (float)game_scene->current_cutscene_frame_texture.height},
            (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
            (Vector2){0, 0}, 0.0f, WHITE);
            
        if (game_scene->cutscene_timer < 5.0f){
            DrawText("Double click 'Enter' to skip", GetScreenWidth() - 300, GetScreenHeight() - 40, 20, LIGHTGRAY);
        }
    } else{
        DrawGameplay(game_scene, game_settings, game_interactive, game_map, player, worldNPCs, worldItems, game_context);
        
        // Draw vignette to create a horror feel
        DrawTexturePro(game_scene->vignette,
            (Rectangle){0, 0, (float)game_scene->vignette.width, (float)game_scene->vignette.height},
            (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
            (Vector2){0, 0}, 0.0f, WHITE);

        // Draw dialogue box
        if (*game_state == DIALOGUE_CUTSCENE){
            int required_height = 200;
            DialogueNode* current_node = &game_dialogue->nodes[game_dialogue->current_node_idx];
            bool has_choices = (game_dialogue->current_line >= game_dialogue->line_count - 1 && current_node->choice_count > 0);
            
            if (has_choices) {
                int needed = 70 + current_node->choice_count * 30 + 30; // margins + spacing
                if (needed > required_height) required_height = needed;
            }
            
            DrawRectangle(0, GetScreenHeight() - required_height, GetScreenWidth(), required_height, Fade(BLACK, 0.8f));
            int box_y = GetScreenHeight() - required_height;
            
            // Get the current node's text
            const char *full_line = game_dialogue->lines[game_dialogue->current_line];
            int line_len = strlen(full_line);
            int start_x = GetScreenWidth() / 2 - MeasureText(full_line, 20) / 2;
            const char* colon = strchr(full_line, ':');
            if (colon) {
                int speaker_len = (int)(colon - full_line);
                char speaker[64] = {0};
                strncpy(speaker, full_line, speaker_len < 63 ? speaker_len : 63);
                const char* dialogue_text = colon + 1;
                while (*dialogue_text == ' ') dialogue_text++;
                
                int typed_chars = game_dialogue->typing_index;
                if (typed_chars <= speaker_len + 2) {
                    const char* typed = TextSubtext(full_line, 0, typed_chars);
                    DrawText(typed, start_x, box_y + 30, 20, GOLD);
                } else {
                    char temp_speaker[64];
                    sprintf(temp_speaker, "%s: ", speaker);
                    DrawText(temp_speaker, start_x, box_y + 30, 20, GOLD);
                    int colon_w = MeasureText(temp_speaker, 20);
                    const char* typed_dialogue = TextSubtext(dialogue_text, 0, typed_chars - (speaker_len + 2));
                    DrawText(typed_dialogue, start_x + colon_w, box_y + 30, 20, WHITE);
                }
            } else {
                const char *typed_line = TextSubtext(full_line, 0, game_dialogue->typing_index);
                DrawText(typed_line, start_x, box_y + 30, 20, WHITE);
            }

            // Draw choices if we're at the end of the current node's text and typing is complete
            if (game_dialogue->typing_index >= line_len) {
                if (has_choices){
                    for (int i = 0; i < current_node->choice_count; i++){
                        char choiceText[128];
                        sprintf(choiceText, "%d. %s", i + 1, current_node->choices[i]);
                        DrawText(choiceText, 100, box_y + 80 + (i * 30), 20, YELLOW);
                    }
                } else if (game_dialogue->current_line < game_dialogue->line_count) {
                    DrawText("Press 'SPACE' to continue", GetScreenWidth() - 300, GetScreenHeight() - 40, 20, GRAY);
                }
            }
        }

        // Draw phone
        DrawPhone(&game_context->phone);
        
        // Draw objectives
        StoryPhase* active_phase = GetActivePhase(&game_context->story);
        if (active_phase && active_phase->quest_count > 0){
            int boxPadding = 15;
            int boxWidth = 420;
            int boxHeight = 40 + (active_phase->quest_count * 28);
            if (boxHeight < 70) boxHeight = 70;

            DrawRectangle(20, 20, boxWidth, (float)boxHeight, Fade(BLACK, 0.8f));
            DrawRectangleLinesEx((Rectangle){20, 20, (float)boxWidth, (float)boxHeight}, 2, GRAY);
            
            DrawText("OBJECTIVES", 35, 30, 15, GOLD);
            for (int i = 0; i < active_phase->quest_count; i++) {
                Quest* q = &active_phase->quests[i];
                Color qColor = q->completed ? LIME : WHITE;
                const char* prefix = q->completed ? "[v] " : "[ ] ";
                char qText[256];
                
                // Dynamic quest progress injection
                if (strstr(q->description, "(0/18)")) {
                    int pots_planted = 0;
                    for (int p = 0; p < 18; p++) if (game_context->pot_registry[p].is_planted) pots_planted++;
                    
                    q->completed = (pots_planted >= 18);
                    qColor = q->completed ? LIME : WHITE;
                    prefix = q->completed ? "[v] " : "[ ] ";

                    char modified_desc[128];
                    strcpy(modified_desc, q->description);
                    char* ptr = strstr(modified_desc, "(0/18)");
                    sprintf(ptr, "(%d/18)", pots_planted);
                    sprintf(qText, "%s%s", prefix, modified_desc);
                } else if (strstr(q->description, "(0/4)")) {
                    int clues_found = 0;
                    for (int c = 0; c < active_phase->condition_count; c++) {
                        if (active_phase->end_conditions[c].type == CONDITION_INTERACT_OBJECT && active_phase->end_conditions[c].met) {
                            clues_found++;
                        }
                    }
                    
                    q->completed = (clues_found >= 4);
                    qColor = q->completed ? LIME : WHITE;
                    prefix = q->completed ? "[v] " : "[ ] ";

                    char modified_desc[128];
                    strcpy(modified_desc, q->description);
                    char* ptr = strstr(modified_desc, "(0/4)");
                    sprintf(ptr, "(%d/4)", clues_found);
                    sprintf(qText, "%s%s", prefix, modified_desc);
                } else {
                    sprintf(qText, "%s%s", prefix, q->description);
                }
                
                DrawText(qText, 35, 55 + (i * 25), 18, qColor);
            }
            
            // Draw tooltip for tutorial in SET1-PHASE1 of Day 1
            if (strcmp(active_phase->name, "SET1-PHASE1") == 0 && strcmp(game_context->story.day_folder, "day1") == 0){
                const char* tooltip = NULL;
                if (!active_phase->quests[0].completed && *game_state == GAMEPLAY) tooltip = "WASD TO MOVE";
                else if (active_phase->quest_count > 1 && !active_phase->quests[1].completed && *game_state == GAMEPLAY) tooltip = "PRESS 'E' TO INTERACT";
                else if (active_phase->quest_count > 2 && !active_phase->quests[2].completed && *game_state == GAMEPLAY) tooltip = "PRESS 'R' TO OPEN PHONE";

                if (tooltip) {
                    int textWidth = MeasureText(tooltip, 30);
                    DrawText(tooltip, GetScreenWidth()/2 - textWidth/2, GetScreenHeight() - 150, 30, WHITE);
                }
            }
        }

        // Draw Phone Message Sequence (sequential notifications)
        if (game_context->story.phone_sequence_active && game_context->story.phone_current_index < game_context->story.phone_active_count) {
            const char* sender = game_context->story.phone_active_sender;
            const char* msg = game_context->story.phone_active_messages[game_context->story.phone_current_index].text;
            
            int margin = 20;
            int boxW = 450;
            int boxH = 80;
            Rectangle box = {
                (float)GetScreenWidth() - boxW - margin,
                (float)GetScreenHeight() - boxH - margin,
                (float)boxW, (float)boxH
            };
            DrawRectangleRec(box, Fade(BLACK, 0.85f));
            DrawRectangleLinesEx(box, 2, GOLD);
            DrawText(sender, box.x + 15, box.y + 10, 15, GOLD);
            DrawText(msg, box.x + 15, box.y + 35, 20, WHITE);
        }

        // Draw Interactive Narration (only when phone is not playing)
        if (*game_state == NARRATION_CUTSCENE && game_context->story.narration_active && active_phase && !game_context->story.phone_sequence_active){
            int required_height = 200;
            if (!game_context->story.narration_showing_response && game_context->story.narration_in_loop) {
                int needed = 60 + active_phase->narration_choice_count * 30 + 30; // margins
                if (needed > required_height) required_height = needed;
            }
            
            DrawRectangle(0, GetScreenHeight() - required_height, GetScreenWidth(), required_height, Fade(BLACK, 0.8f));
            int box_y = GetScreenHeight() - required_height;
            
            if (game_context->story.narration_showing_response){
                // Show the response text for a chosen option
                const char* resp = game_context->story.narration_response_text;
                int resp_len = strlen(resp);
                int start_x = GetScreenWidth() / 2 - MeasureText(resp, 20) / 2;
                const char *typed_resp = TextSubtext(resp, 0, game_context->story.narration_typing_index);
                DrawText(typed_resp, start_x, box_y + 30, 20, WHITE);
                if (game_context->story.narration_typing_index >= resp_len) {
                    DrawText("Press 'SPACE' to continue", GetScreenWidth() - 300, GetScreenHeight() - 40, 20, GRAY);
                }
            } else if (game_context->story.narration_in_loop){
                // Show loop choices
                DrawText("What should I do?", GetScreenWidth() / 2 - MeasureText("What should I do?", 20) / 2, box_y + 20, 20, WHITE);
                int drawn = 0;
                for (int i = 0; i < active_phase->narration_choice_count; i++){
                    char choiceText[128];
                    sprintf(choiceText, "%d. %s", drawn + 1, active_phase->narration_choices[i].label);
                    Color color = active_phase->narration_choices[i].completed ? DARKGRAY : YELLOW;
                    DrawText(choiceText, 100, box_y + 60 + (i * 30), 20, color);
                    drawn++;
                }
            } else{
                // Show current text narration line
                int line_idx = game_context->story.narration_current_line;
                if (line_idx < active_phase->narration_count && active_phase->narration_lines[line_idx].type == 0){
                    const char* ntext = active_phase->narration_lines[line_idx].text;
                    int line_len = strlen(ntext);
                    int start_x = GetScreenWidth() / 2 - MeasureText(ntext, 20) / 2;
                    const char *typed_line = TextSubtext(ntext, 0, game_context->story.narration_typing_index);
                    DrawText(typed_line, start_x, box_y + 30, 20, WHITE);
                    if (game_context->story.narration_typing_index >= line_len) {
                        DrawText("Press 'SPACE' to continue", GetScreenWidth() - 300, GetScreenHeight() - 40, 20, GRAY);
                    }
                }
            }
        }
    }

    // Draw Dream Sequence (only when dream is active)
    if (game_context->dream_active){
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.95f));
        if (game_context->dream_current < game_context->dream_count){
            const char* dtxt = game_context->dream_lines[game_context->dream_current];
            // Remove optional 'BLACK ' prefix if present
            if (strncmp(dtxt, "BLACK ", 6) == 0) dtxt += 6;
            int txtW = MeasureText(dtxt, 20);
            DrawText(dtxt, GetScreenWidth() / 2 - txtW / 2, GetScreenHeight() / 2 - 10, 20, WHITE);
            DrawText(TextFormat("Dreaming... %d/%d", game_context->dream_current+1, game_context->dream_count), 20, 20, 15, GRAY);
        }
    }

    // Draw SCENE overlay (e.g. FLASHBACK)
    if (game_context->story.scene_timer > 0) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 1.0f));
        if (game_context->story.current_scene[0] != '\0') {
            const char* s_text = game_context->story.current_scene;
            int s_txtW = MeasureText(s_text, 30);
            DrawText(s_text, GetScreenWidth() / 2 - s_txtW / 2, GetScreenHeight() / 2 - 15, 30, WHITE);
        }
    }

    // Draw Ending Sequence
    if (*game_state == ENDING_CUTSCENE && game_context->story.ending_active) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
        
        if (game_context->story.ending_show_credits) {
            // Scrolling Credits screen
            game_context->story.ending_credits_y -= 40.0f * GetFrameTime();
            
            bool next_is_role = true;
            for (int i = 0; i < game_context->story.ending_credits_line_count; i++) {
                const char* line = game_context->story.ending_credits_lines[i];
                float line_y = game_context->story.ending_credits_y + (i * 30.0f);
                
                if (strlen(line) == 0) {
                    next_is_role = true;
                    continue;
                }
                
                // Only draw if within screen bounds (plus some margin)
                if (line_y > -50 && line_y < GetScreenHeight() + 50) {
                    int w = MeasureText(line, 24);
                    Color c = next_is_role ? GOLD : WHITE;
                    next_is_role = false; // After the first line (role), subsequent lines are names
                    
                    // Specific hardcoded checks for title and ending
                    if (strcmp(line, "AISLING") == 0 || strcmp(line, "THE END") == 0 || strcmp(line, "THANK YOU FOR PLAYING") == 0) {
                        c = WHITE;
                        w = MeasureText(line, 30);
                        DrawText(line, GetScreenWidth() / 2 - w / 2, line_y, 30, c);
                        next_is_role = true; // reset so the next actual block starts with GOLD
                    } else {
                        DrawText(line, GetScreenWidth() / 2 - w / 2, line_y, 24, c);
                    }
                } else {
                    // Even if offscreen, we must update the state machine
                    if (strcmp(line, "AISLING") == 0 || strcmp(line, "THE END") == 0 || strcmp(line, "THANK YOU FOR PLAYING") == 0) {
                        next_is_role = true;
                    } else {
                        next_is_role = false;
                    }
                }
            }
            
            float last_line_y = game_context->story.ending_credits_y + (game_context->story.ending_credits_line_count * 30.0f);
            if (last_line_y < -50.0f) {
                DrawText("Press 'SPACE' to continue", GetScreenWidth() / 2 - MeasureText("Press 'SPACE' to continue", 20) / 2, GetScreenHeight() / 2 + 60, 20, GRAY);
            }
        } else if (game_context->story.ending_current_line < game_context->story.ending_line_count) {
            // Parse "Speaker: text" format
            const char* full_line = game_context->story.ending_lines[game_context->story.ending_current_line];
            int line_len = strlen(full_line);
            
            // Find colon separator for speaker
            const char* colon = strchr(full_line, ':');
            if (colon) {
                // Draw speaker name in GOLD
                int speaker_len = (int)(colon - full_line);
                char speaker[64] = {0};
                strncpy(speaker, full_line, speaker_len < 63 ? speaker_len : 63);
                
                const char* dialogue_text = colon + 1;
                while (*dialogue_text == ' ') dialogue_text++;
                
                // Fixed X positions based on full line
                int speaker_x = GetScreenWidth() / 2 - MeasureText(full_line, 20) / 2;
                int speaker_w = MeasureText(speaker, 20);
                int colon_w = MeasureText(": ", 20);
                
                // Typing effect: figure out what to show
                int typed_chars = game_context->story.ending_typing_index;
                if (typed_chars <= speaker_len + 2) {
                    // Still typing speaker name + ": "
                    const char* typed = TextSubtext(full_line, 0, typed_chars);
                    DrawText(typed, speaker_x, GetScreenHeight() / 2, 20, GOLD);
                } else {
                    // Speaker fully displayed, now typing dialogue
                    char speaker_prefix[68] = {0};
                    snprintf(speaker_prefix, sizeof(speaker_prefix), "%s: ", speaker);
                    DrawText(speaker_prefix, speaker_x, GetScreenHeight() / 2, 20, GOLD);
                    
                    int dialogue_start = speaker_len + 2; // "Speaker: " length
                    int dialogue_typed = typed_chars - dialogue_start;
                    const char* typed_dialogue = TextSubtext(dialogue_text, 0, dialogue_typed);
                    DrawText(typed_dialogue, speaker_x + speaker_w + colon_w, GetScreenHeight() / 2, 20, WHITE);
                }
            } else {
                // No colon — plain text line
                int start_x = GetScreenWidth() / 2 - MeasureText(full_line, 20) / 2;
                const char* typed = TextSubtext(full_line, 0, game_context->story.ending_typing_index);
                DrawText(typed, start_x, GetScreenHeight() / 2, 20, WHITE);
            }
            
            // Show "Press SPACE to continue" when typing is done
            if (game_context->story.ending_typing_index >= line_len) {
                DrawText("Press 'SPACE' to continue", GetScreenWidth() - 300, GetScreenHeight() - 40, 20, GRAY);
            }
        }
    }

    // Draw fade overlay for cutscenes
    if (game_scene->fade_alpha > 0.01f){
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(game_scene->fade_color, game_scene->fade_alpha));
    }

    EndDrawing();
}

void DrawMainMenu(Scene* scene, Interactive* game_interactive){
    // Draw main menu background
    DrawTexturePro(scene->current_cutscene_frame_texture, 
        (Rectangle){0, 0, (float)scene->current_cutscene_frame_texture.width, (float)scene->current_cutscene_frame_texture.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0}, 0.0f, WHITE);
    
    if (game_interactive->is_continue_hovered) DrawRectangleRec(game_interactive->continue_bounds, Fade(WHITE, 0.3f));
    if (game_interactive->is_new_game_hovered) DrawRectangleRec(game_interactive->new_game_bounds, Fade(WHITE, 0.3f));
    if (game_interactive->is_settings_hovered) DrawRectangleRec(game_interactive->settings_bounds, Fade(WHITE, 0.3f));
    if (game_interactive->is_quit_hovered) DrawRectangleRec(game_interactive->quit_bounds, Fade(WHITE, 0.3f));
}

void DrawPauseMenu(Scene* scene, Settings* game_settings, Interactive* game_interactive){
    // Draw pause menu background (animated)
    DrawTexturePro(scene->current_cutscene_frame_texture, 
        (Rectangle){0, 0, (float)scene->current_cutscene_frame_texture.width, (float)scene->current_cutscene_frame_texture.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0}, 0.0f, WHITE);
    
    if (game_interactive->is_continue_hovered) DrawRectangleRec(game_interactive->continue_bounds, Fade(WHITE, 0.3f));
    if (game_interactive->is_settings_hovered) DrawRectangleRec(game_interactive->settings_bounds, Fade(WHITE, 0.3f));
    if (game_interactive->is_quit_hovered) DrawRectangleRec(game_interactive->quit_bounds, Fade(WHITE, 0.3f));
}

void DrawSettings(Scene* scene, Settings* game_settings, Interactive* game_interactive){
    // Draw settings background
    DrawTexturePro(scene->current_cutscene_frame_texture, 
        (Rectangle){0, 0, (float)scene->current_cutscene_frame_texture.width, (float)scene->current_cutscene_frame_texture.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0}, 0.0f, WHITE);

    DrawTexturePro(scene->current_knob_frame_texture,
        (Rectangle){0, 0, (float)scene->current_knob_frame_texture.width, (float)scene->current_knob_frame_texture.height},
        game_interactive->volume_slider_knob, (Vector2){0, 0}, 0.0f, WHITE);

    if (game_interactive->is_settings_back_hovered) DrawRectangleRec(game_interactive->settings_back_bounds, Fade(WHITE, 0.3f));
}

void DrawGameplay(Scene* scene, Settings* game_settings, Interactive* game_interactive, Map* game_map,
                Character* player, NPC worldNPCs[], Item worldItems[], GameContext* game_context){
    // Draw gameplay
    BeginMode2D(game_context->camera);
    bool day2_active = (strcmp(game_context->story.day_folder, "day2") == 0);
    DrawMap(game_map, game_context->fireplace_on, game_context->main_door_locked, day2_active, game_context->story.current_set_idx, game_context->bear_trap_inside);

    // Draw world items (Only pickable ones)
    for (int i = 0; i < game_context->itemCount; i++) {
        if (!worldItems[i].picked_up && worldItems[i].is_pickup && worldItems[i].base.texture.id != 0) {
            DrawTexturePro(worldItems[i].base.texture, 
                (Rectangle){0, 0, (float)worldItems[i].base.texture.width, (float)worldItems[i].base.texture.height},
                worldItems[i].base.bounds, (Vector2){0, 0}, 0.0f, WHITE);
        }
    }

    // Draw NPCs
    for (int i = 0; i < game_context->npcCount; i++) {
        if (worldNPCs[i].base.texture.id != 0) {
            DrawTexturePro(worldNPCs[i].base.texture, 
                (Rectangle){0, 0, (float)worldNPCs[i].base.texture.width, (float)worldNPCs[i].base.texture.height},
                worldNPCs[i].base.bounds, (Vector2){0, 0}, 0.0f, WHITE);
        }
    }

    bool is_lawnmower_phase = false;
    bool has_lawnmower = false;
    if (strcmp(game_context->story.day_folder, "day3") == 0) {
        StoryPhase* active_phase = GetActivePhase(&game_context->story);
        if (active_phase && strcmp(active_phase->name, "SET2-PHASE2") == 0) {
            is_lawnmower_phase = true;
            for (int i = 0; i < game_context->picked_up_count; i++) {
                if (strcmp(game_context->picked_up_registry[i], "lawnmower") == 0) {
                    has_lawnmower = true; break;
                }
            }
        }
    }

    if (is_lawnmower_phase && has_lawnmower) {
        // Special lawnmower mode drawing
        bool is_moving = IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D);
        
        // Determine effective visual direction (fallback to horizontal if moving vertically)
        int effective_dir = player->direction;
        if (effective_dir == 0 || effective_dir == 3) effective_dir = player->last_horiz_dir;
        bool should_flip = (effective_dir == 1);

        // 1. Draw Character with special animation
        int char_frame = is_moving ? ((int)(GetTime() * 8) % 2) : 1;
        float char_tex_w = (float)player->lawnmower_mode.width;
        float char_tex_h = (float)player->lawnmower_mode.height;
        float frame_w = char_tex_w / 2.0f;
        Rectangle char_src = { (float)char_frame * frame_w, 0, should_flip ? -frame_w : frame_w, char_tex_h };
        Rectangle char_dest = { player->position.x, player->position.y, player->size.x, player->size.y };
        DrawTexturePro(player->lawnmower_mode, char_src, char_dest, (Vector2){0, 0}, 0.0f, WHITE);

        // 2. Draw Lawnmower with animation
        int mower_frame = is_moving ? ((int)(GetTime() * 8) % 2) : 0;
        float mower_tex_w = (float)player->lawnmower_item.width;
        float mower_tex_h = (float)player->lawnmower_item.height;
        float mower_frame_w = mower_tex_w / 2.0f;
        Rectangle mower_src = { (float)mower_frame * mower_frame_w, 0, should_flip ? -mower_frame_w : mower_frame_w, mower_tex_h };
        
        float mower_w = mower_frame_w;
        float mower_h = mower_tex_h;
        Rectangle lawnmower_rect = {0, 0, mower_w, mower_h};
        
        // Lawnmower is always on the side (left/right) in this mode
        if (effective_dir == 1) { // left
            lawnmower_rect.x = player->position.x - mower_w; 
            lawnmower_rect.y = player->position.y + player->size.y - mower_h; 
        } else { // right (effective_dir == 2)
            lawnmower_rect.x = player->position.x + player->size.x; 
            lawnmower_rect.y = player->position.y + player->size.y - mower_h; 
        }     
        DrawTexturePro(player->lawnmower_item, mower_src, lawnmower_rect, (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        DrawCharacter(player);
    }

    // Draw interaction tooltips (on top of entities)
    for (int i = 0; i < game_context->npcCount; i++){
        if (worldNPCs[i].base.isActive) DrawText("!", worldNPCs[i].base.bounds.x + worldNPCs[i].base.bounds.width / 2, worldNPCs[i].base.bounds.y - 40, 50, YELLOW);
    }
    for (int i = 0; i < game_context->itemCount; i++){
        if (!worldItems[i].picked_up){
            if (worldItems[i].base.isActive) DrawText("!", worldItems[i].base.bounds.x + 20, worldItems[i].base.bounds.y - 30, 50, YELLOW);
        }
    }
    
    EndMode2D();
}

void LoadCutsceneFrame(Scene *scene, int frame_index, Settings *game_settings){
    // Load cutscene frame
    if (scene->current_cutscene_frame_texture.id != 0) UnloadTexture(scene->current_cutscene_frame_texture);
    char path[100];
    sprintf(path, "../assets/videos/cutscene/frame%04d.qoi", frame_index);
    scene->current_cutscene_frame_texture = LoadTexture(path);
}

void LoadMenuFrame(Scene *scene, int frame_index, bool is_save_available){
    // Load menu frame
    if (scene->current_cutscene_frame_texture.id != 0) UnloadTexture(scene->current_cutscene_frame_texture);
    char path[100];
    const char* folder = is_save_available ? "saved_game" : "new_game";
    sprintf(path, "../assets/videos/main_menu/%s/frame%04d.qoi", folder, frame_index);
    scene->current_cutscene_frame_texture = LoadTexture(path);
}

void LoadSettingsFrame(Scene *scene, int frame_index){
    // Load settings frame
    if (scene->current_cutscene_frame_texture.id != 0) UnloadTexture(scene->current_cutscene_frame_texture);
    char path[100];
    sprintf(path, "../assets/videos/slider_bar/frame%04d.qoi", frame_index);
    scene->current_cutscene_frame_texture = LoadTexture(path);
}

void LoadKnobFrame(Scene *scene, int frame_index){
    // Load knob frame
    if (scene->current_knob_frame_texture.id != 0) UnloadTexture(scene->current_knob_frame_texture);
    char path[100];
    sprintf(path, "../assets/videos/slider_knob/frame%04d.qoi", frame_index);
    scene->current_knob_frame_texture = LoadTexture(path);
}

void LoadPauseFrame(Scene *scene, int frame_index){
    // Load pause menu frame
    if (scene->current_cutscene_frame_texture.id != 0) UnloadTexture(scene->current_cutscene_frame_texture);
    char path[100];
    sprintf(path, "../assets/videos/esc_option/frame%04d.qoi", frame_index);
    scene->current_cutscene_frame_texture = LoadTexture(path);
}

void ClearCutscene(Scene* scene){
    // Clear cutscene
    if (scene->current_cutscene_frame_texture.id != 0){
        UnloadTexture(scene->current_cutscene_frame_texture);
        scene->current_cutscene_frame_texture.id = 0;
    }
}

void CloseScene(Scene* scene){
    // Close scene
    UnloadTexture(scene->mainmenu_background);
    UnloadTexture(scene->pause_menu_background);
    UnloadTexture(scene->vignette);
    if (scene->current_knob_frame_texture.id != 0) UnloadTexture(scene->current_knob_frame_texture);
    ClearCutscene(scene);
}
