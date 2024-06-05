#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "game_logic.h"

void init_board(board *b, size_t w, size_t h) {
    int x, y;
    b->grid = malloc(sizeof(case_info **) * h);
    for (y = 0; y < h; y++) {
        b->grid[y] = malloc(sizeof(case_info *) * w);
        for (x = 0; x < w; x++) {
            b->grid[y][x] = malloc(sizeof(case_info));
            b->grid[y][x]->x = x;
            b->grid[y][x]->y = y;
        }
    }
    b->width = w;
    b->height = h;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if ((x == 0 && y == 0) || (x == 0 && y == h - 1) || (x == w - 1 && y == 0) || (x == w - 1 && y == h - 1)) {
                if (x == 0 && y == 0) {
                    set_case(b, x, y, PLAYER_1);
                } else if (x == 0 && y == h - 1) {
                    set_case(b, x, y, PLAYER_2);
                } else if (x == w - 1 && y == 0) {
                    set_case(b, x, y, PLAYER_3);
                } else if (x == w - 1 && y == h - 1) {
                    set_case(b, x, y, PLAYER_4);
                }
            } else if (x == 0 || y == 0 || x == w - 1 || y == h - 1) {
                set_case(b, x, y, SOLID_WALL);
            } else {
                set_case(b, x, y, EMPTY);
            }
        }
    }
}

void set_case(board *b, int x, int y, CASE_STATE state)
{
    (b->grid[x][y])->state = state;
}