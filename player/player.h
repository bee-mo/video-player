#ifndef _VIDEO_PLAYER_H_
#define _VIDEO_PLAYER_H_

#include <string>
#include <sys/stat.h>
#include <cstdio>
#include <mutex>
#include <vector>
#include <chrono>
#include <pthread.h>
#include "../window/window.h"

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
    Player () : in_use_(false), paused_(false) {}
    ~Player();

    /**
     * @def Load the file at the path into the
     * video player.
     * */
    void load_file(const std::string& path);
    
    void pause();
    void resume();

private:

    /**
     * Use fmt_mtx whenever format_ctx_ is
     * being used.
     * */
    std::mutex fmt_mtx_;
    AVFormatContext *format_ctx_ = nullptr;
    /**
     * @def
     * true => player is busy playing a video.
     * false => player not playing a video.
     * */
    bool in_use_;
    bool paused_;
    std::mutex paused_mtx_;

    friend void * play_video_thread (void* params);
};

bool file_exists (const char* filepath);
/**
 * @def
 * Given an image buffer, [buf], given in RGB format,
 * rearrange the bytes such that the image is flipped
 * upside down.
 * */
void flip_img (uint8_t *buff, int width, int height);

#endif