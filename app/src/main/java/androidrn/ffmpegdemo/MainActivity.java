package androidrn.ffmpegdemo;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.Toast;

import com.hjq.permissions.OnPermission;
import com.hjq.permissions.Permission;
import com.hjq.permissions.XXPermissions;

import java.io.File;
import java.util.List;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private static final String TAG = "Mainactivity";
    // Used to load the 'native-lib' library on application startup.
    MyVideoView videoView;
    AudioPlayer audioPlayer;
    private Spinner sp_video;
    private Button btAudio;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main2);

        // Example of a call to a native method
//        TextView tv = (TextView) findViewById(R.id.sample_text);
//        tv.setText(stringFromJNI());
        isHasPermission();
        btAudio = findViewById(R.id.bt_audio);
        videoView = findViewById(R.id.surface);
        sp_video = findViewById(R.id.sp_video);
        //多种格式的视频列表
        String[] videoArray = getResources().getStringArray(R.array.video_list);
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_list_item_1,
                android.R.id.text1, videoArray);
        Log.d(TAG, "onCreate: sp_video " + sp_video);
        Log.d(TAG, "onCreate: adapter " + adapter);
        sp_video.setAdapter(adapter);

        btAudio.setOnClickListener(this);
    }

    /**
     *     <uses-permission android:name="android.permission.MODIFY_AUDIO_SETTINGS" />
     <uses-permission android:name="android.permission.RECORD_AUDIO" />
     <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
     */
    public void isHasPermission() {
        if (XXPermissions.isHasPermission(MainActivity.this,  Permission.READ_EXTERNAL_STORAGE,Permission.READ_EXTERNAL_STORAGE,
                Permission.WRITE_EXTERNAL_STORAGE,Permission.RECORD_AUDIO)) {
            Toast.makeText(MainActivity.this, "已经获取到权限，不需要再次申请了", Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(MainActivity.this, "还没有获取权限", Toast.LENGTH_SHORT).show();
            requestPermission();
        }
//        if (XXPermissions.isHasPermission(this, Permission.RECORD_AUDIO)) {
//            Toast.makeText(MainActivity.this, "已经获取到音频", Toast.LENGTH_SHORT).show();
//        }else{
//            Toast.makeText(MainActivity.this, "已经获取到存储权限，不需要再次申请了", Toast.LENGTH_SHORT).show();
//        }
    }

    /**
     * 播放视频
     *
     * @param view
     */
    public void mPlay(View view) {
        String video = sp_video.getSelectedItem().toString();
        String input = new File(Environment.getExternalStorageDirectory(), video).getAbsolutePath();
        Log.d(TAG, "onCreate: 文件路径 " + input);
        videoView.player(input);
    }

    /**
     * 转化音频为pcm
     */
    public void mPlayChangeAudio() {
        audioPlayer = new AudioPlayer();
        String input = new File(Environment.getExternalStorageDirectory(), "input.mp3").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(), "output.pcm").getAbsolutePath();
        Log.d(TAG, "onCreate: 文件路径 " + input);
        audioPlayer.changeSound(input, output);
    }

    /**
     * 播放音频
     */
    public void mPlayAudio() {
        audioPlayer = new AudioPlayer();
        String input = new File(Environment.getExternalStorageDirectory(), "input.mp3").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(), "output.pcm").getAbsolutePath();
        Log.d(TAG, "onCreate: 文件路径 " + input);
        audioPlayer.playSound(input, output);
    }

    /**
     * 利用OpenELSE 播放音频
     */
    public void OpenSLPlay() {
        audioPlayer = new AudioPlayer();
        audioPlayer.OpenSLEsPlay();
    }


    public void requestPermission() {
        XXPermissions.with(this)
                //.constantRequest() //可设置被拒绝后继续申请，直到用户授权或者永久拒绝
//                .permission(Permission.SYSTEM_ALERT_WINDOW, Permission.REQUEST_INSTALL_PACKAGES) //支持请求6.0悬浮窗权限8.0请求安装权限
                .permission(Permission.READ_EXTERNAL_STORAGE,
                        Permission.WRITE_EXTERNAL_STORAGE,Permission.RECORD_AUDIO) //不指定权限则自动获取清单中的危险权限
                .request(new OnPermission() {

                    @Override
                    public void hasPermission(List<String> granted, boolean isAll) {
                        if (isAll) {
                            Toast.makeText(MainActivity.this, "获取权限成功", Toast.LENGTH_SHORT).show();
                        } else {
                            Toast.makeText(MainActivity.this, "获取权限成功，部分权限未正常授予", Toast.LENGTH_SHORT).show();
                        }
                    }

                    @Override
                    public void noPermission(List<String> denied, boolean quick) {
                        if (quick) {
                            Toast.makeText(MainActivity.this, "被永久拒绝授权，请手动授予权限", Toast.LENGTH_SHORT).show();
                            //如果是被永久拒绝就跳转到应用权限系统设置页面
                            XXPermissions.gotoPermissionSettings(MainActivity.this);
                        } else {
                            Toast.makeText(MainActivity.this, "获取权限失败", Toast.LENGTH_SHORT).show();
                        }
                    }
                });
    }
//    public void load(View view) {
//        String input = new File(Environment.getExternalStorageDirectory(), "input.mp4").getAbsolutePath();
//        String  output= new File(Environment.getExternalStorageDirectory(), "output.yuv").getAbsolutePath();
//        openVideo(input, output);
//    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
//    public native String stringFromJNI();
    public native void openVideo(String inputStr, String outStr);

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

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.bt_audio:
                //转化音频
//                mPlayChangeAudio();
                //音频播放
//                mPlayAudio();
                //使用OpenSL 播放
                OpenSLPlay();

                break;
        }
    }
}
