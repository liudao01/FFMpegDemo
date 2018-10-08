//
// Created by liuml on 2018/9/15.
//

#ifndef FFMPEGDEMO_FFMPEGVIDEO_H
#define FFMPEGDEMO_FFMPEGVIDEO_H
#endif //FFMPEGDEMO_FFMPEGVIDEO_H

#include <queue>
#include "unistd.h"
#include "FFmpegAudio.h"

extern "C"
{
#include <pthread.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include "Log.h"
#include <libavcodec/avcodec.h>

#include <unistd.h>
#include "libavutil/imgutils.h"
#include "libavutil/time.h"

class FFmpegVideo {
public:
    FFmpegVideo();

    ~FFmpegVideo();

    int get(AVPacket *packet);

    int put(AVPacket *packet);

    void play();

    void stop();

    void setPlayCall(void(*call)(AVFrame *frame));
    void setFFmpegAudio(FFmpegAudio *audio1);
    void setAvCodecContext(AVCodecContext *codecContext);
    double synchronize(AVFrame *frame, double play);

public:
    //    是否正在播放
    int isPlay;
//    流索引
    int index;
//音频队列
    std::queue<AVPacket *> queue;
//    处理线程
    pthread_t p_playid;
//    解码器上下文
    AVCodecContext *codec;

    AVRational time_base;
//    同步锁
    pthread_mutex_t mutex;
//    条件变量
    pthread_cond_t cond;
    double clock;
    FFmpegAudio *audio;
};

};