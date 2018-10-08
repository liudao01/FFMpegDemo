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

//回调
static void (*video_call)(AVFrame *frame);
int FFmpegVideo::get(AVPacket *packet) {
    LOGE("取数据函数")
    //锁住
    pthread_mutex_lock(&mutex);

    //取出视频帧
    while (isPlay) {
        if (!queue.empty()) {
            LOGE("取出视频队列packet")
            int result = 0;
            try {
                result = av_packet_ref(packet, queue.front());
            } catch (const char *msg) {
                LOGE("执行取出视频队列时候异常")
            }

            //从队列取出一个packet,clone一个 给入参对象. quequ.front() 返回对队列中第一个元素的引用。此元素将是调用pop（）时要删除的第一个元素
            if (result) {
                //clone失败
                LOGE("get video clone 失败");
                break;
            } else {
                LOGE("取出成功出队 视频销毁packet==========");
                //取出成功 出队 销毁packet
                AVPacket *pkt = queue.front();
                queue.pop();
                av_free(pkt);
                LOGE("释放完视频AVPacket");
                break;
            }
        } else {
            LOGE("视频队列为空 阻塞等待");

            //队列为空 阻塞等待
            pthread_cond_wait(&cond, &mutex);
            LOGE("看下是否正在等待");
        }
    }

    //解锁
    LOGE("解锁");
    pthread_mutex_unlock(&mutex);
    LOGE("解锁完毕");
    return 0;
}

int FFmpegVideo::put(AVPacket *packet) {
    //入队  传参一般要新建一个赋值 因为原来的参数有可能销毁.
    AVPacket *packet1 = (AVPacket *) (av_malloc(sizeof(AVPacket)));
    //克隆一下
    try {
        if (av_packet_ref(packet1, packet)) {
            //克隆失败
            return 0;
        }
    } catch (...) {
        LOGE("put video 克隆异常");
    }
    LOGE("入队压入一帧视频数据=======");
    //加锁
    pthread_mutex_lock(&mutex);

    queue.push(packet1);
    //发通知
    pthread_cond_signal(&cond);

    //给消费者解锁
    pthread_mutex_unlock(&mutex);
    return 1;
}

void *play_video(void *arg) {
    FFmpegVideo *vedio = (FFmpegVideo *) arg;
    //像素数据（解码数据）
    AVFrame *frame = av_frame_alloc();

    AVFrame *rgb_frame = av_frame_alloc();
    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_RGBA, vedio->codec->width,
                               vedio->codec->height));
    //设置yuvFrame的缓冲区，像素格式
    avpicture_fill((AVPicture *) rgb_frame, out_buffer, AV_PIX_FMT_RGBA, vedio->codec->width,
                   vedio->codec->height);
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
    int len, got_frame, framecount = 0;
    LOGE("视频打印 宽  %d ,高  %d ", vedio->codec->width, vedio->codec->height);
    //编码数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //6.一帧一帧读取压缩的视频数据AVPacket
    while (vedio->isPlay) {
        LOGE("视频 解码  一帧");
        LOGE("准备调用vedio->get消费视频数据===================");
//        消费者取到一帧数据  没有 阻塞 vedio->get
        vedio->get(packet);
        len = avcodec_decode_video2(vedio->codec, frame, &got_frame, packet);
        LOGE("视频解码 %d", len);

//        转码成rgb
        sws_scale(sws_ctx, (const uint8_t *const *) (frame->data), frame->linesize, 0,
                  vedio->codec->height,
                  rgb_frame->data, rgb_frame->linesize);

        LOGE("转码成rgb");
//        得到了rgb_frame  绘制   frame  rgb     pcm  frame

        LOGE("视频got_frame = %d", got_frame);

    }
    return 0;
}


void FFmpegVideo::play() {
    isPlay = 1;
    LOGE("开启视频解码线程");
    pthread_create(&p_playid, 0, play_video, this);

}

void FFmpegVideo::stop() {

}

void FFmpegVideo::setAvCodecContext(AVCodecContext *codecContext) {
    this->codec = codecContext;
}

//设置回调函数
void FFmpegVideo::setPlayCall(void (*call)(AVFrame *)) {
    video_call = call;

}
