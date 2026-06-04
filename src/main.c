#include "game.h"

int main(void) {
    if (!init_game()) {
        quit_game();
        return -1;
    }

    game_main();

    quit_game();

    return 0;
}
