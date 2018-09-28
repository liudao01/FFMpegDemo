package androidrn.ffmpegdemo;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * @author liuml
 * @explain
 * @time 2018/9/27 16:24
 */
public class VideoPlay implements SurfaceHolder.Callback {

    private SurfaceView surfaceView;

    public void playJava(String path) {
        if (surfaceView == null) {
            return;
        }
        play(path);
    }

    // Used to load the 'native-lib' library on application startup.
    public native void play(String url);

    public native void stop();

    //音视频同步用的方法
    public native void display(Surface surface);

    public void setSurfaceView(SurfaceView surfaceView) {
        this.surfaceView = surfaceView;
        display(surfaceView.getHolder().getSurface());
        surfaceView.getHolder().addCallback(this);

    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        display(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }
}
