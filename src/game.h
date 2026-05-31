#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

bool init_game(void);

void quit_game(void);

bool hit_key(void);

void move_player(void);

void update_map(void);

void print_screen(void);

void wait(void);

#endif  // GAME_H
