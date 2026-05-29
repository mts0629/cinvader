#include "game.h"

#include <stdio.h>

#define FIELD_WIDTH 15
#define FIELD_HEIGHT 10

static int map[FIELD_HEIGHT * FIELD_WIDTH] = {0};

void print_screen(void) {
    map[(FIELD_HEIGHT - 1) * FIELD_WIDTH + FIELD_WIDTH / 2] = 1;

    for (int i = 0; i < FIELD_HEIGHT; i++) {
        for (int j = 0; j < FIELD_WIDTH; j++) {
            if (map[i * FIELD_WIDTH + j] == 0) {
                printf(" ");
            } else if (map[i * FIELD_WIDTH + j] == 1) {
                printf("A");
            }
        }
        printf("\n");
    }

    printf("\r\33[%dA", FIELD_HEIGHT);
}
