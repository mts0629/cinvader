#define _POSIX_C_SOURCE 199309L  // For nanosleep()

#include "game.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
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

static inline void hide_cursor(void) { printf("\033[?25l"); }

static inline void display_cursor(void) { printf("\033[?25h"); }

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
    tcgetattr(1, &org_tty);

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
        return false;
    }

    hide_cursor();

    signal(SIGINT, on_signal);
    signal(SIGQUIT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGHUP, on_signal);

    return true;
}

static Pos player;

static bool continue_game;

bool init_game(void) {
    player.x = FIELD_WIDTH / 2;
    player.y = (FIELD_HEIGHT - 1);

    continue_game = true;

    return init_term();
}

static bool hit_key(void) {
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

static int get_char(void) {
    uint8_t c;
    int n;
    while ((n = read(0, &c, 1)) < 0) {
        if (errno != EINTR) {
            return -1;
        }
    }

    if (n == 0) {
        return -1;
    }

    return (int)c;
}

static void process_command(const int cmd) {
    switch (cmd) {
        case 'q':
            continue_game = false;
            break;
        default:
            break;
    }
}

static void move_player(const int cmd) {
    switch (cmd) {
        case 'a':
            player.x--;
            if (player.x < 0) {
                player.x = 0;
            }
            break;
        case 'd':
            player.x++;
            if (player.x > (FIELD_WIDTH - 1)) {
                player.x = FIELD_WIDTH - 1;
            }
            break;
        default:
            break;
    }
}

static void update_map(void) {
    memset(map, ' ', sizeof(map));

    map[player.y * FIELD_WIDTH + player.x] = PLAYER;
}

static void print_screen(void) {
    clear_screen();
    reset_cursor_pos();

    for (int i = 0; i < FIELD_HEIGHT; i++) {
        for (int j = 0; j < FIELD_WIDTH; j++) {
            printf("%c", map[i * FIELD_WIDTH + j]);
        }
        printf("\r\n");
    }
}

static void wait(void) {
    static struct timespec tv = {0, 100000000};  // 0.1 sec
    nanosleep(&tv, NULL);
}

void game_main(void) {
    while (continue_game) {
        if (hit_key()) {
            int c = get_char();

            process_command(c);

            move_player(c);
        }

        update_map();

        print_screen();

        wait();
    }
}

void quit_game(void) {
    // Restore original terminal settings
    tcsetattr(1, TCSADRAIN, &org_tty);
    clear_screen();
    reset_cursor_pos();
    display_cursor();
}
