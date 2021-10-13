#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <signal.h>
#include <string>
#include <iostream>
#include "player/player.h"
#include <vector>

/**
 * Reference:
 * https://github.com/leandromoreira/ffmpeg-libav-tutorial#video---what-you-see
 * */

void handle_signal(int signal);
void help_prompt();
std::vector<std::string> tokenize(const std::string& line);

int main (int argc, char **argv) {

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    printf("Video Player - Beta\n");
    printf(
        "\tUse 'help' to learn how to use this "
        "video player.\n\n"    
    );

    std::string cmd;
    Player player;

    while (true) {
        std::getline(std::cin, cmd);
        
        auto tokens = tokenize(cmd);
        if (tokens.size() == 0) {
            printf("No command recieved.\n");
            continue;
        }

        if (tokens[0].compare("help") == 0) {
            help_prompt ();
        } else if (tokens[0].compare("load") == 0) {
            if (tokens.size() < 2) {
                fprintf(stderr, 
                    "Error: Invalid number of arguments provided.\n"
                    "\tUsage: open <path_to_file>\n"
                );
            } else {
                player.load_file(tokens[1]);
            }
        } else if (tokens[0].compare("exit") == 0) {
            printf("Terminating Program.\n");
            break;
        } else {
            printf("Unknown command [%s]\n", tokens[0].c_str());
        }

        printf("\n");
    }

    return EXIT_SUCCESS;
}

void handle_signal(int signal) {
    
    if (signal == SIGINT || signal == SIGTERM) {
        printf("Terminating program.\n");
    }

    exit(EXIT_SUCCESS);
}

void help_prompt() {
    printf("-- [Help] --\n");
    printf("\tload <path_to_file>\tLoad a file into the video player.\n");

    printf("\texit\t\tExit the program.\n");
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;

    int start {0}, end{0};

    while (start < line.size() && end <= line.size()) {

        if (end >= line.size() || line[end] == ' ') {
            if (end-start > 1)
                tokens.push_back(line.substr(start, end-start));
            start = end+1;
            end = start;
        }
        else {
            ++end;
        }

    }

    return tokens;
}