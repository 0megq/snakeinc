#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

// Struct definitions
struct SnakeNode
{
	struct SnakeNode *next;
	Vector2 pos;
};

// Constants
typedef enum
{
	RESET,
	PLAYING,
	WON,
	LOST,
} GameStatus;

static const int maxFruitPlaceAtttempts = 400;
static const int screenWidth = 400;
static const int screenHeight = 400;

static const int tileSize = 16;
static const Vector2 tileSizeV = (Vector2){tileSize, tileSize};
static const int boardWidth = 20;
static const int boardHeight = 20;
static const int offsetWidth = (screenWidth - (boardWidth * tileSize)) / 2;
static const int offsetHeight = (screenHeight - (boardHeight * tileSize)) / 2;
static const Vector2 offsetSize = (Vector2){offsetWidth, offsetHeight};

static const Color fruitColor = RED;
static const Color snakeColor = GREEN;
static const Color snakeLostColor = GRAY;
static const float snakeMoveTimerStart = 0.2; // Reciprocal of speed. Seconds per move

// Global Variables
static GameStatus gameStatus;
static float snakeMoveTimer;
static Vector2 snakeDirection;
static Vector2 inputBuffer;
static struct SnakeNode *snakeHead;
static Vector2 fruitPos;
static int score;

// Function declarations
static void UpdateDrawFrame(void);
static void UpdateGame(float delta);
static struct SnakeNode *GetSecondLastNode(struct SnakeNode *snakeNode);
static void ResetGame(void);
static Vector2 GetRandomTilePos(void);
static void DeleteSnakeNode(struct SnakeNode *snakeNode);
static struct SnakeNode *NewSnakeNode(struct SnakeNode *next, Vector2 pos);
static struct SnakeNode *MoveSnakeAndGetHead(Vector2 newPos, bool ateFruit);
static Vector2 GetInputDirection(void);
static void DrawTile(Vector2 tilePos, Color color);
static void DrawSnake(struct SnakeNode *snakeNode);
static bool PlaceFruit(void);
static bool IsSnakeHeadPosOkay(void);
static bool CheckSnakeNodesForPos(const Vector2 *pos, const struct SnakeNode *snakeNode);
static void DrawTextCenteredH(const char *text, int posY, int fontSize, Color color);
static void DrawTextCenteredHV(const char *text, int offsetY, int fontSize, Color color);

// Main entry point
int main(void)
{
	// Initialization
	InitWindow(screenWidth, screenHeight, "Snake In C");

	// Game Initialization
	ResetGame();

	// Set our game to run at 60 frames-per-second
	SetTargetFPS(60);

	// Main game loop
	while (!WindowShouldClose()) // Detect window close button or ESC key
	{
		switch (gameStatus)
		{
		case PLAYING:
			UpdateGame(GetFrameTime());
			break;
		case WON:
		case LOST:
			if (GetKeyPressed() != 0)
				ResetGame();
			break;
		case RESET:
			Vector2 input = GetInputDirection();
			if (!Vector2Equals(input, Vector2Zero()))
			{
				input.x = (input.x && input.y) ? 0 : input.x;
				inputBuffer = input;
				gameStatus = PLAYING;
			}
		}
		UpdateDrawFrame();
	}

	// De-Initialization
	DeleteSnakeNode(snakeHead);
	CloseWindow(); // Close window and OpenGL context

	return 0;
}

static void ResetGame(void)
{
	DeleteSnakeNode(snakeHead);
	snakeHead = NewSnakeNode(NULL, GetRandomTilePos());
	PlaceFruit();
	inputBuffer = Vector2Zero();
	snakeDirection = Vector2Zero();
	gameStatus = RESET;
	score = 0;
	snakeMoveTimer = 0.0f;
}

static Vector2 GetRandomTilePos(void)
{
	return (Vector2){GetRandomValue(0, boardWidth - 1), GetRandomValue(0, boardHeight - 1)};
}

static void UpdateGame(float delta)
{
	// Get input
	Vector2 input = GetInputDirection();
	// If trying to go in opposite or current direction, set the input of that direction to zero, because it is invalid
	input.x = (input.x && snakeDirection.x) ? 0.0f : input.x;
	input.y = (input.y && snakeDirection.y) ? 0.0f : input.y;
	if (!Vector2Equals(input, Vector2Zero()))
		inputBuffer = input;

	// Then move
	snakeMoveTimer -= delta;
	if (snakeMoveTimer <= 0.0f)
	{
		// Get direction from inputBuffer
		if (!Vector2Equals(inputBuffer, Vector2Zero()))
		{
			snakeDirection = inputBuffer;
			inputBuffer = Vector2Zero();
		}
		// Move and eat fruit
		Vector2 newPos = Vector2Add(snakeHead->pos, snakeDirection);
		bool ateFruit = Vector2Equals(newPos, fruitPos);
		if (ateFruit)
		{
			score++;
			gameStatus = PlaceFruit() ? gameStatus : WON;
		}
		snakeHead = MoveSnakeAndGetHead(newPos, ateFruit);
		snakeMoveTimer += snakeMoveTimerStart;
	}
	gameStatus = IsSnakeHeadPosOkay() ? gameStatus : LOST;
}

static bool PlaceFruit(void)
{
	int i = 0;
	do
		fruitPos = GetRandomTilePos();
	while ((CheckSnakeNodesForPos(&fruitPos, snakeHead)) && (++i < maxFruitPlaceAtttempts));
	return (i < maxFruitPlaceAtttempts);
}

static bool IsSnakeHeadPosOkay(void)
{
	Vector2 pos = snakeHead->pos;
	if ((pos.x < 0) || (pos.y < 0) || (pos.x > boardWidth - 1) || (pos.y > boardHeight - 1))
		return false;
	return !(CheckSnakeNodesForPos(&pos, snakeHead->next));
}

static bool CheckSnakeNodesForPos(const Vector2 *pos, const struct SnakeNode *snakeNode)
{
	if (snakeNode == NULL)
		return false;
	if (Vector2Equals(*pos, snakeNode->pos))
		return true;
	return CheckSnakeNodesForPos(pos, snakeNode->next);
}

static Vector2 GetInputDirection(void)
{
	Vector2 input = Vector2Zero();
	if (IsKeyPressed(KEY_UP))
		input.y--;
	if (IsKeyPressed(KEY_DOWN))
		input.y++;
	if (IsKeyPressed(KEY_LEFT))
		input.x--;
	if (IsKeyPressed(KEY_RIGHT))
		input.x++;
	return input;
}

static void DeleteSnakeNode(struct SnakeNode *snakeNode)
{
	if (snakeNode == NULL)
		return;
	DeleteSnakeNode(snakeNode->next);
	MemFree(snakeNode);
}

static struct SnakeNode *NewSnakeNode(struct SnakeNode *next, Vector2 pos)
{
	struct SnakeNode *nodePointer = (struct SnakeNode *)MemAlloc(sizeof(struct SnakeNode));
	if (nodePointer == NULL)
		return NULL;
	nodePointer->next = next;
	nodePointer->pos = pos;
	return nodePointer;
}

static struct SnakeNode *MoveSnakeAndGetHead(Vector2 newPos, bool ateFruit)
{
	if (snakeHead == NULL)
	{
		return NULL; // This should not happen. Why are you giving me a null ptr?
	}

	if (ateFruit)
	{
		struct SnakeNode *newNode = NewSnakeNode(snakeHead, newPos);
		return newNode;
	}

	struct SnakeNode *secondLastNode = GetSecondLastNode(snakeHead);
	if (secondLastNode != NULL)
	{
		struct SnakeNode *lastNode = secondLastNode->next;
		secondLastNode->next = NULL;
		lastNode->next = snakeHead;
		lastNode->pos = newPos;
		return lastNode;
	}

	snakeHead->pos = newPos;
	return snakeHead;
}

static struct SnakeNode *GetSecondLastNode(struct SnakeNode *snakeNode)
{
	if (snakeNode->next == NULL)
	{
		return NULL; // This happens when the snake is only 1 node
	}
	if (snakeNode->next->next == NULL)
	{
		return snakeNode;
	}
	return GetSecondLastNode(snakeNode->next);
}

static void DrawSnake(struct SnakeNode *snakeNode)
{
	if (snakeNode == 0)
		return;
	DrawTile(snakeNode->pos, snakeColor);
	DrawSnake(snakeNode->next);
}

static void DrawTile(Vector2 tilePos, Color color)
{
	Vector2 pos = Vector2Add(Vector2Scale(tilePos, tileSize), offsetSize);
	DrawRectangleV(pos, tileSizeV, color);
	DrawRectangleLines(pos.x, pos.y, tileSize, tileSize, BLACK);
}

static void DrawTextCenteredHV(const char *text, int offsetY, int fontSize, Color color)
{
	DrawTextCenteredH(text, screenHeight / 2 - fontSize / 2 + offsetY, fontSize, color);
}

static void DrawTextCenteredH(const char *text, int posY, int fontSize, Color color)
{
	int textPosition = (screenWidth / 2) - (MeasureText(text, fontSize) / 2);
	DrawText(text, textPosition, posY, fontSize, color);
}

// Update and draw game frame
static void UpdateDrawFrame(void)
{

	// Draw
	BeginDrawing();

	ClearBackground(BLACK);

	DrawSnake(snakeHead);

	if (gameStatus == LOST)
		DrawTile(snakeHead->pos, snakeLostColor);

	DrawTile(fruitPos, fruitColor);

	DrawRectangleLines(offsetWidth, offsetHeight, tileSize * boardWidth, tileSize * boardHeight, WHITE);

	const char *infoText = "";
	switch (gameStatus)
	{
	case LOST:
		DrawTextCenteredHV("You Lost :(", -30, 28, WHITE);
		infoText = "Press any key to play again";
		break;
	case WON:
		DrawTextCenteredHV("You Won!", -30, 28, WHITE);
		infoText = "Press any key to play again";
		break;
	case RESET:
		infoText = "Press a direction to start";
		break;
	case PLAYING:
		break;
	}

	DrawTextCenteredHV(infoText, 0, 28, WHITE);

	DrawTextCenteredH(TextFormat("%i", score), 0, 44, WHITE);

	EndDrawing();
}
