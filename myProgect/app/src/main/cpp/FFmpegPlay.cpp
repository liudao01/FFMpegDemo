//
// Created by liuml on 2018/9/15.
//

#include "FFmpegPlay.h"

const char *path = NULL;
FFmpegVideo *video;
FFmpegAudio *audio;

pthread_t p_tid;//解码线程

int isPlay = 0; //播放状态

ANativeWindow *window = 0;

//播放回调函数
void call_video_play(AVFrame *frame) {
    LOGE("设置播放回调函数");
    if (!window) {
        return;
    }
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        return;
    }

    LOGE("绘制 宽%d,高%d", frame->width, frame->height);
    LOGE("绘制 宽%d,高%d  行字节 %d ", window_buffer.width, window_buffer.height, frame->linesize[0]);
    uint8_t *dst = (uint8_t *) window_buffer.bits;
    int dstStride = window_buffer.stride * 4;
    uint8_t *src = frame->data[0];
    int srcStride = frame->linesize[0];
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(window);
}


//解码函数
void *process(void *args) {
    //解码
    LOGE("开启解码线程");
    //1.注册组件
    av_register_all();
    avformat_network_init();//和本地的不同
    LOGE("avformat_network_init");
    //封装格式上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    LOGE("封装格式上下文");
    while (path==NULL){
        sleep(1);
    }
    LOGE("解码函数 视频路径path %s", path);

    //2.打开输入视频文件
    if (avformat_open_input(&pFormatCtx, path, NULL, NULL) != 0) {
        LOGE("%s", "打开输入视频文件失败");
    } else {
        LOGE("打开输入视频文件成功");
    }

    LOGE(" 看下pFormatCtx  = %p", &pFormatCtx)
    //    LOGE(" 看下pFormatCtx 2  = %p", pFormatCtx)
    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
    } else {
        LOGE("获取视频信息成功");
    }

    //视频解码，需要找到视频和音频对应的AVStream所在pFormatCtx->streams的索引位置
    int i = 0;
    LOGE("看下有几个流 %d", pFormatCtx->nb_streams)
    for (; i < pFormatCtx->nb_streams; i++) {
        //4.获取视频解码器
        AVCodecContext *pCodeCtx = pFormatCtx->streams[i]->codec;
        AVCodec *pCodec = avcodec_find_decoder(pCodeCtx->codec_id);
        if (avcodec_open2(pCodeCtx, pCodec, NULL)) {
            LOGE("解码成功");
        } else {
            LOGE("解码失败");
        }
        //根据类型判断，是否是视频流
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            /*找到视频流*/
            video->setAvCodecContext(pCodeCtx);
            video->index = i;
            //当前是第几份pts
            video->time_base = pFormatCtx->streams[i]->time_base;
            LOGE("找到视频流");
            if (window)
                ANativeWindow_setBuffersGeometry(window, video->codec->width,
                                                 video->codec->height,
                                                 WINDOW_FORMAT_RGBA_8888);

        } else if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            //找到音频流
            audio->setAvCodecContext(pCodeCtx);
            //
            audio->index = i;
            audio->time_base = pFormatCtx->streams[i]->time_base;
            LOGE("找到音频流");
        }

    }

    //开启音频 视频  循环播放
    video->play();
//    audio->play();
//    audio.
    isPlay = 1;
    //解码Packet
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //子线程 解码
    int ret;
    while (isPlay) {
        ret = av_read_frame(pFormatCtx, packet);
        if (ret == 0) {

            //判断packet 的流索引 和视频流索引相等 那么添加到视频队列中
            if (video && video->isPlay && packet->stream_index == video->index) {
                LOGE("如果是视频流 添加到视频队列中");
                video->put(packet);
            } else if (audio && audio->isPlay && packet->stream_index == audio->index) {
                //如果是音频流 同样添加到音频队列中
                LOGE("如果是音频流 添加到音频队列中");

//                audio->put(packet);
            }
//            sleep(1);//睡眠1秒看log
            //音频的间隔 设定26毫秒
//            usleep(26 * 1000);
            //销毁packet产生的内存
            av_packet_unref(packet);
            LOGE("地址解码中");
        } else {
            LOGI("解码完了");
        }
    }
    //视频解码完成 可能视频播放完 也可能视频没播放玩
    isPlay = 0;
    //先播放
    if (video && video->isPlay) {
        video->stop();
    }
    if (audio && audio->isPlay) {
        audio->stop();
    }
    LOGE("全部视频解码完成");
    //播放完了再释放
    av_free_packet(packet);
    avformat_free_context(pFormatCtx);
    pthread_exit(0);


}

void play(char *url) {
    LOGE("play里面的路径 %s", url);
    path = url;
    LOGE("play里面的路径赋值后 %s", path);
    //实例化对象
    video = new FFmpegVideo();
    audio = new FFmpegAudio();
    LOGE("开始设置播放回调函数");
    video->setPlayCall(call_video_play);
    //开启解码线程
    pthread_create(&p_tid, NULL, process, NULL);


}

//绘制音视频同步的图像
void dispaly(JNIEnv *env, jobject instance, jobject surface) {
    // 在这里绘制

    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env, surface);
    if (video && video->codec) {
        ANativeWindow_setBuffersGeometry(window, video->codec->width, video->codec->height,
                                         WINDOW_FORMAT_RGBA_8888);
    }
}

void stop() {

}