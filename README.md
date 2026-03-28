> Project: Aisling
> By Team Potato

# A Game to Thrill the World.

This is a C-based psychological thriller game built with the [Raylib](https://www.raylib.com/) library. It features a modular architecture, Tiled map support, and a dedicated dialogue system, it serves as a foundation for a top-down narrative experience.

## Features

- **Game States**: Fully implemented Main Menu, Settings, Gameplay, and Pause states.
- **Map System**: Supports Tiled JSON maps using the `cute_tiled.h` integration.
- **Character System**: Dynamic movement with idle, walk, and run animations.
- **Dialogue System**: Scripted interactions with world objects through text files.
- **Audio Architecture**: Managed background music and SFX (e.g., screams).
- **Post-Processing**: Screen-space vignette effect for enhanced atmosphere for the game.

## Project Structure

- `code/`: Source code, including:
    - `main.c`: Core game loop and state management.
    - `module/`: Independent modules (audio, character, dialogue, map, etc.).
    - `include/`: Header files for all modules.
- `assets/`: Game resources (images, audios, maps, text).

## Getting Started

There is two ways to run the game:
1. Compile and run it on your terminal.
2. Run the executable file directly.

### Prerequisites

For the first method:
- **Make/GCC**: Standard development tools are required.
- **Our Repository**: Download the source code and assets from the [GitHub repository](https://github.com/Potato-Programmer-Developers/Aisling/tree/release_window).

For the second method:
- **Our Repository**: Download the source code and assets from the [GitHub repository](https://github.com/Potato-Programmer-Developers/Aisling/tree/release_window).

### Build Instructions

For the first method:
1.  Open your terminal and navigate to the project root.
2.  Go into the `code` directory:
3.  Compile the project using the provided Makefile:
4. If your computer is not set up for make, you can use the following command:
    ```
    gcc -Iinclude main.c -o main.exe -Llib -laudio -lsettings -lcharacter -lscene -linteractive -lmap -lgame_context -ldata -ldialogue -lraylib -lopengl32 -lgdi32 -lwinmm
    ```

For the second method:
1. Open your folder and navigate to the `code` folder.

### Running the Game

First Method: After a successful build, run the executable: `.\main.exe`.

Second Method: Run or double-click the `main.exe` file.

Note:
If the second method is not possible in your device, you may need to use the first method.
If you already have MinGW, you can proceed with the steps mentioned above for the first method.
If you do not have MinGW, you can downnload in [MSYS2](https://www.msys2.org) website. A step-by-step instruction to set-up MinGW is also provided on the website.

## 🎮 Controls

- **Movement**: Arrow Keys or WASD
- **Sprinting**: Hold Shift
- **Interaction**: Press `Enter` when near an object
- **Pause/Menu**: Press `Esc`

## 👥 Authors

- **Alvin Lim**
- **Andrew Zhuo**
- **Cornelius Jabez Lim**
- **Kristian Lung Lotama**
- **Steven Kenneth Darwy**
- **Timothy Pardin**
