/*
 * @Author: your name
 * @Date: 2021-02-09 19:05:24
 * @LastEditTime: 2021-02-09 23:31:22
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
    if(argc  < 2)
    {
        std::cout << "Usage: " << argv[0] << " <video file>" << std::endl;
        return -1;
    }
    FFmpegRemuxing remuxing;
#if 0
    remuxing.RemuxingVideoFile("rtsp://192.168.2.66/whl_desktop", "rtsp://192.168.2.66/test", "rtsp");
#else
    cv::VideoCapture capture(argv[1]);
    if(!capture.isOpened())
    {
        return -1;
    }
    
    FFmpegEncodeFrame::VideoParams params;
    params.width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
    params.height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
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
