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

    // Apply player state
    player->position = data->position;
    player->direction = data->direction;
    for (int i = 0; i < data->inventory_count; i++){
        strcpy(player->inventory[i], data->inventory[i]);
        player->item_count[i] = data->item_count[i];
    }
    player->inventory_count = data->inventory_count;
    player->hallucination = data->player_hallucination_level;

    // Apply world item states
    for (int i = 0; i < itemCount; i++){
        worldItems[i].picked_up = data->picked_up_items[i];
    }

    // Apply game settings
    game_settings->game_volume = data->volume;
    SetMasterVolume(game_settings->game_volume);
}

void SaveData(Character* player, Item worldItems[], int itemCount, Settings* game_settings){
    /* Save the data */
    Data data = {0};

    // Save player state
    data.position = player->position;
    data.direction = player->direction;
    for (int i = 0; i < player->inventory_count; i++){
        strcpy(data.inventory[i], player->inventory[i]);
        data.item_count[i] = player->item_count[i];
    }
    data.inventory_count = player->inventory_count;
    data.player_hallucination_level = player->hallucination;

    // Save world item states
    for (int i = 0; i < itemCount; i++){
        data.picked_up_items[i] = worldItems[i].picked_up;
    }

    // Save game settings
    data.volume = game_settings->game_volume;

    // Create data directory if it doesn't exist
    if (!DirectoryExists("../data")){
        MakeDirectory("../data");
    }

    // Save the data
    SaveFileData("../data/data.dat", &data, sizeof(Data));
}