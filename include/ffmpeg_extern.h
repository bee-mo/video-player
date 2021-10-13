extern "C" {

    struct AVFormatContext;
    struct AVDictionary;
    struct AVInputFormat;
    struct AVCodec;
    struct AVCodecParameters;

    AVFormatContext *avformat_alloc_context(void);
    
    int avformat_open_input(AVFormatContext **ps, const char *filename,
                        const AVInputFormat *fmt, AVDictionary **options);

    int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options);

    const AVCodec *avcodec_find_decoder(enum AVCodecID id);
}