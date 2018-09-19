package androidrn.ffmpegdemo;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * @author liuml
 * @explain
 * @time 2018/9/3 15:45
 */
public class MyVideoView extends SurfaceView {


    // Used to load the 'native-lib' library on application startup.
    public native void play(String url);
    public native void stop();


    public MyVideoView(Context context) {
        super(context);
    }

    public MyVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);

        init();
        SurfaceHolder holder = this.getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);
    }


    public MyVideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    private void init() {

    }


    public void player(final String input) {
        ThreadManager.getNormalPool().execute(new Runnable() {
            @Override
            public void run() {
                //把surface 传递到底层  绘制功能就在底层进行了
                render(input, MyVideoView.this.getHolder().getSurface());
            }
        });
    }

    //本地的native方法
    public native void render(String input, Surface surface);
}





















