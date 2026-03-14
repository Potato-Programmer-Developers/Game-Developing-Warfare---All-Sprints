#include <stdbool.h>
#include "settings.h"
#include "raylib.h"

#ifndef INTERACTION_H
#define INTERACTION_H

typedef struct {
    Rectangle bounds;
    const char* dialoguePath;
    bool isActive;
} Interactable;

#endif // INTERACTION_H