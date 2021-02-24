/*
 * @Author: your name
 * @Date: 2021-02-09 19:05:24
 * @LastEditTime: 2021-02-19 17:30:50
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \qt_project\VideoEncodeRtsp\ffmpeg_encode_frame.h
 */
#pragma once
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include "libswscale/swscale.h"
}
#include <functional>
#include <map>

//https://ffmpeg.org/doxygen/trunk/encode_video_8c-example.html
class FFmpegEncodeFrame
{
public:
    struct VideoParams
    {
        std::string codec_name = "libx264";
        /* put sample parameters */
        int bit_rate = 400000;
        /* resolution must be a multiple of two */
        int width = 320;
        int height = 640;
        /* frames per second */
        AVRational time_base{1,25};
        AVRational framerate{25,1};
        /* emit one intra frame every ten frames
        * check frame pict_type before passing frame
        * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I 
        * then gop_size is ignored and the output of encoder
        * will always be I frame irrespective to gop_size
        */
        int gop_size = 10;
        int max_b_frames = 1;
        AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
        AVPixelFormat src_pix_fmt = AV_PIX_FMT_BGR24;
    };
    explicit FFmpegEncodeFrame();

    ~FFmpegEncodeFrame();

    AVCodecContext* Initsize(const VideoParams &params);

    bool Encode(const char *data, int size, int64_t pts, std::function<void(AVPacket*)> call_back);

private:
    void GetFrameSlice(const int width, const int height, uint8_t *data, const AVPixelFormat fmt, uint8_t** &slice_ptr, int * & stride);

    AVPixelFormat src_pix_fmt_;
    const AVCodec *codec_ = nullptr;
    AVCodecContext *ctx_ = nullptr;
    AVFrame *frame_ = nullptr;
    AVPacket *pkt_ = nullptr;
    SwsContext *sws_ctx_ = nullptr;
};