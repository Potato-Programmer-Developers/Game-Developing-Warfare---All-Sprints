/**
 * @file scene.c
 * @brief Implementation of high-level rendering and scene management.
 */

#include "scene.h"
#include <stdio.h>
#include <string.h>
#include "interactive.h"
#include "story.h"

Scene InitScene(Settings* game_settings){
    Scene new_scene = {0};
    new_scene.mainmenu_background = LoadTexture("../assets/images/background/main_menu/main_menu.png");
    new_scene.pause_menu_background = LoadTexture("../assets/images/background/pause/pause.png");
    new_scene.vignette = LoadTexture("../assets/images/background/vignette/vignette.png");
    new_scene.current_cutscene_frame_texture = (Texture2D){0};
    new_scene.current_knob_frame_texture = (Texture2D){0};
    new_scene.cutscene_timer = 0.0f;
    new_scene.current_cutscene_frame = 0;
    return new_scene;
}

void DrawGame(Scene *game_scene, Settings *game_settings, Interactive *game_interactive,
              Map *game_map, Character *player, Dialogue *game_dialogue, GameContext *game_context,
              GameState *game_state, NPC worldNPCs[], Item worldItems[]){
    
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (*game_state == MAINMENU) {
        DrawMainMenu(game_scene, game_interactive);
    } else if (*game_state == SETTINGS) {
        DrawSettings(game_scene, game_settings, game_interactive);
    } else if (*game_state == PAUSE) {
        DrawPauseMenu(game_scene, game_settings, game_interactive);
    } else if (*game_state == PHOTO_CUTSCENE) {
        DrawTexturePro(game_scene->current_cutscene_frame_texture,
            (Rectangle){0, 0, (float)game_scene->current_cutscene_frame_texture.width, (float)game_scene->current_cutscene_frame_texture.height},
            (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
            (Vector2){0, 0}, 0.0f, WHITE);
            
        if (game_scene->cutscene_timer < 5.0f) {
            DrawText("Double click 'Enter' to skip", GetScreenWidth() - 300, GetScreenHeight() - 40, 20, LIGHTGRAY);
        }
    } else {
        DrawGameplay(game_scene, game_settings, game_interactive, game_map, player, worldNPCs, worldItems, game_context);
        
        DrawTexturePro(game_scene->vignette,
            (Rectangle){0, 0, (float)game_scene->vignette.width, (float)game_scene->vignette.height},
            (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
            (Vector2){0, 0}, 0.0f, WHITE);

        if (*game_state == DIALOGUE_CUTSCENE){
            DrawRectangle(0, GetScreenHeight() - 200, GetScreenWidth(), 200, Fade(BLACK, 0.8f));
            const char *line = game_dialogue->lines[game_dialogue->current_line];
            DrawText(line, GetScreenWidth() / 2 - MeasureText(line, 20) / 2, GetScreenHeight() - 170, 20, WHITE);

            if (game_dialogue->current_line >= game_dialogue->line_count - 1 && game_dialogue->choice_count > 0){
                for (int i = 0; i < game_dialogue->choice_count; i++) {
                    char choiceText[128];
                    sprintf(choiceText, "%d. %s", i + 1, game_dialogue->choices[i]);
                    DrawText(choiceText, 100, GetScreenHeight() - 130 + (i * 30), 20, YELLOW);
                }
            } else {
                DrawText("Press 'SPACE' to continue", GetScreenWidth() - 300, GetScreenHeight() - 40, 20, GRAY);
            }
        }
        DrawPhone(&game_context->phone);
        
        StoryPhase* active_phase = GetActivePhase(&game_context->story);
        if (active_phase && active_phase->quest_count > 0) {
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
                sprintf(qText, "%s%s", prefix, q->description);
                DrawText(qText, 35, 55 + (i * 25), 18, qColor);
            }
            
            if (strcmp(active_phase->name, "SET1-PHASE1") == 0) {
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
    }
    EndDrawing();
}

void DrawMainMenu(Scene* scene, Interactive* game_interactive){
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
    DrawTexturePro(scene->pause_menu_background, 
        (Rectangle){0, 0, (float)scene->pause_menu_background.width, (float)scene->pause_menu_background.height},
        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0, 0}, 0.0f, WHITE);
    
    DrawTexture(game_interactive->continue_button, game_interactive->continue_bounds.x, game_interactive->continue_bounds.y, game_interactive->is_continue_hovered ? GRAY : WHITE);
    DrawTexture(game_interactive->settings_button, game_interactive->settings_bounds.x, game_interactive->settings_bounds.y, game_interactive->is_settings_hovered ? GRAY : WHITE);
    DrawTexture(game_interactive->main_menu_button, game_interactive->main_menu_bounds.x, game_interactive->main_menu_bounds.y, game_interactive->is_main_menu_hovered ? GRAY : WHITE);
    DrawTexture(game_interactive->quit_button, game_interactive->quit_bounds.x, game_interactive->quit_bounds.y, game_interactive->is_quit_hovered ? GRAY : WHITE);
}

void DrawSettings(Scene* scene, Settings* game_settings, Interactive* game_interactive){
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
    BeginMode2D(game_context->camera);
    DrawMap(game_map);
    DrawCharacter(player);
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
    if (scene->current_cutscene_frame_texture.id != 0) UnloadTexture(scene->current_cutscene_frame_texture);
    char path[100];
    sprintf(path, "../assets/videos/cutscene/frame%04d.qoi", frame_index);
    scene->current_cutscene_frame_texture = LoadTexture(path);
}

void LoadMenuFrame(Scene *scene, int frame_index, bool is_save_available) {
    if (scene->current_cutscene_frame_texture.id != 0) UnloadTexture(scene->current_cutscene_frame_texture);
    char path[100];
    const char* folder = is_save_available ? "saved_game" : "new_game";
    sprintf(path, "../assets/videos/main_menu/%s/frame%04d.qoi", folder, frame_index);
    scene->current_cutscene_frame_texture = LoadTexture(path);
}

void LoadSettingsFrame(Scene *scene, int frame_index) {
    if (scene->current_cutscene_frame_texture.id != 0) UnloadTexture(scene->current_cutscene_frame_texture);
    char path[100];
    sprintf(path, "../assets/videos/slider_bar/frame%04d.qoi", frame_index);
    scene->current_cutscene_frame_texture = LoadTexture(path);
}

void LoadKnobFrame(Scene *scene, int frame_index) {
    if (scene->current_knob_frame_texture.id != 0) UnloadTexture(scene->current_knob_frame_texture);
    char path[100];
    sprintf(path, "../assets/videos/slider_knob/frame%04d.qoi", frame_index);
    scene->current_knob_frame_texture = LoadTexture(path);
}

void ClearCutscene(Scene* scene){
    if (scene->current_cutscene_frame_texture.id != 0){
        UnloadTexture(scene->current_cutscene_frame_texture);
        scene->current_cutscene_frame_texture.id = 0;
    }
}

void CloseScene(Scene* scene){
    UnloadTexture(scene->mainmenu_background);
    UnloadTexture(scene->pause_menu_background);
    UnloadTexture(scene->vignette);
    if (scene->current_knob_frame_texture.id != 0) UnloadTexture(scene->current_knob_frame_texture);
    ClearCutscene(scene);
}
