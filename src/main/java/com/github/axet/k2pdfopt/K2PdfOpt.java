package com.github.axet.k2pdfopt;

import android.graphics.Bitmap;

public class K2PdfOpt {

    static {
        if (Config.natives) {
            System.loadLibrary("k2pdfoptjni");
        }
    }

    private long handle;

    public K2PdfOpt() {
    }

    public native void create(int w, int h, int dpi);

    public native void load(Bitmap bm);

    public native boolean skipNext();

    public native Bitmap renderNext();

    public native void close();

}
