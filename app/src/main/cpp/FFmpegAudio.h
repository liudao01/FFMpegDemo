//
// Created by liuml on 2018/9/15.
//

#ifndef FFMPEGDEMO_FFMPEGAUDIO_H
#define FFMPEGDEMO_FFMPEGAUDIO_H

#include <jni.h>
#include <string>
#include <jni.h>
#include <string>
#include <android/log.h>
#include <queue>
#include <unistd.h>
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

#include <libswresample/swresample.h>

#include "Log.h"
};



class FFmpegAudio {
public:
    FFmpegAudio();

    ~FFmpegAudio();

    //出队  队列
    int get(AVPacket *packet);

    //入队
    int put(AVPacket *packet);

    //播放
    void play();

    //停止
    void stop();

    //设置解码器上下文
    void setAvCodecContext(AVCodecContext *codecContext);

//成员变量 属性
public:
    //是否正在播放
    int isPlay;
    //流的索引
    int index;
    //音频队列 queue 系统的
    std::queue<AVPacket *> queue;
    //处理线程
    pthread_t p_playid;

    //解码器上下文
    AVCodecContext *codec;

    //加锁
    pthread_mutex_t mutex;
    //条件变量
    pthread_cond_t cond;

};
#endif //FFMPEGDEMO_FFMPEGAUDIO_H