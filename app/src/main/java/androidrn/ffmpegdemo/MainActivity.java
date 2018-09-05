package androidrn.ffmpegdemo;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.Toast;

import com.hjq.permissions.OnPermission;
import com.hjq.permissions.Permission;
import com.hjq.permissions.XXPermissions;

import java.io.File;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "Mainactivity";
    // Used to load the 'native-lib' library on application startup.
    MyVideoView videoView;
    private Spinner sp_video;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main2);

        // Example of a call to a native method
//        TextView tv = (TextView) findViewById(R.id.sample_text);
//        tv.setText(stringFromJNI());
        isHasPermission();

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
    }

    public void isHasPermission() {
        if (XXPermissions.isHasPermission(MainActivity.this, Permission.Group.STORAGE)) {
            Toast.makeText(MainActivity.this, "已经获取到权限，不需要再次申请了", Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(MainActivity.this, "还没有获取到权限或者部分权限未授予", Toast.LENGTH_SHORT).show();
            requestPermission();
        }
    }

    public void mPlay(View view) {
        String video = sp_video.getSelectedItem().toString();
        String input = new File(Environment.getExternalStorageDirectory(), video).getAbsolutePath();
        Log.d(TAG, "onCreate: 文件路径 " + input);
        videoView.player(input);
    }

    public void requestPermission() {
        XXPermissions.with(this)
                //.constantRequest() //可设置被拒绝后继续申请，直到用户授权或者永久拒绝
                //.permission(Permission.SYSTEM_ALERT_WINDOW, Permission.REQUEST_INSTALL_PACKAGES) //支持请求6.0悬浮窗权限8.0请求安装权限
                .permission(Permission.Group.STORAGE) //不指定权限则自动获取清单中的危险权限
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
}
