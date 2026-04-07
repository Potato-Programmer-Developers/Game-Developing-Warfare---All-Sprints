/**
 * @file phone.h
 * @brief Logic and data structures for the in-game mobile phone, notifications, and interaction.
 * 
 * Update History:
 * - 2026-03-22: Definition of the basic `Phone` struct and single-message notifications. (Goal: Create 
 *                a simple notification system for player-facing story context.)
 * - 2026-04-03: Expanded `PhoneChoice` and `PhoneMessage` for branching narrative. (Goal: Support 
 *                a more complex phone experience with reply options and dream sequence hand-offs.)
 * - 2026-04-05: Integrated the "Sequential Message Queue" data structure. (Goal: Enable 
 *                multi-message conversations within a single notification sequence.)
 * 
 * Revision Details:
 * - Added `PhoneChoice` struct with `dream_lines` and `dream_count` for nocturnal event triggers.
 * - Expanded the `Phone` struct to hold a `sequence` of up to 8 `PhoneMessage` objects.
 * - Introduced `PhoneState` enum entries for `PHONE_SHOWING_REPLY` and `PHONE_NOTIFICATION`.
 * - Unified the notification and opened UI into a single state-machine-driven module.
 * 
 * Authors: Andrew Zhuo
 */

#ifndef PHONE_H
#define PHONE_H

#include "raylib.h"
#include <stdbool.h>

struct GameContext;

typedef enum {
    PHONE_IDLE,               // Hidden/Inactive state
    PHONE_NOTIFICATION,       // Showing a pop-up alert
    PHONE_OPENED,             // Full-screen messaging interface active
    PHONE_SHOWING_REPLY       // Displaying the player's chosen response, waits a bit before next message
} PhoneState;

/**
 * @brief A phone choice with an optional dream sequence attached.
 */
typedef struct {
    char text[128];             // Reply text or "DON'T RESPOND"
    char dream_lines[4][128];   // [FULL TEXT] lines attached to this choice
    int dream_count;            // Number of dream lines
} PhoneChoice;

/**
 * @brief A single incoming phone message with optional replies.
 */
typedef struct {
    char text[128];              // Message text
    PhoneChoice choices[4];      // Choices for the message
    int choice_count;            // Number of choices
} PhoneMessage;

/**
 * @brief Container for phone data and active message state.
 */
typedef struct {
    int notif_width;                   // Width of the notification pop-up
    int notif_height;                  // Height of the notification pop-up

    int phone_width;                   // Width of the full phone screen
    int phone_height;                  // Height of the full phone screen
    Rectangle phoneBox;                // Hitbox for the full phone screen

    char sender[64];                   // Sender name
    PhoneMessage sequence[8];          // Sequential messages
    int sequence_count;                // Total messages in sequence
    int current_msg_index;             // Index of the current message being shown
    
    char selected_reply[64];           // The reply chosen by the player (if any)
    PhoneState state;                  // Current status of the phone UI
    float notif_timer;                 // Internal timer for auto-hiding notifications
    float reply_timer;                 // Internal timer for showing reply before moving to next message
    int hover_index;                   // Which button is currently hovered (-1 for none)
    bool already_triggered;            // To prevent multiple triggers of the same event
} Phone;

/**
 * @brief Resets phone data to a clean, idle state.
 *
 * @param phone Pointer to the phone structure.
 */
void InitPhone(Phone *phone);

/**
 * @brief Triggers a phone notification with a single message and two replies.
 * 
 * @param phone Pointer to phone
 * @param sender Sender name
 * @param msg Message body (legacy, will be added as first sequence item).
 * @param reply1 The first reply option (legacy).
 * @param reply2 The second reply option (legacy).
 */
void TriggerPhoneNotification(Phone *phone, const char *sender, const char *msg, const char *reply1, const char *reply2);

/**
 * @brief Triggers a new sequence of messages.
 * 
 * @param phone Pointer to phone
 * @param sender Sender name
 * @param msgs Array of messages
 * @param count Count of messages
 */
void TriggerPhoneSequence(Phone *phone, const char *sender, PhoneMessage *msgs, int count);

/**
 * @brief Updates phone timers and auto-state transitions.
 *
 * @param phone Pointer to the phone.
 * @param delta Time since last frame.
 */
void UpdatePhone(Phone *phone, float delta);

/**
 * @brief Renders the phone UI (notification or full screen) to the screen.
 *
 * @param phone Pointer to the phone to draw.
 */
void DrawPhone(Phone *phone);

/**
 * @brief Handles player input for selecting replies and opening/closing the phone.
 *
 * @param phone Pointer to the phone.
 * @param ctx Pointer to the game context (to store dream choices).
 */
void HandlePhoneInput(Phone *phone, struct GameContext *ctx);

#endif
