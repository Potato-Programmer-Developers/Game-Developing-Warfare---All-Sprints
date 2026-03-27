/**
 * @file phone.c
 * @brief Implementation of the in-game mobile phone notification system.
 * 
 * Handles pop-up notifications and a dedicated phone UI for reading
 * and responding to narrative messages.
 * 
 * Authors: Andrew Zhuo
 */

#include "phone.h"
#include "raylib.h"
#include <string.h>

/** @brief Resets phone state and clears message buffers. */
void InitPhone(Phone *phone){
    phone->notif_width = 300;
    phone->notif_height = 60;
    phone->phone_width = 400;
    phone->phone_height = 600;
    
    phone->state = PHONE_IDLE;
    phone->notif_timer = 0;
    phone->hover_index = -1;
    phone->already_triggered = false;
    memset(phone->message, 0, sizeof(phone->message));
    memset(phone->replies, 0, sizeof(phone->replies));
    memset(phone->selected_reply, 0, sizeof(phone->selected_reply));
}

/**
 * @brief Triggers a new message notification.
 * 
 * Sets the notification to auto-dismiss after 5 seconds if not opened.
 */
void TriggerPhoneNotification(Phone *phone, const char *msg, const char *reply1, const char *reply2){
    strncpy(phone->message, msg, sizeof(phone->message) - 1);
    strncpy(phone->replies[0], reply1, sizeof(phone->replies[0]) - 1);
    strncpy(phone->replies[1], reply2, sizeof(phone->replies[1]) - 1);

    // Init 5-second countdown
    phone->notif_timer = 5.0f;
    phone->state = PHONE_NOTIFICATION;
    phone->already_triggered = true;
}

/** @brief Updates the notification auto-dismiss timer. */
void UpdatePhone(Phone *phone, float delta){
    if (phone->state == PHONE_NOTIFICATION){
        phone->notif_timer -= delta;
        if (phone->notif_timer <= 0){
            phone->state = PHONE_IDLE;
        }
    }
}

/**
 * @brief Renders the phone UI overlays.
 * 
 * Handles both the small 'New Message' pop-up and the full-screen 
 * messaging interface.
 */
void DrawPhone(Phone *phone){
    if (phone->state == PHONE_IDLE) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    if (phone->state == PHONE_NOTIFICATION){
        // --- Pop-up Notification (Bottom Right) ---
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
        // --- Full Phone Interface (Center) ---
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
        DrawText("Sender:", screen.x + 20, screen.y + 20, 15, WHITE);
        DrawText(phone->message, screen.x + 20, screen.y + 40, 20, WHITE);

        if (phone->state == PHONE_OPENED){
            int btnHeight = 50;
            Rectangle btn1 = {phone->phoneBox.x + 20, phone->phoneBox.y + phone->phoneBox.height - 120, phone->phoneBox.width - 40, (float)btnHeight};
            Rectangle btn2 = {phone->phoneBox.x + 20, phone->phoneBox.y + phone->phoneBox.height - 60, phone->phoneBox.width - 40, (float)btnHeight};

            // Button 1
            DrawRectangleRec(btn1, (phone->hover_index == 0) ? LIGHTGRAY : GRAY);
            DrawRectangleLinesEx(btn1, 2, (phone->hover_index == 0) ? YELLOW : BLACK);
            DrawText(TextFormat("1: %s", phone->replies[0]), btn1.x + 10, btn1.y + 15, 20, BLACK);

            // Button 2
            DrawRectangleRec(btn2, (phone->hover_index == 1) ? LIGHTGRAY : GRAY);
            DrawRectangleLinesEx(btn2, 2, (phone->hover_index == 1) ? YELLOW : BLACK);
            DrawText(TextFormat("2: %s", phone->replies[1]), btn2.x + 10, btn2.y + 15, 20, BLACK);
        } else if (phone->state == PHONE_SHOWING_REPLY){
            // Narrative display of sent message
            DrawText("Me:", screen.x + 20, screen.y + 100, 15, GREEN);
            DrawText(phone->selected_reply, screen.x + 20, screen.y + 120, 20, WHITE);
            DrawText("Press 'R' to close", phone->phoneBox.x + 20, phone->phoneBox.y + phone->phoneBox.height - 40, 15, GRAY);
        }
    }
}

/**
 * @brief Handles keyboard input for opening/closing/replying on the phone.
 * 
 * Uses 'R' as the primary interact key and 'ENTER' for replying.
 */
void HandlePhoneInput(Phone *phone){
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
        // Reply buttons layout (must match DrawPhone)
        int btnHeight = 50;
        Rectangle btn1 = {phone->phoneBox.x + 20, phone->phoneBox.y + phone->phoneBox.height - 120, phone->phoneBox.width - 40, (float)btnHeight};
        Rectangle btn2 = {phone->phoneBox.x + 20, phone->phoneBox.y + phone->phoneBox.height - 60, phone->phoneBox.width - 40, (float)btnHeight};

        // Handle Hover Selection
        Vector2 mousePos = GetMousePosition();
        if (CheckCollisionPointRec(mousePos, btn1)) phone->hover_index = 0;
        else if (CheckCollisionPointRec(mousePos, btn2)) phone->hover_index = 1;
        else phone->hover_index = -1;

        // Handle Selection (Click or Keys 1/2)
        bool selected = false;
        int choice = -1;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && phone->hover_index != -1) {
            selected = true;
            choice = phone->hover_index;
        } else if (IsKeyPressed(KEY_ONE)) {
            selected = true;
            choice = 0;
        } else if (IsKeyPressed(KEY_TWO)) {
            selected = true;
            choice = 1;
        }

        if (selected) {
            strncpy(phone->selected_reply, phone->replies[choice], sizeof(phone->selected_reply) - 1);
            phone->state = PHONE_SHOWING_REPLY;
        }
    }
}
