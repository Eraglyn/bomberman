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

#define TEXT_SIZE 255
#define MODE_SIZE 6 // Size to accommodate "\team" and "\all"
typedef struct line
{
    char data[TEXT_SIZE];
    int cursor;
    int start_pos; // Start position of user input
    char mode[MODE_SIZE];
} line;

#define ADDR "::1"
#define TEXT_SIZE 255
#define MODE_SIZE 6 // Size to accommodate "\team" and "\all"

void setup_board(board *board);

void free_board(board *board);

int get_grid(board *b, int x, int y);

void set_grid(board *b, int x, int y, int v);

void refresh_game(board *b, line *l);

void print_line(line *l);

void clean_line(line *l);

void init_window();