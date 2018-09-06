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

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"jnilib",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"jnilib",FORMAT,##__VA_ARGS__);

//mp3声音转化pcm
extern "C"
JNIEXPORT void JNICALL
Java_androidrn_ffmpegdemo_AudioPlayer_changeSound(JNIEnv *env, jobject instance, jstring input_,
                                                jstring output_) {
    const char *input = env->GetStringUTFChars(input_, 0);
    const char *output = env->GetStringUTFChars(output_, 0);

    //前面和之前一样 获取输入的信息 但是现在是找到音频的AVMEDIA_TYPE_AUDIO
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
    int audio_stream_ids = -1;
    for (int i = 0; i < pContext->nb_streams; ++i) {
        LOGE("循环 %d", i);
        //如果填充的视频流信息 -> 编解码,解码器 -> 流的类型 == 视频的类型
        //codec 每一个流 对应的解码上下文 codec_type 流的类型
        if (pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_ids = i;
            LOGE("找到音频id %d", i);
            break;
        }
    }

    //解码器. mp3的解码器 解码器
    // ----------  获取到解码器上下文 获取视音频解码器
    AVCodecContext *pCodecCtx = pContext->streams[audio_stream_ids]->codec;
    LOGE("获取解码器上下文")
    //----------解码器
    AVCodec *pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
    //打开解码器  为什么avcodec_open2 版本升级的原因
    if (avcodec_open2(pCodecCtx, pCodex, NULL) < 0) {
        LOGE("解码失败");
        return;
    }

    //得到解封装 读取 解封每一帧 读取每一帧的压缩数据
    //初始化avpacket 分配内存  FFMpeg 没有自动分配内存 必须手动分匹配手动释放  不过有分配函数
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    //初始化AVFrame
    AVFrame *frame = av_frame_alloc();

    //mp3 里面所包含的编码格式 转化成pcm
    //#include <libswresample/swresample.h>
    SwrContext *swrContext = swr_alloc();//获取转换的上下文

    int frame_count = 0;

    //目的转化成YUV  写到一个文件里面去 声明文件
    FILE *pcm_file = fopen(output, "wb");


    //AVFormatContext *s, AVPacket *pkt  上下文,avpacket 是数据包的意思
    //packet 入参
    int got_frame;

    int length = 0;

    //定义缓冲区输出的  需要多大的采样率  采样 44100  多少个字节.  双通道需要乘以2
    uint8_t  *out_buffer =  static_cast<uint8_t *>(av_malloc(44100 * 2));//一秒的缓冲区数量


    /**
     * struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
     *                                  //输出的
                                      int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                                      //输入的
                                      int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
                                      //偏移量
                                      int log_offset, void *log_ctx);

     */
    //输出声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;//立体声
    //输出采样位数 16位 现在基本都是16位  位数越高 声音越清晰
    enum  AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;
    //输出的采样率 必须与输入的相同
    int out_sample_rate = pCodecCtx->sample_rate;



    //https://blog.csdn.net/explorer_day/article/details/76332556  文档
    //swr_alloc_set_opts将PCM源文件的采样格式转换为自己希望的采样格式
    swr_alloc_set_opts(swrContext, out_ch_layout, out_formart, out_sample_rate,
                       pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate,
                       0, NULL);

    //初始化转换器
    swr_init(swrContext);

    //求出通道数
    int out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);

    //读取frame
    while (av_read_frame(pContext,packet)>=0){
        if(packet->stream_index == audio_stream_ids){
            //解码  现在编码格式frame 需要转化成pcm
            avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
            if(got_frame){
                //真正的解码
                LOGE("开始解码");
                //转换得到out_buffer
                swr_convert(swrContext, &out_buffer, 44100 * 2,
                            reinterpret_cast<const uint8_t **>(frame->data), frame->nb_samples);

                //求缓冲区实际的大小  通道数  frame->nb_samples 采样的点
                int size = av_samples_get_buffer_size(NULL,out_channer_nb,frame->nb_samples,
                                           AV_SAMPLE_FMT_S16,1);
                //写入到文件
                fwrite(out_buffer,1,size,pcm_file);
            }
        }

    }


    //回收
    fclose(pcm_file);
    av_frame_free(&frame);
    avcodec_close(pCodecCtx);
    avformat_free_context(pContext);

    env->ReleaseStringUTFChars(input_, input);
    env->ReleaseStringUTFChars(output_, output);
}
//播放视频
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
            LOGE("找到视频id %d", i);
            break;
        }
    }
    // ----------  获取到解码器上下文 获取视频编解码器
    AVCodecContext *pCodecCtx = pContext->streams[video_stream_ids]->codec;
    //----------解码器
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

// MP4转化YUV
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

    LOGE("宽 %d  高 %d", pCodecCtx->width, pCodecCtx->height);

    //填充YUM缓冲区
    int re = avpicture_fill(reinterpret_cast<AVPicture *>(yuvFrame), out_buffer, AV_PIX_FMT_YUV420P,
                            pCodecCtx->width, pCodecCtx->height);
    LOGE("申请YUM内存%d   ", re);


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

    int length = 0;
    //解码每一帧图片
    while (av_read_frame(pContext, packet) >= 0) {
//        解封装
        //参数
        //AVCodecContext *avctx, 解码器的上下文
        // AVFrame *picture, 已经解封装的frame,是直接能在手机上播放的frame
//        int *got_picture_ptr, 入参出参对象, 可以根据出参判断是否解析完成
//        const AVPacket *avpkt 压缩的packet数据


        //把packet压缩的数据 解压 赋给frame
        length = avcodec_decode_video2(pCodecCtx, frame, &got_frame, packet);
        LOGE(" 获得长度   %d ", length);

        //判断是否读完
        if (got_frame > 0) {
            LOGE("解码第 %d 个 frame ", frame_count++);
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



