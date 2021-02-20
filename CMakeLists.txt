cmake_minimum_required (VERSION 3.9 FATAL_ERROR)

project (video_encode_rtsp
        LANGUAGES CXX
        VERSION "7.0.0.11"
        DESCRIPTION "video_push_rtsp")

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

#ADD_DEFINITIONS(-D_DEBUG_TEST) 

set(OpenCV_DIR E:/opencv3.4.0-x64/x64/vc15/lib)
find_package(OpenCV REQUIRED)

if (WIN32)
        set(FFMPEG_INCLUDE_DIRS E:/ffmpeg/x64/include)
        link_directories(E:/ffmpeg/x64/lib)
        list(APPEND FFMPEG_LIBRARIES avcodec avdevice avfilter avformat avutil postproc swresample swscale)
elseif (UNIX)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(PC_AVDEVICE REQUIRED libavdevice)
        pkg_check_modules(PC_AVFORMAT REQUIRED libavformat)
        pkg_check_modules(PC_AVUTIL REQUIRED libavutil)
        pkg_check_modules(PC_AVCODEC REQUIRED libavcodec)
        pkg_check_modules(PC_SWSCALE REQUIRED libswscale)
        find_library(AVDEVICE_LIBRARY avdevice HINTS ${PC_AVDEVICE_LIBDIR} ${PC_AVDEVICE_LIBRARY_DIRS})
        find_library(AVFORMAT_LIBRARY avformat HINTS ${PC_AVFORMAT_LIBDIR} ${PC_AVFORMAT_LIBRARY_DIRS})
        find_library(AVUTIL_LIBRARY avutil HINTS ${PC_AVUTIL_LIBDIR} ${PC_AVUTIL_LIBRARY_DIRS})
        find_library(AVCODEC_LIBRAR avcodec HINTS ${PC_AVCODEC_LIBDIR} ${PC_AVCODEC_LIBRARY_DIRS})
        find_library(SWSCALE_LIBRAR swscale HINTS ${PC_SWSCALE_LIBDIR} ${PC_SWSCALE_LIBRARY_DIRS})
        list(APPEND FFMPEG_INCLUDE_DIRS ${PC_AVFORMAT_INCLUDEDIR})
        list(APPEND FFMPEG_LIBRARIES ${AVDEVICE_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${AVCODEC_LIBRAR} ${SWSCALE_LIBRAR})
endif ()

include_directories(${FFMPEG_INCLUDE_DIRS})

# Sources
file(GLOB SRCS *.cpp *.h)

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} pthread)