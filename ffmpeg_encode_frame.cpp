#include <iostream>
#include "ffmpeg_encode_frame.h"

FFmpegEncodeFrame::FFmpegEncodeFrame()
{
    
}

FFmpegEncodeFrame::~FFmpegEncodeFrame()
{
    if(codec_)
    {
        avcodec_close(ctx_);
    }

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

    if(sws_ctx_)
    {
        sws_freeContext(sws_ctx_);
    }
}

AVCodecContext* FFmpegEncodeFrame::Initsize(const VideoParams &params)
{
    // codec_ = avcodec_find_encoder(AV_CODEC_ID_H264);  //AV_CODEC_ID_H264
    codec_ = avcodec_find_encoder_by_name(params.codec_name.data());
    if(!codec_)
    {
        std::cout << "Codec " << params.codec_name << " not found" << std::endl;
        return nullptr;
    }

    ctx_ = avcodec_alloc_context3(codec_);
    if(!ctx_)
    {
        std::cout << "Could not allocate video codec context" << std::endl;
        return nullptr;
    }

    ctx_->bit_rate = params.bit_rate;
    ctx_->width = params.width;
    ctx_->height = params.height;
    ctx_->time_base = params.time_base;
    ctx_->framerate = params.framerate;
    ctx_->gop_size = params.gop_size;
    ctx_->pix_fmt = AVPixelFormat(params.pix_fmt);
    src_pix_fmt_ = params.src_pix_fmt;
    ctx_->keyint_min = 30;
    ctx_->qmax = 35;
    ctx_->qmin = 30;
    ctx_->me_range = 16;
    ctx_->max_qdiff = 4;
    ctx_->thread_count = 10;
    ctx_->qcompress = 0.6;
    ctx_->b_frame_strategy = true;

    frame_ = av_frame_alloc();
    if(!frame_)
    {
        std::cout << "Could not allocate video frame" << std::endl;
        return nullptr;
    }
    frame_->format = ctx_->pix_fmt;
    frame_->width = ctx_->width;
    frame_->height = ctx_->height;
    if(codec_->id == AV_CODEC_ID_H264)
    {
        av_opt_set(ctx_->priv_data, "preset", "fast", 0);
        av_opt_set(ctx_->priv_data, "tune", "zerolatency", 0);
    }
    
    AVDictionary *opt = nullptr;
    // av_dict_set(&opt,"buffer_size","425984",0);
    av_dict_set(&opt,"max_delay","0.1",0);
    // av_dict_set(&opt, "rtbufsize", "1024000", 0);
    // av_dict_set(&opt, "muxdelay", "0.1", 0);
    av_dict_set(&opt, "preset", "fast", 0); //ultrafast fast slow
    av_dict_set(&opt, "tune", "zerolatency", 0);
    int ret = avcodec_open2(ctx_, codec_, &opt);
    char errbuf[512]{0};
    if(ret < 0)
    {
        av_make_error_string(errbuf, sizeof(errbuf), ret);
        std::cout << "Could not open codec " << errbuf << std::endl;
        return nullptr;
    }

    ret = av_frame_get_buffer(frame_, 0);
    if(ret < 0)
    {
        std::cout << "Could not allocate the video frame data" << std::endl;
        return nullptr;
    }

    pkt_ = av_packet_alloc();
    if(!pkt_)
    {
        std::cout << "Could not allocate the packet" << std::endl;
        return nullptr;
    }

    sws_ctx_ = sws_getContext(frame_->width, frame_->height, params.src_pix_fmt, frame_->width, frame_->height, params.pix_fmt, SWS_BICUBIC,nullptr,nullptr,nullptr);
    if(!sws_ctx_)
    {
        std::cout << "FFmpegEncodeFrame::Initsize sws_getContext nullptr" << std::endl;
        return nullptr;
    }

    return ctx_;
}

bool FFmpegEncodeFrame::Encode(const char *data, int size, int64_t pts, std::function<void(AVPacket*)> call_back)
{
    int ret;
    ret = av_frame_make_writable(frame_);
    if(ret < 0)
    {
        std::cout << "FFmpegEncodeFrame::Encode av_frame_make_writable error, code " << ret << std::endl;
        return false;
    }
    int *srcStride = nullptr;
    uint8_t** srcBuf = nullptr;
    GetFrameSlice(frame_->width, frame_->height, (uint8_t *)data, src_pix_fmt_, srcBuf, srcStride);
    if(!srcStride || !srcBuf)
    {
        std::cout << "FFmpegEncodeFrame::Encode GetFrameSlice failed" << std::endl;
        return false;
    }
    sws_scale(sws_ctx_, srcBuf, srcStride, 0, frame_->height, frame_->data, frame_->linesize);
    delete srcStride;
    delete srcBuf;
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

void FFmpegEncodeFrame::GetFrameSlice(const int width, const int height, uint8_t *data, const AVPixelFormat fmt, uint8_t** &slice_ptr, int * & stride)
{
    if(fmt == AV_PIX_FMT_BGR24 || fmt == AV_PIX_FMT_RGB24)
    {
        slice_ptr = new uint8_t*[1];
        stride = new int[1];
        slice_ptr[0] = data;
        stride[0] = width * 3;
    }else if(fmt == AV_PIX_FMT_YUV420P)
    {
        slice_ptr = new uint8_t*[3];
        stride = new int[3];
        slice_ptr[0] = data;
        stride[0] = width;
        slice_ptr[1] = data + width * height;
        stride[1] = width >> 1;
        slice_ptr[2] = data + width * height * 5 / 4;
        stride[2] = width >> 1;
    }else if(fmt == AV_PIX_FMT_NV12)
    {
        slice_ptr = new uint8_t*[2];
        stride = new int[2];
        slice_ptr[0] = data;
        stride[0] = width;
        slice_ptr[1] = data + width * height;
        stride[1] = width;
    }
}