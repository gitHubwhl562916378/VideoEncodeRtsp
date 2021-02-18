#include <iostream>
#include "ffmpeg_encode_frame.h"

FFmpegEncodeFrame::FFmpegEncodeFrame()
{
    mem_cp_func_map_.insert(std::make_pair(AV_PIX_FMT_YUV420P, std::bind(&FFmpegEncodeFrame::CopyYuvFrameData, this, std::placeholders::_1, std::placeholders::_2)));
}

FFmpegEncodeFrame::~FFmpegEncodeFrame()
{
    if(ctx_)
    {
        avcodec_free_context(&ctx_);
    }

    if(frame_)
    {
        av_frame_free(&frame_);
    }

    if(pkt_)
    {
        av_packet_free(&pkt_);
    }
}

bool FFmpegEncodeFrame::Initsize(const VideoParams &params)
{
    // codec_ = avcodec_find_encoder(code_id);  AV_CODEC_ID_H264
    codec_ = avcodec_find_encoder_by_name(params.codec_name.data());
    if(!codec_)
    {
        std::cout << "Codec " << params.codec_name << " not found" << std::endl;
        return false;
    }

    ctx_ = avcodec_alloc_context3(codec_);
    if(!ctx_)
    {
        std::cout << "Could not allocate video codec context" << std::endl;
        return false;
    }

    ctx_->bit_rate = params.bit_rate;
    ctx_->width = params.width;
    ctx_->height = params.height;
    ctx_->time_base = params.time_base;
    ctx_->framerate = params.framerate;
    ctx_->gop_size = params.gop_size;
    ctx_->pix_fmt = AVPixelFormat(params.pix_fmt);

    frame_->format = ctx_->pix_fmt;
    frame_->width = ctx_->width;
    frame_->height = ctx_->height;
    
    int ret = avcodec_open2(ctx_, codec_, nullptr);
    char errbuf[512]{0};
    if(ret < 0)
    {
        av_make_error_string(errbuf, sizeof(errbuf), ret);
        std::cout << "Could not open codec " << errbuf << std::endl;
        return false;
    }

    frame_ = av_frame_alloc();
    if(!frame_)
    {
        std::cout << "Could not allocate video frame" << std::endl;
        return false;
    }

    ret = av_frame_get_buffer(frame_, 0);
    if(ret < 0)
    {
        std::cout << "Could not allocate the video frame data" << std::endl;
        return false;
    }

    auto iter = mem_cp_func_map_.find(AVPixelFormat(ctx_->pix_fmt));
    if(iter == mem_cp_func_map_.end())
    {
        std::cout << "Could suport pixformat" << std::endl;
        return false;
    }
    mem_cp_func_ = iter->second;

    pkt_ = av_packet_alloc();
    if(!pkt_)
    {
        std::cout << "Could not allocate the packet" << std::endl;
        return false;
    }

    return true;
}

bool FFmpegEncodeFrame::Encode(char *data, int size, int type, int64_t pts, std::function<void(AVPacket*)> call_back)
{
    int ret;
    ret = av_frame_make_writable(frame_);
    if(ret < 0)
    {
        std::cout << "FFmpegEncodeFrame::Encode av_frame_make_writable error, code " << ret << std::endl;
        return false;
    }

    mem_cp_func_(data, frame_);
    frame_->pts = pts;

    ret = avcodec_send_frame(ctx_, frame_);
    if(ret < 0)
    {
        std::cout << "Error sending a frame for encoding" << std::endl;
        return false;
    }

    while(ret >= 0)
    {
        ret = avcodec_receive_packet(ctx_, pkt_);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            break;
        }else if(ret < 0)
        {
            std::cout << "Error during encoding" << std::endl;
            break;
        }

        if(call_back)
        {
            call_back(pkt_);
        }

        av_packet_unref(pkt_);
    }
}

void FFmpegEncodeFrame::CopyYuvFrameData(char *src, AVFrame *frame)
{
    for(int y = 0; y < frame->height; y++)
    {
        ::memcpy(frame->data[0] + y * frame->width, src + y * frame->width, frame->width);
    }

    int pos = frame->width * frame->height;
    for(int y = 0; y < (frame->height >> 1); y++)
    {
        ::memcpy(frame->data[1] + pos + y * frame->width / 2, src + pos + y * frame->width / 2, frame->width / 2);
    }

    pos += (frame->width / 2 * frame->height / 2);
    for(int y = 0; y < (frame->height >> 1); y++)
    {
        ::memcpy(frame->data[2] + pos + y * frame->width / 2, src + pos + y * frame->width / 2, frame->width / 2);
    }
}