/*
 * @Author: your name
 * @Date: 2021-02-09 19:48:52
 * @LastEditTime: 2021-02-18 22:14:52
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \qt_project\VideoEncodeRtsp\ffmpeg_remuxing.cpp
 */
#include <iostream>
extern "C"
{
    #include <libavutil/time.h>
}
#include "ffmpeg_remuxing.h"
#include "ffmpeg_encode_frame.h"

FFmpegRemuxing::FFmpegRemuxing()
{
    av_log_set_level(AV_LOG_INFO);

    running_.store(true);
}

bool FFmpegRemuxing::RemuxingVideoFile(const std::string &input, const std::string &output, const std::string &oformat)
{
    AVFormatContext *format_ctx = nullptr;
    AVFormatContext *output_format = nullptr;
    AVDictionary *opt = nullptr;
    char errbuf[512]{0};
	
    av_dict_set(&opt, "rtsp_transport", "tcp", 0);
    av_dict_set(&opt, "stimeout", "10000000", 0);

    int ret = avformat_open_input(&format_ctx, input.data(), nullptr, &opt);
    if(ret !=0 )
    {
        av_make_error_string(errbuf, sizeof(errbuf), ret);
        std::cout << __FUNCTION__ << " " << errbuf << std::endl;
        avformat_close_input(&format_ctx);
        avformat_free_context(format_ctx);
        return false;
    }

    ret = avformat_find_stream_info(format_ctx, nullptr);
    if(ret != 0)
    {
        av_make_error_string(errbuf, sizeof(errbuf), ret);
        std::cout << __FUNCTION__ << " " << errbuf << std::endl;
        avformat_close_input(&format_ctx);
        avformat_free_context(format_ctx);
        return false;
    }

    av_dump_format(format_ctx, 0, input.data(), 0);
    ret = avformat_alloc_output_context2(&output_format, nullptr, oformat.data(), output.data());
    if(ret != 0)
    {
        av_make_error_string(errbuf, sizeof(errbuf), ret);
        std::cout << __FUNCTION__ << " " << errbuf << std::endl;
        avformat_close_input(&format_ctx);
        avformat_free_context(format_ctx);
        avformat_free_context(output_format);
        return false;
    }

    for(int i = 0; i < format_ctx->nb_streams; i++)
    {
        AVStream *in_stream = format_ctx->streams[i];
        if(in_stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
        {
            continue;
        }

        AVStream *out_stream = avformat_new_stream(output_format, nullptr);
        if(!out_stream)
        {
            std::cout << "FFmpegRemuxing::RemuxingStream avformat_new_stream failed" << std::endl;
            avformat_close_input(&format_ctx);
            avformat_free_context(format_ctx);
            avformat_free_context(output_format);
            return false;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        if(ret != 0)
        {
            av_make_error_string(errbuf, sizeof(errbuf), ret);
            std::cout << __FUNCTION__ << " " << errbuf << std::endl;
            avformat_close_input(&format_ctx);
            avformat_free_context(format_ctx);
            avformat_free_context(output_format);
            return false;
        }

        out_stream->codecpar->codec_tag = 0;
    }

    av_dump_format(output_format, 0, output.data(), 1);
    if(!(output_format->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&output_format->pb, output.data(), AVIO_FLAG_WRITE);
        if (ret != 0)
        {
            av_make_error_string(errbuf, sizeof(errbuf), ret);
            std::cout << __FUNCTION__ << " " << errbuf << std::endl;
            avformat_close_input(&format_ctx);
            avformat_free_context(format_ctx);
            avformat_free_context(output_format);
            return false;
        }
    }

    // av_dict_set(&opt,"buffer_size","425984",0);
    // av_dict_set(&opt,"max_delay","0",0);
    // av_dict_set(&opt, "rtbufsize", "1024000", 0);
    // av_dict_set(&opt, "muxdelay", "0.1", 0);
    // av_dict_set(&opt, "preset", "ultrafast", 0);
    // av_dict_set(&opt, "tune", "zerolatency", 0);
    av_dict_set(&opt, "rtsp_transport", "tcp", 0);
    ret = avformat_write_header(output_format, &opt);
    if(ret != 0)
    {
        av_make_error_string(errbuf, sizeof(errbuf), ret);
        std::cout << __FUNCTION__ << " " << errbuf << std::endl;
        avformat_close_input(&format_ctx);
        avformat_free_context(format_ctx);
        avformat_free_context(output_format);
        return false;
    }

    int64_t start_time = av_gettime();
    AVPacket packet;
    running_.store(true);
    while (running_.load())
    {
        AVStream *in_stream, *out_stream;
        ret = av_read_frame(format_ctx, &packet);
        if(ret != 0)
        {
            break;
        }
        
        if(packet.pts < 0 || packet.dts < 0)
        {
            continue;
        }

        in_stream  = format_ctx->streams[packet.stream_index];
        out_stream = output_format->streams[packet.stream_index];

        if(in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            AVRational time_base = in_stream->time_base;
            AVRational time_base_q{1, AV_TIME_BASE};
            int64_t pts_time = av_rescale_q(packet.dts, time_base, time_base_q);
            int64_t now_time = av_gettime() - start_time;
            if(pts_time > now_time)
                av_usleep(pts_time - now_time);
        }

        packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);

        ret = av_interleaved_write_frame(output_format, &packet);
        if (ret < 0) {
            av_make_error_string(errbuf, sizeof(errbuf), ret);
            std::cout << "Error muxing packet: code " << ret << ", msg " << errbuf << std::endl;
            break;
        }

        av_packet_unref(&packet);
    }
    av_write_trailer(output_format);

    if(!(output_format->oformat->flags & AVFMT_NOFILE))
    {
        avio_close(output_format->pb);
    }
    avformat_close_input(&format_ctx);

    avformat_free_context(output_format);
    avformat_free_context(format_ctx);

    running_.store(false);
}

bool FFmpegRemuxing::RemuxingImage(const std::string &output, const std::string &oformat, const FFmpegEncodeFrame::VideoParams &params)
{
    AVFormatContext *output_format = nullptr;
    AVOutputFormat *fmt = nullptr;
    AVStream *video_st = nullptr;
    char errbuf[512]{0};
    int ret = avformat_alloc_output_context2(&output_format, nullptr, oformat.data(), output.data());
    if(ret != 0)
    {
        av_make_error_string(errbuf, sizeof(errbuf), ret);
        std::cout << __FUNCTION__ << " " << errbuf << std::endl;
        avformat_free_context(output_format);
        return false;
    }

    if(!(output_format->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&output_format->pb, output.data(), AVIO_FLAG_WRITE);
        if (ret != 0)
        {
            av_make_error_string(errbuf, sizeof(errbuf), ret);
            std::cout << __FUNCTION__ << " " << errbuf << std::endl;
            avformat_free_context(output_format);
            return false;
        }
    }

    video_st = avformat_new_stream(output_format, 0);
    if(!video_st)
    {
        avformat_free_context(output_format);
        return false;
    }
    video_st->time_base = params.time_base;
    
    FFmpegEncodeFrame encoder;
    encoder.Initsize(params);

    AVDictionary *opt = nullptr;
    // av_dict_set(&opt,"buffer_size","425984",0);
    // av_dict_set(&opt,"max_delay","0",0);
    // av_dict_set(&opt, "rtbufsize", "1024000", 0);
    // av_dict_set(&opt, "muxdelay", "0.1", 0);
    // av_dict_set(&opt, "preset", "ultrafast", 0);
    // av_dict_set(&opt, "tune", "zerolatency", 0);
    av_dict_set(&opt, "rtsp_transport", "tcp", 0);
    ret = avformat_write_header(output_format, &opt);
    if(ret != 0)
    {
        av_make_error_string(errbuf, sizeof(errbuf), ret);
        std::cout << __FUNCTION__ << " " << errbuf << std::endl;
        avformat_free_context(output_format);
        return false;
    }
    
    frame_size_ = av_image_get_buffer_size(params.pix_fmt, params.width, params.height, 1);
    frame_buffer_ = new char[frame_size_];

    int64_t start_time = av_gettime();
    AVPacket packet;
    running_.store(true);
    while (running_.load())
    {
        AVRational time_base = params.time_base;
        AVRational time_base_q{1, AV_TIME_BASE};
        int64_t pts_time = av_rescale_q(packet.dts, time_base, time_base_q);
        int64_t now_time = av_gettime() - start_time;
        if(pts_time > now_time)
            av_usleep(pts_time - now_time);
        
        std::lock_guard<std::mutex> lock(frame_mtx_);
        encoder.Encode(frame_buffer_, frame_size_, pix_fmt_, now_time, [&](AVPacket *packet){
            ret = av_interleaved_write_frame(output_format, packet);
            if (ret < 0) {
                av_make_error_string(errbuf, sizeof(errbuf), ret);
                std::cout << "Error muxing packet: code " << ret << ", msg " << errbuf << std::endl;
                return;
            }
        });
    }
    
    running_.store(false);
    delete frame_buffer_;
    
    return true;
}

void FFmpegRemuxing::PutImageFrame(const char *src, const int64_t size, const AVPixelFormat &fmt)
{
    std::lock_guard<std::mutex> lock(frame_mtx_);
    ::memcpy(frame_buffer_, src, size);
    frame_size_ = size;
    pix_fmt_ = fmt;
}

void FFmpegRemuxing::Stop()
{
    running_.store(false);
}
