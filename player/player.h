#ifndef _VIDEO_PLAYER_H_
#define _VIDEO_PLAYER_H_

#include <string>
#include <sys/stat.h>
#include <cstdio>
#include <mutex>
#include <pthread.h>

// #include <libavcodec/codec_id.h>
// #include <libavutil/avutil.h>
#include <ffmpeg_extern.h>
// #include <libavformat/avformat.h>
// #include <libavcodec/avcodec.h>

class Player;

struct VideoInfo {
    Player *player;
    AVCodecContext *codec_ctx;
    int stream_index = -1;
};

void * play_video_thread (void* params);
class Player {

public:
    Player () = default;
    ~Player();

    /**
     * @def Load the file at the path into the
     * video player.
     * */
    void load_file(const std::string& path);

private:

    /**
     * Use fmt_mtx whenever format_ctx_ is
     * being used.
     * */
    std::mutex fmt_mtx_;
    AVFormatContext *format_ctx_ = nullptr;

    friend void * play_video_thread (void* params);
};

bool file_exists (const char* filepath);

#endif