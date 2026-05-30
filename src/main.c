#include "game.h"

int main(void) {
    init_game();

    while (1) {
        move_player();

        update_map();

        print_screen();

        wait();

        // For debug
        static int n = 0;
        n++;
        if (n == 5) {
            break;
        }
    }

    return 0;
}
