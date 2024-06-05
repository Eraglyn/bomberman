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
#define TEXT_SIZE 255
#define MODE_SIZE 6 // Size to accommodate "\team" and "\all"

void setup_board(board *board)
{
    int lines, columns;
    getmaxyx(stdscr, lines, columns);
    board->height = lines - 2 - 1; // 2 rows reserved for border, 1 row for chat
    board->width = columns - 2;    // 2 columns reserved for border
    board->grid = calloc((board->width) * (board->height), sizeof(char));
}

void free_board(board *board)
{
    free(board->grid);
}

int get_grid(board *b, int x, int y)
{
    return (b->grid[y][x])->state;
}

void set_grid(board *b, int x, int y, int v)
{
    (b->grid[y][x])->state = v;
}

void refresh_game(board *b, line *l) {
    int x, y;
    for (y = 0; y < b->height; y++) {
        for (x = 0; x < b->width; x++) {
            char c;
            switch (get_grid(b, x, y)) {
            case 0:
                c = ' ';
                break;
            case 1:
                c = 'O';
                break;
            default:
                c = '?';
                break;
            }
            mvaddch(y + 1, x + 1, c);
        }
    }
    for (x = 0; x < b->width + 2; x++) {
        mvaddch(0, x, '-');
        mvaddch(b->height + 1, x, '-');
    }
    for (y = 0; y < b->height + 2; y++) {
        mvaddch(y, 0, '|');
        mvaddch(y, b->width + 1, '|');
    }

    attron(COLOR_PAIR(1));
    attron(A_BOLD);
    mvprintw(b->height + 2, 1, "%s ", l->mode); // Print the mode at the beginning with a space
    for (x = 0; x < b->width - l->start_pos; x++) {
        if (x >= TEXT_SIZE || x >= l->cursor)
            mvaddch(b->height + 2, x + l->start_pos + 1, ' ');
        else
            mvaddch(b->height + 2, x + l->start_pos + 1, l->data[x]);
    }
    attroff(A_BOLD);
    attroff(COLOR_PAIR(1));
    refresh();
}

void print_line(line *l)
{
    l->data[l->cursor] = '\0';
    //printw("%s\n", l->data);
    l->cursor = 0;
    memset(l->data, ' ', TEXT_SIZE);
}

void clean_line(line *l)
{
    int i;
    for (i = l->cursor; i < TEXT_SIZE; i++)
    {
        l->data[i] = '\0'; // Set all characters after cursor to null terminator
    }
}

void init_window()
{
    initscr();                               /* Start curses mode */
    raw();                                   /* Disable line buffering */
    intrflush(stdscr, FALSE);                /* No need to flush when intr key is pressed */
    keypad(stdscr, TRUE);                    /* Required in order to get events from keyboard */
    nodelay(stdscr, TRUE);                   /* Make getch non-blocking */
    noecho();                                /* Don't echo() while we do getch (we will manually print characters when relevant) */
    curs_set(0);                             // Set the cursor to invisible
    start_color();                           // Enable colors
    init_pair(1, COLOR_YELLOW, COLOR_BLACK); // Define a new color style (text is yellow, background is black)
}
