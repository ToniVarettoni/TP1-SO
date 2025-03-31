gcc player.c -Wall -o player
gcc vista.c -Wall -o vista
docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0