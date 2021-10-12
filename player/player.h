#ifndef _VIDEO_PLAYER_H_
#define _VIDEO_PLAYER_H_

#include <string>
#include <sys/stat.h>
#include <cstdio>

class Player {

public:
    Player () = default;

    /**
     * @def Load the file at the path into the
     * video player.
     * */
    void load_file(const std::string& path);

private:

};

bool file_exists (const char* filepath);

#endif