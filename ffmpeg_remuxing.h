/*
 * @Author: your name
 * @Date: 2021-02-09 19:48:44
 * @LastEditTime: 2021-02-09 23:30:50
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \qt_project\VideoEncodeRtsp\ffmpeg_remuxing.h
 */
#ifdef Win32
#define __STDC_CONSTANT_MACROS
#endif

#include <atomic>
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

    bool RemuxingStream(const std::string &input, const std::string &output, const std::string &oformat);

    bool RemuxingImage(const FFmpegEncodeFrame::VideoParams &params, const std::string &output, const std::string &oformat);

    void Stop();

private:
    std::atomic_bool running_;
};