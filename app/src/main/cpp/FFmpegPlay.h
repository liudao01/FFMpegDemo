//
// Created by liuml on 2018/9/15.
//

#ifndef FFMPEGDEMO_FFMPEGPLAY_H
#define FFMPEGDEMO_FFMPEGPLAY_H
extern "C" {

#include "FFmpegAudio.h"
#include "FFmpegVideo.h"
#include "FFmpegMusic.h"
#include "Log.h"

};
#endif //FFMPEGDEMO_FFMPEGPLAY_H

void play(char *url);

void stop();