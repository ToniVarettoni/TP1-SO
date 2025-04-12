#include <sys/mman.h>
#include <sys/stat.h>        
#include <string.h>
#include <fcntl.h>           
#include <sys/types.h>  
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include "shared_memory.h"
#include "game_utils.h"
#include "process_manager.h"
#include <sys/wait.h>

#define ARGUMENT_ERROR "Usage: ./master [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ..."

#define DEF_WIDTH 10
#define DEF_HEIGHT 10
#define DEF_DELAY 200
#define DEF_TIMEOUT 10
#define MICRO_TO_MILI 1000


void checkArguments(GameState * gameState);

int main(int argc, char *argv[]) {

    shm_unlink("/game_state");
    shm_unlink("/game_sync");
    
    int height = DEF_HEIGHT;
    int width = DEF_WIDTH;

    char players[MAX_PLAYERS][MAX_LENGTH_PATH];
    int playerAmount = 0;
    char view[MAX_LENGTH_PATH];
    view[0] = '\0';
    unsigned int seed = time(NULL);
    unsigned int delay = DEF_DELAY;
    unsigned int timeout = DEF_TIMEOUT;
    int playerPipes[MAX_PLAYERS];

    for(int i = 0 ; i < argc ; i++) {
        if(argv[i][0] == '-'){
            char c = argv[i][1];
            if(c != '\0'){
                if(c != 's' && c != 'w' && c != 'h' && c != 'd' && c != 't' && c != 'v' && c != 'p'){
                    printf("./master: invalid option -- '%c'\n%s\n", c, ARGUMENT_ERROR);
                    exit(EXIT_FAILURE);
                }else{
                    if(i+1 == argc || argv[i+1][0] == '-') {
                        printf("./master: option requires an argument -- '%c'\n%s\n", c, ARGUMENT_ERROR);
                        exit(EXIT_FAILURE);
                    }
                    switch(c){
                        case 's': 
                            seed = atoi(argv[++i]);
                            break;
                        case 'w':
                            width = atoi(argv[++i]);
                            break;
                        case 'h':
                            height = atoi(argv[++i]);
                            break;
                        case 'd':
                            delay = atoi(argv[++i]);
                            break;
                        case 't':
                            timeout = atoi(argv[++i]);
                            break;
                        case 'v':
                            strcpy(view, argv[++i]);
                            break;
                        case 'p':
                            while(i + 1 < argc && argv[i + 1][0] != '-'){
                                strcpy(players[playerAmount++], argv[i + 1]);
                                i++;
                            }
                            break;
                    }
                }
            }
        }
    }

    int gameState_fd;
    int syncState_fd;

    GameState * gameState = (GameState *)shm_create_and_map("/game_state", sizeof(GameState) + sizeof(int) * height * width, RW, &gameState_fd);
    gameSync * syncState = (gameSync *)shm_create_and_map("/game_sync", sizeof(gameSync), RW, &syncState_fd);

    if (sem_init(&syncState->masterSem, 1, 1) == -1) {
        perror("Error initializing masterSem");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&syncState->stateSem, 1, 1) == -1) {
        perror("Error initializing stateSem");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&syncState->currReadingSem, 1, 1) == -1) {
        perror("Error initializing currReadingSem");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&syncState->readyToPrint, 1, 0) == -1) {
        perror("Error initializing readyToPrint sem");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&syncState->printDone, 1, 1) == -1) {
        perror("Error initializing print done Sem");
        exit(EXIT_FAILURE);
    }

    int viewPid;
    gameState->height = height;
    gameState->width = width;
    gameState->isOver = false;
    syncState->currReading = 0;


    setMap(gameState, seed);

    if(view[0] != '\0'){
        initView(gameState, view, &viewPid);
    }

    for(gameState->playerAmount = 0; gameState->playerAmount < playerAmount; gameState->playerAmount++){
        strcpy(gameState->players[gameState->playerAmount].name, players[gameState->playerAmount]);
        playerPipes[gameState->playerAmount] = initPlayer(gameState, gameState->playerAmount);
        spawnPlayer(gameState, gameState->playerAmount);
        updateMap(gameState, gameState->playerAmount);
        gameState->players[gameState->playerAmount].cantMove = false;
    }

    checkArguments(gameState); 

    int currentPlayer = 0;
    int playingPlayers = gameState->playerAmount;
    time_t lastMoveTime = time(NULL); 
    time_t currentTime;

    while(playingPlayers > 0){

        if (view[0] != '\0'){
            sem_post(&syncState->readyToPrint);
            sem_wait(&syncState->printDone);
        }

        sem_wait(&syncState->masterSem);
        sem_wait(&syncState->stateSem);
        sem_post(&syncState->masterSem);

        currentPlayer++;

        for (size_t i = 0; i < playerAmount; i++){

            if (view[0] != '\0'){
                usleep(delay * MICRO_TO_MILI);
                sem_post(&syncState->readyToPrint);
                sem_wait(&syncState->printDone);
            }

            currentPlayer = (currentPlayer + i) % gameState->playerAmount;

            int fd = playerPipes[currentPlayer];

            if (fd == -1)
                continue;

            if (checkCantMove(gameState, currentPlayer)){ 
                gameState->players[currentPlayer].cantMove = true;
                playingPlayers--;
                close(fd);
                playerPipes[currentPlayer] = -1;
                continue;
            }

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);

            struct timeval timeout = {0, 10};

            int ready = select(fd + 1, &readfds, NULL, NULL, &timeout);

            if (ready < 0 ){
                perror("select");
                continue;
            }

            if (ready > 0 && FD_ISSET(fd, &readfds)){

                unsigned char move;
                int n = read(fd, &move, sizeof(move));

                if (n <= 0){
                    close(fd);
                    playerPipes[currentPlayer] = -1;
                    playingPlayers--;
                }else if(processMove(gameState, currentPlayer, move)){
                    updateMap(gameState, currentPlayer);
                }
                lastMoveTime = time(NULL);
            }
        }
        
        currentTime = time(NULL);
        if ((int)difftime(currentTime, lastMoveTime) >= timeout || playingPlayers <= 0) {
            gameState->isOver = true;
        }

        
        sem_post(&syncState->stateSem);
    }

    sem_post(&syncState->masterSem);               
    sem_post(&syncState->stateSem);  
    
    if(view[0] != '\0'){
        waitpid(viewPid, NULL, 0);
    }


    shm_cleanup(gameState_fd, gameState, sizeof(GameState) + sizeof(int) * height * width);
    shm_cleanup(syncState_fd, syncState, sizeof(gameSync));

    shm_unlink("/game_state");
    shm_unlink("/game_sync");

    if(sem_destroy(&syncState->masterSem) == -1) {
        perror("Error destroying masterSem");
        exit(EXIT_FAILURE);
    }
    if(sem_destroy(&syncState->stateSem) == -1) {
        perror("Error destroying stateSem");
        exit(EXIT_FAILURE);
    }
    if(sem_destroy(&syncState->currReadingSem) == -1) {
        perror("Error destroying currReadingSem");
        exit(EXIT_FAILURE);
    }
    if(sem_destroy(&syncState->readyToPrint) == -1) {
        perror("Error destroying readyToPrint sem");
        exit(EXIT_FAILURE);
    }
    if(sem_destroy(&syncState->printDone) == -1) {
        perror("Error destroying print done Sem");
        exit(EXIT_FAILURE);
    }


    return 0;
}

void checkArguments(GameState * gameState){
    if (gameState->height < DEF_HEIGHT || gameState->width < DEF_WIDTH){
        printf("Error: Minimal board dimensions: 10x10. Given %dx%d\n", gameState->height, gameState->width);
        exit(EXIT_FAILURE);
    }
    if (gameState->playerAmount == 0){
        printf("Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }
    if (gameState->playerAmount > 9){
        printf("Error: At most 9 players can be specified using -p.\n");
        exit(EXIT_FAILURE);
    }
    return;
}
