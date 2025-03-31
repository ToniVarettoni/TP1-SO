#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdio.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include "structs.h"


unsigned char getDir(int x, int y, int i, int j);

int main(int argc, char * args[]){

    srand(time(NULL));

    int gameState_fd = shm_open("/game_state", O_RDONLY, 0666);
    if (gameState_fd == -1) {
        perror("Error abriendo la memoria compartida de state");
        exit(EXIT_FAILURE);
    }

    int sync_fd = shm_open("/game_sync", O_RDWR, 0666);
    if (sync_fd == -1) {
        perror("Error abriendo la memoria compartida de sync");
        exit(EXIT_FAILURE);
    }

    GameState *gameState = (GameState *) mmap(NULL, sizeof(GameState), PROT_READ, MAP_SHARED, gameState_fd, 0);

    if (gameState == MAP_FAILED) {
        perror("Error mapeando la memoria compartida de state");
        close(gameState_fd);
        exit(EXIT_FAILURE);
    }

    gameSync *syncState = (gameSync *) mmap(NULL, sizeof(gameSync), PROT_READ | PROT_WRITE, MAP_SHARED, sync_fd, 0);

    if (syncState == MAP_FAILED) {
        perror("Error mapeando la memoria compartida de sync");
        close(gameState_fd);
        exit(EXIT_FAILURE);
    }

    pid_t myPid = getpid();
    int id = 0;

    for (size_t i = 0; i < gameState->playerAmount; i++)
    {
        if (myPid == gameState->players[i].pid)
        {
            id = i;
            break;
        }
        
    }

    
    
    while (1)
    {
        sem_wait(&syncState->masterSem); // espero al master
    

        sem_wait(&syncState->currReadingSem);
        syncState->currReading++;
        sem_post(&syncState->currReadingSem);
        if (syncState->currReading == 1)
        {
            sem_wait(&syncState->stateSem);
        }
        
        
        
        if (gameState->players[id].cantMove)
        {
            sem_wait(&syncState->currReadingSem);
            syncState->currReading--;
            sem_post(&syncState->currReadingSem);
            if (syncState->currReading == 0)
            {
                sem_post(&syncState->stateSem);
            }
            sem_post(&syncState->masterSem);
            break;
        }

        int h = gameState->height;
        int w = gameState->width;
        int x = gameState->players[id].x;
        int y = gameState->players[id].y;
        int max = 0;
        unsigned char move = 0;


        for (int i = x - 1; i <= x + 1; i++)
        {
            for (int j = y - 1; j <= y + 1; j++)
            {
                if (i >= 0 && j >= 0 && i < w && j < h){
                    if (max < gameState->map[i + w*j])  //si la casilla corresponde a un jugador -> gameState->map[i + w*j] < 0 siempre
                    {
                        max = gameState->map[i + w*j];
                        move = getDir(x, y, i, j);
                    }
                }
            }
            
        }
        

        write(1, &move, 1);

        sem_wait(&syncState->currReadingSem);
        syncState->currReading--;
        sem_post(&syncState->currReadingSem);
        if (syncState->currReading == 0)
        {
            sem_post(&syncState->stateSem);
        }
        
        sem_post(&syncState->masterSem);
        usleep(500000);
    }
    

    return 0;
}




unsigned char getDir(int x, int y, int i, int j){

    if (i == x - 1){

        if (j == y - 1)
            return 7;
        else if(j == y)
            return 6;        
        return 5;

    }else if(i == x){

        if (j == y - 1)
             return 0;
        // x == i && y == i corresponde al player, no lo contemplo
        return 4;

    }else if (j == y - 1){
        return 1;

    }else if (j == y)
        return 2;

    return 3;
    
    


}