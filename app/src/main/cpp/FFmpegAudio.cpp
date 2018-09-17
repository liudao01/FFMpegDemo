//
// Created by liuml on 2018/9/15.
//

#include "FFmpegAudio.h"
#include "Log.h"

int FFmpegAudio::get(AVPacket *packet) {
    return 0;
}

int FFmpegAudio::put(AVPacket *packet) {
    //入队  传参一般要新建一个赋值 因为原来的参数有可能销毁.
    AVPacket *packet1 = (AVPacket *) (av_malloc(sizeof(AVPacket)));
    //克隆一下
    if (av_packet_ref(packet1, packet)) {
        //克隆失败
        return 0;
    }

    LOGE("入队压入一帧数据");
    quequ.push(packet1);
    return 0;
}

void FFmpegAudio::play() {

}

void FFmpegAudio::stop() {

}

void FFmpegAudio::setAvCodecContext(AVCodecContext *codecContext) {

}

FFmpegAudio::FFmpegAudio() {

}

FFmpegAudio::~FFmpegAudio() {

}
