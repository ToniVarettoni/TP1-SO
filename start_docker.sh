gcc -g -O0 player.c -Wall -o player
gcc -g -O0 vista.c -Wall -o vista
gcc -g -O0 maxPlayer.c -Wall -o MAX
gcc master.c -Wall -o master -lm
docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes ./master
getopt

void spawnPlayer(GameState * gameState, int index) {
    double centerX = gameState->width / 2.0;
    double centerY = gameState->height / 2.0;

    double horizontalSemiAxis = gameState->width / 2.5;
    double verticalSemiAxis = gameState->height / 2.5;

    double angle = (2 * PI * index) / gameState->playerAmount;
    int x = (int)round(centerX + horizontalSemiAxis * cos(angle));
    int y = (int)round(centerY + verticalSemiAxis * sin(angle));

    if (x < 0) x = 0; 
    if (x >= gameState->width) x = gameState->width - 1;
    if (y < 0) y = 0;
    if (y >= gameState->height) y = gameState->height - 1;

    gameState->players[index].x = x;
    gameState->players[index].y = y;
}