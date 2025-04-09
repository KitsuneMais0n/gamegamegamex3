#include "raylib.h"
#include <cstdlib>
#include <ctime>
#include <vector>
#include <cmath>  // Include cmath for sqrtf and other math functions

// Player settings
const float playerSpeed = 5.0f;
Vector2 playerPosition = { 200, 200 };

// Monster settings
const float monsterSpeed = 2.0f;
Vector2 monsterPosition = { 600, 400 };
bool isMonsterStunned = false;  // Flag for stun effect

// Health settings
int playerHealth = 100;
const int maxHealth = 100;
const int healthBarWidth = 200;
const int healthBarHeight = 20;

// Game over flag
bool gameOver = false;
bool gameWon = false; // Flag for win condition

// Item settings
bool hasFlashlight = false;
bool hasTaser = false;
bool flashlightOn = false;
float taserCooldown = 0.0f;
const float maxTaserCooldown = 5.0f; // Taser cooldown period

// Puzzle settings
int correctCombination[3] = { 7, 3, 5 }; // The correct combination for the puzzle
int playerCombination[3] = { -1, -1, -1 }; // The player's current combination attempt
bool isPuzzleSolved = false; // Whether the puzzle is solved or not
bool isNearPuzzle = false; // Whether the player is near the puzzle or not

// Teleportation settings
const float teleportDistance = 100.0f;  // How far the player can teleport
float teleportCooldown = 0.0f;          // Cooldown timer for teleport
const float maxTeleportCooldown = 3.0f; // 3 seconds cooldown

// Map settings
const int gridWidth = 16;
const int gridHeight = 12;
const int tileSize = 50;
std::vector<std::vector<bool>> walls(gridHeight, std::vector<bool>(gridWidth, false));

// Exit (goal) settings
Vector2 exitPosition = { 700, 500 }; // Position of the exit (win condition)

// Hiding spot settings
struct HidingSpot {
    Vector2 position;
    bool isHidden; // Track if the player is currently hidden
};
std::vector<HidingSpot> hidingSpots; // List of hiding spots

// Functions to initialize the environment
void GenerateEnvironment() {
    // Generate walls
    for (int i = 0; i < gridHeight; i++) {
        for (int j = 0; j < gridWidth; j++) {
            if (i == 0 || j == 0 || i == gridHeight - 1 || j == gridWidth - 1) {
                walls[i][j] = true; // Make the outer edges walls
            } else {
                if (rand() % 4 == 0) { // 25% chance for a wall
                    walls[i][j] = true;
                }
            }
        }
    }

    // Generate hiding spots
    hidingSpots.clear();
    for (int i = 0; i < 5; i++) { // 5 hiding spots randomly placed
        HidingSpot newSpot;
        newSpot.position = { static_cast<float>(rand() % (GetScreenWidth() - 50)), 
                             static_cast<float>(rand() % (GetScreenHeight() - 50)) }; // Cast to float
        newSpot.isHidden = false;
        hidingSpots.push_back(newSpot);
    }
}

// Player movement in the environment (taking into account walls)
void UpdatePlayer() {
    if (IsKeyDown(KEY_W) && !walls[(int)(playerPosition.y / tileSize) - 1][(int)(playerPosition.x / tileSize)]) 
        playerPosition.y -= playerSpeed; // Move up
    if (IsKeyDown(KEY_S) && !walls[(int)(playerPosition.y / tileSize) + 1][(int)(playerPosition.x / tileSize)]) 
        playerPosition.y += playerSpeed; // Move down
    if (IsKeyDown(KEY_A) && !walls[(int)(playerPosition.y / tileSize)][(int)(playerPosition.x / tileSize) - 1]) 
        playerPosition.x -= playerSpeed; // Move left
    if (IsKeyDown(KEY_D) && !walls[(int)(playerPosition.y / tileSize)][(int)(playerPosition.x / tileSize) + 1]) 
        playerPosition.x += playerSpeed; // Move right
}

// Flashlight toggle
void ToggleFlashlight() {
    if (IsKeyPressed(KEY_F) && hasFlashlight) {
        flashlightOn = !flashlightOn;  // Toggle flashlight on and off
    }
}

// Update the flashlight effect on the screen
void UpdateFlashlight() {
    if (flashlightOn) {
        DrawCircle(playerPosition.x + 25, playerPosition.y + 25, 100, Color{ 255, 255, 255, 200 });
    }
}

// Taser mechanic
void HandleTaser() {
    if (hasTaser && taserCooldown <= 0.0f && IsKeyPressed(KEY_T)) {
        // Use taser and stun the monster temporarily
        taserCooldown = maxTaserCooldown;
        isMonsterStunned = true;  // Set monster stunned flag
    }
}

// Teleportation mechanic
void HandleTeleportation() {
    if (teleportCooldown <= 0.0f && IsKeyPressed(KEY_SPACE)) {
        int direction = rand() % 4; // 0 = up, 1 = down, 2 = left, 3 = right
        Vector2 newPosition = playerPosition;

        switch (direction) {
            case 0: newPosition.y -= teleportDistance; break;
            case 1: newPosition.y += teleportDistance; break;
            case 2: newPosition.x -= teleportDistance; break;
            case 3: newPosition.x += teleportDistance; break;
        }

        // Ensure the teleport doesn't go outside the map bounds
        if (newPosition.x < 0) newPosition.x = 0;
        if (newPosition.y < 0) newPosition.y = 0;
        if (newPosition.x > GetScreenWidth() - tileSize) newPosition.x = GetScreenWidth() - tileSize;
        if (newPosition.y > GetScreenHeight() - tileSize) newPosition.y = GetScreenHeight() - tileSize;

        playerPosition = newPosition; // Move the player to the new position
        teleportCooldown = maxTeleportCooldown; // Reset cooldown
    }
}

// Monster AI (simple follow player)
void UpdateMonster() {
    if (!isMonsterStunned) {
        Vector2 direction = { playerPosition.x - monsterPosition.x, playerPosition.y - monsterPosition.y };
        float length = sqrt(direction.x * direction.x + direction.y * direction.y);  // Use sqrt from <cmath>

        if (length != 0) {
            direction.x /= length;
            direction.y /= length;
        }

        monsterPosition.x += direction.x * monsterSpeed;
        monsterPosition.y += direction.y * monsterSpeed;
    } else {
        // Reduce stun duration (for simplicity, just a delay before monster can move again)
        static float stunTimer = 2.0f;  // Stun lasts for 2 seconds
        stunTimer -= GetFrameTime();
        if (stunTimer <= 0.0f) {
            isMonsterStunned = false;
            stunTimer = 2.0f;  // Reset stun timer
        }
    }
}

// Draw the health bar
void DrawHealthBar() {
    DrawRectangle(10, 10, healthBarWidth, healthBarHeight, GRAY); // Background of health bar
    float healthWidth = (float)playerHealth / maxHealth * healthBarWidth;
    DrawRectangle(10, 10, healthWidth, healthBarHeight, RED); // Health as a red bar
}

// Check the game win condition
void CheckWinCondition() {
    if (CheckCollisionRecs({ playerPosition.x, playerPosition.y, 50, 50 },
                           { exitPosition.x, exitPosition.y, 50, 50 })) {
        gameWon = true;
    }
}

// Draw the environment
void DrawEnvironment() {
    DrawRectangle(600, 300, 100, 100, DARKGRAY); // Locked door
    DrawText("LOCKED", 620, 350, 20, WHITE); // Indicating the door is locked
}

// Main function
int main() {
    // Initialization
    InitWindow(800, 600, "Horror Game with Flashlight, Taser, and Puzzle");

    SetTargetFPS(60);

    // Generate environment
    GenerateEnvironment();

    while (!WindowShouldClose()) {
        // Update
        if (!gameOver && !gameWon) {
            UpdatePlayer();  // Update player movement
            ToggleFlashlight();  // Toggle flashlight
            HandleTaser();  // Handle taser usage
            HandleTeleportation();  // Handle teleportation mechanic
            UpdateMonster();  // Update monster movement
            CheckWinCondition(); // Check if the player reached the exit
        }

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw environment and player
        DrawHealthBar();
        DrawRectangleV(playerPosition, { 50, 50 }, BLUE);  // Draw player
        DrawRectangleV(monsterPosition, { 50, 50 }, RED);  // Draw monster
        DrawRectangleV(exitPosition, { 50, 50 }, GREEN);  // Draw exit

        // Draw flashlight
        UpdateFlashlight();

        // Display taser cooldown
        if (taserCooldown > 0.0f) {
            taserCooldown -= GetFrameTime();  // Decrease cooldown
        }
        if (taserCooldown > 0.0f) {
            DrawText(TextFormat("Taser Cooldown: %.1f", taserCooldown), 10, 40, 20, YELLOW);
        }

        // Check win condition
        if (gameWon) {
            DrawText("YOU WIN!", 300, 250, 30, GREEN);
        }

        EndDrawing();
    }

    // De-Initialization
    CloseWindow(); // Close window and OpenGL context

    return 0;
}
