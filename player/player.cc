#include "player.h"

Player::~Player () {
}

void Player::load_file(const std::string& path) {
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
    if (vid_params->player != nullptr) {
        const std::lock_guard<std::mutex> fmt_lock(vid_params->player->fmt_mtx_);
        AVPacket *packet = av_packet_alloc();
        AVFrame *frame = av_frame_alloc();

        bool first_frame_saved {false};
        if (packet == nullptr || frame == nullptr) {
               fprintf(stderr, "Error: Failed to initialize packet or frame.\n");
        } else {
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

                                // For now, just write the first frame into
                                // a pgm file.
                                if (!first_frame_saved) {
                                    first_frame_saved = true;
                                    FILE *f = fopen("first_frame.pgm", "w");

                                    // write the header of the file
                                    fprintf(f, "P5\n%d %d\n%d\n",
                                        frame->width, frame->height, 255);
                                    for (int q = 0; q < frame->height; ++q) {
                                        fwrite(frame->data[0] + q * frame->linesize[0],
                                            1, frame->width, f);
                                    }

                                    fclose(f);

                                    printf("Success: First frame saved!\n");
                                }
                            }
                        }
                        
                    }
                }

                av_packet_unref(packet);
            }
        }
    }

    delete vid_params;
    return (void*) nullptr;
}

bool file_exists (const char* filepath) {
    struct stat buff;
    return stat(filepath, &buff) == 0;
}