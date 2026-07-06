#include "raylib.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

enum GameState {
    STATE_PLAYING,
    STATE_WON,
    STATE_LOST
};

// --- Data Structures ---

struct Anomaly {
    int x;
    int y;
};

class Cell {
public:
    int x = 0;          // Grid coordinate X
    int y = 0;          // Grid coordinate Y
    bool isAnomaly = false;
    bool isRevealed = false;
    bool isFlagged = false;

    // Vector field metrics pointing to the closest anomaly
    float vectorX = 0.0f;
    float vectorY = 0.0f;
    float distanceToClosest = 0.0f;

    void CalculateVector(const Anomaly& closest) {
        float dx = static_cast<float>(closest.x - x);
        float dy = static_cast<float>(closest.y - y);
        distanceToClosest = std::sqrt(dx * dx + dy * dy);

        if (distanceToClosest > 0.0f) {
            // Normalize to a unit vector so it's easy to scale for drawing
            vectorX = dx / distanceToClosest;
            vectorY = dy / distanceToClosest;
        }
    }
};

// --- Grid Management Engine ---

class GameGrid {
private:
    int rows;
    int cols;
    int cellSize;
    int offsetX; // Centers the grid horizontally in the window
    int offsetY; // Centers the grid vertically in the window

    std::vector<std::vector<Cell>> matrix;
    std::vector<Anomaly> anomalies;

    GameState currentStatus = STATE_PLAYING;
    int totalMines = 0;
    int revealedSafeCells = 0;

public:
    GameGrid(int numRows, int numCols, int size, int screenWidth, int screenHeight)
        : rows(numRows), cols(numCols), cellSize(size) {

        // Calculate offsets to perfectly center the board
        offsetX = (screenWidth - (cols * cellSize)) / 2;
        offsetY = (screenHeight - (rows * cellSize)) / 2;

        // 1. Initialize empty matrix
        matrix.resize(rows, std::vector<Cell>(cols));
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                matrix[r][c].x = c;
                matrix[r][c].y = r;
            }
        }
    }

    GameState GetStatus() const { return currentStatus; }

    void GenerateMap(int totalAnomalies) {

        // 1. Reset state variables and clear grid
            totalMines = totalAnomalies;
        revealedSafeCells = 0;
        currentStatus = STATE_PLAYING;
        anomalies.clear();

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                matrix[r][c].isAnomaly = false;
                matrix[r][c].isRevealed = false;
                matrix[r][c].isFlagged = false;
                matrix[r][c].vectorX = 0.0f;
                matrix[r][c].vectorY = 0.0f;
                matrix[r][c].distanceToClosest = 0.0f;
            }
        }

        // 2. seed the new anomalies randomly...
        int seeded = 0;
        while (seeded < totalAnomalies) {
            int r = std::rand() % rows;
            int c = std::rand() % cols;

            if (!matrix[r][c].isAnomaly) {
                matrix[r][c].isAnomaly = true;
                anomalies.push_back(Anomaly{ c, r });
                seeded++;
            }
        }

        // 3. Pre-calculate the closest vector for every grid cell
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (matrix[r][c].isAnomaly) continue;

                const Anomaly* closest = &anomalies[0];
                float minDistance = 999999.0f;

                for (const auto& anomaly : anomalies) {
                    float dx = static_cast<float>(anomaly.x - c);
                    float dy = static_cast<float>(anomaly.y - r);
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < minDistance) {
                        minDistance = dist;
                        closest = &anomaly;
                    }
                }
                matrix[r][c].CalculateVector(*closest);
            }
        }
    }

    void Draw() {
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                int posX = offsetX + (c * cellSize);
                int posY = offsetY + (r * cellSize);

                // --- 1. Draw Unrevealed vs Revealed Background ---
                Color tileColor = matrix[r][c].isRevealed ? Color{ 40, 40, 45, 255 } : DARKGRAY;
                DrawRectangle(posX, posY, cellSize - 2, cellSize - 2, tileColor);

                // --- 2. Draw Flag Overlay ---
                if (!matrix[r][c].isRevealed && matrix[r][c].isFlagged) {
                    // Draw a simple triangle/marker representing a flag container field
                    DrawTriangle(
                        Vector2{ (float)posX + cellSize / 2, (float)posY + cellSize / 4 },
                        Vector2{ (float)posX + cellSize / 4, (float)posY + 3 * cellSize / 4 },
                        Vector2{ (float)posX + 3 * cellSize / 4, (float)posY + 3 * cellSize / 4 },
                        GOLD
                    );
                    continue; // Skip drawing anything else inside this cell
                }

                // --- 3. Draw Revealed Cell Overlays ---
                if (matrix[r][c].isRevealed) {
                    if (matrix[r][c].isAnomaly) {
                        // Blew up: Draw a bright anomaly singularity core
                        DrawCircle(posX + cellSize / 2, posY + cellSize / 2, cellSize / 3, PURPLE);
                    }
                    else {
                        // Safe tile: Draw the directional vector field line pointing to danger
                        int centerX = posX + cellSize / 2;
                        int centerY = posY + cellSize / 2;
                        int lineLength = cellSize / 3;

                        int endX = centerX + static_cast<int>(matrix[r][c].vectorX * lineLength);
                        int endY = centerY + static_cast<int>(matrix[r][c].vectorY * lineLength);

                        // Color indicator based on proximity thresholds
                        Color vectorColor = GREEN; // Safe distance
                        if (matrix[r][c].distanceToClosest <= 1.5f) vectorColor = RED;       // Danger close
                        else if (matrix[r][c].distanceToClosest <= 3.0f) vectorColor = ORANGE; // Cautionary yellow

                        DrawLineEx(Vector2{ (float)centerX, (float)centerY }, Vector2{ (float)endX, (float)endY }, 2.5f, vectorColor);
                        // Draw a small dot at the tip of the vector to clarify heading orientation
                        DrawCircle(endX, endY, 2.0f, vectorColor);
                    }
                }
            }
        }
    }

    void HandleInput() {

        if (currentStatus != STATE_PLAYING) return;

        // Only do calculations if a click actually occurred
        bool leftClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool rightClicked = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

        if (leftClicked || rightClicked) {
            Vector2 mousePos = GetMousePosition();

            // 1. Subtract the offsets to get coordinates relative to the grid top-left
            float relativeX = mousePos.x - offsetX;
            float relativeY = mousePos.y - offsetY;

            // 2. Convert raw pixels to matrix indices using integer division
            int c = static_cast<int>(relativeX / cellSize);
            int r = static_cast<int>(relativeY / cellSize);

            // 3. Guard check: Make sure the click was actually inside the grid bounds
            if (c >= 0 && c < cols && r >= 0 && r < rows) {
                // Double check they didn't click outside the boundary lines of the final cells
                if (relativeX >= 0 && relativeY >= 0) {

                    if (leftClicked) {
                        RevealCell(r, c);
                    }
                    else if (rightClicked) {
                        FlagCell(r, c);
                    }
                }
            }
        }
    }

private:
    void RevealCell(int r, int c) {
        if (matrix[r][c].isFlagged || matrix[r][c].isRevealed) return;

        matrix[r][c].isRevealed = true;

        // LOSS CONDITION
        if (matrix[r][c].isAnomaly) {
            currentStatus = STATE_LOST;
            RevealAllAnomalies(); // Custom helper to show the player where they went wrong
            return;
        }

        // WIN CONDITION TRACKING
        revealedSafeCells++;
        int totalSafeCells = (rows * cols) - totalMines;
        if (revealedSafeCells == totalSafeCells) {
            currentStatus = STATE_WON;
        }
    }

    void RevealAllAnomalies() {
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (matrix[r][c].isAnomaly) {
                    matrix[r][c].isRevealed = true;
                }
            }
        }
    }

    void FlagCell(int r, int c) {
        if (matrix[r][c].isRevealed) return; // Can't flag revealed tiles

        // Toggle flag state
        matrix[r][c].isFlagged = !matrix[r][c].isFlagged;
    }
};

// --- Execution Entry ---

int main() {
    const int screenWidth = 900;
    const int screenHeight = 650;

    InitWindow(screenWidth, screenHeight, "Vector Sweeper");
    SetTargetFPS(60);

    GameGrid gameBoard(12, 12, 40, screenWidth, screenHeight);
    gameBoard.GenerateMap(15); // Start with 15 hidden cores

    while (!WindowShouldClose()) {
        // 1. Process Input Logic
        gameBoard.HandleInput();

        // Restart Handler: Pressing 'R' resets the board anytime
        if (IsKeyPressed(KEY_R)) {
            gameBoard.GenerateMap(15);
        }

        // 2. Render Graphics Frame
        BeginDrawing();
        ClearBackground(Color{ 15, 15, 18, 255 });

        // HUD Text Headers
        DrawText("VECTOR SWEEPER", 35, 20, 22, RAYWHITE);
        DrawText("Left-Click to query vector. Right-Click to flag core. Press 'R' to restart.", 35, 50, 14, LIGHTGRAY);

        // Render the core grid board array
        gameBoard.Draw();

        // 3. Render Post-Game UI Overlays
        GameState status = gameBoard.GetStatus();
        if (status == STATE_LOST) {
            // Dim the screen with a semi-transparent dark red tint
            DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 150, 0, 0, 80 });

            // Draw a nice centered warning box
            DrawRectangle(screenWidth / 2 - 200, screenHeight / 2 - 50, 400, 100, Color{ 20, 20, 25, 230 });
            DrawText("CRITICAL FAILURE", screenWidth / 2 - 130, screenHeight / 2 - 30, 26, RED);
            DrawText("Vector Field Collapsed. Press 'R' to Retry.", screenWidth / 2 - 150, screenHeight / 2 + 10, 15, RAYWHITE);
        }
        else if (status == STATE_WON) {
            // Dim the screen with a semi-transparent dark green tint
            DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 150, 0, 80 });

            // Draw a nice centered success box
            DrawRectangle(screenWidth / 2 - 200, screenHeight / 2 - 50, 400, 100, Color{ 20, 20, 25, 230 });
            DrawText("SECTOR CLEARED", screenWidth / 2 - 120, screenHeight / 2 - 30, 26, GREEN);
            DrawText("All singularities contained. Press 'R' to play again.", screenWidth / 2 - 170, screenHeight / 2 + 10, 15, RAYWHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}