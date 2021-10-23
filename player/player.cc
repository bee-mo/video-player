#include "player.h"

Player::~Player () {}

void Player::pause () {
    if (!in_use_) {
        printf("No video to pause.\n");
        return;
    }
    std::lock_guard<std::mutex> lock(paused_mtx_);
    if (!paused_) {
        paused_ = true;
    }
}

void Player::resume () {
    if (!in_use_) {
        printf("No video to resume.\n");
        return;
    }
    std::lock_guard<std::mutex> lock(paused_mtx_);
    if (paused_) {
        paused_ = false;
    }
}

void Player::load_file(const std::string& path) {
    if (in_use_) {
        printf("Player is in use. Type \"end\" to stop the player.\n");
        return;
    }

    const std::lock_guard<std::mutex> lock(fmt_mtx_);
    printf("Loading file:\t%s\n", path.c_str());

    if (!file_exists(path.c_str())) {
        fprintf(stderr, "Error: File does not exist.\n");
        return;
    }

    printf("Initializing format context.\n");
    format_ctx_ = avformat_alloc_context();
    if (format_ctx_ == nullptr) {
        fprintf(stderr, "Error: Failed to initialize format context.\n");
        return;
    }

    // open the file to read its headers
    int res = avformat_open_input(
        &format_ctx_, 
        path.c_str(),
        nullptr, nullptr
    );

    if (res != 0) {
        fprintf(stderr, "Error: Failed to open input.\n");
        return;
    }

    // print information about the file
    printf(
        "\nFile Information:\n"
        "\tformat:\t\t%s(%s)\n"
        "\tduration:\t%ld\n"
        "\tbit rate:\t%ld\n"
        "\tnb streams:\t%u\n\n",
        format_ctx_->iformat->name,
        format_ctx_->iformat->long_name,
        format_ctx_->duration,
        format_ctx_->bit_rate,
        format_ctx_->nb_streams
    );

    // Find the stream information
    printf("Finding stream info...\n\n");
    res = avformat_find_stream_info(format_ctx_, nullptr);
    if (res < 0) {
        fprintf(stderr, "Error: Failed to find stream info.\n");
        return;
    }

    AVCodec *codec {nullptr};
    AVCodecParameters *codec_params {nullptr};

    bool video_initialized {false};
    int stream_index {-1};
    AVCodecContext *codec_ctx {nullptr};

    printf("Stream Info:\n");
    for (int i = 0; i < format_ctx_->nb_streams; ++i) {
        printf("\n\t[Stream #%d]\n", i+1);

        printf("\tBefore Opening Codec:\n");
        AVCodecParameters *local_params {nullptr};
        local_params = format_ctx_->streams[i]->codecpar;

        printf("\t\tTime Base:\t%d/%d\n", 
            format_ctx_->streams[i]->time_base.num,
            format_ctx_->streams[i]->time_base.den
        );
        printf("\t\tFrame Rate:\t%d/%d\n",
            format_ctx_->streams[i]->r_frame_rate.num,
            format_ctx_->streams[i]->r_frame_rate.den
        );
        printf("\t\tStart Time:\t%" PRId64 "\n", 
            format_ctx_->streams[i]->start_time
        );
        printf("\t\tDuration:\t%" PRId64 "\n", 
            format_ctx_->streams[i]->duration
        );

        const AVCodec *p_codec = avcodec_find_decoder(local_params->codec_id);
        if (p_codec == nullptr) {
            fprintf(stderr, "Error: Could not find local coded (codec id=%d)\n",
                local_params->codec_id
            );
            continue;
        }

        if (local_params->codec_type == AVMEDIA_TYPE_VIDEO) {
            printf("\t\tStream Type:\tVideo\n");
            printf("\t\tWidth:\t%d\n", local_params->width);
            printf("\t\tHeight:\t%d\n", local_params->height);

            if (!video_initialized) {
                codec_ctx = avcodec_alloc_context3(p_codec);
                if (codec_ctx == nullptr) {
                    fprintf(stderr, "Error: Codec ctx initialization failed "
                        "for this video stream codec.. Trying next stream.\n"
                        "(stream id=%d)\n", i);
                    continue;
                }

                if (avcodec_parameters_to_context(codec_ctx, local_params) < 0) {
                    fprintf(stderr, "Error: Failed to fill codec context from parameters.\n");
                    fprintf(stderr, "Trying next video stream.\n(stream id =%d)\n", i);
                    continue;
                }

                // tell the codec context to use the codec
                // for this video stream
                if (avcodec_open2(codec_ctx, p_codec, nullptr) < 0) {
                    fprintf(stderr, "Error: Failed to open codec with avcodec_open2\n");
                    fprintf(stderr, "Trying next video stream.\n(stream id=%d)\n", i);
                    continue;
                }

                video_initialized = true;
                stream_index = i;
            }

        } else if (local_params->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("\t\tStream Type:\tAudio\n");
            printf("\t\tChannels:\t%d\n", local_params->channels);
            printf("\t\tSample Rate:\t%d\n", local_params->sample_rate);
        } else {
            printf("\t\tUnknown stream type.\n");
        }
    }

    if (!video_initialized) {
        fprintf(stderr, "Error: No valid video stream found to play.\n");
        if (codec_ctx != nullptr) avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx_);
    } else {

        in_use_ = true;

        auto *vid_params = new VideoInfo;
        vid_params->player = this;
        vid_params->stream_index = stream_index;
        vid_params->codec_ctx = codec_ctx;

        pthread_t tid;
        pthread_create(
            &tid,
            nullptr,
            play_video_thread,
            (void *) vid_params
        );
    }
}

void * play_video_thread (void* params) {
    // TODO play the video information
    struct VideoInfo *vid_params = (VideoInfo*) params;
    if (vid_params->player != nullptr
        && vid_params->stream_index >= 0
        && vid_params->stream_index < vid_params->player->format_ctx_->nb_streams) {
        const std::lock_guard<std::mutex> fmt_lock(vid_params->player->fmt_mtx_);
        AVPacket *packet = av_packet_alloc();
        AVFrame *frame = av_frame_alloc();

        // create swscale context for converting from yuv to rgb
        const uint WIDTH {640};
        const uint HEIGHT {360};
        struct SwsContext *sws_ctx = sws_getContext(
            WIDTH, HEIGHT, AV_PIX_FMT_YUV420P,
            WIDTH, HEIGHT, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        uint8_t *dst_img_buff[4];
        int dst_linesize[4];
        auto dst_buffsize = av_image_alloc(dst_img_buff, dst_linesize,
            WIDTH, HEIGHT, AV_PIX_FMT_RGB24, 1);

        // Initialize window here!
        // Fuck sfml. It's asking me for too much >_>

        printf("\nBeginning Frame Extraction.\n");

        if (packet == nullptr || frame == nullptr) {
           fprintf(stderr, "Error: Failed to initialize packet or frame.\n");
        } else if (sws_ctx == nullptr) {
            fprintf(stderr, "Error: Failed to initialize swscale.\n");
        } else if (dst_buffsize < 0) {
            fprintf(stderr, "Error: Failed to allocate av iamge buffer.\n");
        } else {
            window win;
            win.init(WIDTH, HEIGHT);
            auto frame_rate_rat = 
                vid_params->player->format_ctx_->streams[vid_params->stream_index]->r_frame_rate;
            auto frame_rate = av_q2d(frame_rate_rat);
            if (frame_rate == 0) frame_rate = (double) 1.0f;
            auto last_frame_time = std::chrono::system_clock::now();

            while (av_read_frame(
                vid_params->player->format_ctx_,
                packet
            ) >= 0) {

                if (packet->stream_index == vid_params->stream_index) {

                    // decode the packet into a frame
                    int res = avcodec_send_packet(vid_params->codec_ctx, packet);
                    if (res < 0) {
                        fprintf(stderr, "Error while sending packet to decoder.\n");
                    } else {

                        while (res >= 0) {
                            res = avcodec_receive_frame(vid_params->codec_ctx, frame);

                            if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
                                break;
                            } else if (res >= 0) {
                                /* printf("Frame %d (size=%d bytes, format=%d) "
                                    "pts %ld key_frame %d dts %d\n",
                                    vid_params->codec_ctx->frame_number,
                                    // av_get_picture_type_char(frame->pict_type),
                                    frame->pkt_size,
                                    frame->format,
                                    frame->pts,
                                    frame->key_frame,
                                    frame->coded_picture_number); */
                                if (frame->format == AV_PIX_FMT_YUV420P) {
                                    // convert the frame into rgb
                                    
                                    sws_scale(sws_ctx, frame->data, frame->linesize,
                                        0, frame->height, dst_img_buff, dst_linesize);

                                    while (true) {
                                        std::lock_guard<std::mutex> lock(vid_params->player->paused_mtx_);
                                        if (!vid_params->player->paused_) 
                                            break;
                                    }

                                    flip_img(dst_img_buff[0], WIDTH, HEIGHT);
                                    std::chrono::duration<float> diff;
                                    do {
                                        diff = std::chrono::system_clock::now() - last_frame_time;
                                    } while (diff.count() < 1.0f/frame_rate);

                                    win.draw_image((const uint8_t *)dst_img_buff[0], 
                                        WIDTH, HEIGHT);
                                    last_frame_time = std::chrono::system_clock::now();
                                }
                            }
                        }
                        
                    }
                }

                av_packet_unref(packet);
            }

            av_freep(&dst_img_buff[0]);
            sws_freeContext(sws_ctx);
        }
    }

    printf("Exiting load video task.\n");
    delete vid_params;
    return (void*) nullptr;
}

bool file_exists (const char* filepath) {
    struct stat buff;
    return stat(filepath, &buff) == 0;
}

void flip_img (uint8_t *buff, int width, int height) {

    int line_a {0}, line_b {height - 1};

    while (line_a < line_b) {

        for (int i = 0; i < width * 3; ++i) {
            auto temp = buff[(line_a * width * 3) + i];
            buff[(line_a * width * 3) + i] = buff[(line_b * width * 3) + i];
            buff[(line_b * width * 3) + i] = temp;
        }
        ++line_a; --line_b;
    }

}