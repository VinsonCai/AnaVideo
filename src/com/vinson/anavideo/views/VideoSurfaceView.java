package com.vinson.anavideo.views;

import java.nio.ByteBuffer;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.os.Environment;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;

import com.lisa.testplayer.jni.LocalPlayer;

public class VideoSurfaceView extends SurfaceView {
	private static final String TAG = "VideoSurfaceView";
	private static final int WIDTH = 1920;
	private static final int HEIGHT = 1080;
	private static final String PATH = Environment.getExternalStorageDirectory().getAbsolutePath() + "/a1.mp4";
	private DrawingThread mDrawingThread;
	private static final int VIDEO_WIDTH = 1280;
	private static final int VIDEO_HEIGHT = 720;
	byte[] mPixel = new byte[VIDEO_WIDTH * VIDEO_HEIGHT * 2];
	ByteBuffer mBuffer = ByteBuffer.wrap(mPixel);
	Bitmap mVideoBit = Bitmap.createBitmap(VIDEO_WIDTH, VIDEO_HEIGHT, Config.RGB_565);
	private Path mFrontPath;
	private Path mBackgroundPath;
	private Paint mPaint;

	public VideoSurfaceView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		initView();

	}

	public VideoSurfaceView(Context context, AttributeSet attrs) {
		super(context, attrs);
		initView();
	}

	public VideoSurfaceView(Context context) {
		super(context);
		initView();

	}

	private void buildPath() {
		mFrontPath = new Path();
		mFrontPath.moveTo(0, 0);
		mFrontPath.lineTo(WIDTH, HEIGHT);

		mBackgroundPath = new Path();
		mBackgroundPath.moveTo(WIDTH, 0);
		mBackgroundPath.lineTo(0, HEIGHT);

		mPaint = new Paint();
		mPaint.setAntiAlias(true);
		mPaint.setStrokeWidth(8);
		mPaint.setColor(Color.RED);
		mPaint.setStyle(Style.STROKE);
	}

	private void initView() {
		buildPath();
		SurfaceHolder holder = getHolder();
		holder.addCallback(new Callback() {

			@Override
			public void surfaceDestroyed(SurfaceHolder holder) {
				onDestroy();
			}

			@Override
			public void surfaceCreated(SurfaceHolder holder) {
				LocalPlayer.open(PATH);
				LocalPlayer.initTranslate(VIDEO_WIDTH, VIDEO_HEIGHT);
			}

			@Override
			public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
			}
		});

		mDrawingThread = new DrawingThread();
		mDrawingThread.start();
	}

	private void onDestroy() {
		if (null != mDrawingThread) {
			mDrawingThread.stopDrawing();
			mDrawingThread = null;
		}
	}

	private class DrawingThread extends Thread {

		private boolean mRunningDrawing = false;

		@Override
		public void run() {
			super.run();
			mRunningDrawing = true;
			while (mRunningDrawing) {
				int ret = LocalPlayer.translateColor(mPixel);
				if (ret != -1) {
					Canvas canvas = getHolder().lockCanvas(null);
					mVideoBit.copyPixelsFromBuffer(mBuffer);
					mBuffer.position(0);
					canvas.drawColor(Color.WHITE);
					canvas.drawPath(mBackgroundPath, mPaint);
//					Log.e("lisa", mVideoBit.getWidth()+"...");
					canvas.drawBitmap(mVideoBit, 0, 0, null);
					canvas.drawPath(mFrontPath, mPaint);
					getHolder().unlockCanvasAndPost(canvas);
//					Log.v(TAG, "after draw bitmap:" + System.currentTimeMillis());
				} else {
					try {
						Thread.sleep(5);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}
			}
		}

		private void output() {
			System.out.println("");
		}

		public void stopDrawing() {
			mRunningDrawing = false;

		}
	}
}
