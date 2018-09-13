//
// Created by liuml on 2018/9/11.
//
#include <jni.h>
#include <string>
#include <jni.h>
#include <string>
#include <android/log.h>
//extern "C" 主要作用就是为了能够正确实现C++代码调用其他C语言代码 加上extern "C"后，会指示编译器这部分代码按C语言的进行编译，而不是C++的。
extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
//用于android 绘制图像的
#include <android/native_window_jni.h>
#include <unistd.h>
#include <libswresample/swresample.h>
}

#ifndef FFMPEGDEMO_FFMPEGMUSIC_H
#define FFMPEGDEMO_FFMPEGMUSIC_H
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"jnilib",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"jnilib",FORMAT,##__VA_ARGS__);
#endif //FFMPEGDEMO_FFMPEGMUSIC_H

int createFFmpeg(int *rate,int *channel);

void getPcm(void **pcm,size_t *pcm_size);

void releaseFFmpeg();