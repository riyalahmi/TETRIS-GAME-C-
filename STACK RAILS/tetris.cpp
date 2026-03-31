// ============================================================
//  TETRIS - C++ with Raylib
//  DSA Concepts Used:
//    - 2D Array  : game board grid
//    - Stack     : undo last placed piece (up to 3 moves)
//    - Queue     : next-piece preview (FIFO bag of upcoming pieces)
//  OOP Concepts Used:
//    - Classes   : Tetromino, Board, Game
//    - Encapsulation, Methods
// ============================================================

#include "raylib.h"
#include <array>
#include <stack>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <string>

// ─── Constants ───────────────────────────────────────────────
const int COLS       = 10;
const int ROWS       = 20;
const int CELL       = 32;          // pixel size of one cell
const int PANEL_W    = 160;         // right panel width
const int SCR_W      = COLS * CELL + PANEL_W;
const int SCR_H      = ROWS * CELL;
const int PREVIEW_Q  = 3;           // how many next pieces to show

// ─── Piece shapes  (4 rotations × 4 cells each) ──────────────
//  Each piece has 4 rotations; each rotation is 4 {row,col} offsets
//  relative to a pivot point.
struct Cell { int r, c; };

const int NUM_PIECES = 7;

// shapes[piece][rotation][cell]
const Cell SHAPES[NUM_PIECES][4][4] = {
    // I
    { {{0,0},{0,1},{0,2},{0,3}}, {{0,1},{1,1},{2,1},{3,1}},
      {{1,0},{1,1},{1,2},{1,3}}, {{0,2},{1,2},{2,2},{3,2}} },
    // O
    { {{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}},
      {{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}} },
    // T
    { {{0,1},{1,0},{1,1},{1,2}}, {{0,1},{1,1},{2,1},{1,2}},
      {{1,0},{1,1},{1,2},{2,1}}, {{0,1},{1,0},{1,1},{2,1}} },
    // S
    { {{0,1},{0,2},{1,0},{1,1}}, {{0,1},{1,1},{1,2},{2,2}},
      {{1,1},{1,2},{2,0},{2,1}}, {{0,0},{1,0},{1,1},{2,1}} },
    // Z
    { {{0,0},{0,1},{1,1},{1,2}}, {{0,2},{1,1},{1,2},{2,1}},
      {{1,0},{1,1},{2,1},{2,2}}, {{0,1},{1,0},{1,1},{2,0}} },
    // J
    { {{0,0},{1,0},{1,1},{1,2}}, {{0,1},{0,2},{1,1},{2,1}},
      {{1,0},{1,1},{1,2},{2,2}}, {{0,1},{1,1},{2,0},{2,1}} },
    // L
    { {{0,2},{1,0},{1,1},{1,2}}, {{0,1},{1,1},{2,1},{2,2}},
      {{1,0},{1,1},{1,2},{2,0}}, {{0,0},{0,1},{1,1},{2,1}} },
};

// Colors for each piece
const Color PIECE_COLORS[NUM_PIECES] = {
    SKYBLUE,    // I
    YELLOW,     // O
    PURPLE,     // T
    GREEN,      // S
    RED,        // Z
    BLUE,       // J
    ORANGE,     // L
};

// ─── Tetromino class ─────────────────────────────────────────
class Tetromino {
public:
    int type, rot, row, col;

    Tetromino() : type(0), rot(0), row(0), col(3) {}
    Tetromino(int t) : type(t), rot(0), row(0), col(3) {}

    // Return the 4 board cells this piece occupies
    std::array<Cell, 4> cells() const {
        std::array<Cell, 4> result;
        for (int i = 0; i < 4; i++) {
            result[i] = { row + SHAPES[type][rot][i].r,
                          col + SHAPES[type][rot][i].c };
        }
        return result;
    }

    Color color() const { return PIECE_COLORS[type]; }
};

// ─── Board class  (2D Array) ──────────────────────────────────
class Board {
public:
    // grid[r][c] = 0 means empty, else piece-type+1
    int grid[ROWS][COLS];

    Board() { clear(); }

    void clear() {
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                grid[r][c] = 0;
    }

    bool inBounds(int r, int c) const {
        return r >= 0 && r < ROWS && c >= 0 && c < COLS;
    }

    bool isFree(int r, int c) const {
        return inBounds(r, c) && grid[r][c] == 0;
    }

    // Place piece onto board
    void place(const Tetromino& t) {
        for (auto& cell : t.cells())
            if (inBounds(cell.r, cell.c))
                grid[cell.r][cell.c] = t.type + 1;
    }

    // Remove piece from board (for undo)
    void remove(const Tetromino& t) {
        for (auto& cell : t.cells())
            if (inBounds(cell.r, cell.c))
                grid[cell.r][cell.c] = 0;
    }

    // Clear full lines; return count of lines cleared
    int clearLines() {
        int cleared = 0;
        for (int r = ROWS - 1; r >= 0; r--) {
            bool full = true;
            for (int c = 0; c < COLS; c++)
                if (grid[r][c] == 0) { full = false; break; }
            if (full) {
                // Shift rows down
                for (int rr = r; rr > 0; rr--)
                    for (int c = 0; c < COLS; c++)
                        grid[rr][c] = grid[rr-1][c];
                for (int c = 0; c < COLS; c++) grid[0][c] = 0;
                cleared++;
                r++; // recheck same row
            }
        }
        return cleared;
    }

    Color cellColor(int r, int c) const {
        int v = grid[r][c];
        return (v > 0) ? PIECE_COLORS[v - 1] : DARKGRAY;
    }
};

// ─── Game class ───────────────────────────────────────────────
class Game {
    Board board;
    Tetromino current;

    // Queue (DSA) – upcoming pieces preview
    std::queue<int> nextQueue;

    // Stack (DSA) – undo last placed pieces (max 3)
    struct Snapshot { Board board; Tetromino piece; int score; };
    std::stack<Snapshot> undoStack;
    static const int MAX_UNDO = 3;

    int score;
    int level;
    int linesCleared;
    bool gameOver;
    bool paused;

    float fallTimer;
    float fallInterval; // seconds between auto-drops

    // Fill queue with random pieces
    void fillQueue() {
        while ((int)nextQueue.size() < PREVIEW_Q + 1)
            nextQueue.push(rand() % NUM_PIECES);
    }

    Tetromino spawnNext() {
        fillQueue();
        int t = nextQueue.front();
        nextQueue.pop();
        fillQueue();
        return Tetromino(t);
    }

    bool canPlace(const Tetromino& t) const {
        for (auto& cell : t.cells())
            if (!board.isFree(cell.r, cell.c)) return false;
        return true;
    }

    // Ghost piece: drop shadow showing where piece will land
    Tetromino ghost() const {
        Tetromino g = current;
        while (true) {
            Tetromino moved = g;
            moved.row++;
            bool ok = true;
            for (auto& cell : moved.cells())
                if (!board.isFree(cell.r, cell.c)) { ok = false; break; }
            if (!ok) break;
            g = moved;
        }
        return g;
    }

    void lockPiece() {
        // Save snapshot for undo (keep stack size ≤ MAX_UNDO)
        if ((int)undoStack.size() >= MAX_UNDO) {
            // Can't pop from middle of std::stack easily; just limit pushes
            // Rebuild limited stack
            std::stack<Snapshot> temp;
            while (!undoStack.empty()) { temp.push(undoStack.top()); undoStack.pop(); }
            // discard oldest (bottom)
            std::stack<Snapshot> trimmed;
            int cnt = 0;
            while (!temp.empty()) {
                if (cnt < MAX_UNDO - 1) { trimmed.push(temp.top()); cnt++; }
                temp.pop();
            }
            undoStack = trimmed;
        }
        undoStack.push({ board, current, score });

        board.place(current);
        int lines = board.clearLines();
        if (lines > 0) {
            // Classic scoring
            int pts[] = {0, 100, 300, 500, 800};
            score += pts[lines] * level;
            linesCleared += lines;
            level = linesCleared / 10 + 1;
            fallInterval = std::max(0.05f, 1.0f - (level - 1) * 0.08f);
        }

        current = spawnNext();
        if (!canPlace(current)) gameOver = true;
    }

public:
    Game() { reset(); }

    void reset() {
        srand((unsigned)time(nullptr));
        board.clear();
        while (!nextQueue.empty()) nextQueue.pop();
        while (!undoStack.empty()) undoStack.pop();
        score = 0; level = 1; linesCleared = 0;
        gameOver = false; paused = false;
        fallTimer = 0; fallInterval = 1.0f;
        fillQueue();
        current = spawnNext();
    }

    void update(float dt) {
        if (gameOver || paused) return;

        fallTimer += dt;
        if (fallTimer >= fallInterval) {
            fallTimer = 0;
            moveDown();
        }
    }

    // ── Input actions ──
    void moveLeft() {
        Tetromino t = current; t.col--;
        if (canPlace(t)) current = t;
    }
    void moveRight() {
        Tetromino t = current; t.col++;
        if (canPlace(t)) current = t;
    }
    void moveDown() {
        Tetromino t = current; t.row++;
        if (canPlace(t)) { current = t; fallTimer = 0; }
        else lockPiece();
    }
    void hardDrop() {
        while (true) {
            Tetromino t = current; t.row++;
            bool ok = true;
            for (auto& cell : t.cells())
                if (!board.isFree(cell.r, cell.c)) { ok = false; break; }
            if (!ok) break;
            current = t;
        }
        lockPiece();
    }
    void rotate() {
        Tetromino t = current;
        t.rot = (t.rot + 1) % 4;
        // Wall kick: try original col, then +1, -1, +2, -2
        for (int kick : {0, 1, -1, 2, -2}) {
            t.col = current.col + kick;
            if (canPlace(t)) { current = t; return; }
        }
    }
    void undo() {
        if (undoStack.empty()) return;
        Snapshot snap = undoStack.top(); undoStack.pop();
        board = snap.board;
        current = snap.piece;
        score = snap.score;
        gameOver = false;
    }
    void togglePause() { paused = !paused; }

    // ── Draw ──
    void draw() const {
        // Board background
        DrawRectangle(0, 0, COLS * CELL, ROWS * CELL, BLACK);

        // Grid lines
        for (int r = 0; r <= ROWS; r++)
            DrawLine(0, r * CELL, COLS * CELL, r * CELL, {40,40,40,255});
        for (int c = 0; c <= COLS; c++)
            DrawLine(c * CELL, 0, c * CELL, ROWS * CELL, {40,40,40,255});

        // Placed cells
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (board.grid[r][c]) {
                    Color col = board.cellColor(r, c);
                    DrawRectangle(c*CELL+1, r*CELL+1, CELL-2, CELL-2, col);
                    DrawRectangleLines(c*CELL, r*CELL, CELL, CELL, WHITE);
                }

        // Ghost piece
        Tetromino g = ghost();
        for (auto& cell : g.cells()) {
            Color gc = current.color(); gc.a = 60;
            DrawRectangle(cell.c*CELL+1, cell.r*CELL+1, CELL-2, CELL-2, gc);
        }

        // Current piece
        for (auto& cell : current.cells()) {
            DrawRectangle(cell.c*CELL+1, cell.r*CELL+1, CELL-2, CELL-2, current.color());
            DrawRectangleLines(cell.c*CELL, cell.r*CELL, CELL, CELL, WHITE);
        }

        // ── Right panel ──
        int px = COLS * CELL + 10;
        DrawRectangle(COLS * CELL, 0, PANEL_W, SCR_H, {20,20,20,255});

        // Score / Level / Lines
        DrawText("SCORE", px, 10, 14, LIGHTGRAY);
        DrawText(std::to_string(score).c_str(), px, 28, 18, WHITE);
        DrawText("LEVEL", px, 60, 14, LIGHTGRAY);
        DrawText(std::to_string(level).c_str(), px, 78, 18, WHITE);
        DrawText("LINES", px, 110, 14, LIGHTGRAY);
        DrawText(std::to_string(linesCleared).c_str(), px, 128, 18, WHITE);

        // Next pieces (Queue preview)
        DrawText("NEXT", px, 165, 14, LIGHTGRAY);
        int previewSize = (int)nextQueue.size();
        // Copy queue to iterate (queue doesn't support iteration)
        std::queue<int> tmp = nextQueue;
        int shown = 0;
        while (!tmp.empty() && shown < PREVIEW_Q) {
            int t = tmp.front(); tmp.pop();
            int baseY = 185 + shown * 60;
            int mini = 10; // mini cell size
            for (int i = 0; i < 4; i++) {
                int r = SHAPES[t][0][i].r;
                int c = SHAPES[t][0][i].c;
                DrawRectangle(px + c*mini, baseY + r*mini, mini-1, mini-1, PIECE_COLORS[t]);
            }
            shown++;
        }

        // Undo count
        DrawText("UNDO (Z)", px, SCR_H - 90, 13, LIGHTGRAY);
        std::string undoInfo = std::to_string(undoStack.size()) + "/" + std::to_string(MAX_UNDO);
        DrawText(undoInfo.c_str(), px, SCR_H - 74, 16, WHITE);

        // Controls
        DrawText("Controls:", px, SCR_H - 55, 11, GRAY);
        DrawText("←→ Move  ↑ Rot", px, SCR_H - 42, 10, GRAY);
        DrawText("↓ Soft  Space Hard", px, SCR_H - 30, 10, GRAY);
        DrawText("Z Undo  P Pause  R Reset", px, SCR_H - 18, 10, GRAY);

        // Overlays
        if (paused) {
            DrawRectangle(0, 0, COLS*CELL, ROWS*CELL, {0,0,0,160});
            DrawText("PAUSED", COLS*CELL/2 - 60, ROWS*CELL/2 - 20, 36, WHITE);
        }
        if (gameOver) {
            DrawRectangle(0, 0, COLS*CELL, ROWS*CELL, {0,0,0,180});
            DrawText("GAME OVER", COLS*CELL/2 - 90, ROWS*CELL/2 - 30, 32, RED);
            DrawText("Press R to restart", COLS*CELL/2 - 85, ROWS*CELL/2 + 10, 18, WHITE);
        }
    }

    bool isGameOver() const { return gameOver; }
    bool isPaused()   const { return paused; }
};

// ─── Main ─────────────────────────────────────────────────────
int main() {
    InitWindow(SCR_W, SCR_H, "Tetris - C++ Raylib (Arrays + Stack + Queue)");
    SetTargetFPS(60);

    Game game;

    // Input repeat timers (DAS – delayed auto shift)
    float leftTimer = 0, rightTimer = 0, downTimer = 0;
    const float DAS = 0.15f, ARR = 0.05f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // ── Input handling ──
        if (!game.isPaused() && !game.isGameOver()) {

            // Rotate
            if (IsKeyPressed(KEY_UP)) game.rotate();

            // Hard drop
            if (IsKeyPressed(KEY_SPACE)) game.hardDrop();

            // Soft drop (held)
            if (IsKeyDown(KEY_DOWN)) {
                downTimer += dt;
                if (IsKeyPressed(KEY_DOWN)) { game.moveDown(); downTimer = 0; }
                else if (downTimer > ARR) { game.moveDown(); downTimer = 0; }
            } else downTimer = 0;

            // Move left (with DAS)
            if (IsKeyDown(KEY_LEFT)) {
                if (IsKeyPressed(KEY_LEFT)) { game.moveLeft(); leftTimer = 0; }
                else { leftTimer += dt; if (leftTimer > DAS) { game.moveLeft(); leftTimer = DAS - ARR; } }
            } else leftTimer = 0;

            // Move right (with DAS)
            if (IsKeyDown(KEY_RIGHT)) {
                if (IsKeyPressed(KEY_RIGHT)) { game.moveRight(); rightTimer = 0; }
                else { rightTimer += dt; if (rightTimer > DAS) { game.moveRight(); rightTimer = DAS - ARR; } }
            } else rightTimer = 0;

            // Undo
            if (IsKeyPressed(KEY_Z)) game.undo();
        }

        if (IsKeyPressed(KEY_P)) game.togglePause();
        if (IsKeyPressed(KEY_R)) game.reset();

        game.update(dt);

        // ── Draw ──
        BeginDrawing();
        ClearBackground(BLACK);
        game.draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
