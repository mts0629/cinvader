#include "game.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FIELD_WIDTH 15
#define FIELD_HEIGHT 10

#define PLAYER 'A'

static char map[FIELD_HEIGHT * FIELD_WIDTH] = {0};

typedef struct {
    int x, y;
} Pos;

static Pos player;

void init_game(void) {
    player.x = FIELD_WIDTH / 2;
    player.y = (FIELD_HEIGHT - 1);
}

void move_player(void) { player.x++; }

void update_map(void) {
    memset(map, ' ', sizeof(map));

    map[player.y * FIELD_WIDTH + player.x] = PLAYER;
}

static inline void clear_screen(void) { printf("\033[2J"); }

static inline void reset_cursor_pos(void) { printf("\033[0;0H"); }

void print_screen(void) {
    clear_screen();
    reset_cursor_pos();

    for (int i = 0; i < FIELD_HEIGHT; i++) {
        for (int j = 0; j < FIELD_WIDTH; j++) {
            printf("%c", map[i * FIELD_WIDTH + j]);
        }
        printf("\n");
    }
}

void wait(void) { sleep(1); }
