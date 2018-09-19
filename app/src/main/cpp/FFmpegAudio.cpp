//
// Created by liuml on 2018/9/15.
//

#include "FFmpegAudio.h"


//消费者
int FFmpegAudio::get(AVPacket *packet) {
    //锁住
    pthread_mutex_lock(&mutex);

    //取出音频帧
    while (isPlay) {
        if (!quequ.empty()) {
            LOGE("取出队列packet")
            //从队列取出一个packet,clone一个 给入参对象. quequ.front() 返回对队列中第一个元素的引用。此元素将是调用pop（）时要删除的第一个元素
            if (av_packet_ref(packet, quequ.front()) < 0) {
                //取出失败
                break;
            } else {
                //取出成功 出队 销毁packet
                AVPacket *pkt = quequ.front();
                quequ.pop();
                av_free_packet(pkt);
                break;
            }
        } else {
            //队列为空 阻塞等待
            pthread_cond_wait(&cond, &mutex);
        }
    }


    return 1;
}

//生产者
int FFmpegAudio::put(AVPacket *packet) {
    //入队  传参一般要新建一个赋值 因为原来的参数有可能销毁.
    AVPacket *packet1 = (AVPacket *) (av_malloc(sizeof(AVPacket)));
    //克隆一下
    if (av_packet_ref(packet1, packet)) {
        //克隆失败
        return 0;
    }

    //锁住
    pthread_mutex_lock(&mutex);
    LOGE("入队压入一帧数据");
    quequ.push(packet1);
    //发通知
    pthread_cond_signal(&cond);

    //给消费者解锁
    pthread_mutex_unlock(&mutex);
    return 1;
}

//线程的函数播放
void *play_audio(void *arg) {
    LOGE("开启音频线程");
    FFmpegAudio *audio = (FFmpegAudio *) arg;
    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //    mp3  里面所包含的编码格式   转换成  pcm
    SwrContext *swrContext = swr_alloc();
    int length = 0;
    //    输出采样位数
    //输出的采样率必须与输入相同

    swr_alloc_set_opts(swrContext, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16,
                       audio->codec->sample_rate,
                       audio->codec->channel_layout, audio->codec->sample_fmt,
                       audio->codec->sample_rate, 0,
                       0);
    swr_init(swrContext);
    uint8_t *out_buffer = (uint8_t *) av_malloc(44100 * 2 * 2);
    //获取通道数
    int channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    int got_frame;
    //轮询去取音频帧
    while (audio->isPlay) {
        //取音频帧
        audio->get(packet);
        //把packet数据转化成frame
        avcodec_decode_audio4(audio->codec, frame, &got_frame, packet);
        if (got_frame) {
            LOGE("解码音频帧");
            //把frame 数据转换到缓冲区里面来
            swr_convert(swrContext, &out_buffer, 44100 * 2 * 2,
                        (const uint8_t **) (frame->data), frame->nb_samples);
            //求出音频大小
            int out_buffer_siza = av_samples_get_buffer_size(NULL, channels, frame->nb_samples,
                                                             AV_SAMPLE_FMT_S16, 1);
            //得到了pcm的out_butter缓冲区 可以播放了

        }
    }
    LOGE("初始化FFmpeg完毕");
    return 0;
}

void FFmpegAudio::play() {
    isPlay = 1;
    //开启一个解码音频的线程
    pthread_create(&p_playid, NULL, play_audio, this);
}

void FFmpegAudio::stop() {

}

void FFmpegAudio::setAvCodecContext(AVCodecContext *codecContext) {
    codec = codecContext;
}

FFmpegAudio::FFmpegAudio() {
    //构造方法里面实例化同步锁,
    pthread_mutex_init(&mutex, NULL);
    //条件变量
    pthread_cond_init(&cond, NULL);
}

FFmpegAudio::~FFmpegAudio() {

}
