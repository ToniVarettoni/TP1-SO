gcc player.c shared_memory.c -o player -lrt -Wall -O0
gcc maxPlayer.c shared_memory.c -o MAX -lrt -Wall -O0
gcc vista.c shared_memory.c -o vista -lrt -Wall -O0
gcc master.c shared_memory.c -o master -lrt -Wall -O0
docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0

#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes ./master
#getopt
#lsof: lista las openfiles de un cierto programa