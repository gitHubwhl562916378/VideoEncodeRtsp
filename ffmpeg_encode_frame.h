/*
 * @Author: your name
 * @Date: 2021-02-09 19:05:24
 * @LastEditTime: 2021-02-18 21:53:11
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
        int bit_rate = 160000;
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
    };
    explicit FFmpegEncodeFrame();

    ~FFmpegEncodeFrame();

    AVCodecContext* Initsize(const VideoParams &params);

    bool Initsize(AVCodecContext* pcodec_ctx, const VideoParams &params);

    bool Encode(const char *data, int size, int type, int64_t pts, std::function<void(AVPacket*)> call_back);

private:
    using Mem_Option_Func = std::function<void(const char*, AVFrame*)>;
    void CopyYuvFrameData(const char *src, AVFrame *frame);

    const AVCodec *codec_ = nullptr;
    AVCodecContext *ctx_ = nullptr;
    AVFrame *frame_ = nullptr;
    AVPacket *pkt_ = nullptr;
    std::map<AVPixelFormat, Mem_Option_Func> mem_cp_func_map_;
    Mem_Option_Func mem_cp_func_;
};