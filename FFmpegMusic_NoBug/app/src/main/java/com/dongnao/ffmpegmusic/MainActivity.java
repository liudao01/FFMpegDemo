package com.dongnao.ffmpegmusic;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceView;
import android.view.View;

public class MainActivity extends AppCompatActivity {

    DavidPlayer davidPlayer;
    SurfaceView surfaceView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = (SurfaceView) findViewById(R.id.surface);
        davidPlayer = new DavidPlayer();
        davidPlayer.setSurfaceView(surfaceView);

    }
    public void player(View view) {
//        davidPlayer.playJava("rtmp://live.hkstv.hk.lxdns.com/live/hks");
        davidPlayer.playJava("rtmp://202.69.69.180:443/webcast/bshdlive-pc");
//        davidPlayer.playJava(file.getAbsolutePath());
    }
    public void stop(View view) {
        davidPlayer.release();
    }
}
