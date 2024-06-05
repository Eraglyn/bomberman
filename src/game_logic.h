#ifndef game_logic_h
#define game_logic_h

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum CASE_STATE
{
    EMPTY,
    SOLID_WALL,
    FRAGILE_WALL,
    BOMB,
    EXPLODED,
    PLAYER_1,
    PLAYER_2,
    PLAYER_3,
    PLAYER_4
} CASE_STATE;

typedef struct case_info
{
    CASE_STATE state;
    int x;
    int y;
} case_info;

typedef struct board
{
    size_t width;
    size_t height;
    case_info ***grid;
} board;

typedef enum ACTION
{
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    QUIT,
    ENTER,
    SWITCH_MODE
} ACTION;

void init_board(board *b, size_t width, size_t height);
void free_board(board *b);
void set_case(board *b, int x, int y, CASE_STATE state);
void print_board(board *b);

#endif
