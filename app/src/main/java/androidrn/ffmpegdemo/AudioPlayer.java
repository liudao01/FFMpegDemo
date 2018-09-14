package androidrn.ffmpegdemo;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/**
 * @author liuml
 * @explain
 * @time 2018/9/5 17:14
 */
public class AudioPlayer {

    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("native-lib");

    }
    public native void changeSound(String input, String output);
    public native void playSound(String input, String output);

    public native void OpenSLEsPlay();
    public native void OpenSlESStop();

    private AudioTrack audioTrack;

    /**
     * 创建AudioTrack 初始化
     *
     * @param sampleRateInHz 音频的采样率
     * @param nb_channals    音频的声道
     */
    public void createAudio(int sampleRateInHz, int nb_channals) {
        int channaleConfig;
        //根据通道数判断 如果是2 就是立体声 不能直接用c层的 因为定义的不一样
        if (nb_channals == 1) {
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;// 默认声道
        } else if (nb_channals == 2) {
            channaleConfig = AudioFormat.CHANNEL_OUT_STEREO;//立体声
        } else {
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;//默认声道
        }

        //音频播放缓冲区计算  算出最低需要的缓冲区大小  因为录制的时候默认的就是16位 可以看下系统音频设置.
        int buffersize = AudioTrack.getMinBufferSize(sampleRateInHz, channaleConfig, AudioFormat.ENCODING_PCM_16BIT);

        /**
         * 1指定在流的类型 2. 设置音频数据的采样率 如果是44.1k就是44100 3. 设置输出声道为双声道立体声，而CHANNEL_OUT_MONO类型是单声道.
         * 4.  设置音频数据块是8位还是16位，这里设置为16位。好像现在绝大多数的音频都是16位的了
         * 5. 设置模式类型，在这里设置为流类型，另外一种MODE_STATIC貌似没有什么效果
         */
        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,sampleRateInHz,channaleConfig,
                AudioFormat.ENCODING_PCM_16BIT,buffersize,AudioTrack.MODE_STREAM);
        audioTrack.play();
    }


    /**
     * 暴露一个给c播放传递音频数据的方法
     *
     * @param buffer
     * @param lenth  长度
     */
    public void playTrack(byte[] buffer, int lenth) {

        if (audioTrack != null && audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            audioTrack.write(buffer, 0, lenth);
        }
    }

}








