package com.fihtdc.libbrotlijni;

/**
 * Created by LukeSLLin on 2017/2/17.
 */

public class BrotliUtils {
    static {
        System.loadLibrary("BrotliUtils");
    }
    public native int compress(String inputPath, String outputPath, int quality);
    public native int decompress(String inputPath, String outputPath);
}
