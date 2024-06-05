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
#include <ncurses.h>
#include <fcntl.h>

#include "game_logic.h"
#include "global_server.h"
#include "game_render.h"

#define ADDR "::1"

typedef struct pos
{
    int x;
    int y;
} pos;

board *b = NULL;
line *l = NULL;
pos *p = NULL;
char **tchat;

int sock_tcp;
int sock_udp;
int sock_udp_mult;

struct sockaddr_in6 adr_mult;

u_int16_t game_type = 0;
u_int16_t id = 0;
u_int16_t eq = 0;

void ask_to_join_game()
{
    u_int16_t req;
    printf("Bienvenue sur le jeu bomberman, avant de rejoindre une partie, veuillez précisez\nquelle type de partie vous souhaitez rejoindre\n");
    char buf[1];
    while (1)
    {
        printf("Team (T) ou FFA (F) :");
        scanf("%s", buf);
        if (buf[0] == 'T' || buf[0] == 'F')
        {
            printf("Vous avez choisi %c\n", buf[0]);
            game_type = buf[0] == 'T' ? GS_TEAM_TYPE : GS_FFA_TYPE;
            req = htons((buf[0] == 'T' ? GS_TEAM_TYPE : GS_FFA_TYPE) << 3);
            send(sock_tcp, &req, sizeof(req), 0);
            break;
        }
    }
    printf("req : %d a été envoyé\n", ntohs(req >> 3));
}

void join_game_multicast()
{
    u_int16_t ans[11];
    recv(sock_tcp, ans, sizeof(ans), 0);
    u_int16_t req_id_eq = ntohs(ans[0]);
    eq = (req_id_eq & 0x0001);
    id = (req_id_eq >> 1) & 0x0002;
    u_int16_t req = req_id_eq >> 3 & 0x1FFF;
    printf("%d données recues : req = %d, eq = %d, id = %d\n", req_id_eq, req, eq, id);
    u_int16_t port = ntohs(ans[1]);
    printf("port = %d\n", port);
    unsigned char tmp_s6_addr[16];
    char ipv6_str[INET6_ADDRSTRLEN]; // Buffer to hold the IPv6 address string

    printf("récuperation de l'adresse IPV6 multicast: ");
    for (int i = 0; i < 8; i++)
    {
        // printf("%02x", (ntohs(ans[3 + i]) >> 8) & 0xFF);
        // printf("%02x:", (ntohs(ans[3 + i])) & 0xFF);
        // printf("\nSECOND : ");
        tmp_s6_addr[i] = ((ntohs(ans[3 + i])) >> 8) & 0xFF;
        tmp_s6_addr[i + 1] = (ntohs(ans[3 + i])) & 0xFF;
        // printf("%02x", tmp_s6_addr[i]);
        // printf("%02x:", tmp_s6_addr[i + 1]);
        sprintf(ipv6_str + (i * 5), "%02x%02x:", tmp_s6_addr[i], tmp_s6_addr[i + 1]);
    }

    ipv6_str[strlen(ipv6_str) - 1] = '\0';
    printf("\n%s\n", ipv6_str);

    // faire la connection multicast

    if ((sock_udp_mult = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
    {
        perror("echec de socket");
        exit(EXIT_FAILURE);
    }

    int ok = 1;
    if (setsockopt(sock_udp_mult, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) < 0)
    {
        perror("echec de SO_REUSEADDR");
        close(sock_udp_mult);
        exit(EXIT_FAILURE);
    }

    ok = 1;
    if (setsockopt(sock_udp_mult, SOL_SOCKET, SO_REUSEPORT, &ok, sizeof(ok)) < 0)
    {
        perror("echec de SO_REUSEPORT");
        close(sock_udp_mult);
        exit(EXIT_FAILURE);
    }

    memset(&adr_mult, 0, sizeof(adr_mult));
    adr_mult.sin6_family = AF_INET6;
    adr_mult.sin6_addr = in6addr_any;
    // inet_pton(AF_INET6, ADDR, &adr_mult.sin6_addr);
    adr_mult.sin6_port = htons(port);

    if (bind(sock_udp_mult, (struct sockaddr *)&adr_mult, sizeof(adr_mult)) < 0)
    {
        perror("echec de bind");
    }

    int ifindex = if_nametoindex("lo0");

    struct ipv6_mreq group;
    memccpy(group.ipv6mr_multiaddr.s6_addr, tmp_s6_addr, 0, sizeof(group.ipv6mr_multiaddr.s6_addr));
    inet_ntop(AF_INET6, tmp_s6_addr, ipv6_str, INET6_ADDRSTRLEN);
    printf("%s\n", ipv6_str);
    // inet_pton(AF_INET6, ipv6_str, &group.ipv6mr_multiaddr.s6_addr);
    inet_pton(AF_INET6, "ff12::1:2:3", &group.ipv6mr_multiaddr.s6_addr);
    group.ipv6mr_interface = ifindex;

    if (setsockopt(sock_udp_mult, IPPROTO_IPV6, IPV6_JOIN_GROUP, &group, sizeof(group)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // printf("Joining multicast group\n");
}

void ask_ready_for_game()
{
    while (1)
    {
        char buf[1];
        printf("Voulez-vous rejoindre la partie (Y) ? :");
        scanf("%s", buf);
        if (buf[0] == 'Y')
        {
            printf("attente des adversaires...\n");
            break;
        }
    }
    u_int16_t req = htons((game_type == (GS_TEAM_TYPE) ? (4 & 0x1FFF) : (3 & 0x1FFF)) << 3);
    send(sock_tcp, &req, sizeof(req), 0);
    socklen_t len_mult = sizeof(adr_mult);
    char buf[60];
    int recu = recvfrom(sock_udp_mult, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&adr_mult, &len_mult);
    printf("recu : %d, mess : %s\n", recu, buf);
}

void recieve_full_board()
{
    u_int16_t full_grid[((BOARD_WIDTH * BOARD_HEIGHT) / 2) + 2];
    socklen_t addrlen = sizeof(adr_mult);

    int n = recvfrom(sock_udp_mult, full_grid, sizeof(full_grid), 0, (struct sockaddr *)&adr_mult, &addrlen);
    if (n < 0)
    {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }
    // DEBOGAGE
    /*
        u_int16_t req_id_eq = ntohs(full_grid[0]);
        u_int16_t gs_num_update = ntohs(full_grid[1]);
        u_int16_t dimensions = ntohs(full_grid[2]);
        u_int8_t height = (dimensions >> 8) & 0xFF;
        u_int8_t width = dimensions & 0xFF;
    */
    for (size_t i = 0; i < BOARD_HEIGHT; i++)
    {
        for (size_t j = 0; j < BOARD_WIDTH / 2; j++)
        {
            u_int16_t cell = ntohs(full_grid[3 + i * (BOARD_WIDTH / 2) + j]);
            u_int8_t part_left = (cell >> 8) & 0xFF;
            u_int8_t part_right = cell & 0xFF;
            set_grid(b, 2 * j, i, part_left);
            set_grid(b, 2 * j + 1, i, part_right);
        }
    }
}

void init_tcp_client(const char *port)
{
    sock_tcp = socket(AF_INET6, SOCK_STREAM, 0);

    struct sockaddr_in6 addr;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(atoi(port));
    inet_pton(AF_INET6, ADDR, &addr.sin6_addr);
    printf("Connecting to %s:%s\n", ADDR, port);
    if (connect(sock_tcp, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("FAILED Connecting to %s:%s\n", ADDR, port);
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to %s:%s\n", ADDR, port);
}

ACTION control(line *l)
{
    int c;
    int prev_c = ERR;
    while ((c = getch()) != ERR)
    {
        if (prev_c != ERR && prev_c != c)
        {
            ungetch(c);
            break;
        }
        prev_c = c;
    }
    ACTION a = NONE;
    switch (prev_c)
    {
    case ERR:
        break;
    case KEY_LEFT:
        a = LEFT;
        break;
    case KEY_RIGHT:
        a = RIGHT;
        break;
    case KEY_UP:
        a = UP;
        break;
    case KEY_DOWN:
        a = DOWN;
        break;
    case '\t':
        a = SWITCH_MODE;
        break;
    case '~':
        a = QUIT;
        break;
    case KEY_BACKSPACE:
    case 127:
        if (l->cursor > 0)
        {
            l->cursor--;
            l->data[l->cursor] = ' ';
        }
        break;
    case '\n':
    case '\r':
        a = ENTER;
        break;
    default:
        if (prev_c >= ' ' && prev_c <= '~' && l->cursor < TEXT_SIZE)
        {
            l->data[l->cursor] = prev_c;
            l->cursor++;
        }
        break;
    }
    return a;
}

int send_to_server(int sock, const char *message, const char *mode)
{
    u_int16_t msg[2 + TEXT_SIZE / 4];
    int codereq;

    if (strcmp(mode, "\\all") == 0)
    {
        codereq = 7;
    }
    else
    {
        codereq = 8;
    }

    // First byte: code request
    msg[0] = (codereq << 3);

    // Second byte: id and eq (you can adjust this if you need more complex packing)
    msg[0] = (id << 1) | (eq); // Assuming eq is 1 bit
    msg[0] = htons(msg[0]);

    // Copy the message into the buffer, ensuring it fits
    size_t message_len = strlen(message);
    if (message_len > TEXT_SIZE)
    {
        message_len = TEXT_SIZE; // Truncate message if too long
    }

    msg[1] = htons(message_len << 8);
    msg[1] |= message[0];

    int count = 1;
    while (message[count] == '\0')
    {
        msg[count] = (message[count] << 8);
        count += 1;
        msg[count / 2] = message[count];
        count += 1;
    }

    // Send the message
    ssize_t bytes_sent = send(sock, msg, sizeof(msg), 0);
    if (bytes_sent < 0)
    {
        perror("send");
        return -1;
    }

    return 0;
}

bool perform_action(board *b, pos *p, ACTION a, line *l, int sock)
{
    switch (a)
    {
    case LEFT:
        break;
    case RIGHT:
        break;
    case UP:
        break;
    case DOWN:
        break;
    case ENTER:
        clean_line(l);                          // Clean the line before sending
        send_to_server(sock, l->data, l->mode); // Send message to server with mode
        print_line(l);
        break;
    case SWITCH_MODE:
        if (strcmp(l->mode, "\\all") == 0)
        {
            strcpy(l->mode, "\\team");
        }
        else
        {
            strcpy(l->mode, "\\all");
        }
        l->start_pos = strlen(l->mode) + 1; // Adjust start position
        break;
    case QUIT:
        return true;
    default:
        break;
    }

    return false;
}
void read_tchat()
{
    for (int i = 0; i < 10; i++)
    {
        if (tchat[i][0] != '0')
        {
            printf("%s\n", tchat[i]);
            memset(tchat[i], '0', TEXT_SIZE);
        }
    }
}

void recv_from_tchat(void *fd)
{
    int sock = *(int *)fd;
    uint16_t msg[TEXT_SIZE + 2];
    ssize_t bits_received = recv(sock, msg, TEXT_SIZE + 2, 0);
    if (bits_received < 0)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    u_int8_t len = ntohs((msg[1] & 0xFF00) >> 8);
    char buff[len];
    memset(buff, 0, len);
    buff[0] = msg[1] & 0x00FF;
    int b = 1;

    for (int i = 2; i < (len - 1) / 2; i++)
    {
        buff[b] = (msg[i] & 0xFF00) >> 8;
        b++;
        buff[b] = msg[i] & 0x00FF;
    }
}

void init_tchat()
{
    tchat = malloc(sizeof(char *) * 10);
    for (int i = 0; i < 10; i++)
    {
        tchat[i] = malloc(sizeof(char) * TEXT_SIZE);
        memset(tchat[i], '0', TEXT_SIZE);
    }
}

void play()
{
    // game loop
    while (1)
    {
        ACTION a = control(l);
        if (perform_action(b, p, a, l, sock_tcp) == 1)
        {
            break;
        }
        refresh_game(b, l);
        read_tchat();
    }
}

void init_board_line_pos()
{
    b = malloc(sizeof(board));
    init_board(b, BOARD_WIDTH, BOARD_HEIGHT);

    l = malloc(sizeof(line));
    l->cursor = 0;
    l->start_pos = strlen("\\all") + 1;
    memset(l->data, ' ', TEXT_SIZE);
    strcpy(l->mode, "\\all");

    p = malloc(sizeof(pos));
    p->x = 0;
    p->y = 0;
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    init_tchat();
    init_tcp_client(argv[1]);
    ask_to_join_game();
    join_game_multicast();
    ask_ready_for_game();
    init_window();
    init_board_line_pos();

    pthread_t tchat_thread;
    pthread_create(&tchat_thread, NULL, (void *)recieve_full_board, NULL);
    pthread_detach(tchat_thread);

    pthread_t tchat_read_thread;
    pthread_create(&tchat_read_thread, NULL, (void *)recv_from_tchat, (void *)&sock_tcp);
    pthread_detach(tchat_read_thread);

    play();

    free_board(b);
    curs_set(1); // Set the cursor to visible again
    endwin();    // End curses mode

    free(p);
    free(l);
    free(b);
    close(sock_tcp);
    close(sock_udp);
    close(sock_udp_mult);

    return 0;
}
