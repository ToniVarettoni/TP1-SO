gcc -g -O0 player.c -Wall -o player
gcc -g -O0 vista.c -Wall -o vista
gcc -g -O0 maxPlayer.c -Wall -o MAX
docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes ./ChompChamps
getopt