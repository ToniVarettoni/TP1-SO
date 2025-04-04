#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>           

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

    &syncState->currReading = 0;

    // Pongo a todos en su default
    gameState->width = DEF_WIDTH;
    gameState->height = DEF_HEIGHT;
    char * view = NULL;
    unsigned int seed = time(NULL);
    unsigned int delay = DEF_DELAY;
    unsigned int timeout = DEF_TIMEOUT;

    // Ver argumentos para agregar jugadores o cambiar estado
    for(int i = 0 ; i < argc ; i++) {
        if(strcmp(argv[i], SEED) == 0) {
            if(i+1 == argc || argv[i+1][0] == '-') {
                perror("./master: option requires an argument -- 's'\n
                Usage: ./ChompChamps [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ...\n");
                exit(EXIT_FAILURE);
            } else if()
            seed = atoi(argv[i+1]);
        } else if (strcmp(argv[i], WIDTH) == 0) {
            if(i+1 == argc || argv[i+1][0] == '-') {
                perror("./master: option requires an argument -- 's'\n
                Usage: ./ChompChamps [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ...\n");
                exit(EXIT_FAILURE);
            }
            gameState->width = atoi(argv[i+1]);
        } else if (strcmp(argv[i], HEIGHT) == 0) {
            gameState->height = atoi(argv[i+1]);
        } else if (strcmp(argv[i], DELAY) == 0) {
            delay = atoi(argv[i+1]);
        } else if (strcmp(argv[i], TIMEOUT) == 0) {
            timeout = atoi(argv[i+1]);
        } else if (strcmp(argv[i], VIEW) == 0) {
            view = argv[i+1];
        } else if (strcmp(argv[i], PLAYERS) == 0) {
            gameState->playerAmount = atoi(argv[i+1]);
        }
    }

}