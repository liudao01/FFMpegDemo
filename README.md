# FFMpegDemo
ffmpeg play video from android


# 1 利用FFMpeg进行MP3视频转YUV格式

## 理论:

YUV，是一种颜色编码方法

详细看这里

https://blog.csdn.net/junzia/article/details/76315120

为什么需要转yuv格式

现在绝大多数视频解码后播放的格式都是YUV 所以需要做下YUV格式

一个通道.
前面放Y 后面放UV  比例是 4:1:1

视频yuv 音频 是pcm

YUV
- “Y”表示明亮度（Luminance、Luma），“U”和“V”则是色度、浓度（Chrominance、Chroma）。

封装格式: MP4 rmvb等
编码格式: 对应响应的编码 MPEG2,MPEG4,H.264

视频播放一般有两个 流  音频流 视频流, 有时还有个一流 是字幕流

### 下面开始预先操作

1. 把ffmpeg 的so库 还有头文件导入到android studio 工程中去.
2. 在native-lib中导入头文件.
```
extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
}
```
3. 在手机根目录放个mp4视频

视频信息
![mark](http://ovji4jgcd.bkt.clouddn.com/blog/180903/KAKFf4f4EL.png?imageslim)

可以看到:
- 封装格式mp4
- 编码格式avc1格式

大致流程图:

![mark](http://ovji4jgcd.bkt.clouddn.com/blog/180904/LAc070eE40.png?imageslim)

## 代码

注释都有


```c
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
}

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"jnilib",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"jnilib",FORMAT,##__VA_ARGS__);
JNICALL extern "C"
JNIEXPORT void JNICALL
Java_androidrn_ffmpegdemo_MainActivity_openVideo(JNIEnv *env, jobject instance, jstring inputStr_,
                                                 jstring outStr_) {
    const char *inputStr = env->GetStringUTFChars(inputStr_, 0);
    const char *outStr = env->GetStringUTFChars(outStr_, 0);

    //无论编码还是解码 都要调用这个 注册各大组件
    av_register_all();

    //获取AVFormatContext  比特率 时长 文件路径 流的信息(nustream) 都封装在这里面
    AVFormatContext *pContext = avformat_alloc_context();

    //AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options
    //上下文  文件名  打开文件格式 获取信息(AVDictionary)  凡是AVDictionary字典 都是获取视频文件信息
    if (avformat_open_input(&pContext, inputStr, NULL, NULL) < 0) {
        LOGE("打开失败");
        return;
    }

    //给nbstram填充信息
    if (avformat_find_stream_info(pContext, NULL) < 0) {
        LOGE("获取信息失败");
        return;
    }

    //找到视频流
    int video_stream_ids = -1;
    for (int i = 0; i < pContext->nb_streams; ++i) {
        LOGE("循环 %d", i);
        //如果填充的视频流信息 -> 编解码,解码器 -> 流的类型 == 视频的类型
        //codec 每一个流 对应的解码上下文 codec_type 流的类型
        if (pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_ids = i;
        }
    }
    // 获取到解码器上下文
    AVCodecContext *pCodecCtx = pContext->streams[video_stream_ids]->codec;
    //解码器
    AVCodec *pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
    //打开解码器  为什么avcodec_open2 版本升级的原因
    if (avcodec_open2(pCodecCtx, pCodex, NULL) < 0) {
        LOGE("解码失败");
        return;
    }

    //得到解封装 读取 解封每一帧 读取每一帧的压缩数据
    //初始化avpacket 分配内存  FFMpeg 没有自动分配内存 必须手动分匹配手动释放  不过有分配函数
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    //初始化packet  每一个包是一个完整的数据帧,来暂存解复用之后、解码之前的媒体数据（一个音/视频帧、一个字幕包等）及附加信息（解码时间戳、显示时间戳、时长等）
    av_init_packet(packet);

    //初始化AVFrame
    AVFrame *frame = av_frame_alloc();

    //目的数据 YUV的frame 声明yuvFrame
    AVFrame *yuvFrame = av_frame_alloc();
    //给yuv 缓冲区初始化
    //avpicture_get_size 计算给定宽度和高度的图片的字节大小 如果以给定的图片格式存储。
    // uint8_t 8位无符号整型数(int)
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size((AV_PIX_FMT_YUV420P), pCodecCtx->width, pCodecCtx->height));

    //填充YUM缓冲区
    int re = avpicture_fill(reinterpret_cast<AVPicture *>(yuvFrame), out_buffer, AV_PIX_FMT_YUV420P,
                            pCodecCtx->width, pCodecCtx->height);

    LOGE("宽 %d  高 %d",pCodecCtx->width,pCodecCtx->height);

    //获取转化的上下文   参数 通过解码器的上下文获取到   pCodecCtx->pix_fmt 是mp4的上下文
    SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                            SWS_BILINEAR, NULL, NULL, NULL);

    int frame_count = 0;

    //目的转化成YUV  写到一个文件里面去 声明文件
    FILE *fp_yuv = fopen(outStr, "wb");


    //AVFormatContext *s, AVPacket *pkt  上下文,avpacket 是数据包的意思
    //packet 入参
    int got_frame;
    //解码每一帧图片
    while (av_read_frame(pContext, packet) >= 0) {
//        解封装
        //参数
        //AVCodecContext *avctx, 解码器的上下文
        // AVFrame *picture, 已经解封装的frame,是直接能在手机上播放的frame
//        int *got_picture_ptr, 入参出参对象, 可以根据出参判断是否解析完成
//        const AVPacket *avpkt 压缩的packet数据


        //把packet压缩的数据 解压 赋给frame
        avcodec_decode_video2(pCodecCtx, frame, &got_frame, packet);

        //判断是否读完
        if (got_frame > 0) {
            LOGE("解码第 %d 个 frame ",frame_count++);
            // 获取到了frame数据  视频像素的数据
            // 目的转化为yuv格式    sws_scale 函数
            sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                      frame->linesize, 0, frame->height, yuvFrame->data, yuvFrame->linesize);

            //得到所有的大小 宽度乘以高度  解释: 在R G B 中有多少像素 就是宽度乘以高度, 在YUV中 有多少像素是由Y决定的 如果只有Y 那么只有亮度 就是黑白的
            int y_size = pCodecCtx->width * pCodecCtx->height;
            //Y亮度信息
            fwrite(yuvFrame->data[0], 1, y_size, fp_yuv);
            //色度
            fwrite(yuvFrame->data[1], 1, y_size / 4, fp_yuv);
            //浓度
            fwrite(yuvFrame->data[2], 1, y_size / 4, fp_yuv);
        }
        //释放
        av_free_packet(packet);

    }
    fclose(fp_yuv);
    av_frame_free(&yuvFrame);
    av_frame_free(&frame);
    avcodec_close(pCodecCtx);
    avformat_free_context(pContext);

    env->ReleaseStringUTFChars(inputStr_, inputStr);
    env->ReleaseStringUTFChars(outStr_, outStr);
}








```

## 结果

![mark](http://ovji4jgcd.bkt.clouddn.com/blog/180903/GkJ6CeIfBK.png?imageslim)
![mark](http://ovji4jgcd.bkt.clouddn.com/blog/180903/fjkgC8f28j.png?imageslim)
![mark](http://ovji4jgcd.bkt.clouddn.com/blog/180903/K86HHa2h7d.png?imageslim)
---
- sws_getContext 函数解析

```
// 初始化sws_scale
struct SwsContext *sws_getContext(int srcW, int srcH, enum AVPixelFormat srcFormat,
                                  int dstW, int dstH, enum AVPixelFormat dstFormat,
                                  int flags,
                                  SwsFilter *srcFilter, SwsFilter *dstFilter, const double *param);
参数int srcW, int srcH, enum AVPixelFormat srcFormat定义输入图像信息（寬、高、颜色空间（像素格式））
参数int dstW, int dstH, enum AVPixelFormat dstFormat定义输出图像信息寬、高、颜色空间（像素格式））。
参数int flags选择缩放算法（只有当输入输出图像大小不同时有效）
参数SwsFilter *srcFilter, SwsFilter *dstFilter分别定义输入/输出图像滤波器信息，如果不做前后图像滤波，输入NULL
参数const double *param定义特定缩放算法需要的参数(?)，默认为NULL
函数返回SwsContext结构体，定义了基本变换信息。
如果是对一个序列的所有帧做相同的处理，函数sws_getContext只需要调用一次就可以了。
sws_getContext(w, h, YV12, w, h, NV12, 0, NULL, NULL, NULL);      // YV12->NV12 色彩空间转换
sws_getContext(w, h, YV12, w/2, h/2, YV12, 0, NULL, NULL, NULL);  // YV12图像缩小到原图1/4
sws_getContext(w, h, YV12, 2w, 2h, YN12, 0, NULL, NULL, NULL);    // YV12图像放大到原图4倍，并转换为NV12结构

```


```
 sws_scale
 缩放SRCPLACE中的图像切片并将其缩放

需要注意的是第四个参数srcSliceY 这个代表的是第一列要处理的位置，如果要从头开始处理，直接填0即可
int sws_scale(struct SwsContext *c, const uint8_t *const srcSlice[],
              const int srcStride[], int srcSliceY,int srcSliceH,
              uint8_t *const dst[], const int dstStride[]);
*

 -C以前创建的缩放上下文

* SWSYGETCONTRONTHECT（）

*@ PARAM SRCPLACE包含数组指向平面的指针

*源切片

*@ PARAM SrcReST包含每个平面的步幅的数组

*源图像

*@ PARAM SRCLSICY在切片的源图像中的位置

*进程，即数字（从开始计数）

*0）在切片的第一行的图像中

*@ PARAM SRCLSICH源数组的高度，即

*切片中的行

*@ PARAM DST包含指向平面的指针的数组

＊目的地形象

*@ PARAM-DSSTRIDE包含每个平面的步长的数组

＊目的地形象

*@返回输出条的高度

```
---


# 利用surfaceview播放

## 理论

解码部分和上面的代码基本上差不多.解码出来会得到了rgbframe  (和yuvframe一样 不过格式不是YUV格式,因为surfaceview不能识别YUV,只能识别RGB.).
主要问题是解码后如何给surfaceview播放. ? c++ 里面是利用 ANativeWindow 播放. ANativeWindow 播放需要一个缓冲区buffer.
这个buffer 可以通过rgbframe 一行一行的写入缓冲区(ANativeWindow的缓冲区).

导入头文件:

#include <android/native_window_jni.h> 导入头文件后需要导入so库实现, 在cmakelists里面加上 可以和log 库导入一样,使用find_library,也可以直接在target_link_libraries加上android

```
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
}

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"jnilib",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"jnilib",FORMAT,##__VA_ARGS__);

extern "C"
JNIEXPORT void JNICALL
Java_androidrn_ffmpegdemo_MyVideoView_render(JNIEnv *env, jobject instance, jstring input_,
                                             jobject surface) {
    const char *input = env->GetStringUTFChars(input_, 0);

    // TODO
    //无论编码还是解码 都要调用这个 注册各大组件
    av_register_all();

    //获取AVFormatContext  比特率 时长 文件路径 流的信息(nustream) 都封装在这里面
    AVFormatContext *pContext = avformat_alloc_context();

    //AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options
    //上下文  文件名  打开文件格式 获取信息(AVDictionary)  凡是AVDictionary字典 都是获取视频文件信息
    if (avformat_open_input(&pContext, input, NULL, NULL) < 0) {
        LOGE("打开失败");
        return;
    }

    //给nbstram填充信息
    if (avformat_find_stream_info(pContext, NULL) < 0) {
        LOGE("获取信息失败");
        return;
    }

    //找到视频流
    int video_stream_ids = -1;
    for (int i = 0; i < pContext->nb_streams; ++i) {
        LOGE("循环 %d", i);
        //如果填充的视频流信息 -> 编解码,解码器 -> 流的类型 == 视频的类型
        //codec 每一个流 对应的解码上下文 codec_type 流的类型
        if (pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_ids = i;
            break;
        }
    }
    // 获取到解码器上下文 获取视频编解码器
    AVCodecContext *pCodecCtx = pContext->streams[video_stream_ids]->codec;
    //解码器
    AVCodec *pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
    //打开解码器  为什么avcodec_open2 版本升级的原因
    if (avcodec_open2(pCodecCtx, pCodex, NULL) < 0) {
        LOGE("解码失败");
        return;
    }

    //得到解封装 读取 解封每一帧 读取每一帧的压缩数据
    //初始化avpacket 分配内存  FFMpeg 没有自动分配内存 必须手动分匹配手动释放  不过有分配函数
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    //初始化packet  每一个包是一个完整的数据帧,来暂存解复用之后、解码之前的媒体数据（一个音/视频帧、一个字幕包等）及附加信息（解码时间戳、显示时间戳、时长等）
//    av_init_packet(packet);

    //初始化AVFrame
    AVFrame *frame = av_frame_alloc();

    //目的数据 RGB的frame 声明RGBFrame
    AVFrame *rgbFrame = av_frame_alloc();
    //给RGB 缓冲区初始化 分配内存
    //avpicture_get_size 计算给定宽度和高度的图片的字节大小 如果以给定的图片格式存储。
    // uint8_t 8位无符号整型数(int)
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size((AV_PIX_FMT_RGBA), pCodecCtx->width, pCodecCtx->height));

    LOGE("render 宽 %d  render 高 %d", pCodecCtx->width, pCodecCtx->height);

//    //填充rgb缓冲区
    int re = avpicture_fill(reinterpret_cast<AVPicture *>(rgbFrame), out_buffer, AV_PIX_FMT_RGBA,
                            pCodecCtx->width, pCodecCtx->height);
    LOGE("申请YUM内存%d   ", re);


    //获取转化的上下文   参数 通过解码器的上下文获取到   pCodecCtx->pix_fmt 是mp4的上下文  SWS_BICUBIC 是级别 越往下效率越高清晰度越低
    //AV_PIX_FMT_RGBA  这个是需要转化的格式
    SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGBA,
                                            SWS_BICUBIC, NULL, NULL, NULL);
    //AVFormatContext *s, AVPacket *pkt  上下文,avpacket 是数据包的意思
    //packet 入参
    int got_frame;

    int length = 0;

    //获取ANtiviewindow  ANativeWindow_fromSurface
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    //声明buffer 视频的缓冲区   rgbFrame 的缓冲区
    ANativeWindow_Buffer outBuffer;

    int frame_count = 0;
    //解码每一帧图片
    while (av_read_frame(pContext, packet) >= 0) {
//        解封装
        //参数
        //AVCodecContext *avctx, 解码器的上下文
        // AVFrame *picture, 已经解封装的frame,是直接能在手机上播放的frame
//        int *got_picture_ptr, 入参出参对象, 可以根据出参判断是否解析完成
//        const AVPacket *avpkt 压缩的packet数据

        if (packet->stream_index == video_stream_ids) {

            //把packet压缩的数据 解压 赋给默认frame
            length = avcodec_decode_video2(pCodecCtx, frame, &got_frame, packet);
            LOGE(" 获得长度   %d ", length);

            //判断是否读完
            if (got_frame) {
                LOGE("解码第 %d 个 frame ", frame_count++);
                //绘制之前 需要配置一些信息. 宽高 输出的格式  可以修改 pCodecCtx->width 和height
                ANativeWindow_setBuffersGeometry(nativeWindow, pCodecCtx->width, pCodecCtx->height,
                                                 WINDOW_FORMAT_RGBA_8888);//AV_PIX_FMT_RGBA  这个也是上面转化的格式

                //因为surfaceview 只支持RGB所以 这里不转成YUV, 电视机 机顶盒都是YUV
                //TODO buffer很重要
                //先锁定当前的window  和surfaceview 类似 需要先锁定 , 第一个参数就是上面获取的ANtiviewindow, 第二个参数是buffer 这个buffer非常重要他是需要绘制的缓冲区.
                ANativeWindow_lock(nativeWindow, &outBuffer, NULL);

                // 调用转化方式,目的转化为RGB格式    sws_scale 函数
                sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                          frame->linesize, 0, pCodecCtx->height, rgbFrame->data,
                          rgbFrame->linesize);

                //获取window 被输出的画面的首地址  window首地址
                uint8_t *dst = static_cast<uint8_t *>(outBuffer.bits);

                //拿到一行有多少字节  为什么需要×4  rgba
                int destStride = outBuffer.stride * 4;//这里就是window的一行数据 字节

                //获取rgbframe(像素数据)的 首地址
                uint8_t *src = rgbFrame->data[0];
                //实际内存一行的数量
                int srcStride = rgbFrame->linesize[0];
                LOGE("实际内存第一行一行的数量 = %d", srcStride);
                //把rgbframe拷贝到缓冲区  到解码的高度
                for (int i = 0; i < pCodecCtx->height; ++i) {

                    //dest 目的地址 src 源地址    n 数量
                    //void *memcpy(void *dest, const void *src, size_t n);
                    //从源src所指的内存地址的起始位置 开始拷贝n个字节 到目标dest 所指的内存地址的起始位置中
                    //window首地址  变化的  真正拷贝的字节数 srcStride
                    memcpy(dst + i * destStride,
                           reinterpret_cast<const void *>(src + i * srcStride),
                           srcStride);
                }//这个循环完成就完成拷贝了.
                LOGE("解锁画布 睡眠16毫秒 ");
                //解锁画布
                ANativeWindow_unlockAndPost(nativeWindow);
                //画面绘制完之后 画面停止16毫秒   usleep在#include <unistd.h> 中
                usleep(1000 * 16);
            }
        }
        //释放
        av_free_packet(packet);
    }

    ANativeWindow_release(nativeWindow);

    av_frame_free(&rgbFrame);
    av_frame_free(&frame);
    avcodec_close(pCodecCtx);
    avformat_free_context(pContext);


    env->ReleaseStringUTFChars(input_, input);
}
```

结果:



![image](![mark](http://ovji4jgcd.bkt.clouddn.com/blog/180905/K9L81K7JK5.gif))