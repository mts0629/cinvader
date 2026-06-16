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
#define ENEMY_BLOCK_HEIGHT 3
#define ENEMY_BLOCK_WIDTH 7
#define ENEMY_BLOCK_ORG_Y 1
#define ENEMY_BLOCK_ORG_X 4
#define ENEMY_MAX (ENEMY_BLOCK_HEIGHT * ENEMY_BLOCK_WIDTH)
#define INTERVAL 2
#define SHELL_MAX 3

#define PLAYER 'A'
#define ENEMY 'W'
#define SHELL 'i'

typedef struct {
    int x, y;
} Vec;

typedef struct {
    Vec pos;
    Vec v;
    bool exist;
} Object;

struct termios org_tty, new_tty;

static inline void clear_screen(void) { printf("\033[2J"); }

static inline void reset_cursor_pos(void) { printf("\033[0;0H"); }

static inline void set_cursor_pos(const int x, const int y) {
    printf("\033[%d;%dH", y, x);
}

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

static Object player;
static Object enemy[ENEMY_MAX];
static Object shell[SHELL_MAX];

static bool continue_game;

bool init_game(void) {
    player.pos.x = FIELD_WIDTH / 2;
    player.pos.y = (FIELD_HEIGHT - 1);
    player.v.x = 0;
    player.v.y = 0;
    player.exist = true;

    int idx = 0;
    for (int i = ENEMY_BLOCK_ORG_Y;
         i < (ENEMY_BLOCK_ORG_Y + ENEMY_BLOCK_HEIGHT); i++) {
        for (int j = ENEMY_BLOCK_ORG_X;
             j < (ENEMY_BLOCK_ORG_X + ENEMY_BLOCK_WIDTH); j++) {
            enemy[idx].pos.x = j;
            enemy[idx].pos.y = i;
            enemy[idx].v.x = 1;
            enemy[idx].v.y = 0;
            enemy[idx].exist = true;
            idx++;
        }
    }

    for (int i = 0; i < SHELL_MAX; i++) {
        shell[i].exist = false;
    }

    srand(time(NULL));

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

static void action(const int cmd) {
    player.v.x = 0;
    switch (cmd) {
        case 'j':
            for (int i = 0; i < SHELL_MAX; i++) {
                if (!shell[i].exist) {
                    shell[i].pos.x = player.pos.x;
                    shell[i].pos.y = player.pos.y - 1;
                    shell[i].v.x = 0;
                    shell[i].v.y = -1;
                    shell[i].exist = true;
                    break;
                }
            }
            break;
        case 'a':
            player.v.x = -1;
            break;
        case 'd':
            player.v.x = 1;
            break;
        default:
            break;
    }
}

static void move_player(void) {
    player.pos.x += player.v.x;

    if (player.pos.x < 0) {
        player.pos.x = 0;
    }
    if (player.pos.x > (FIELD_WIDTH - 1)) {
        player.pos.x = FIELD_WIDTH - 1;
    }
}

static void move_shells(void) {
    for (int i = 0; i < SHELL_MAX; i++) {
        if (shell[i].exist) {
            shell[i].pos.y += shell[i].v.y;

            if (shell[i].pos.y < 0) {
                shell[i].exist = false;
            }
        }
    }
}

static void move_enemies(void) {
    static int interval = INTERVAL;
    if (interval) {
        interval--;
        return;
    }

    static int move_dir = 0;

    bool switch_dir = false;
    for (int i = 0; i < ENEMY_MAX; i++) {
        if (enemy[i].exist) {
            // Switch moving direction if one of enemies reach to the edge
            if (move_dir == 0) {
                enemy[i].v.x = 1;
                enemy[i].v.y = 0;
            } else if (move_dir == 1) {
                enemy[i].v.x = 0;
                enemy[i].v.y = 1;
            } else if (move_dir == 2) {
                enemy[i].v.x = -1;
                enemy[i].v.y = 0;
            } else {
                enemy[i].v.x = 0;
                enemy[i].v.y = 1;
            }

            enemy[i].pos.x += enemy[i].v.x;
            enemy[i].pos.y += enemy[i].v.y;

            if ((enemy[i].pos.x == 1) ||
                (enemy[i].pos.x == (FIELD_WIDTH - 1))) {
                switch_dir = true;
            }
        }
    }

    // Rotate direction:
    // right -> down -> left -> down -> ...
    if (switch_dir) {
        move_dir = (move_dir + 1) % 4;
    }

    interval = INTERVAL;
}

static void check_collision(void) {
    for (int i = 0; i < SHELL_MAX; i++) {
        if (!shell[i].exist) {
            continue;
        }

        for (int j = 0; j < ENEMY_MAX; j++) {
            if (!enemy[j].exist) {
                continue;
            }

            if ((enemy[j].pos.x == shell[i].pos.x) &&
                (enemy[j].pos.y == shell[i].pos.y)) {
                enemy[j].exist = false;
                shell[i].exist = false;
            }
        }
    }
}

static void print_screen(void) {
    clear_screen();

    for (int i = 0; i < ENEMY_MAX; i++) {
        if (enemy[i].exist) {
            set_cursor_pos(enemy[i].pos.x, enemy[i].pos.y);
            printf("%c", ENEMY);
        }
    }

    for (int i = 0; i < SHELL_MAX; i++) {
        if (shell[i].exist) {
            set_cursor_pos(shell[i].pos.x, shell[i].pos.y);
            printf("%c", SHELL);
        }
    }

    set_cursor_pos(player.pos.x, player.pos.y);
    printf("%c", PLAYER);

    fflush(stdout);
}

static void wait_ms(const uint32_t n) {
    struct timespec tv = {0, 0};
    tv.tv_sec = n / 1000;
    tv.tv_nsec = (n % 1000) * 1000000;
    nanosleep(&tv, NULL);
}

static void check_gameover(void) {
    bool no_enemies = true;

    for (int i = 0; i < ENEMY_MAX; i++) {
        if (enemy[i].exist) {
            no_enemies = false;
            if (enemy[i].pos.y == (FIELD_HEIGHT - 1)) {
                set_cursor_pos((FIELD_WIDTH / 2) - 4, (FIELD_HEIGHT / 2));
                printf("GAMEOVER");
                fflush(stdout);
                wait_ms(1000);
                continue_game = false;
            }
        }
    }

    if (no_enemies) {
        set_cursor_pos((FIELD_WIDTH / 2) - 3, (FIELD_HEIGHT / 2));
        printf("CLEAR!");
        fflush(stdout);
        wait_ms(1000);
        continue_game = false;
    }
}

void game_main(void) {
    while (continue_game) {
        int cmd = 0;

        if (hit_key()) {
            cmd = get_char();
            process_command(cmd);
        }

        action(cmd);

        print_screen();

        check_gameover();

        move_player();

        move_shells();

        move_enemies();

        check_collision();

        wait_ms(100);
    }
}

void quit_game(void) {
    // Restore original terminal settings
    tcsetattr(1, TCSADRAIN, &org_tty);
    clear_screen();
    reset_cursor_pos();
    display_cursor();
}
