package androidrn.ffmpegdemo;

import android.app.Application;

/**
 * @author liuml
 * @explain
 * @time 2018/9/26 20:22
 */
public class MyApplication extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
      //  CrashReport.initCrashReport(getApplicationContext(), "注册时申请的APPID", false);
    }
}
