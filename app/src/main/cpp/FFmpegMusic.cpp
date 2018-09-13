//
// Created by liuml on 2018/9/11.
//
#include <jni.h>
#include <string>
#include <jni.h>
#include <string>
#include <android/log.h>
#include "FFmpegMusic.h"

AVFormatContext *pContext;
AVCodecContext *pCodecCtx;
AVCodec *pCodex;
AVPacket *packet;
AVFrame *frame;
SwrContext *swrContext;
uint8_t *out_buffer;
int out_sample_rate;
int out_channer_nb = -1;
//找到视频流
int audio_stream_ids = -1;
/**
 * 初始化.   openSL ES 调用这个函数
 * @param rate 采样率
 * @param channel 通道数
 * @return
 */
int createFFmpeg(int *rate, int *channel) {
    //前面和之前一样 获取输入的信息 但是现在是找到音频的AVMEDIA_TYPE_AUDIO
    // TODO
    //无论编码还是解码 都要调用这个 注册各大组件
    av_register_all();

    //获取AVFormatContext  比特率 时长 文件路径 流的信息(nustream) 都封装在这里面
    pContext = avformat_alloc_context();

    const  char *input = "/sdcard/input.mp3";
    //AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options
    //上下文  文件名  打开文件格式 获取信息(AVDictionary)  凡是AVDictionary字典 都是获取视频文件信息
    if (avformat_open_input(&pContext, input, NULL, NULL) < 0) {
        LOGE("打开失败");
        return -1;
    }

    //给nbstram填充信息
    if (avformat_find_stream_info(pContext, NULL) < 0) {
        LOGE("获取信息失败");
        return -1;
    }


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
    pCodecCtx = pContext->streams[audio_stream_ids]->codec;
    LOGE("获取解码器上下文");
    //----------解码器
    pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
    LOGE("获取解码器");
    //打开解码器  为什么avcodec_open2 版本升级的原因
    if (avcodec_open2(pCodecCtx, pCodex, NULL) < 0) {
        LOGE("解码失败");
        return -1;
    }

    //得到解封装 读取 解封每一帧 读取每一帧的压缩数据
    //初始化avpacket 分配内存  FFMpeg 没有自动分配内存 必须手动分匹配手动释放  不过有分配函数
    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    //初始化AVFrame
    frame = av_frame_alloc();

    //mp3 里面所包含的编码格式 转化成pcm
    //#include <libswresample/swresample.h>
    swrContext = swr_alloc();//获取转换的上下文

    int frame_count = 0;

    //AVFormatContext *s, AVPacket *pkt  上下文,avpacket 是数据包的意思
    //packet 入参
    int got_frame;

    int length = 0;

    //定义缓冲区输出的  需要多大的采样率  采样 44100  多少个字节.  双通道需要乘以2
    out_buffer = static_cast<uint8_t *>(av_malloc(44100 * 2));//一秒的缓冲区数量


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
    enum AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;
    //输出的采样率 必须与输入的相同
    out_sample_rate = pCodecCtx->sample_rate;



    //https://blog.csdn.net/explorer_day/article/details/76332556  文档
    //swr_alloc_set_opts将PCM源文件的采样格式转换为自己希望的采样格式
    swr_alloc_set_opts(swrContext, out_ch_layout, out_formart, out_sample_rate,
                       pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate,
                       0, NULL);

    //初始化转换器
    swr_init(swrContext);

    //求出通道数
    out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);

    *rate = pCodecCtx->sample_rate;//通过解析出来的 采样率 赋值给调用的openSL ES 的采样率
    *channel = pCodecCtx->channels;//同理赋值 通道数.
    LOGE("赋值采样率 通道数");
    return 0 ;

}

/**
 * 获取pcm数据
 * @param pcm 二级指针 可以修改(缓冲区数组的地址)
 * @param size  大小
 * @return
 */
void getPcm(void **pcm,size_t *pcm_size) {
    int got_frame;
    int count = 0;
    //读取frame
    while (av_read_frame(pContext, packet) >= 0) {
        if (packet->stream_index == audio_stream_ids) {
            //解码  现在编码格式frame 需要转化成pcm
            int ret = avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
            LOGE("正在解码 %d", count++);
            if (ret < 0) {
                LOGE("解码完成");
            }
            //解码一帧
            if (got_frame > 0) {
                //真正的解码
                LOGE("开始解码");
                //转换得到out_buffer
                swr_convert(swrContext, &out_buffer, 44100 * 2,
                            (const uint8_t **) (frame->data), frame->nb_samples);

                LOGE("转换得到out_buffer");
                //求缓冲区实际的大小  通道数  frame->nb_samples 采样的点
                int size = av_samples_get_buffer_size(NULL, out_channer_nb, frame->nb_samples,
                                                      AV_SAMPLE_FMT_S16, 1);
                LOGE("求缓冲区实际的大小 %d", size);
                *pcm = out_buffer;
                *pcm_size = size;
                break;
            }
        }

    }
    LOGE("完成")

}

/**
 * 释放
 */
void releaseFFmpeg() {
    //回收
    av_free_packet(packet);
    av_free(out_buffer);

    av_frame_free(&frame);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_free_context(pContext);


}