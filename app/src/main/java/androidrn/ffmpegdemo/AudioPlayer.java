package androidrn.ffmpegdemo;

import android.media.AudioFormat;
import android.media.AudioTrack;

/**
 * @author liuml
 * @explain
 * @time 2018/9/5 17:14
 */
public class AudioPlayer {
    public native void changeSound(String input, String output);

    private AudioTrack audioTrack;

    /**
     *  创建AudioTrack
     * @param sampleRateInHz 音频的采样率
     * @param nb_channals   音频的声道
     */
    public void createAudio(int sampleRateInHz, int nb_channals){
        int channaleConfig;
        //根据通道数判断 如果是2 就是立体声 不能直接用c层的 因为定义的不一样
        if(nb_channals ==1){
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;// 默认声道
        }else if(nb_channals == 2){
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;//立体声
        }else{
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;//默认声道
        }

        //音频播放缓冲区计算  算出最低需要的缓冲区大小  因为录制的时候默认的就是16位 可以看下系统音频设置.
        int buffersize = AudioTrack.getMinBufferSize(sampleRateInHz, channaleConfig, AudioFormat.ENCODING_PCM_16BIT);
        //36分.
//        audioTrack = new AudioTrack()
    }

}
