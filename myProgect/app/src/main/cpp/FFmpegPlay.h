//
// Created by liuml on 2018/9/15.
//

#ifndef FFMPEGDEMO_FFMPEGPLAY_H
#define FFMPEGDEMO_FFMPEGPLAY_H

#include "FFmpegAudio.h"
#include "FFmpegVideo.h"
#include "FFmpegMusic.h"
#include "Log.h"
#include "android/native_window_jni.h"

#endif //FFMPEGDEMO_FFMPEGPLAY_H

void play(char *url);

void stop();

void dispaly(JNIEnv *env, jobject instance, jobject surface);