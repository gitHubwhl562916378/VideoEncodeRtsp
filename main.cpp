/*
 * @Author: your name
 * @Date: 2021-02-09 19:05:24
 * @LastEditTime: 2021-02-19 18:19:41
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \qt_project\VideoEncodeRtsp\main.cpp
 */
 #include <chrono>
#include <thread>
#include "opencv2/opencv.hpp"
#include "ffmpeg_remuxing.h"

int main(int argc, char **argv)
{
    if(argc < 4)
    {
        std::cout << "Usage: " << argv[0] << " <input file or stream> <out file or stream> <output format>. sample: " << std::endl
        << "./video_encode_rtsp 103.mp4 rtsp://192.168.2.66/test rtsp" << std::endl;;
        return -1;
    }

    char *video_file = argv[1];
    char *out_file = argv[2];
    char *oformat = argv[3];
    FFmpegRemuxing remuxing;
#if 0
    remuxing.RemuxingVideoFile(video_file, out_file, oformat);
#else
    cv::VideoCapture capture(video_file);
    if(!capture.isOpened())
    {
        std::cout << "open " << video_file << " failed" << std::endl;
        return -1;
    }
    
    FFmpegEncodeFrame::VideoParams params;
    params.width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
    params.height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
    params.src_pix_fmt = AV_PIX_FMT_BGR24; //AV_PIX_FMT_YUV420P AV_PIX_FMT_BGR24
    if(!params.width || !params.height)
    {
        params.width = 1920;
        params.height = 1080;
    }
    std::cout << params.width << " " << params.height << std::endl;
    remuxing.RemuxingImage(out_file, oformat, params);

    while(true)
    {
        cv::Mat frame;
        capture >> frame;
        if(frame.empty())
        {
            break;
        }

        // cv::cvtColor(frame, frame, cv::COLOR_BGR2YUV_I420);
        remuxing.PutImageFrame((char *)frame.data, frame.total() * frame.elemSize());
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/25));
    }

    remuxing.Stop();
#endif
    return 0;
}
