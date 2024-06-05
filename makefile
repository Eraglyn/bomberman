# Compilateur et options de compilation
CC = gcc
CFLAGS = -Wall

# Répertoire contenant les fichiers source
SRC_DIR = src

# Fichiers sources et objets pour le serveur
SERVER_SRC = $(SRC_DIR)/global_server.c $(SRC_DIR)/game_logic.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)

# Fichiers sources et objets pour le client
CLIENT_SRC = $(SRC_DIR)/client.c $(SRC_DIR)/game_logic.c $(SRC_DIR)/game_render.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# Noms des exécutables
SERVER_EXEC = global_server
CLIENT_EXEC = client

# Règle par défaut
all: $(CLIENT_EXEC) $(SERVER_EXEC) 

# Compilation de l'exécutable du serveur
$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation de l'exécutable du client
$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses

# Règle générique pour compiler les fichiers objets
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Règle pour nettoyer les fichiers générés
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SERVER_EXEC) $(CLIENT_EXEC)

# Spécification des dépendances
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
