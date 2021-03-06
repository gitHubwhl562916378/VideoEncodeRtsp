/*
 * @Author: your name
 * @Date: 2021-02-09 19:05:24
 * @LastEditTime: 2021-03-06 14:52:51
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \qt_project\VideoEncodeRtsp\main.cpp
 */
#include <chrono>
#include <thread>
#ifdef WIN32
#include <filesystem>
#else
#include <experimental/filesystem>
#endif
#include "opencv2/opencv.hpp"
#include "ffmpeg_remuxing.h"
#define USE3

int main(int argc, char **argv)
{
#if 0
    cv::VideoCapture capture_save("4.mp4");
    if(!capture_save.isOpened())
    {
        std::cout << "open failed" << std::endl;
        return -1;
    }

    int frame_index = 0;
    while(true)
    {
        cv::Mat frame;
        capture_save >> frame;
        if(frame.empty())
        {
            break;
        }

        cv::imwrite("images/" + std::to_string(frame_index++) + ".jpg", frame);
    }

    return 0;
#else
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
#ifdef USE1
    remuxing.RemuxingVideoFile(video_file, out_file, oformat);
#elif defined USE2
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
#elif defined USE3
#ifdef WIN32
    std::filesystem::path cur_path(std::filesystem::current_path().string() + "/images");
    int count = std::count_if(std::filesystem::recursive_directory_iterator(cur_path), std::filesystem::recursive_directory_iterator(),
        [](const std::filesystem::directory_entry &p)
        {
            std::cout << p << " " << p.file_size() << std::endl;
            return p.is_regular_file();
        }
    );
    std::string cur_path_str = cur_path.string();
#else
    std::string cur_path_str = std::experimental::filesystem::current_path();
    std::experimental::filesystem::path cur_path(cur_path_str + "/images");
    cur_path_str = cur_path;
    int count = std::count_if(std::experimental::filesystem::recursive_directory_iterator(cur_path), std::experimental::filesystem::recursive_directory_iterator(), 
        [](const std::experimental::filesystem::directory_entry & p)
        {
            return p.status().type() == std::experimental::filesystem::file_type::regular; //regular, symlink, directory, character,fifo,socker
        }
    );
#endif
    for(size_t i = 0; i < count; i++)
    {
        cv::Mat frame = cv::imread(cur_path_str + "/" + std::to_string(i) + ".jpg");
        if(!remuxing.isRunning())
        {
            FFmpegEncodeFrame::VideoParams params;
            params.width = frame.cols;
            params.height = frame.rows;
            params.src_pix_fmt = AV_PIX_FMT_BGR24; //AV_PIX_FMT_YUV420P AV_PIX_FMT_BGR24
            remuxing.RemuxingImage(out_file, oformat, params);
        }
        // cv::cvtColor(frame, frame, cv::COLOR_BGR2YUV_I420);
        remuxing.PutImageFrame((char *)frame.data, frame.total() * frame.elemSize());
		// std::this_thread::sleep_for(std::chrono::milliseconds(1000/25));
    }

    remuxing.Stop();
#endif
#endif
    return 0;
}
