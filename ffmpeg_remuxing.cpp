/*
 * @Author: your name
 * @Date: 2021-02-09 19:48:52
 * @LastEditTime: 2021-02-09 23:26:52
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
    av_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_INFO);

    running_.store(true);
}

bool FFmpegRemuxing::RemuxingStream(const std::string &input, const std::string &output, const std::string &oformat)
{
    AVFormatContext *format_ctx = nullptr;
    AVFormatContext *output_format = nullptr;
    AVDictionary *opt = nullptr;
    char errbuf[512]{0};
	
    // av_dict_set(&opt,"buffer_size","1024000",0);
    // av_dict_set(&opt,"max_delay","0",0);
    // av_dict_set(&opt, "rtbufsize", "1024000", 0);
    // av_dict_set(&opt, "muxdelay", "0.1", 0);
    // av_dict_set(&opt, "preset", "ultrafast", 0);
    // av_dict_set(&opt, "tune", "zerolatency", 0);
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

    ret = avformat_write_header(output_format, &opt);
    if(ret != 0)
    {
        avformat_free_context(format_ctx);
        avformat_close_input(&format_ctx);
        avformat_free_context(output_format);
    }

    int64_t start_time = av_gettime();
    AVPacket packet;
    running_.store(true);
    int frame_index = 0;
    while (running_.load())
    {
        AVStream *in_stream, *out_stream;
        ret = av_read_frame(format_ctx, &packet);
        if(ret != 0)
        {
            break;
        }
    
        in_stream  = format_ctx->streams[packet.stream_index];
        out_stream = output_format->streams[packet.stream_index];

        if(packet.pts == AV_NOPTS_VALUE)
        {
            AVRational time_base1 = format_ctx->streams[packet.stream_index]->time_base;
            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(format_ctx->streams[packet.stream_index]->r_frame_rate);
            packet.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
            packet.dts = packet.pts;
            packet.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
        }

        if(in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            AVRational time_base = in_stream->time_base;
            AVRational time_base_q = AV_TIME_BASE_Q;
            int64_t pts_time = av_rescale_q(packet.dts, time_base, time_base_q);
            int64_t now_time = av_gettime() - start_time;
            if(pts_time > now_time)
                av_usleep(pts_time - now_time);
        }

        packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
        frame_index++;

        ret = av_write_frame(output_format, &packet);
        if (ret < 0) {
            av_make_error_string(errbuf, sizeof(errbuf), ret);
            std::cout << "Error muxing packet: " << errbuf << std::endl;
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
        // std::string codec_name;
        // /* put sample parameters */
        // int bit_rate = 400000;
        // /* resolution must be a multiple of two */
        // int width = 352;
        // int height = 288;
        // /* frames per second */
        // AVRational time_base{1,25};
        // AVRational framerate{25,1};
        // /* emit one intra frame every ten frames
        // * check frame pict_type before passing frame
        // * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I 
        // * then gop_size is ignored and the output of encoder
        // * will always be I frame irrespective to gop_size
        // */
        // int gop_size = 10;
        // int max_b_frames = 1;
        // int pix_fmt = AV_PIX_FMT_YUV420P;
bool FFmpegRemuxing::RemuxingImage(const FFmpegEncodeFrame::VideoParams &params, const std::string &output, const std::string &oformat)
{
    AVFormatContext *output_format;
    char errbuf[512]{0};
    int ret = avformat_alloc_output_context2(&output_format, nullptr, oformat.data(), output.data());
    if(ret != 0)
    {
        av_make_error_string(errbuf, sizeof(errbuf), ret);
        std::cout << __FUNCTION__ << " " << errbuf << std::endl;
        avformat_free_context(output_format);
        return false;
    }

    // output_format->codec_name = 
    return true;
}

void FFmpegRemuxing::Stop()
{
    running_.store(false);
}