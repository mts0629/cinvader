#include "game.h"

int main(void) {
    if (!init_game()) {
        quit_game();
        return -1;
    }

    while (1) {
        if (hit_key()) {
            move_player();
        }

        update_map();

        print_screen();

        wait();
    }

    quit_game();

    return 0;
}
