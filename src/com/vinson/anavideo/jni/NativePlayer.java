package com.vinson.anavideo.jni;

public class NativePlayer {

	public static native int open(String filePath);

	public static native int prepare();

	public static native int extractNextFrame();

	public static native int getFrameColor(byte[] outColors);

	public static native int destroy();
}
