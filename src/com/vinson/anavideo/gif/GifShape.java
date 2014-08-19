package com.vinson.anavideo.gif;

import java.io.FileInputStream;
import java.io.FileNotFoundException;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;

public class GifShape implements GifAction {

	private static final Object LOCKER_OBJECT = new Object();

	/** gif解码器 */
	private GifDecoder gifDecoder = null;
	/** 当前要画的帧的图 */
	private Bitmap currentImage = null;

	private boolean isRun = true;

	private boolean pause = false;

	private int showWidth = -1;
	private int showHeight = -1;
	private Rect rect = null;
	private float mX = 100;
	private float mY = 100;

	private DecodeThread mDecodeThread;

	public GifShape() {

	}

	public void decode(String path) {
		try {
			gifDecoder = new GifDecoder(new FileInputStream(path), this);
			gifDecoder.start();
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}
	}

	public void destroy() {
		stopDecodeThread();
		gifDecoder.free();
	}

	@Override
	public void parseOk(boolean parseStatus, int frameIndex) {
		if (parseStatus) {
			if (null != gifDecoder) {
				if (frameIndex == -1) {
					if (gifDecoder.getFrameCount() > 1) {
						startDecodeThread();
					} else {
						currentImage = gifDecoder.getImage();
					}
				}
			}
		}
	}

	private void startDecodeThread() {
		if (null == mDecodeThread || !mDecodeThread.isAlive()) {
			mDecodeThread = new DecodeThread();
			mDecodeThread.start();
		}
	}

	private void stopDecodeThread() {
		if (null != mDecodeThread) {
			mDecodeThread.stopThread();
			mDecodeThread = null;
		}
	}

	public void setPosition(float x, float y) {
		mX = x;
		mY = y;
	}

	public void setSize(int width, int height) {
		showWidth = width;
		showHeight = height;
	}

	public void drawSelf(Canvas canvas) {
		if (null != currentImage) {
			canvas.drawBitmap(currentImage, mX, mY, null);
		}
	}

	private class DecodeThread extends Thread {
		private boolean mIsRunning = false;
		private Thread mCurrentThread;

		@Override
		public void run() {
			super.run();

			mIsRunning = true;
			mCurrentThread = Thread.currentThread();

			while (mIsRunning) {
				GifFrame frame = gifDecoder.next();
				currentImage = frame.image;
				long sp = frame.delay;
				try {
					Thread.sleep(sp);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}

		public void stopThread() {
			mIsRunning = false;
			if (null != mCurrentThread) {
				mCurrentThread.interrupt();
				mCurrentThread = null;
			}
		}
	}
}
