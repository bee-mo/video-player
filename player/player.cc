#include "player.h"

void Player::load_file(const std::string& path) {
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
        } else if (local_params->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("\t\tStream Type:\tAudio\n");
            printf("\t\tChannels:\t%d\n", local_params->channels);
            printf("\t\tSample Rate:\t%d\n", local_params->sample_rate);
        } else {
            printf("\t\tUnknown stream type.\n");
        }
    }

    // todo -> decode video stream and serve
    // frames to GUI video player
}

bool file_exists (const char* filepath) {
    struct stat buff;
    return stat(filepath, &buff) == 0;
}