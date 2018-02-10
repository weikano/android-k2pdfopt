#include <jni.h>
#include <stdlib.h>
#include <stdio.h>

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/bitmap.h>

extern "C" {

#include <willus.h>
#include <k2pdfopt.h>

}

typedef struct {
    K2PDFOPT_SETTINGS k2settings;
    MASTERINFO masterinfo;
} k2pdfopt, *k2pdfopt_t;

int compositeAlpha(int foregroundAlpha, int backgroundAlpha) {
    return 0xFF - (((0xFF - backgroundAlpha) * (0xFF - foregroundAlpha)) / 0xFF);
}

int compositeComponent(int fgC, int fgA, int bgC, int bgA, int a) {
    if (a == 0) return 0;
    return ((0xFF * fgC * fgA) + (bgC * bgA * (0xFF - fgA))) / (a * 0xFF);
}

void bmp_32_to_24(unsigned char *src, unsigned char *dst, int w, int h) {
    int srcl = 4;
    int rownum;
    int colnum;
    int dstl = 3;
    for (rownum=0;rownum<h;rownum++)
    {
        unsigned char *oldp,*newp;
        oldp = &src[srcl*rownum*w];
        newp = &dst[dstl*rownum*w];
        for (colnum=0;colnum<w;colnum++,oldp+=srcl,newp+=dstl)
        {
            unsigned char r,g,b,a;
            r = oldp[0];
            g = oldp[1];
            b = oldp[2];
            a = oldp[3];
            unsigned char A = compositeAlpha(a, 255);
            newp[0] = compositeComponent(r, a, 255, 255, A);
            newp[1] = compositeComponent(g, a, 255, 255, A);
            newp[2] = compositeComponent(b, a, 255, 255, A);
        }
    }
}

void bmp_24_to_32(unsigned char *src, unsigned char *dst, int w, int h) {
    int srcl = 3;
    int rownum;
    int colnum;
    int dstl = 4;
    for (rownum=0;rownum<h;rownum++)
    {
        unsigned char *oldp,*newp;
        oldp = &src[srcl*rownum*w];
        newp = &dst[dstl*rownum*w];
        for (colnum=0;colnum<w;colnum++,oldp+=srcl,newp+=dstl)
        {
            unsigned char r,g,b;
            r = oldp[0];
            g = oldp[1];
            b = oldp[2];
            newp[0] = r;
            newp[1] = g;
            newp[2] = b;
            newp[3] = 255;
        }
    }
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_create(JNIEnv *env, jobject thiz, jint w, jint h, jint dpi) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");

    k2pdfopt_t k2pdfopt = new ::k2pdfopt();
    env->SetLongField(thiz, fid, (jlong) k2pdfopt);

    MASTERINFO *masterinfo = &k2pdfopt->masterinfo;
    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    k2pdfopt_settings_init(k2settings);
    k2settings->document_scale_factor = 1.0;
    k2settings->usegs = -1;
    k2settings->dst_width = w;
    k2settings->dst_height = h;
    k2settings->dst_dpi = dpi;
    k2settings->dst_bpc = 8;
    k2settings->debug = 0;
    k2settings->verbose = 0;
    k2settings->dst_color = 1;
    k2settings->show_marked_source = 0;
    k2settings->text_wrap = 1;

    masterinfo_init(masterinfo, k2settings);
}

JNIEXPORT void JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_load(JNIEnv *env, jobject thiz, jobject bm, jint dpi) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    AndroidBitmapInfo info;
    int ret;
    if ((ret = AndroidBitmap_getInfo(env, bm, &info)) < 0) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), strerror(ret * -1));
        return;
    }

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888 &&
        info.format != ANDROID_BITMAP_FORMAT_RGB_565) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), "bm not supported format");
        return;
    }

    unsigned char *buf;
    if ((ret = AndroidBitmap_lockPixels(env, bm, (void **) &buf)) != 0) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), strerror(ret * -1));
        return;
    }

    int pageno = 0;
    int nextpage = -1;
    int pagecount = 0;
    int pages_done = 0;
    double rot_deg = 0, bormean;
    char rotstr[128];

    WILLUSBITMAP _src, *src = &_src;
    WILLUSBITMAP _srcgrey, *srcgrey = &_srcgrey;
    WILLUSBITMAP _marked, *marked = &_marked;

    FONTSIZE_HISTOGRAM fsh;
    fontsize_histogram_init(&fsh);

    MASTERINFO *masterinfo = &k2pdfopt->masterinfo;
    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    /* Must be called once per conversion to init margins / devsize / output size */
    k2pdfopt_settings_new_source_document_init(k2settings);

    if(masterinfo->wrapbmp.wrectmaps.wrectmap != NULL)
        masterinfo_free(masterinfo, k2settings);
    masterinfo_init(masterinfo, k2settings);

    masterinfo->preview_bitmap = 0;
    k2settings->src_dpi = dpi;

    bmp_init(src);
    bmp_init(marked);
    bmp_init(srcgrey);

    src->width = info.width;
    src->height = info.height;
    src->bpp = 24;
    bmp_alloc(src);
    bmp_32_to_24(buf, src->data, info.width, info.height);

    AndroidBitmap_unlockPixels(env, bm);

    {
        BMPREGION region;
        int mstatus;

        /* Got Good Page Render */
        bmpregion_init(&region);
        bmpregion_k2pagebreakmarks_allocate(&region);
        mstatus = masterinfo_new_source_page_init(masterinfo, k2settings, src, srcgrey, marked,
                                                  &region, rot_deg, &bormean, rotstr, pageno,
                                                  nextpage, stdout);
        if (mstatus == 0) {
            bmpregion_free(&region);
            return;
        }

        /* If user has set output size by font size, determine source font size (v2.34) */
        {
            double src_fontsize_pts;

            if (k2settings->dst_fontsize_pts > 0.)
                src_fontsize_pts = fontsize_histogram_median(&fsh, 0);
            else if (k2settings->dst_fontsize_pts < 0.) {
                int si;

                si = fsh.n;
                k2proc_get_fontsize_histogram(&region, masterinfo, k2settings, &fsh);
                src_fontsize_pts = fontsize_histogram_median(&fsh, si);
                if (k2settings->verbose)
                    k2printf("    %d text rows on page %d\n", fsh.n - si, pageno);
            } else
                src_fontsize_pts = -1.;

            /* v2.34:  Set destination size (flush output bitmap if it changes) */
            k2pdfopt_settings_set_margins_and_devsize(k2settings, &region, masterinfo,
                                                      src_fontsize_pts, 0);
            if (!k2settings->preview_page) {
                if (pageno < 0)
                    k2printf("\n" TTEXT_HEADER "COVER PAGE (%s)", k2settings->dst_coverimage);
                else
                    k2printf("\n" TTEXT_HEADER "SOURCE PAGE %d", pageno);
            }
            if (pagecount > 0) {
                if (!k2settings->preview_page && pageno >= 0) {
                    if (k2settings->pagelist[0] != '\0')
                        k2printf(" (%d of %d)", pages_done + 1, pagecount);
                    else
                        k2printf(" of %d", pagecount);
                }
            }
            if (!k2settings->preview_page) {
                k2printf(TTEXT_NORMAL
                                 " (%.1f x %.1f in",
                         (double) srcgrey->width / k2settings->src_dpi,
                         (double) srcgrey->height / k2settings->src_dpi);
                if (k2settings->dst_fontsize_pts < 0.) {
                    if (src_fontsize_pts < 0)
                        k2printf(", fs=undet.");
                    else
                        k2printf(", fs=%.1fpts", src_fontsize_pts);
                }
                k2printf(") ... %s", rotstr);
                fflush(stdout);
            }
        } /* End of scope with src_fontsize_pts */

        /* Parse the source bitmap for viewable regions */
        /* v2.34:  If cover page, use special function */
        if (pageno < 0)
            bmpregion_add_cover_image(&region, k2settings, masterinfo);
        else
            bmpregion_source_page_add(&region, k2settings, masterinfo, 1, pages_done++);
        /* v2.15 memory leak fix */
        bmpregion_free(&region);
    } /* End declaration of BMPREGION region */
    if (k2settings->verbose) {
        k2printf("    master->rows=%d\n", masterinfo->rows);
        k2printf("Publishing...\n");
    }
    /* Reset the display order for this source page (v2.34--don't call if on cover image) */
    if (k2settings->show_marked_source && pageno >= 0)
        mark_source_page(k2settings, masterinfo, NULL, 0, 0xf);

    bmp_free(marked);
    bmp_free(srcgrey);
    bmp_free(src);
    fontsize_histogram_free(&fsh);
  }

JNIEXPORT void JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_skipNext(JNIEnv *env, jobject thiz) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    jclass bitmapConfig = env->FindClass("android/graphics/Bitmap$Config");
    jfieldID rgb8888FieldID = env->GetStaticFieldID(bitmapConfig, "ARGB_8888",
                                                    "Landroid/graphics/Bitmap$Config;");
    jobject rgb8888Obj = env->GetStaticObjectField(bitmapConfig, rgb8888FieldID);

    jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmapMethodID = env->GetStaticMethodID(bitmapClass, "createBitmap",
                                                            "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    MASTERINFO *masterinfo = &k2pdfopt->masterinfo;
    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    WILLUSBITMAP _bmp, *bmp;
    double bmpdpi;
    int size_reduction;
    void *ocrwords = 0;
    int flush_output = 1;
    bmp = &_bmp;
    bmp_init(bmp);

    WILLUSBITMAP preview_internal;
    masterinfo->preview_bitmap=&preview_internal;
    k2settings->preview_page = masterinfo->published_pages;
    bmp_init(masterinfo->preview_bitmap);

    if (masterinfo_get_next_output_page(masterinfo, k2settings, flush_output, bmp, &bmpdpi,
                                        &size_reduction, ocrwords) > 0) {
        masterinfo->output_page_count++;
    }

    bmp_free(bmp);
    bmp_free(masterinfo->preview_bitmap);
}

JNIEXPORT jobject JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_renderNext(JNIEnv *env, jobject thiz) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    jclass bitmapConfig = env->FindClass("android/graphics/Bitmap$Config");
    jfieldID rgb8888FieldID = env->GetStaticFieldID(bitmapConfig, "ARGB_8888",
                                                    "Landroid/graphics/Bitmap$Config;");
    jobject rgb8888Obj = env->GetStaticObjectField(bitmapConfig, rgb8888FieldID);

    jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmapMethodID = env->GetStaticMethodID(bitmapClass, "createBitmap",
                                                            "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    MASTERINFO *masterinfo = &k2pdfopt->masterinfo;
    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    WILLUSBITMAP _bmp, *bmp;
    double bmpdpi;
    int size_reduction;
    void *ocrwords = 0;
    int flush_output = 1;
    bmp = &_bmp;
    bmp_init(bmp);

    masterinfo->preview_bitmap = 0;
    k2settings->preview_page = 0;

    jobject bm = 0;

    if (masterinfo_get_next_output_page(masterinfo, k2settings, flush_output, bmp, &bmpdpi,
                                           &size_reduction, ocrwords) > 0) {
        masterinfo->output_page_count++;

        bm = env->CallStaticObjectMethod(bitmapClass, createBitmapMethodID,
                                                 bmp->width, bmp->height, rgb8888Obj);

        int ret;
        unsigned char *buf;
        if ((ret = AndroidBitmap_lockPixels(env, bm, (void **) &buf)) != 0) {
            env->ThrowNew(env->FindClass("java/lang/RuntimeException"), strerror(ret * -1));
            return 0;
        }

        bmp_24_to_32(bmp->data, buf, bmp->width, bmp->height);

        AndroidBitmap_unlockPixels(env, bm);
    }

    bmp_free(bmp);

    return bm;
}

JNIEXPORT void JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_close(JNIEnv *env, jobject thiz) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    MASTERINFO *masterinfo = &k2pdfopt->masterinfo;
    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;
    masterinfo_free(masterinfo, k2settings);

    delete k2pdfopt;
    env->SetLongField(thiz, fid, 0);
    return;
}

}
