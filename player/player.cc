#include "player.h"

void Player::load_file(const std::string& path) {
    printf("Loading file:\t%s\n", path.c_str());

    if (!file_exists(path.c_str())) {
        fprintf(stderr, "Error: File does not exist.\n");
        return;
    }

    // todo handle file load
    printf("TODO load file\n");
}

bool file_exists (const char* filepath) {
    struct stat buff;
    return stat(filepath, &buff) == 0;
}