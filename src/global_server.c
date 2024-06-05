#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#include <errno.h>

#include "global_server.h"
#include "game_logic.h"

#define TEXT_SIZE 255
#define MAX_NB_GAMES 100
#define MAX_PLAYERS 4
#define MUTLICAST_ADDR "ff12::1:2:3"

int port_udp = 4243;

t_game_server_info *ffa_games[MAX_NB_GAMES];
t_game_server_info *team_games[MAX_NB_GAMES];

pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER;

void send_full_board(t_game_server_info *game_info)
{
    u_int16_t full_grid[((BOARD_WIDTH * BOARD_HEIGHT) / 2) + 3];
    u_int16_t req_id_eq = (11 & 0x1FFF) | (0 & 0x2) << 2 | 0;
    full_grid[0] = htons(req_id_eq);
    full_grid[1] = htons(game_info->gs_num_update);
    game_info->gs_num_update++;
    full_grid[2] = htons((BOARD_HEIGHT & 0xFF) << 8 | (BOARD_WIDTH & 0xFF));
    for (size_t i = 0; i < BOARD_HEIGHT; i++)
    {
        for (size_t j = 0; j < BOARD_WIDTH / 2; j++)
        {
            u_int16_t part_left = (game_info->gs_board->grid)[i][j]->state & 0xFF;
            u_int16_t part_right = (game_info->gs_board->grid)[i][j + 1]->state & 0xFF;
            full_grid[3 + i * (BOARD_WIDTH / 2) + j] = htons(part_left << 8 | part_right);
        }
    }

    // Envoi de la grille
    int n = sendto(game_info->gs_multicast_sock, full_grid, sizeof(full_grid), 0, (struct sockaddr *)&game_info->gs_multicast_addr, sizeof(game_info->gs_multicast_addr));
    if (n < 0)
    {
        perror("send");
        exit(EXIT_FAILURE);
    }

    printf("Partie[%d] : Envoi de la grille au serveur de jeu : %d\n", game_info->gs_multicast_addr.sin6_port, n);
}

void handle_game(t_game_server_info *game_info)
{
    // Gestion du serveur de jeu
    while (game_info->gs_players_ready < MAX_PLAYERS)
    {
    }
    game_info->gs_state = RUNNING;
    printf("Partie[%d] : Tous les joueurs sont pret, lancement de la partie...\n", game_info->gs_multicast_addr.sin6_port);
    // char buf[100];
    // sprintf(buf, "Partie[%d] : Tous les joueurs sont pret, lancement de la partie...\n", game_info->gs_multicast_addr.sin6_port);
    // int sent = sendto(game_info->gs_multicast_sock, buf, strlen(buf), 0, (struct sockaddr *)&game_info->gs_multicast_addr, sizeof(game_info->gs_multicast_addr));
    // printf("%d sent\n", sent);

    // Lancement de la partie
    // Envoi de la grille
    pthread_t gs_thread;
    pthread_create(&gs_thread, NULL, (void *)redistrib_tchat, game_info);
    pthread_detach(gs_thread);
    // Gérer l'échec de la création du thread si nécessaire
    while (game_info->gs_state == RUNNING)
    {
        send_full_board(game_info);
        sleep(1);
    }
}

t_game_server_info *init_game()
{
    // Initialisation du serveur de jeu
    t_game_server_info *game_info = malloc(sizeof(t_game_server_info));
    memset(game_info, 0, sizeof(t_game_server_info));
    game_info->gs_state = WAITING;
    game_info->gs_nb_players = 0;

    // Initialisation du plateau
    board *b = malloc(sizeof(board));
    memset(b, 0, sizeof(board));
    init_board(b, BOARD_WIDTH, BOARD_HEIGHT);
    game_info->gs_board = b;

    // Initialisation du socket UDP
    int udp_sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (udp_sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in6 address;
    memset(&address, 0, sizeof(address));
    address.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &address.sin6_addr);
    address.sin6_port = htons(port_udp);
    if (bind(udp_sock, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind");
    }

    game_info->gs_udp_sock = udp_sock;

    int multicast_sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (multicast_sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Initialisation du multicast
    struct sockaddr_in6 multicast_addr;
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin6_family = AF_INET6;
    multicast_addr.sin6_port = htons(port_udp);
    port_udp++;
    inet_pton(AF_INET6, MUTLICAST_ADDR, &multicast_addr.sin6_addr);

    int ifindex = if_nametoindex("lo0");
    if (ifindex == 0)
        perror("if_nametoindex");
    multicast_addr.sin6_scope_id = ifindex; // ou 0 pour interface par défaut

    game_info->gs_multicast_sock = multicast_sock;
    game_info->gs_multicast_addr = multicast_addr;

    // Initialisation du thread de gestion du serveur de jeu
    pthread_t gs_thread;
    pthread_create(&gs_thread, NULL, (void *)handle_game, game_info);
    pthread_detach(gs_thread);

    return game_info;
}

void pp_game_info(t_game_server_info *game_info)
{
    printf("Game info : \n");
    printf("State : %d\n", game_info->gs_state);
    printf("Nb players : %zu\n", game_info->gs_nb_players);
}

t_game_server_info *find_game(t_game_server_info *games_list[MAX_NB_GAMES])
{
    for (size_t i = 0; i < MAX_NB_GAMES; i++)
    {
        t_game_server_info *game_info = games_list[i];
        if (!(game_info == NULL))
        {
            // Si il y a une game disponible, on la retourne
            if (game_info->gs_nb_players < 4)
            {
                return game_info;
            }
        }
    }
    // sinon on créer une nouvelle game
    t_game_server_info *new_game = init_game();
    for (size_t i = 0; i < MAX_NB_GAMES; i++)
    {
        t_game_server_info *game_info = games_list[i];
        if (game_info == NULL)
        {
            games_list[i] = new_game;
        }
    }
    return new_game;
}

// Gestion des clients
void handle_client(void *client_socket)
{
    // reception du premier message du client
    int client_socket_fd = *(int *)client_socket;
    u_int16_t result;
    u_int16_t code_req;
    if (recv(client_socket_fd, &result, sizeof(result), 0) < 0)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    code_req = ntohs(result) >> 3;
    printf("Received request %d\n", code_req);
    uint16_t mask = 0x1FFF;

    t_game_server_info *game_info = NULL;

    pthread_mutex_lock(&verrou);
    if ((code_req & mask) == (GS_FFA_TYPE))
    {
        printf("Received FFA request\n");
        game_info = find_game(ffa_games);
        code_req = GS_FFA_TYPE;
    }
    else if ((code_req & mask) == (GS_TEAM_TYPE))
    {
        printf("Received TEAM request\n");
        game_info = find_game(team_games);
        code_req = GS_TEAM_TYPE;
    }
    else
    {
        printf("Received unknown request\n");
    }

    if (game_info == NULL)
    {
        pthread_mutex_unlock(&verrou);
        printf("No game available\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Game available\n");
        // Ajouter le joueur a la partie
        game_info->gs_player_list[game_info->gs_nb_players] = client_socket_fd;
        game_info->gs_nb_players++;
        pthread_mutex_unlock(&verrou);
        pp_game_info(game_info);
        // Envoyer le message
        u_int16_t join_message[11];
        // PREMIERE PARTIE DU MESSAGE
        u_int16_t id = (game_info->gs_nb_players - 1) & 0x0003;
        u_int16_t eq = ((game_info->gs_nb_players - 1) / 2) & 0x0002;
        if (code_req == (GS_FFA_TYPE))
        {
            eq = 0;
        }
        u_int16_t req_id_eq = code_req << 3 | id << 1 | eq;
        join_message[0] = htons(req_id_eq);
        join_message[1] = game_info->gs_multicast_addr.sin6_port;
        join_message[2] = game_info->gs_multicast_addr.sin6_port;
        // Pointer to the 16 bytes of the IPv6 address
        unsigned char tmp_s6_addr[16];
        memcpy(&tmp_s6_addr, game_info->gs_multicast_addr.sin6_addr.s6_addr, sizeof(game_info->gs_multicast_addr.sin6_addr.s6_addr));

        // Combine each pair of bytes into a uint16_t and store in segments
        for (int i = 0; i < 8; i++)
        {
            join_message[3 + i] = htons(((tmp_s6_addr[2 * i] & 0xFF) << 8) | ((tmp_s6_addr[2 * i + 1]) & 0xFF));
            // printf("%04x:", ntohs(join_message[3 + i]));
        }

        // printf("\n");

        if (send(client_socket_fd, join_message, sizeof(join_message), 0) < 0)
        {
            perror("send");
            exit(EXIT_FAILURE);
        }

        // Attendre que le joueur soit pret a jouer
        u_int16_t ready;
        if (recv(client_socket_fd, &ready, sizeof(ready), 0) < 0)
        {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        printf("Partie[%d] : Joueur[%d] pret\n", game_info->gs_multicast_addr.sin6_port, id);
        game_info->gs_players_ready++;
    }
}

// Initialisation du socket global pour accueillir les clients
int init_global_socket(const char *port_arg)
{
    int port = atoi(port_arg);
    int global_socket;
    if ((global_socket = socket(AF_INET6, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in6 address;
    memset(&address, 0, sizeof(address));
    address.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &address.sin6_addr);
    address.sin6_port = htons(port);

    if (bind(global_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(global_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(global_socket, 3) < 0)
    {
        perror("listen");
        close(global_socket);
        exit(EXIT_FAILURE);
    }

    return global_socket;
}

// Fonction exécutée par le thread
void *redistrib_tchat(void *game_info)
{
    int *client_sockets = ((t_game_server_info *)game_info)->gs_player_list;
    fd_set readfds; // Ensemble de descripteurs de fichiers pour select
    while (1)
    {
        FD_ZERO(&readfds);
        int maxfd = client_sockets[0];
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            FD_SET(client_sockets[i], &readfds);
            if (client_sockets[i] > maxfd)
            {
                maxfd = client_sockets[i];
            }
        }

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            int client_socket = client_sockets[i];
            if (client_socket != -1 && FD_ISSET(client_socket, &readfds))
            {
                u_int16_t buffer[TEXT_SIZE + 2];
                ssize_t bits_received = recv(client_socket, buffer, sizeof(buffer), 0);
                if (bits_received <= 0)
                {
                    close(client_socket);
                    printf("Client %d disconnected.\n", client_socket);
                    client_sockets[i] = -1;
                }
                else
                {
                    printf("Received message from client %d: %hn\n", client_socket, buffer);
                    for (int j = 0; j < MAX_PLAYERS; j++)
                    {
                        int dest_socket = client_sockets[j];
                        if (dest_socket != -1)
                        {
                            int bits_sent = send(dest_socket, buffer, bits_received, 0);
                            if (bits_sent > -1)
                            {
                                printf("Sent %d bits to client %d\n", bits_sent, dest_socket);
                            }
                        }
                    }
                }
            }
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int global_socket;
    global_socket = init_global_socket(argv[1]);
    while (1)
    {
        struct sockaddr_in6 addr_client;
        memset(&addr_client, 0, sizeof(addr_client));
        socklen_t size = sizeof(addr_client);
        int client_socket = accept(global_socket, (struct sockaddr *)&addr_client, &size);

        printf("Connection accepted, client_socket = %d\n", client_socket);
        if (client_socket < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("Client connected, client_socket = %d\n", client_socket);
        pthread_t thread;
        pthread_create(&thread, NULL, (void *)handle_client, (void *)&client_socket);
        pthread_detach(thread);
    }
    return 0;
}
