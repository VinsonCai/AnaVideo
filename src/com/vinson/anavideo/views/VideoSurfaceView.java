package com.vinson.anavideo.views;

import java.nio.ByteBuffer;
import java.util.ArrayList;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.os.Environment;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;

import com.lisa.testplayer.jni.LocalPlayer;
import com.vinson.anavideo.events.FPSEvent;
import com.vinson.anavideo.gif.GifShape;

import de.greenrobot.event.EventBus;

public class VideoSurfaceView extends SurfaceView {
	private static final String TAG = "VideoSurfaceView";
	private static final int WIDTH = 1920;
	private static final int HEIGHT = 1080;
	private static final String PATH = Environment.getExternalStorageDirectory().getAbsolutePath() + "/Music/a.mp4";
	private static final String GIF_PATH = Environment.getExternalStorageDirectory().getAbsolutePath() + "/460.gif";
	private DrawingThread mDrawingThread;
	private static final int VIDEO_WIDTH = 400;
	private static final int VIDEO_HEIGHT = 300;
	byte[] mPixel = new byte[VIDEO_WIDTH * VIDEO_HEIGHT * 2];
	ByteBuffer mBuffer = ByteBuffer.wrap(mPixel);
	Bitmap mVideoBit = Bitmap.createBitmap(VIDEO_WIDTH, VIDEO_HEIGHT, Config.RGB_565);
	private Path mFrontPath;
	private Path mBackgroundPath;
	private Paint mPaint;
	private Matrix matrix = new Matrix();

	private ArrayList<Path> mPaths = new ArrayList<Path>();
	private Path mCurrentPath;
	private final static Object LOCKER = new Object();

	private GifShape mGifShape;

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

	private void initGifShape() {
		mGifShape = new GifShape();
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {

		int action = event.getAction();
		float x = event.getX();
		float y = event.getY();

		switch (action) {
		case MotionEvent.ACTION_DOWN:
			mCurrentPath = new Path();
			mCurrentPath.moveTo(x, y);
			break;

		case MotionEvent.ACTION_MOVE:
			mCurrentPath.lineTo(x, y);

			break;
		case MotionEvent.ACTION_UP:
			mCurrentPath.lineTo(x, y);
			synchronized (LOCKER) {
				mPaths.add(mCurrentPath);
			}
			mCurrentPath = null;
			break;

		default:
			break;
		}

		return true;
	}

	private void initView() {
		buildPath();
		initGifShape();
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

				mGifShape.decode(GIF_PATH);
			}

			@Override
			public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
			}
		});

		mDrawingThread = new DrawingThread();
		mDrawingThread.start();

		matrix.preTranslate(100, 100);
		matrix.postScale(2.5f, 2.5f);
	}

	private void onDestroy() {
		if (null != mDrawingThread) {
			mDrawingThread.stopDrawing();
			mDrawingThread = null;
		}

		mGifShape.destroy();
	}

	private class DrawingThread extends Thread {

		private boolean mRunningDrawing = false;

		@Override
		public void run() {
			super.run();
			mRunningDrawing = true;

			int fps = 0;
			FPSEvent event = new FPSEvent();
			long start = System.currentTimeMillis();
			while (mRunningDrawing) {

				long beforeLock = System.currentTimeMillis();

				int ret = LocalPlayer.translateColor(mPixel);
				if (ret != -1) {
					fps++;
					Canvas canvas = getHolder().lockCanvas(null);
					mVideoBit.copyPixelsFromBuffer(mBuffer);
					mBuffer.position(0);
					canvas.drawColor(Color.WHITE);
					canvas.drawPath(mBackgroundPath, mPaint);
//					Log.e("lisa", mVideoBit.getWidth()+"...");
//					canvas.drawBitmap(mVideoBit, 0, 0, null);
					canvas.drawBitmap(mVideoBit, matrix, null);
					canvas.drawPath(mFrontPath, mPaint);

					synchronized (LOCKER) {
						for (Path path : mPaths) {
							canvas.drawPath(path, mPaint);
						}
					}

					if (null != mCurrentPath) {
						canvas.drawPath(mCurrentPath, mPaint);
					}

					mGifShape.drawSelf(canvas);

					getHolder().unlockCanvasAndPost(canvas);

					long end = System.currentTimeMillis();
					if (end - start > 1000) {
						start = end;
						event.mFpsCount = fps;
						fps = 0;
						EventBus.getDefault().post(event);
					}
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
