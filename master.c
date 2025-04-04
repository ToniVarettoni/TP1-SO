#include <sys/mman.h>
#include <sys/stat.h>        
#include <string.h>
#include <fcntl.h>           
#include <structs.h>
#include <sys/types.h>  
#include <unistd.h>
#define SEED "-s"
#define WIDTH "-w"
#define HEIGHT "-h"
#define DELAY "-d"
#define TIMEOUT "-t"
#define VIEW "-v"
#define PLAYERS "-p"

#define DEF_WIDTH 10
#define DEF_HEIGHT 10
#define DEF_DELAY 200
#define DEF_TIMEOUT 10


void initView(GameState * gameState, char * view);
void checkArguments(GameState * gameState);
int initPlayer(GameState * gameState, int i);

int main(int argc, char *argv[]) {

    int gameState_fd = shm_open("/game_state", O_CREAT | O_RDWR, 0666);
    int sync_fd = shm_open("/game_sync", O_CREAT | O_RDWR, 0666);

    if (gameState_fd == -1 || sync_fd == -1) {
        perror("Error creating shared memory");
        exit(EXIT_FAILURE);
    }

    ftruncate(gameState_fd, sizeof(GameState));
    ftruncate(sync_fd, sizeof(gameSync));

    GameState *gameState =  (GameState *) mmap(NULL, sizeof(GameState), PROT_READ | PROT_WRITE, MAP_SHARED, gameState_fd, 0);
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

    syncState->currReading = 0;

    // Pongo a todos en su default
    gameState->width = DEF_WIDTH;
    gameState->height = DEF_HEIGHT;
    char * view = NULL;
    unsigned int seed = time(NULL);
    unsigned int delay = DEF_DELAY;
    unsigned int timeout = DEF_TIMEOUT;
    int playerPipes[9];

    // Ver argumentos para agregar jugadores o cambiar estado
    for(int i = 0 ; i < argc ; i++) {
        if(strcmp(argv[i], SEED) == 0) {
            if(i+1 == argc || argv[i+1][0] == '-') {
                perror("./master: option requires an argument -- 's'\nUsage: ./ChompChamps [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ...\n");
                exit(EXIT_FAILURE);
            }
            seed = atoi(argv[i+1]);
        } else if (strcmp(argv[i], WIDTH) == 0) {
            if(i+1 == argc || argv[i+1][0] == '-') {
                perror("./master: option requires an argument -- 'w'\nUsage: ./ChompChamps [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ...\n");
                exit(EXIT_FAILURE);
            }
            gameState->width = atoi(argv[i+1]);
        } else if (strcmp(argv[i], HEIGHT) == 0) {
            if(i+1 == argc || argv[i+1][0] == '-') {
                perror("./master: option requires an argument -- 'h'\nUsage: ./ChompChamps [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ...\n");
                exit(EXIT_FAILURE);
            }
                gameState->height = atoi(argv[i+1]);
        } else if (strcmp(argv[i], DELAY) == 0) {
            if(i+1 == argc || argv[i+1][0] == '-') {
                perror("./master: option requires an argument -- 'd'\nUsage: ./ChompChamps [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ...\n");
                exit(EXIT_FAILURE);
            }
            delay = atoi(argv[i+1]);
        } else if (strcmp(argv[i], TIMEOUT) == 0) {
            if(i+1 == argc || argv[i+1][0] == '-') {
                perror("./master: option requires an argument -- 't'\nUsage: ./ChompChamps [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ...\n");
                exit(EXIT_FAILURE);
            }
            timeout = atoi(argv[i+1]);
        } else if (strcmp(argv[i], VIEW) == 0) {
            if(i+1 == argc || argv[i+1][0] == '-') {
                perror("./master: option requires an argument -- 'v'\nUsage: ./ChompChamps [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ...\n");
                exit(EXIT_FAILURE);
            }
            initView(gameState, argv[i+1]);

        } else if (strcmp(argv[i], PLAYERS) == 0) {
            size_t j = 1;
            for (; i + j < argc && argv[i+j][0] != '-'; j++){
                gameState->playerAmount++;
                for (size_t k = 0; k < strlen(argv[i+j] && k < 16); k++){
                    gameState->players[j].name[k] = argv[i+j];
                }
                playerPipes[j] = initPlayer(gameState, j);
                spawnPlayer(gameState, j);
                
                 

            }
            i += j;
        }
    }

    checkArguments(gameState); 

}


void checkArguments(GameState * gameState){

    if (gameState->height < 10 || gameState->width < 10){
        printf("Error: Minimal board dimensions: 10x10. Given %dx%d\n", gameState->height, gameState->width);
        exit(EXIT_FAILURE);
    }
    if (gameState->playerAmount == 0){
        printf("Error: At least one player must be specified using -p.\n");
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
        char * path = strcat("./", view);
        char *args[] = {path, gameState->height, gameState->width, NULL };
        execve(path, args, NULL);
        perror("execve");   /* execve() returns only on error */
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
        char *args[] = {path, gameState->height, gameState->width, NULL };
        execve(path, args, NULL);
        perror("execve");   /* execve() returns only on error */
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
