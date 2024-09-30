#include <raylib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define WALL_SIZE 62
#define PLAYER_RADIUS 8
#define ROTATION_AMOUNT 0.05f
#define SPEED 1.5f
#define MAP_BOUNDS_X 8
#define MAP_BOUNDS_Y 8

typedef struct
{
    float posX, posY;
    float rotation, deltaX, deltaY;
} Player;

Player player;

void InitGame();
void UpdateDrawGame();
void UpdateGame();
void UpdatePlayer();
void CorrectBadPosition();
void CastRays();
void DrawGame();
void DrawLeftScreen();
void DrawWalls(int index);
void DrawPlayer();

int XToPixel(int x);
int IndexToPixel(int index);
int PixelToIndex(int pixel);
int PosToIndex(int x, int y);

float degToRad(float angle);

bool CanGoForward();
bool CanGoBackward();

static const int screenWidth = 1024;
static const int screenHeight = 512;

static const int grid[64] = {   1,1,1,1,1,1,1,1,
                                1,0,0,0,0,0,0,1,
                                1,0,0,0,0,1,0,1,
                                1,0,0,1,0,1,0,1,
                                1,0,0,0,0,1,0,1,
                                1,0,1,1,0,1,0,1,
                                1,0,0,0,0,1,0,1,
                                1,1,1,1,1,1,1,1 };

int main(void)
{
    InitWindow(screenWidth, screenHeight, "Raycaster");

    SetTargetFPS(60);

    InitGame();               

    while (!WindowShouldClose()) 
    {
        UpdateDrawGame();
    }

    CloseWindow();       

    return 0;
}

void InitGame()
{
    player = (Player){86.0f, 86.0f, 0.0f, 0.0f, 0.0f};
}

void UpdateGame()
{
    DrawRectangleGradientV(528, 0, 480, 256, RAYWHITE, BLACK);
    DrawRectangleGradientV(528, 256, 480, 256, BLACK, GRAY);

    UpdatePlayer();
}

void UpdatePlayer()
{
    if (IsKeyDown(KEY_LEFT))    { player.rotation -= ROTATION_AMOUNT; }
    if (player.rotation < 0)    { player.rotation += 2*PI; }

    if (IsKeyDown(KEY_RIGHT))   { player.rotation += ROTATION_AMOUNT; }
    if (player.rotation > 2*PI) { player.rotation -= 2*PI; }

    player.deltaX = cos(player.rotation);
    player.deltaY = sin(player.rotation);
    float moveX = player.deltaX * SPEED;
    float moveY = player.deltaY * SPEED;

    if (IsKeyDown(KEY_UP) && CanGoForward())
    {
        player.posX += moveX;
        player.posY += moveY;
    }
    if (IsKeyDown(KEY_DOWN) && CanGoBackward())
    {
        player.posX -= moveX;
        player.posY -= moveY;
    }

    CastRays();    
}

void CastRays()
{
    int fov = 40;
    for (int ray = -fov; ray < fov; ray++)
    {

    float rayAngle, rayX, rayY, offsetX, offsetY, distanceH, distanceV, Tan, iTan;
    float horizontalX, horizontalY;
    int dof, mapX, mapY, mapIndex;
    Color wallColor;

    rayAngle = player.rotation + degToRad(ray);
    Tan = tan(rayAngle); iTan = (Tan == 0) ? 0 : 1.0 / Tan;
    rayX = player.posX;
    rayY = player.posY;

    //Check the horizontal lines, start with looking up
    distanceH = 100000000; dof = 0;
    if (sin(rayAngle) < -0.01)
    {
        rayY = (((int)rayY>>6)<<6) - 0.0001; rayX = player.posX + (player.posY - rayY) * -iTan;
        offsetY = 64; offsetX = offsetY * -iTan;
    }
    else if (sin(rayAngle) > 0.01)
    {
        rayY = (((int)rayY>>6)<<6) + 64; rayX = player.posX + (player.posY - rayY) * -iTan;
        offsetY = -64; offsetX = offsetY * -iTan; 
    }
    else 
    {
        rayX = player.posX; rayY = player.posY; dof = 8; 
    }
    
    while (dof < 8)
    {
        mapX = ((int)rayX>>6); mapY = ((int)rayY>>6); mapIndex = mapY * 8 + mapX;
        if (grid[mapIndex] == 1 && mapIndex >= 0 && mapIndex < MAP_BOUNDS_X * MAP_BOUNDS_Y && mapX < MAP_BOUNDS_X)
        {
            float x = abs(player.posX - rayX); float y = abs(player.posY - rayY);
            distanceH = x * x + y * y;
            dof = 8;
        }
        else
        { 
            rayX += offsetX; rayY -= offsetY; 
            dof += 1;
        }
    }
    horizontalX = rayX; horizontalY = rayY;
    rayX = player.posX; rayY = player.posY;

    //Check the vertical lines, start with looking left
    distanceV = 100000000; dof = 0;
    if (cos(rayAngle) < -0.01)
    {
        rayX = (((int)rayX>>6)<<6) - 0.0001; rayY = player.posY + (player.posX - rayX) * -Tan;
        offsetX = 64; offsetY = offsetX * -Tan;
    }
    else if (cos(rayAngle) > 0.01)
    {
        rayX = (((int)rayX>>6)<<6) + 64; rayY = player.posY + (player.posX - rayX) * -Tan;
        offsetX = -64; offsetY = offsetX * -Tan;
    }
    else 
    {
        rayX = player.posX; rayY = player.posY; dof = 8; 
    }

    while (dof < 8)
    {
        //DrawCircle(rayX, rayY, 2, BLUE);
        mapX = ((int)rayX>>6); mapY = ((int)rayY>>6); mapIndex = mapY * 8 + mapX;
        if (grid[mapIndex] == 1 && mapIndex >= 0 && mapIndex < MAP_BOUNDS_X * MAP_BOUNDS_Y && mapX < MAP_BOUNDS_X)
        {
            float x = abs(player.posX - rayX); float y = abs(player.posY - rayY);
            distanceV = x * x + y * y;
            dof = 8;
        }
        else
        { 
            rayX -= offsetX; rayY += offsetY; 
            dof += 1;
        }
    }

    if (distanceH < distanceV)
    { 
        rayX = horizontalX; rayY = horizontalY;
        distanceH = sqrt(distanceH);
        int fadeOff = ((int)distanceH>>1); if (fadeOff > 225) { fadeOff = 225; }
        wallColor = (Color){ 0, 50 - fadeOff / 4.5, 255 - fadeOff, 255 };
    }
    else
    { 
        distanceV = sqrt(distanceV); distanceH = distanceV;
        int fadeOff = ((int)distanceH>>1); if (fadeOff > 225) { fadeOff = 225; }
        wallColor = (Color){ 0, 100 - fadeOff / 2.4, 255 - fadeOff, 255 };
    } 
    
    float correctedAngle = player.rotation - rayAngle; distanceH = distanceH * cos(correctedAngle);
    
    int lineHeight = (screenHeight * 32) / distanceH;
    if (lineHeight > screenHeight) { lineHeight = screenHeight; }
    int lineY = 256 - (lineHeight>>1); int lineX = 768 + (ray * (512 / (fov*2)));

    DrawRectangle(lineX, lineY, (512 / (fov*2)), lineHeight, wallColor);

    DrawLine(player.posX, player.posY, rayX, rayY, GREEN);    

    }


}

void DrawGame()
{ 
    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawLeftScreen();
    DrawPlayer();

    EndDrawing();
}

void DrawLeftScreen()
{
    for (int i = 0; i < 64; i++)
    {
        if (grid[i] == 1)
        {
            DrawWalls(i);
        }
    }
}

void DrawWalls(int index)
{
    int indexX = index % 8;
    int indexY = index / 8;
    int wallPosX = IndexToPixel(indexX) + 1;
    int wallPosY = IndexToPixel(indexY) + 1;

    DrawRectangle(wallPosX, wallPosY, WALL_SIZE, WALL_SIZE, BLACK);
}

void DrawPlayer()
{
    DrawCircle(player.posX, player.posY, PLAYER_RADIUS, RED);
    DrawLine(player.posX, player.posY, player.posX + (player.deltaX*20), player.posY + (player.deltaY*20), GREEN);
}

void UpdateDrawGame()
{
    UpdateGame();
    DrawGame();
}

int IndexToPixel(int index) { return index * 64; }
int PixelToIndex(int pixel) { return ((pixel + 63) / 64) - 1; }

int PosToIndex(int x, int y)
{
    int currentIndexX = PixelToIndex(x);
    int currentIndexY = PixelToIndex(y);

    return (currentIndexY * 8) + currentIndexX;
}

float degToRad(float angle)
{
    return (angle / 180) * PI;
}

bool CanGoForward()
{
    if (grid[PosToIndex(player.posX + player.deltaX*8, player.posY + player.deltaY * 8)]  == 1) { return false; }
    else { return true; }
}

bool CanGoBackward()
{
    if (grid[PosToIndex(player.posX - player.deltaX*8, player.posY - player.deltaY * 8)]  == 1) { return false; }
    else { return true; }
}