#include <sys/mman.h>
#include <sys/stat.h>        
#include <string.h>
#include <fcntl.h>           
#include "structs.h"
#include <sys/types.h>  
#include <unistd.h>
#include <time.h>

#define ARGUMENT_ERROR "Usage: ./master [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ..."

#define DEF_WIDTH 10
#define DEF_HEIGHT 10
#define DEF_DELAY 200
#define DEF_TIMEOUT 10
#define MAX_LENGTH_NUM 10
#define MAX_LENGTH_PATH 100


void initView(GameState * gameState, char * view);
void checkArguments(GameState * gameState);
int initPlayer(GameState * gameState, int i);
void spawnPlayer(GameState * gameState, int i);
void updateMap(GameState * gameState, int index);
void setMap(GameState * gameState, unsigned int seed);
void get_moves();

int main(int argc, char *argv[]) {

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

    int gameState_fd = shm_open("/game_state", O_CREAT | O_RDWR, 0666);
    int sync_fd = shm_open("/game_sync", O_CREAT | O_RDWR, 0666);

    if (gameState_fd == -1 || sync_fd == -1) {
        perror("Error creating shared memory");
        exit(EXIT_FAILURE);
    }

    ftruncate(gameState_fd, sizeof(GameState) + sizeof(int) * height * width);
    ftruncate(sync_fd, sizeof(gameSync));

    GameState *gameState =  (GameState *) mmap(NULL, sizeof(GameState) + sizeof(int) * height * width, PROT_READ | PROT_WRITE, MAP_SHARED, gameState_fd, 0);
    gameSync *syncState = (gameSync *) mmap(NULL, sizeof(gameSync), PROT_READ | PROT_WRITE, MAP_SHARED, sync_fd, 0);

    if (gameState == MAP_FAILED || syncState == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&syncState->masterSem, 1, 1) == -1) {
    perror("Error initializing masterSem");
    exit(EXIT_FAILURE);
    }

    if (sem_init(&syncState->stateSem, 1, 0) == -1) {
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

    setMap(gameState, seed);

    if(view[0] != '\0'){
        initView(gameState, view);
    }

    for(gameState->playerAmount = 0; gameState->playerAmount < playerAmount; gameState->playerAmount++){
        strcpy(gameState->players[gameState->playerAmount].name, players[gameState->playerAmount]);
        playerPipes[gameState->playerAmount] = initPlayer(gameState, gameState->playerAmount);
        spawnPlayer(gameState, gameState->playerAmount);
        updateMap(gameState, gameState->playerAmount);
    }

    gameState->height = height;
    gameState->width = width;

    checkArguments(gameState); 
    
    while(1){
        sem_post(&syncState->readyToPrint);
        sem_wait(&syncState->printDone);

        // get_moves();

        sem_wait(&syncState->stateSem);
        // updateMoves();
        sem_post(&syncState->stateSem);
    }

}

void get_moves(){
    fd_set readfds;
    int maxfd = 0;
    unsigned char moves[MAX_PLAYERS];

    for(size_t i = 0; i < MAX_PLAYERS; i++){
        unsigned char move;
        read(playerPipes[i], &move, sizeof(move));
        moves[i] = move;
    }
}

void updateMoves(unsigned char moves[MAX_PLAYERS]){
    for(size_t i = 0; i < MAX_PLAYERS; i++){
        switch(moves[i])
        gameState->player[i].x
    }
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

void initView(GameState * gameState, char * view){

    int p = fork();
    
    if (p == -1){
        perror("fork failed");
        exit(EXIT_FAILURE);
    }else if (p == 0){
        char path[MAX_LENGTH_PATH];
        strcpy(path, "./"); 
        strcat(path, view);
        char height[MAX_LENGTH_NUM];
        char width[MAX_LENGTH_NUM];

        sprintf(height, "%d", gameState->height);
        sprintf(width, "%d", gameState->width);

        char *args[] = {path, height, width, NULL};
        execve(path, args, NULL);
        perror("execve view");
        exit(EXIT_FAILURE);
    }
}

int initPlayer(GameState * gameState, int i){


    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int p = fork(); 
    
    if (p == -1){
        perror("fork failed");
        exit(EXIT_FAILURE);
    }else if (p == 0){
        gameState->players[i].pid = getpid();
        
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);

        char * path = strcat("./", gameState->players[i].name);
        char height[MAX_LENGTH_NUM];
        char width[MAX_LENGTH_NUM];

        sprintf(height, "%d", gameState->height);
        sprintf(width, "%d", gameState->width);


        char *args[] = {path, height, width, NULL };
        execve(path, args, NULL);
        perror("execve player");
        exit(EXIT_FAILURE);
    }   

    close(pipefd[1]);
    return pipefd[0];

}

void spawnPlayer(GameState * gameState, int i){

    switch (i)
    {
    case 0:
        gameState->players[i].x = 0;
        gameState->players[i].y = 0;
        break;
    case 1:
        gameState->players[i].x = gameState->width - 1;
        gameState->players[i].y = 0;
        break;
    case 2: 
        gameState->players[i].x = 0;
        gameState->players[i].y = gameState->height - 1;
        break;
    case 3:
        gameState->players[i].x = gameState->width - 1;
        gameState->players[i].y = gameState->height - 1;
        break;
    case 4:
        gameState->players[i].x = gameState->width/2;
        gameState->players[i].y = 0; 
        break;
    case 5:
        gameState->players[i].x = gameState->width/2;
        gameState->players[i].y = gameState->height - 1; 
        break;
    case 6:
        gameState->players[i].x = 0;
        gameState->players[i].y = gameState->height / 2; 
        break;
    case 7:
        gameState->players[i].x = gameState->width - 1;
        gameState->players[i].y = gameState->height / 2; 
        break;
    case 8:
        gameState->players[i].x = gameState->width/2;
        gameState->players[i].y = gameState->height/2; 
        break;    

    default:
        break;
    }
}

void updateMap(GameState * gameState, int index){
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

