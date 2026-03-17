/*
This file contains the implementation of the data structures.

Made by Andrew Zhuo.
*/

#include <string.h>
#include "data.h"
#include "raylib.h"
#include "settings.h"

Data LoadData(Settings* game_settings){
    /* Load the data */
    Data data = {0};
    int file_size = 0;
    if (FileExists("../data/data.dat")){
        unsigned char* file_data = LoadFileData("../data/data.dat", &file_size);

        if (file_data != NULL && file_size == sizeof(Data)){
            memcpy(&data, file_data, sizeof(Data));
        }

        UnloadFileData(file_data);
    } else{
        data.position = (Vector2){
            (float)game_settings->window_width / 2,
            (float)game_settings->window_height / 2
        };
        data.volume = game_settings->game_volume;
    }

    return data;
}

void ApplyData(Character* player, Item worldItems[], int itemCount, Settings* game_settings, Data* data){
    /* Apply the data */
    player->position = data->position;
    player->direction = data->direction;
    for (int i = 0; i < data->inventory_count; i++){
        strcpy(player->inventory[i], data->inventory[i]);
    }
    player->inventory_count = data->inventory_count;

    // Apply world item states
    for (int i = 0; i < itemCount; i++){
        worldItems[i].picked_up = data->picked_up_items[i];
    }

    game_settings->game_volume = data->volume;
    SetMasterVolume(game_settings->game_volume);
}

void SaveData(Character* player, Item worldItems[], int itemCount, Settings* game_settings){
    /* Save the data */
    Data data = {0};
    data.position = player->position;
    data.direction = player->direction;
    for (int i = 0; i < player->inventory_count; i++){
        strcpy(data.inventory[i], player->inventory[i]);
    }
    data.inventory_count = player->inventory_count;

    // Save world item states
    for (int i = 0; i < itemCount; i++){
        data.picked_up_items[i] = worldItems[i].picked_up;
    }

    data.volume = game_settings->game_volume;
    SaveFileData("../data/data.dat", &data, sizeof(Data));
}