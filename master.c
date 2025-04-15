#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <string.h>
#include <fcntl.h>           
#include "include/structs.h"
#include <sys/types.h>  
#include <time.h>
#include <signal.h>
#include <sys/select.h>
#include "include/shared_memory.h"
#include "include/game_utils.h"
#include "include/process_manager.h"
#include <sys/wait.h>

#define ARGUMENT_ERROR "Usage: ./master [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ..."

#define DEF_WIDTH 10
#define DEF_HEIGHT 10
#define DEF_DELAY 200
#define DEF_TIMEOUT 10
#define MAX_LENGTH_PATH 100
#define MAX_LENGTH_NAME 16
#define MILI_TO_SEC 0.001
#define MILI_TO_NANO 1000000

void checkArguments(int height, int width, int playingPlayers);
void printStart(GameState * gameState, int delay, int timeout, int seed, char * view);

int main(int argc, char *argv[]) {
    
    int height = DEF_HEIGHT;
    int width = DEF_WIDTH;

    char players[MAX_PLAYERS][MAX_LENGTH_NAME];
    int playingPlayers = 0;
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
                                strcpy(players[playingPlayers++], argv[i + 1]);
                                i++;
                            }
                            break;
                    }
                }
            }
        }
    }

    checkArguments(height, width, playingPlayers); 

    int gameState_fd;
    int syncState_fd;

    GameState * gameState = (GameState *)shm_create_and_map("/game_state", sizeof(GameState) + sizeof(int) * height * width, RW, &gameState_fd);
    GameSync * syncState = (GameSync *)shm_create_and_map("/game_sync", sizeof(GameSync), RW, &syncState_fd);

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

    for(gameState->playerAmount = 0; gameState->playerAmount < playingPlayers; gameState->playerAmount++){
        strcpy(gameState->players[gameState->playerAmount].name, players[gameState->playerAmount]);
        playerPipes[gameState->playerAmount] = initPlayer(gameState, gameState->playerAmount);
        spawnPlayer(gameState, gameState->playerAmount, playingPlayers);
        gameState->map[gameState->players[gameState->playerAmount].x + gameState->players[gameState->playerAmount].y * (gameState->width)] = 0 - gameState->playerAmount;
    }

    int roundRobinIdx = 0;
    time_t lastMoveTime = time(NULL); 
    time_t currentTime;

    printStart(gameState, delay, timeout, seed, view);

    while(!gameState->isOver){

        sem_wait(&syncState->masterSem);
        sem_wait(&syncState->stateSem);
        sem_post(&syncState->masterSem);

        fd_set readfds;
        FD_ZERO(&readfds);

        int max_fd = -1;

        for (int i = 0; i < gameState->playerAmount; i++) {
            int fd = playerPipes[i];
            if (fd != -1) {
                FD_SET(fd, &readfds);
                if (fd > max_fd)
                    max_fd = fd;
            }
        }

        struct timeval select_timeout = {0, 10};
        int ready = select(max_fd + 1, &readfds, NULL, NULL, &select_timeout);

        if (ready < 0 ){
            perror("select");
            continue;
        }

        if (ready > 0) {
            for (int i = 0; i < gameState->playerAmount; i++) {
                int currentPlayer = (i + roundRobinIdx) % gameState->playerAmount;
                if(!gameState->players[currentPlayer].cantMove){
                    int fd = playerPipes[currentPlayer];
                    if (checkCantMove(gameState, currentPlayer)){ 
                        gameState->players[currentPlayer].cantMove = true;
                        playingPlayers--;
                        close(fd);
                        playerPipes[currentPlayer] = -1;
                        continue;
                    }
                    if (FD_ISSET(fd, &readfds)) {
                        unsigned char move;
                        int n = read(fd, &move, sizeof(move));
                        if (n <= 0) {
                            close(fd);
                            playerPipes[i] = -1;
                            playingPlayers--;
                        } else if (processMove(gameState, currentPlayer, move)) {
                            updateMap(gameState, currentPlayer);
                            if (view[0] != '\0'){
                                struct timespec ts;
                                ts.tv_sec = delay * MILI_TO_SEC;
                                ts.tv_nsec = delay * MILI_TO_NANO;
                                nanosleep(&ts, NULL);
                                sem_post(&syncState->readyToPrint);
                                sem_wait(&syncState->printDone);
                            }
                        }
                        lastMoveTime = time(NULL);
                    }
                }
                lastMoveTime = time(NULL);
            }
        }
        
        roundRobinIdx++;
        
        currentTime = time(NULL);
        if ((int)difftime(currentTime, lastMoveTime) >= timeout || playingPlayers <= 0) {
            gameState->isOver = true;
        }

        sem_post(&syncState->stateSem);
    }

    sem_post(&syncState->masterSem);               
    sem_post(&syncState->stateSem);  
    
    int status;

    if(view[0] != '\0'){
        sem_post(&syncState->readyToPrint);
        waitpid(viewPid, &status, 0);
        printf("\nView exited (%d)\n", status);
    }else{
        printf("\033[H\033[J");
    }

    for (int i = 0; i < gameState->playerAmount; i++){
        PlayerState p = gameState->players[i];

        if (p.pid > 0) kill(p.pid, SIGTERM);
        if (playerPipes[i] != -1) close(playerPipes[i]);

        waitpid(p.pid, &status, 0);
        printf("Player %s (%d) exited (%d) with a score of %d / %d / %d\n", p.name, i, status, p.score, p.validMoves, p.invalidMoves);
    }

    shm_cleanup(gameState_fd, gameState, sizeof(GameState) + sizeof(int) * height * width);
    shm_cleanup(syncState_fd, syncState, sizeof(GameSync));

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

void checkArguments(int height, int width, int playingPlayers){
    if (height < DEF_HEIGHT || width < DEF_WIDTH){
        printf("Error: Minimal board dimensions: 10x10. Given %dx%d\n", height, width);
        exit(EXIT_FAILURE);
    }
    if (playingPlayers == 0){
        printf("Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }
    if (playingPlayers > MAX_PLAYERS){
        printf("Error: At most %d players can be specified using -p.\n", MAX_PLAYERS);
        exit(EXIT_FAILURE);
    }
    return;
}

void printStart(GameState * gameState, int delay, int timeout, int seed, char * view){
    printf("width: %d\nheight: %d\ndelay: %d\ntimeout: %d\nseed: %d\nview: %s\n", gameState->width, gameState->height, delay, timeout, seed, view[0] != '\0'? view : "-");
    printf("num_players: %d\n", gameState->playerAmount);
    for(int i = 0; i < gameState->playerAmount; i++){
        printf("\t%s\n", gameState->players[i].name);
    }
    sleep(2);
}

