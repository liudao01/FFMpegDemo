//
// Created by liuml on 2018/9/15.
//

#include "FFmpegAudio.h"

/**
 * 获取pcm数据
 * @param pcm 二级指针 可以修改(缓冲区数组的地址)
 * @param size  大小
 * @return
 */
int getPcm(FFmpegAudio *audio) {
    int got_frame;
    int out_buffer_size;
    int count = 0;
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    AVFrame *frame = av_frame_alloc();
    int out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    //读取frame
    while (audio->isPlay) {

        out_buffer_size = 0;
        //获取数据
        audio->get(packet);
        //修正音频  packet->pts就是当前的pts
        if(packet->pts!=AV_NOPTS_VALUE){
//            typedef struct AVRational{
//                int num; ///< numerator  1秒
//                int den; ///< denominator 25毫秒
//            } AVRational;

            //假设1S 分成25等份  40ms  那么当前的pts*40毫秒就是当前的时间
            audio->clock = packet->pts * av_q2d(audio->time_base);//当前的份数 乘以 每份是多少数据
        }
        //解码  现在编码格式frame 需要转化成pcm
        int ret = avcodec_decode_audio4(audio->codec, frame, &got_frame, packet);
        LOGE("正在解码 %d", count++);
        if (ret < 0) {
            LOGE("解码完成");
        }
        //解码一帧
        if (got_frame) {
            //真正的解码
            LOGE("开始解码");
            //转换得到out_buffer
            swr_convert(audio->swrContext, &audio->out_buffer, 44100 * 2,
                        (const uint8_t **) (frame->data), frame->nb_samples);

            LOGE("转换得到out_buffer");
            //求缓冲区实际的大小  通道数  frame->nb_samples 采样的点
            out_buffer_size = av_samples_get_buffer_size(NULL, out_channer_nb, frame->nb_samples,
                                                         AV_SAMPLE_FMT_S16, 1);
            LOGE("求缓冲区实际的大小 %d", out_buffer_size);
            break;
        }

    }
    LOGE("完成");
    //释放
    av_free(packet);
    av_frame_free(&frame);

    return out_buffer_size;
}

//第一次主动调用在调用线程
//之后在新线程中回调
//当喇叭播放完声音回调此函数,添加pcm数据到缓冲区
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    FFmpegAudio *audio = (FFmpegAudio *) context;
    int bufferSize = getPcm(audio);//获取pcm数据
    if (bufferSize > 0) {
//        计算时间
        double time = bufferSize / ((double) 44100 * 2 * 2);
        //时间累加
        audio->clock += time;
        LOGE("播放音频帧 time = %lf  clock = %lf",time,audio->clock);
        SLresult result;//结果
        //播放帧
        result = (*bq)->Enqueue(bq, audio->out_buffer, bufferSize);
        LOGE("回调 bqPlayerCallback 字节 : %d", bufferSize);

    } else {
        LOGE("获取PCM失败")
    }
}

void createFFmpeg(FFmpegAudio *audio) {
//    mp3  里面所包含的编码格式   转换成  pcm   SwcContext
    audio->swrContext = swr_alloc();

    int length = 0;
    int got_frame;
//    44100*2
    audio->out_buffer = (uint8_t *) av_malloc(44100 * 2);
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
//    输出采样位数  16位
    enum AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;
//输出的采样率必须与输入相同
    int out_sample_rate = audio->codec->sample_rate;


    swr_alloc_set_opts(audio->swrContext, out_ch_layout, out_formart, out_sample_rate,
                       audio->codec->channel_layout, audio->codec->sample_fmt,
                       audio->codec->sample_rate, 0,
                       NULL);

    swr_init(audio->swrContext);
//    获取通道数  2
    audio->out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    LOGE("------>通道数%d  ", audio->out_channer_nb);
}


int FFmpegAudio::createPlayer() {
    SLresult result;
    // 创建引擎engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 实现引擎engineObject
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 获取引擎接口engineEngine
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                           &engineEngine);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 创建混音器outputMixObject
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0,
                                              0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 实现混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    if (SL_RESULT_SUCCESS == result) {
        (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &settings);
    }


    //======================
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
//   新建一个数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};
//    设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};

    SLDataSink audioSnk = {&outputMix, NULL};
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
    //先讲这个
    (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &slDataSource,
                                       &audioSnk, 2,
                                       ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

//    得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);

//    注册回调缓冲区 //获取缓冲队列接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueue);
    //缓冲接口回调
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
//    获取音量接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);

//    获取播放状态接口
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    bqPlayerCallback(bqPlayerBufferQueue, this);
    return 1;
}


//消费者
int FFmpegAudio::get(AVPacket *packet) {
    //锁住
    pthread_mutex_lock(&mutex);

    //取出音频帧
    while (isPlay) {
        if (!queue.empty()) {
            LOGE("执行取出音频队列操作")
            if (!queue.empty()) {
//            从队列取出一个packet   clone一个 给入参对象
                if (av_packet_ref(packet, queue.front())) {
                    break;
                }
//            取成功了  弹出队列  销毁packet
                AVPacket *pkt = queue.front();
                LOGE("取出一 个音频帧%d", queue.size());
                queue.pop();
                av_free(pkt);
                LOGE("释放pkt");
                break;
            } else {
//            如果队列里面没有数据的话  一直等待阻塞
                pthread_cond_wait(&cond, &mutex);
            }
        } else {
            //队列为空 阻塞等待h
            LOGE("音频队列为空 阻塞等待");
            pthread_cond_wait(&cond, &mutex);
            LOGE("不等待了 Audio ");
        }
    }
    //解锁
    pthread_mutex_unlock(&mutex);
    return 0;
}

//生产者
int FFmpegAudio::put(AVPacket *packet) {
    //入队  传参一般要新建一个赋值 因为原来的参数有可能销毁.
    AVPacket *packet1 = (AVPacket *) (av_malloc(sizeof(AVPacket)));
    //克隆一下
    try {
        if (av_packet_ref(packet1, packet)) {
            //克隆失败
            return 0;
        }
    } catch (...) {
        LOGE("put  audio 克隆异常");
    }
    //锁住
    LOGE("入队压入一帧音频数据=========");
    //加锁
    pthread_mutex_lock(&mutex);
    queue.push(packet1);
    //发通知
    pthread_cond_signal(&cond);

    //给消费者解锁
    pthread_mutex_unlock(&mutex);
    return 1;
}

//线程的函数播放
void *play_audio(void *arg) {

    FFmpegAudio *audio = (FFmpegAudio *) arg;
    //这里是阻塞的
    audio->createPlayer();
    pthread_exit(0);
}

void FFmpegAudio::play() {
    isPlay = 1;
    //开启一个解码音频的线程
    LOGE("开启音频解码线程");
    pthread_create(&p_playid, NULL, play_audio, this);
}

void FFmpegAudio::stop() {

}

//设置上下文
void FFmpegAudio::setAvCodecContext(AVCodecContext *codecContext) {
    this->codec = codecContext;
    createFFmpeg(this);
}

FFmpegAudio::FFmpegAudio() {
    //构造方法里面实例化同步锁,
    pthread_mutex_init(&mutex, NULL);
    //条件变量
    pthread_cond_init(&cond, NULL);
    //初始化音频播放时间
    clock = 0;
}

FFmpegAudio::~FFmpegAudio() {

}

