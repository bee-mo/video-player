# Video Player

## Goal
The goal of this project is to create a functional video player that can load several
video formats and control the video playback through the terminal.

### Resources
- Tutorial on how to use ffmpeg can be found [here](https://github.com/leandromoreira/ffmpeg-libav-tutorial#chapter-1---syncing-audio-and-video).
- Converting frame data from one color space to another:
    - Converting between colorspace is handled through libavswscale.
    - An example of converting from YUV to RGB can be found [here](https://ffmpeg.org/doxygen/2.3/scaling_video_8c-example.html#a12).