package com.lisa.testplayer.jni;

public class LocalPlayer {
	static {
		System.loadLibrary("local-player");
	}

	public native static float open(String pFilePath);

	public static native float drawFrame();

	public static native void surfaceCreated(int pWidth, int pHeight);

	public static native int translateColor(byte[] out);
	
	public static native void initTranslate(int width, int height);
}
