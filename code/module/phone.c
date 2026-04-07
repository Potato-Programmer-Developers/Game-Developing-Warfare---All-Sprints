/**
 * @file phone.c
 * @brief Manages the in-game mobile device, sequential messaging, interactive replies, and dream sequence hand-offs.
 * 
 * Update History:
 * - 2026-03-22: Prototype implementation of "Passive Notifications." (Goal: Provide a way for the 
 *                player to receive story context without interrupting gameplay.)
 * - 2026-04-04: Implemented the "Interactive Phone Engine" and Queue. (Goal: Support full-blown 
 *                conversations on the phone where the player can choose multiple replies, leading 
 *                 to branching narrative paths.)
 * - 2026-04-05: Added "Manual UI persistence" and stability fixes. (Goal: Fix the UX issue where 
 *                the phone would auto-close if no new messages were active, allowing manual use.)
 * - 2026-04-05: Integrated "Dream Sequence" triggers into choice selection. (Goal: Bridge the 
 *                phone system to the screen-fade systems for seamless transitions into nightmare scenes.)
 * 
 * Revision Details:
 * - Migrated from a single-message buffer to an 8-message `PhoneMessage` queue in `PhoneSequence`.
 * - Implemented choice tracking for up to 4 branching paths per message with hover detection.
 * - Integrated `notif.wav` playback triggers into the phone's internal state machine updates.
 * - Added a timer-based delay for "Sent" message displays for better narrative pacing.
 * - Re-wrote `HandlePhoneInput` to maintain the `PHONE_OPENED` state regardless of message queue status.
 * 
 * Authors: Andrew Zhuo
 */

#include "phone.h"
#include "raylib.h"
#include "game_context.h"
#include <string.h>

void InitPhone(Phone *phone){
    // Initialize phone and clear message size
    phone->notif_width = 300;
    phone->notif_height = 60;
    phone->phone_width = 400;
    phone->phone_height = 600;

    // Initialize phone state and clear message buffers
    phone->state = PHONE_IDLE;
    phone->notif_timer = 0;
    phone->reply_timer = 0;
    phone->hover_index = -1;
    phone->already_triggered = false;
    memset(phone->sender, 0, sizeof(phone->sender));
    memset(phone->sequence, 0, sizeof(phone->sequence));
    phone->sequence_count = 0;
    phone->current_msg_index = 0;
    memset(phone->selected_reply, 0, sizeof(phone->selected_reply));
}

void TriggerPhoneNotification(Phone *phone, const char *sender, const char *msg, const char *reply1, const char *reply2){
    // Legacy helper to trigger single message
    InitPhone(phone);
    phone->sequence_count = 1;
    phone->current_msg_index = 0;
    if (sender) strncpy(phone->sender, sender, sizeof(phone->sender)-1);

    // Set the message and choices
    PhoneMessage *m = &phone->sequence[0];
    strncpy(m->text, msg, sizeof(m->text)-1);
    m->choice_count = 0;
    if (reply1 && strlen(reply1) > 0) {
        strncpy(m->choices[0].text, reply1, sizeof(m->choices[0].text)-1);
        m->choice_count++;
    }
    if (reply2 && strlen(reply2) > 0) {
        strncpy(m->choices[1].text, reply2, sizeof(m->choices[1].text)-1);
        m->choice_count++;
    }

    // Init 5-second countdown
    phone->notif_timer = 5.0f;
    phone->state = PHONE_NOTIFICATION;
    phone->already_triggered = true;
}

void TriggerPhoneSequence(Phone *phone, const char *sender, PhoneMessage *msgs, int count) {
    InitPhone(phone);
    if (sender) strncpy(phone->sender, sender, sizeof(phone->sender)-1);
    phone->sequence_count = (count > 8) ? 8 : count;
    
    // Copy messages to phone sequence
    for (int i = 0; i < phone->sequence_count; i++){
        phone->sequence[i] = msgs[i];
    }
    phone->notif_timer = 5.0f;
    phone->state = PHONE_NOTIFICATION;
    phone->already_triggered = true;
}

void UpdatePhone(Phone *phone, float delta){
    // Update the notification auto-dismiss timer
    if (phone->state == PHONE_NOTIFICATION){
        phone->notif_timer -= delta;
        if (phone->notif_timer <= 0){
            phone->state = PHONE_IDLE;
        }
    } else if (phone->state == PHONE_SHOWING_REPLY) {
        // Wait briefly after choosing a reply before showing the next message
        phone->reply_timer += delta;
        if (phone->reply_timer >= 1.5f) {
            phone->reply_timer = 0;
            phone->current_msg_index++;
            if (phone->current_msg_index >= phone->sequence_count) {
                 phone->state = PHONE_IDLE; // Sequence complete
            } else {
                 phone->state = PHONE_OPENED; // Show next message
            }
        }
    }
}

void DrawPhone(Phone *phone){
    // Draw the phone UI overlays
    if (phone->state == PHONE_IDLE) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    if (phone->state == PHONE_NOTIFICATION){
        // Pop-up Notification (Bottom Right)
        int margin = 20;
        Rectangle box = {
            (float)screenWidth - phone->notif_width - margin,
            (float)screenHeight - phone->notif_height - margin,
            (float)phone->notif_width, (float)phone->notif_height
        };
        
        DrawRectangleRec(box, Fade(BLACK, 0.8f));
        DrawRectangleLinesEx(box, 2, GOLD);
        DrawText("New Message! Press 'R'", box.x + 10, box.y + 20, 20, WHITE);
    } else if (phone->state == PHONE_OPENED || phone->state == PHONE_SHOWING_REPLY){
        // Full Phone Interface (Center)
        phone->phoneBox = (Rectangle){
            (float)screenWidth / 2.0f - phone->phone_width / 2.0f,
            (float)screenHeight / 2.0f - phone->phone_height / 2.0f,
            (float)phone->phone_width, (float)phone->phone_height
        };

        // UI Frame
        DrawRectangleRec(phone->phoneBox, DARKGRAY);
        DrawRectangleLinesEx(phone->phoneBox, 4, BLACK);
        
        // Status Bar
        DrawRectangle(phone->phoneBox.x, phone->phoneBox.y, phone->phoneBox.width, 40, BLACK);
        DrawText("PHONE", phone->phoneBox.x + phone->phoneBox.width / 2.0f - MeasureText("PHONE", 20) / 2.0f, phone->phoneBox.y + 10, 20, RAYWHITE);

        // Screen area
        Rectangle screen = {phone->phoneBox.x + 10, phone->phoneBox.y + 50, phone->phoneBox.width - 20, phone->phoneBox.height - 180};
        DrawRectangleRec(screen, Fade(SKYBLUE, 0.2f));

        // Get the current message
        PhoneMessage *cm = NULL;
        if (phone->current_msg_index < phone->sequence_count){
            cm = &phone->sequence[phone->current_msg_index];
        }

        // Draw the current message
        if (cm){
            DrawText("Sender:", screen.x + 20, screen.y + 20, 15, WHITE);
            DrawText(phone->sender, screen.x + 80, screen.y + 20, 15, GOLD);
            DrawText(cm->text, screen.x + 20, screen.y + 40, 20, WHITE);
        }

        // Draw choices if in OPENED state
        if (phone->state == PHONE_OPENED && cm){
            int btnHeight = 40;
            int margin = 5;
            for (int i = 0; i < cm->choice_count; i++){
                Rectangle btn = {
                    phone->phoneBox.x + 20,
                    phone->phoneBox.y + phone->phoneBox.height - 180 + (i * (btnHeight + margin)),
                    phone->phoneBox.width - 40,
                    (float)btnHeight
                };
                DrawRectangleRec(btn, (phone->hover_index == i) ? LIGHTGRAY : GRAY);
                DrawRectangleLinesEx(btn, 2, (phone->hover_index == i) ? YELLOW : BLACK);
                DrawText(TextFormat("%d: %s", i + 1, cm->choices[i].text), btn.x + 10, btn.y + 10, 20, BLACK);
            }
        } else if (phone->state == PHONE_SHOWING_REPLY){
            // Narrative display of sent message
            if (strcmp(phone->selected_reply, "DON'T RESPOND") != 0){
                DrawText("Me:", screen.x + 20, screen.y + 100, 15, GREEN);
                DrawText(phone->selected_reply, screen.x + 20, screen.y + 120, 20, WHITE);
            }
            DrawText("Press 'R' to close", phone->phoneBox.x + 20, phone->phoneBox.y + phone->phoneBox.height - 40, 15, GRAY);
        }
    }
}

void HandlePhoneInput(Phone *phone, struct GameContext *ctx){
    // Toggle Open/Close
    if (IsKeyPressed(KEY_R)){
        if (phone->state == PHONE_IDLE || phone->state == PHONE_NOTIFICATION) {
            phone->state = PHONE_OPENED;
        } else {
            phone->state = PHONE_IDLE;
        }
        return;
    }

    if (phone->state == PHONE_IDLE) return;

    if (phone->state == PHONE_OPENED){
        PhoneMessage *cm = NULL;

        // Get the current message
        if (phone->current_msg_index < phone->sequence_count){
            cm = &phone->sequence[phone->current_msg_index];
        }

        if (!cm){
            // Stay in OPENED state even if no message is active
            // This allows the player to manually open the phone in the tutorial
            return;
        }

        if (cm->choice_count == 0){
            // Auto advance since no choices
            phone->reply_timer = 0;
            phone->selected_reply[0] = '\0';
            phone->state = PHONE_SHOWING_REPLY;
            return;
        }

        int btnHeight = 40;
        int margin = 5;
        Vector2 mousePos = GetMousePosition();
        phone->hover_index = -1;

        // Check for hover
        for (int i = 0; i < cm->choice_count; i++){
            Rectangle btn = {
                phone->phoneBox.x + 20,
                phone->phoneBox.y + phone->phoneBox.height - 180 + (i * (btnHeight + margin)),
                phone->phoneBox.width - 40,
                (float)btnHeight
            };
            if (CheckCollisionPointRec(mousePos, btn)){
                phone->hover_index = i;
            }
        }

        // Handle Selection (Click or Keys 1/2/3/4)
        bool selected = false;
        int choice = -1;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && phone->hover_index != -1){
            selected = true; choice = phone->hover_index;
        } else if (IsKeyPressed(KEY_ONE) && cm->choice_count >= 1){
            selected = true; choice = 0;
        } else if (IsKeyPressed(KEY_TWO) && cm->choice_count >= 2){
            selected = true; choice = 1;
        } else if (IsKeyPressed(KEY_THREE) && cm->choice_count >= 3){
            selected = true; choice = 2;
        } else if (IsKeyPressed(KEY_FOUR) && cm->choice_count >= 4){
            selected = true; choice = 3;
        }

        // Handle Selection
        if (selected){
            strncpy(phone->selected_reply, cm->choices[choice].text, sizeof(phone->selected_reply) - 1);
            phone->state = PHONE_SHOWING_REPLY;
            phone->reply_timer = 0; // reset wait timer
            
            // If the choice had dream lines, copy them to GameContext
            if (ctx && cm->choices[choice].dream_count > 0){
                ctx->dream_count = cm->choices[choice].dream_count;
                ctx->dream_current = 0;
                for (int i = 0; i < ctx->dream_count; i++){
                    strncpy(ctx->dream_lines[i], cm->choices[choice].dream_lines[i], 127);
                }
            }
        }
    }
}
