# IN CASE THE .EXE ISN'T WORKING, TRY RUNNING THIS MAKEFILE (Write in the Terminal)
# Open code directory (cd code)
# 1. Code: mingw32-make build - This will build the .exe and run it immediately
# 2. Code: mingw32-make run - This will just run the .exe (useful if you just want to test changes without rebuilding)
# 3. Code: mingw32-make clean - This will delete the .exe and the obj folder (useful for a fresh start)
# N1: Make sure you have mingw32-make installed and added to your PATH to use these commands.
# N2: If you have a different make tool, you may need to adjust the commands accordingly (e.g., 'make' instead of 'mingw32-make').
# N3: Ensure that your terminal is in the 'code' directory where this Makefile is located before running these commands.
# N4: If the make file does not work, you may type in the terminal:
# BUILD: gcc main.c module/*.c -Wall -std=c99 -Iinclude -Llib -lraylib -lopengl32 -lgdi32 -lwinmm -o main.exe
# RUN: main.exe
# CLEAN (.exe): del /f /q main.exe
# CLEAN (obj folder): rmdir /s /q module/obj
# CLEAN (data file): del /f /q ../data/data.dat

# ~~~ Project Settings ~~~
# This puts the .exe in the root folder so it's easy to find
PROJECT_NAME = main.exe
CC = gcc

# ~~~ Path Definitions (Relative to the 'code' folder) ~~~
INC_DIR = include
LIB_DIR = lib
MODULE_DIR = module
# This puts the obj folder inside the module folder as requested
OBJ_DIR = $(MODULE_DIR)/obj

# ~~~ Automatic File Discovery ~~~
# Finds main.c here and all .c files in the module folder
SRCS_MAIN = main.c
SRCS_MOD  = $(wildcard $(MODULE_DIR)/*.c)

# Creates the list of .o files inside module/obj/
OBJS = $(OBJ_DIR)/main.o $(patsubst $(MODULE_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS_MOD))

# ~~~ Compiler Flags ~~~
# -I$(INC_DIR) connects your .h files
CFLAGS = -Wall -std=c99 -I$(INC_DIR)
LDFLAGS = -L$(LIB_DIR) -lraylib -lopengl32 -lgdi32 -lwinmm

# ~~~ Build Rules ~~~

all: prepare $(PROJECT_NAME)

# Create the obj directory if it doesn't exist
prepare:
	@if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"

# Linking Rule
$(PROJECT_NAME): $(OBJS)
	$(CC) $(OBJS) -o $(PROJECT_NAME) $(LDFLAGS)

# Rule for main.o (lives in code/)
$(OBJ_DIR)/main.o: $(SRCS_MAIN)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for module .o files (lives in code/module/)
$(OBJ_DIR)/%.o: $(MODULE_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ~~~ Utilities ~~~

# Run command: Moves into the root folder to run the exe so paths work
build: all
	$(PROJECT_NAME)

run:
	$(PROJECT_NAME)

clean:
	@if exist "$(OBJ_DIR)" rmdir /s /q "$(OBJ_DIR)"
	@if exist "$(PROJECT_NAME)" del /f /q "$(PROJECT_NAME)"
	@if exist "../data/data.dat" del /f /q "../data/data.dat"