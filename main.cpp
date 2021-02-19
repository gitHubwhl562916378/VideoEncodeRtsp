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
    if(argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <video file>" << std::endl;
        return -1;
    }

    char *video_file = argv[1];
    FFmpegRemuxing remuxing;
#if 0
    remuxing.RemuxingVideoFile(video_file, "rtsp://192.168.2.66/test", "rtsp");
#else
    cv::VideoCapture capture(video_file);
    if(!capture.isOpened())
    {
        std::cout << "open " << video_file << " failed" << std::endl;
        return -1;
    }
    
    FFmpegEncodeFrame::VideoParams params;
    // params.width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
    // params.height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
    params.width = 1920;
    params.height = 1080;
    std::cout << params.width << " " << params.height << std::endl;
    if(!remuxing.RemuxingImage("rtsp://192.168.2.66/test", "rtsp", params))
    {
        return -1;
    }
    while(true)
    {
        cv::Mat frame;
        capture >> frame;
        if(frame.empty())
        {
            break;
        }

        cv::cvtColor(frame, frame, cv::COLOR_BGR2YUV_I420);
        remuxing.PutImageFrame((char *)frame.data, frame.total() * frame.elemSize(), AV_PIX_FMT_YUV420P);
		// std::this_thread::sleep_for(std::chrono::milliseconds(1000/25));
    }
#endif
    return 0;
}
