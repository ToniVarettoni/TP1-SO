#include <stdio.h>
#include <unistd.h>
#include "structs.h"


int main(int argc, char *argv[]){

    int gameState_fd = shm_open("/game_state", O_RDONLY, 0666);
    if (gameState_fd == -1) {
        perror("Error abriendo la memoria compartida");
        exit(EXIT_FAILURE);
    }

    int sync_fd = shm_open("/game_sync", O_RDWR, 0666);
    if (sync_fd == -1) {
        perror("Error abriendo la memoria compartida");
        exit(EXIT_FAILURE);
    }

    GameState *gameState = (GameState *) mmap(NULL, sizeof(GameState), PROT_READ, MAP_SHARED, gameState_fd, 0);

    if (gameState == MAP_FAILED) {
        perror("Error mapeando la memoria compartida");
        close(gameState_fd);
        exit(EXIT_FAILURE);
    }

    gameSync *syncState = (gameSync *) mmap(NULL, sizeof(gameSync), PROT_READ | PROT_WRITE, MAP_SHARED, sync_fd, 0);

    if (syncState == MAP_FAILED) {
        perror("Error mapeando la memoria compartida");
        close(gameState_fd);
        exit(EXIT_FAILURE);
    }

    int h = gameState->height;
    int w = gameState->width;


    while (!gameState->isOver)
    {
        sem_wait(&syncState->readyToPrint);

        printf("\033[H\033[J");

        /*for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                if (gameState->map[i * w + j] > 0) {
                    printf("%d ", gameState->map[i * w + j]); 
                } else {
                    printf(". ");
                }
            }
            printf("\n");
        }*/

        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                if (gameState->map[i * w + j] <= 0) {
                    printf("%d\t", gameState->map[i * w + j]); 
                } else {
                    printf(".\t");
                }
            }
            printf("\n");
        }
        
        sem_post(&syncState->printDone);
    }   
    return 0;
    }
    
    



