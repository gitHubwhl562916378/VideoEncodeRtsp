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
    char errbuf[512]{0};
    int ret = avformat_alloc_output_context2(&output_format_, nullptr, oformat.data(), output.data());
    if(ret != 0)
    {
        av_make_error_string(errbuf, sizeof(errbuf), ret);
        std::cout << __FUNCTION__ << " " << errbuf << std::endl;
        avformat_free_context(output_format_);
        return false;
    }

    video_st_ = avformat_new_stream(output_format_, 0);
    if(!video_st_)
    {
        avformat_free_context(output_format_);
        return false;
    }
    video_st_->time_base = params.time_base;
    
    encoder_.reset(new FFmpegEncodeFrame);

#if 1
    auto pcodec_ctx = encoder_->Initsize(params);
    if(!pcodec_ctx)
    {
        return false;
    }
    avcodec_parameters_from_context(video_st_->codecpar, pcodec_ctx);
#else
    bool is_ok = encoder_->Initsize(video_st_->codec, params);
    avcodec_parameters_from_context(video_st_->codecpar, video_st_->codec);
#endif

    if(!(output_format_->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&output_format_->pb, output.data(), AVIO_FLAG_WRITE);
        if (ret != 0)
        {
            av_make_error_string(errbuf, sizeof(errbuf), ret);
            std::cout << __FUNCTION__ << " " << errbuf << std::endl;
            avformat_free_context(output_format_);
            return false;
        }
    }

    AVDictionary *opt = nullptr;
    // av_dict_set(&opt,"buffer_size","425984",0);
    // av_dict_set(&opt,"max_delay","0",0);
    // av_dict_set(&opt, "rtbufsize", "1024000", 0);
    // av_dict_set(&opt, "muxdelay", "0.1", 0);
    // av_dict_set(&opt, "preset", "ultrafast", 0);
    // av_dict_set(&opt, "tune", "zerolatency", 0);
    av_dict_set(&opt, "rtsp_transport", "tcp", 0);
    ret = avformat_write_header(output_format_, &opt);
    if(ret != 0)
    {
        av_make_error_string(errbuf, sizeof(errbuf), ret);
        std::cout << __FUNCTION__ << " " << errbuf << std::endl;
        avformat_free_context(output_format_);
        return false;
    }

    return true;
}

void FFmpegRemuxing::PutImageFrame(const char *src, const int64_t size, const AVPixelFormat &fmt)
{
    char errbuf[512]{0};
    static int64_t start_pt = av_gettime();;
    encoder_->Encode(src, size, fmt, av_gettime() - start_pt, [&](AVPacket *packet){
        int ret = av_interleaved_write_frame(output_format_, packet);
        if (ret < 0) {
            av_make_error_string(errbuf, sizeof(errbuf), ret);
            std::cout << "Error muxing packet: code " << ret << ", msg " << errbuf << std::endl;
            return;
        }
    });
}

void FFmpegRemuxing::Stop()
{
    running_.store(false);
}
