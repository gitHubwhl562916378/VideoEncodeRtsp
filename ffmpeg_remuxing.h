/*
 * @Author: your name
 * @Date: 2021-02-09 19:48:44
 * @LastEditTime: 2021-02-18 22:13:32
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \qt_project\VideoEncodeRtsp\ffmpeg_remuxing.h
 */
#pragma once
#ifdef Win32
#define __STDC_CONSTANT_MACROS
#endif

#include <atomic>
#include <mutex>
extern "C"
{
    #include <libavutil/timestamp.h>
    #include <libavformat/avformat.h>
}
#include "ffmpeg_encode_frame.h"

class FFmpegRemuxing
{
public:
    explicit FFmpegRemuxing();

    bool RemuxingVideoFile(const std::string &input, const std::string &output, const std::string &oformat);

    bool RemuxingImage(const std::string &output, const std::string &oformat, const FFmpegEncodeFrame::VideoParams &params = FFmpegEncodeFrame::VideoParams());

    void PutImageFrame(const char *src, const int64_t size, const AVPixelFormat &fmt);

    void Stop();

private:
    std::atomic_bool running_;
    std::mutex frame_mtx_;
    char *frame_buffer_ = nullptr;
    int64_t frame_size_ = 0;
    AVPixelFormat pix_fmt_;
};