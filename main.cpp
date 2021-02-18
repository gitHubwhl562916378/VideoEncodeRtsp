/*
 * @Author: your name
 * @Date: 2021-02-09 19:05:24
 * @LastEditTime: 2021-02-09 23:31:22
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \qt_project\VideoEncodeRtsp\main.cpp
 */
#include "ffmpeg_remuxing.h"

int main()
{
    FFmpegRemuxing remuxing;
    remuxing.RemuxingVideoFile("rtsp://192.168.2.66/whl_desktop", "rtsp://192.168.2.66/test", "rtsp");

    return 0;
}
