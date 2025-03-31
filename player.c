#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdio.h>
#include <semaphore.h>
#include "structs.h"
#include <time.h>
#include <unistd.h>


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


        unsigned char move = (unsigned char) rand()%8;
        write(1, &move , sizeof(move));

        sem_wait(&syncState->currReadingSem);
        syncState->currReading--;
        sem_post(&syncState->currReadingSem);
        if (syncState->currReading == 0)
        {
            sem_post(&syncState->stateSem);
        }
        
        sem_post(&syncState->masterSem);
        usleep(500);
    }
    

    return 0;
}
