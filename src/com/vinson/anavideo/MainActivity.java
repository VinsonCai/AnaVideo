package com.vinson.anavideo;

import android.app.Activity;
import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;

import com.vinson.anavideo.events.FPSEvent;
import com.vinson.anavideo.views.VideoSurfaceView;

import de.greenrobot.event.EventBus;

public class MainActivity extends Activity {

	private Button mDrawButton;
	private Button mSelectButton;
	private TextView mFpsTextView;
	private VideoSurfaceView mVideoSurfaceView;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		mVideoSurfaceView = (VideoSurfaceView) findViewById(R.id.video_surfaceView);
		mFpsTextView = (TextView) findViewById(R.id.fps_textView);
	}

	@Override
	protected void onPause() {
		super.onPause();
		EventBus.getDefault().unregister(this);
	}

	@Override
	protected void onResume() {
		super.onResume();
		EventBus.getDefault().register(this);
	}

	public void onEventMainThread(FPSEvent event) {
		mFpsTextView.setText(getString(R.string.fps_rate, event.mFpsCount));
	}

	@Override
	protected void onStop() {
		super.onStop();

		System.exit(0);
	}
}
