#include "game_utils.h"
#include <stdlib.h>
#include <math.h>

#define PI 3.1415

void spawnPlayer(GameState * gameState, int index, int totalPlayers){

    gameState->players[index].score = 0;
    gameState->players[index].invalidMoves = 0;
    gameState->players[index].validMoves = 0;
    gameState->players[index].cantMove = false;

    double marginX = gameState->width * 0.2;
    double marginY = gameState->height * 0.2;

    double centerX = gameState->width / 2.0 - 1;
    double centerY = gameState->height / 2.0 - 1;

    double radiusX = (gameState->width - 2 * marginX) / 2.0;
    double radiusY = (gameState->height - 2 * marginY) / 2.0;

    double angle = (2 * PI * index) / totalPlayers;

    int x = (int)round(centerX + radiusX * cos(angle));
    int y = (int)round(centerY + radiusY * sin(angle));

    if (x < 0) x = 0;
    if (x >= gameState->width) x = gameState->width - 1;
    if (y < 0) y = 0;
    if (y >= gameState->height) y = gameState->height - 1;

    gameState->players[index].x = x;
    gameState->players[index].y = y;
}

void updateMap(GameState * gameState, int index){
    gameState->players[index].score += gameState->map[gameState->players[index].x + gameState->players[index].y * (gameState->width)];
    gameState->map[gameState->players[index].x + gameState->players[index].y * (gameState->width)] = 0 - index;
}

void setMap(GameState * gameState, unsigned int seed){
    srand(seed);
    for(size_t i = 0; i < gameState->height; i++){
        for(size_t j = 0; j < gameState->width; j++){
            gameState->map[i * gameState->width + j] = rand() % MAX_PLAYERS + 1;
        }
    }
}

bool processMove(GameState * gameState, int currentPlayer, unsigned char move){
    int x = gameState->players[currentPlayer].x;
    int y = gameState->players[currentPlayer].y;
    int w = gameState->width;
    int h = gameState->height;

    switch (move){
    case 0:
        if (y > 0 && gameState->map[x + (y-1)*w] > 0){
            gameState->players[currentPlayer].y--;
            gameState->players[currentPlayer].validMoves++;
            return true;
        }break;

    case 1:
        if (y > 0 && x < w-1 && gameState->map[(x+1) + (y-1)*w] > 0){
            gameState->players[currentPlayer].x++;
            gameState->players[currentPlayer].y--;
            gameState->players[currentPlayer].validMoves++;
            return true;
        }break;
    case 2:
        if (x < w-1 && gameState->map[(x+1) + y*w] > 0){
            gameState->players[currentPlayer].x++;
            gameState->players[currentPlayer].validMoves++;
            return true;
        }break;
    case 3:
        if (x < w-1 && y < h-1 && gameState->map[(x+1) + (y+1)*w] > 0){
            gameState->players[currentPlayer].x++;
            gameState->players[currentPlayer].y++;
            gameState->players[currentPlayer].validMoves++;
            return true;
        }break;
    case 4:
        if (y < h-1 && gameState->map[x + (y+1)*w] > 0){
            gameState->players[currentPlayer].y++;
            gameState->players[currentPlayer].validMoves++;
            return true;
        }break;
    case 5:
        if (x > 0 && y < h-1 && gameState->map[(x-1) + (y+1)*w] > 0){
            gameState->players[currentPlayer].x--;
            gameState->players[currentPlayer].y++;
            gameState->players[currentPlayer].validMoves++;
            return true;
        }break; 
    case 6: 
        if (x > 0 && gameState->map[(x-1) + y*w] > 0){
            gameState->players[currentPlayer].x--;
            gameState->players[currentPlayer].validMoves++;
            return true;
        }break;
    case 7:
        if (x > 0 && y > 0 && gameState->map[(x-1) + (y-1)*w] > 0){
            gameState->players[currentPlayer].x--;
            gameState->players[currentPlayer].y--;
            gameState->players[currentPlayer].validMoves++;
            return true;
        }break; 
    }
    
    gameState->players[currentPlayer].invalidMoves++;
    return false;
}


bool checkCantMove(GameState * gameState, int currentPlayer){

    int x = gameState->players[currentPlayer].x;
    int y = gameState->players[currentPlayer].y;
    int w = gameState->width;
    int h = gameState->height;

    for (int i = x - 1; i <= x + 1; i++){
        for (int j = y - 1; j <= y + 1; j++){
            if (i >= 0 && j >= 0 && i < w && j < h){
                if (gameState->map[i + w*j] > 0){
                    return !true;
                }
            }
        }    
    }
    return !false;

}
