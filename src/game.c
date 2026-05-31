#include "game.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#define FIELD_WIDTH 15
#define FIELD_HEIGHT 10

#define PLAYER 'A'

static char map[FIELD_HEIGHT * FIELD_WIDTH] = {0};

typedef struct {
    int x, y;
} Pos;

struct termios org_tty, new_tty;

static inline void clear_screen(void) { printf("\033[2J"); }

static inline void reset_cursor_pos(void) { printf("\033[0;0H"); }

static void on_signal(const int sig) {
    signal(sig, SIG_IGN);  // Ignore signal temporally

    switch (sig) {
        // Quit the program
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
        case SIGHUP:
            quit_game();
            exit(0);
            break;
        default:
            break;
    }
}

static bool init_term(void) {
    // Get current STDOUT settings
    if (tcgetattr(1, &org_tty) < 0) {
        return false;
    }

    new_tty = org_tty;

    // Ignore some input processings
    // clang-format off
    new_tty.c_iflag &= ~(
        INLCR | ICRNL | // CR ('\r')- NL ('\n') conversion
        IXON | IXOFF |  // Flow control
        ISTRIP // Strip 8-th bit
    );
    // clang-format on

    // Ignore postprocessing
    new_tty.c_oflag &= ~(OPOST);
    // Ignore the canonical mode, character printing
    new_tty.c_lflag &= ~(ICANON | ECHO);
    // Wait reading 1 byte in minimum
    new_tty.c_cc[VMIN] = 1;
    // No timeout
    new_tty.c_cc[VTIME] = 0;

    if (tcsetattr(1, TCSADRAIN, &new_tty)) {
        tcsetattr(1, TCSADRAIN, &org_tty);
        return false;
    }

    signal(SIGINT, on_signal);
    signal(SIGQUIT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGHUP, on_signal);

    return true;
}

static Pos player;

bool init_game(void) {
    player.x = FIELD_WIDTH / 2;
    player.y = (FIELD_HEIGHT - 1);

    return init_term();
}

void quit_game(void) {
    // Restore original terminal settings
    tcsetattr(1, TCSADRAIN, &org_tty);
    clear_screen();
    reset_cursor_pos();
}

bool hit_key(void) {
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(0, &rfd);  // STDIN

    struct timeval timeout = {0, 0};  // No waiting
    // Check read readiness of STDIN
    if (select(1, &rfd, NULL, NULL, &timeout) == 1) {
        return true;
    }

    return false;
}

void move_player(void) { player.x++; }

void update_map(void) {
    memset(map, ' ', sizeof(map));

    map[player.y * FIELD_WIDTH + player.x] = PLAYER;
}

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
