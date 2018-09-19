//
// Created by liuml on 2018/9/15.
//

#ifndef FFMPEGDEMO_FFMPEGVIDEO_H
#define FFMPEGDEMO_FFMPEGVIDEO_H

#endif //FFMPEGDEMO_FFMPEGVIDEO_H
extern "C"
{
#include <pthread.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include <libavcodec/avcodec.h>
#include <sys/types.h>
#include <queue>
#include "Log.h"

class FFmpegVideo {
public:
    FFmpegVideo();

    ~FFmpegVideo();

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
    std::queue<AVPacket *> quequ;
    //处理线程
    pthread_t p_playid;

    //解码器上下文
    AVCodecContext *codec;

    //加锁
    pthread_mutex_t mutex;
    //条件变量
    pthread_cond_t cond;
};
};