# Tetris (C++ + Raylib) — Build Instructions

## Prerequisites — Install Raylib

### Windows (MinGW/MSYS2)
```bash
pacman -S mingw-w64-x86_64-raylib
```
Then compile:
```bash
g++ tetris.cpp -o tetris.exe -lraylib -lopengl32 -lgdi32 -lwinmm -std=c++17
./tetris.exe
```

### Linux (Ubuntu/Debian)
```bash
sudo apt install libraylib-dev   # or build from source
g++ tetris.cpp -o tetris -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -std=c++17
./tetris
```

### macOS (Homebrew)
```bash
brew install raylib
g++ tetris.cpp -o tetris -lraylib -framework OpenGL -framework Cocoa -framework IOKit -std=c++17
./tetris
```

### All Platforms (cmake / raylib from source)
1. Download raylib from https://github.com/raysan5/raylib/releases
2. Build & install following raylib README
3. Use the compile commands above

---

## DSA Concepts in this Code

| Concept     | Where used                                              |
|-------------|--------------------------------------------------------|
| 2D Array    | `Board::grid[ROWS][COLS]` — the game board             |
| Stack       | `undoStack` — stores last 3 board snapshots for undo   |
| Queue       | `nextQueue` — FIFO bag of upcoming pieces (preview)    |

## OOP Concepts

| Class        | Responsibility                                          |
|--------------|--------------------------------------------------------|
| `Tetromino`  | Piece type, rotation, position, cell offsets           |
| `Board`      | 2D grid state, placement, line clearing                |
| `Game`       | Orchestrates everything — input, physics, draw         |

## Controls

| Key         | Action          |
|-------------|-----------------|
| ← →         | Move left/right |
| ↑           | Rotate          |
| ↓           | Soft drop       |
| Space       | Hard drop       |
| Z           | Undo (up to 3)  |
| P           | Pause/Resume    |
| R           | Restart         |
