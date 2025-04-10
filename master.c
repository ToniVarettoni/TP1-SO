#include <sys/mman.h>
#include <sys/stat.h>        
#include <string.h>
#include <fcntl.h>           
#include "structs.h"
#include <sys/types.h>  
#include <unistd.h>
#include <time.h>
#include <sys/select.h>

#define ARGUMENT_ERROR "Usage: ./master [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ..."

#define DEF_WIDTH 10
#define DEF_HEIGHT 10
#define DEF_DELAY 200
#define DEF_TIMEOUT 10
#define MAX_LENGTH_NUM 10
#define MAX_LENGTH_PATH 100
#define MICRO_TO_MILI 1000


void initView(GameState * gameState, char * view);
void checkArguments(GameState * gameState);
int initPlayer(GameState * gameState, int i);
void spawnPlayer(GameState * gameState, int i);
void updateMap(GameState * gameState, int index);
void setMap(GameState * gameState, unsigned int seed);
void get_moves(int playerPipes[]);
void updateMoves(unsigned char moves[MAX_PLAYERS], GameState * gameState);
bool processMove(GameState * gameState, int currentPlayer, unsigned char move);
bool checkCantMove(GameState * gameState, int currentPlayer);

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
    gameState->isOver = false;
    syncState->currReading = 0;

    checkArguments(gameState); 

    int currentPlayer = 0;
    int playingPlayers = gameState->playerAmount;
    time_t lastMoveTime = time(NULL); 
    time_t currentTime;
    
    sleep(1);
    while(!gameState->isOver){

        sem_wait(&syncState->masterSem);
        sem_wait(&syncState->stateSem);
        sem_post(&syncState->masterSem);
        
        currentPlayer++;

        for (size_t i = 0; i < playerAmount; i++){

            if (view[0] != '\0'){
                sem_post(&syncState->readyToPrint);
                sem_wait(&syncState->printDone);
            }

            currentPlayer = (currentPlayer + i) % gameState->playerAmount;
            
            int fd = playerPipes[currentPlayer];

            if (fd == -1)
                continue;
            
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);
    
            struct timeval timeout = {0, 1};
    
            int ready = select(fd + 1, &readfds, NULL, NULL, &timeout);
        
            if (ready < 0 ){
                perror("select");
                break;
            }
            
            if (ready > 0 && FD_ISSET(fd, &readfds)){

                unsigned char move;
                int n = read(fd, &move, sizeof(move));
                
                if (n <= 0){
                    close(fd);
                    playerPipes[currentPlayer] = -1;
                    playingPlayers--;
                }else if(processMove(gameState, currentPlayer, move) == 1){
                    updateMap(gameState, currentPlayer);
                    if (checkCantMove(gameState, currentPlayer)){
                        gameState->players[currentPlayer].cantMove = true;
                        playingPlayers--;
                    }
                }

            }
                lastMoveTime = time(NULL);
                usleep(delay * MICRO_TO_MILI);
        }
        
        currentTime = time(NULL);
        if ((int)difftime(currentTime, lastMoveTime) >= timeout || playingPlayers == 0) {
            gameState->isOver = true;
        }

        
        sem_post(&syncState->stateSem);               
    }

    sem_post(&syncState->masterSem);               
    sem_post(&syncState->stateSem);               


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

        char path[MAX_LENGTH_PATH];
        strcpy(path, "./"); 
        strcat(path, gameState->players[i].name);
        char height[MAX_LENGTH_NUM];
        char width[MAX_LENGTH_NUM];

        sprintf(height, "%d", gameState->height);
        sprintf(width, "%d", gameState->width);

        char *args[] = {path, height, width, NULL};        
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
                    return !true;                   //UN POCO CONFUSO PERO ESTA FUNCION TIENE MAS SENTIDO
                }
            }
        }    
    }
    return !false;

}
