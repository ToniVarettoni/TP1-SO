#!/bin/bash

# Nombre del contenedor
CONTAINER_NAME="TP1-SO"

# Directorio dentro del contenedor donde están los archivos
CONTAINER_DIR="/root"

# Asegurar que el contenedor está corriendo
if ! docker ps | grep -q "$CONTAINER_NAME"; then
    echo "El contenedor $CONTAINER_NAME no está corriendo. Inícialo primero."
    exit 1
fi

# Copiar los archivos locales al contenedor
docker cp vista.c "$CONTAINER_NAME":"$CONTAINER_DIR"
docker cp player.c "$CONTAINER_NAME":"$CONTAINER_DIR"

# Ejecutar la compilación dentro del contenedor
docker exec "$CONTAINER_NAME" bash -c "cd $CONTAINER_DIR && gcc -o vista vista.c"
docker exec "$CONTAINER_NAME" bash -c "cd $CONTAINER_DIR && gcc -o player player.c"

# Extraer los ejecutables compilados de vuelta a la máquina local
docker cp "$CONTAINER_NAME":"$CONTAINER_DIR/vista" .
docker cp "$CONTAINER_NAME":"$CONTAINER_DIR/player" .

echo "Compilación completada. Ejecutables generados: ./vista y ./player"

