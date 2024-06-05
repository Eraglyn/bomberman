#ifndef GLOBAL_SERVER_H
#define GLOBAL_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "game_logic.h"
#define BOARD_HEIGHT 25
#define BOARD_WIDTH 25

#define GS_FFA_TYPE 9 & 0x1FFF
#define GS_TEAM_TYPE 10 & 0x1FFF

#define GS_READY 4

typedef enum
{
    WAITING,
    RUNNING,
    GAME_OVER
} t_game_state;

// Structure contenant les informations sur un serveur de jeu
typedef struct
{
    u_int16_t gs_num_update;               // Nombre de mise à jour
    struct board *gs_board;                // Grille du jeu
    int gs_udp_sock;                       // Socket UDP
    int gs_multicast_sock;                 // Socket multicast
    int gs_player_list[4];                 // Liste des identifiants des joueurs
    struct sockaddr_in6 gs_multicast_addr; // Adresse multicast
    size_t gs_nb_players;                  // Nombre de joueurs dans le serveur
    t_game_state gs_state;                 // Etat du jeu
    size_t gs_players_ready;               // Nombre de joueurs prets
} t_game_server_info;

void *redistrib_tchat(void *game_info);

#endif