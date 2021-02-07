extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}
#include <map>

//https://ffmpeg.org/doxygen/trunk/encode_video_8c-example.html
class FFmpegEncodeFrame
{
public:
    struct VideoParams
    {
        std::string codec_name;
        /* put sample parameters */
        int bit_rate = 400000;
        /* resolution must be a multiple of two */
        int width = 352;
        int height = 288;
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
        int pix_fmt = AV_PIX_FMT_YUV420P;
    };
    explicit FFmpegEncodeFrame();
    
    ~FFmpegEncodeFrame();

    bool Initsize(const VideoParams &params);

    bool Encode(void *data, int size, int type, int64_t pts, std::function<void(AVPacket*)> call_back);

private:
    void CopyYuvFrameData(void *src, AVFrame *frame);

    const AVCodec *codec_ = nullptr;
    AVCodecContext *ctx_ = nullptr;
    AVFrame *frame_ = nullptr;
    AVPacket *pkt_ = nullptr;
    std::map<int, std::function<void(void*, AVFrame*)>> mem_cp_func_map_;
    std::function<void(void*, AVFrame*)> mem_cp_func_;
};