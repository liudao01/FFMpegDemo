//
// Created by liuml on 2018/9/15.
//

#include "FFmpegVideo.h"

FFmpegVideo::FFmpegVideo() {
    //构造方法里面实例化同步锁,
    pthread_mutex_init(&mutex, NULL);
    //条件变量
    pthread_cond_init(&cond, NULL);
}

FFmpegVideo::~FFmpegVideo() {

}

int FFmpegVideo::get(AVPacket *packet) {
    //锁住
    pthread_mutex_lock(&mutex);

    //取出视频帧
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

int FFmpegVideo::put(AVPacket *packet) {
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
void *play_video(void *arg){
    FFmpegVideo *vedio = (FFmpegVideo *) arg;
    //像素数据（解码数据）
    AVFrame *frame = av_frame_alloc();

    AVFrame *rgb_frame = av_frame_alloc();
    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA, vedio->codec->width,
                                                                  vedio->codec->height));
    //设置yuvFrame的缓冲区，像素格式
    avpicture_fill((AVPicture *)rgb_frame, out_buffer
            , AV_PIX_FMT_RGBA, vedio->codec->width, vedio->codec->height);
    //native绘制
    // 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
    struct SwsContext *sws_ctx = sws_getContext(vedio->codec->width,
                                                vedio->codec->height,
                                                vedio->codec->pix_fmt,
                                                vedio->codec->width,
                                                vedio->codec->height,
                                                AV_PIX_FMT_RGBA,
                                                SWS_BICUBIC,
                                                NULL,
                                                NULL,
                                                NULL);
    int len ,got_frame, framecount = 0;
    LOGE("宽  %d ,高  %d ",vedio->codec->width,vedio->codec->height);
    //编码数据
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    //6.一阵一阵读取压缩的视频数据AVPacket
    while (vedio->isPlay) {
        LOGE("视频 解码  一帧");
//        消费者取到一帧数据  没有 阻塞 vedio->get
        vedio->get(packet);
        len = avcodec_decode_video2(vedio->codec,frame, &got_frame, packet);
//        转码成rgb
        sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0,
                  vedio->codec->height,
                  rgb_frame->data, rgb_frame->linesize);

//        得到了rgb_frame  绘制   frame  rgb     pcm  frame




    }


}
void FFmpegVideo::play() {
    isPlay = 1;
    pthread_create(&p_playid, 0, play_video, this);

}

void FFmpegVideo::stop() {

}

void FFmpegVideo::setAvCodecContext(AVCodecContext *codecContext) {
    codec = codecContext;
}
