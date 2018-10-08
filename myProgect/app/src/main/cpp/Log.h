//
// Created by liuml on 2018/9/17.
//

#ifndef FFMPEGDEMO_LOG_H
#define FFMPEGDEMO_LOG_H
#include <android/log.h>
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"jnilib",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"jnilib",FORMAT,##__VA_ARGS__);
#endif //FFMPEGDEMO_LOG_H
