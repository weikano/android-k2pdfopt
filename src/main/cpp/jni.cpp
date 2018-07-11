#include <jni.h>
#include <stdlib.h>
#include <stdio.h>

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/bitmap.h>
#include <vector>

extern "C" {

#include <willus.h>
#include <k2pdfopt.h>

}

typedef struct {
    WILLUSBITMAP bmp;
    double dpi;
} PAGE;

typedef std::vector<WRECTMAP> PAGERECTS;

typedef struct {
    K2PDFOPT_SETTINGS k2settings;
    MASTERINFO masterinfo;
    std::vector<PAGERECTS *> rects;
    std::vector<PAGE *> pages;
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
    for (rownum = 0; rownum < h; rownum++) {
        unsigned char *oldp, *newp;
        oldp = &src[srcl * rownum * w];
        newp = &dst[dstl * rownum * w];
        for (colnum = 0; colnum < w; colnum++, oldp += srcl, newp += dstl) {
            unsigned char r, g, b, a;
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

void bmp_565_to_24(unsigned char *src, unsigned char *dst, int w, int h) {
    int srcl = 2;
    int rownum;
    int colnum;
    int dstl = 3;
    for (rownum = 0; rownum < h; rownum++) {
        unsigned char *oldp, *newp;
        oldp = &src[srcl * rownum * w];
        newp = &dst[dstl * rownum * w];
        for (colnum = 0; colnum < w; colnum++, oldp += srcl, newp += dstl) {
            unsigned char R5 = (oldp[1] >> 3) & 0x1F;
            unsigned char G6 = (((oldp[0] & 0xE0) >> 5) | ((oldp[1] & 0x07) << 3)) & 0x3F;
            unsigned char B5 = oldp[0] & 0x1F;
            newp[0] = (R5 * 527 + 23) >> 6;
            newp[1] = (G6 * 259 + 33) >> 6;
            newp[2] = (B5 * 527 + 23) >> 6;
        }
    }
}

void bmp_24_to_32(unsigned char *src, unsigned char *dst, int w, int h) {
    int srcl = 3;
    int rownum;
    int colnum;
    int dstl = 4;
    for (rownum = 0; rownum < h; rownum++) {
        unsigned char *oldp, *newp;
        oldp = &src[srcl * rownum * w];
        newp = &dst[dstl * rownum * w];
        for (colnum = 0; colnum < w; colnum++, oldp += srcl, newp += dstl) {
            unsigned char r, g, b;
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

uint16_t rgb_to_565(unsigned char R8, unsigned char G8, unsigned char B8) {
    unsigned char R5 = (R8 * 249 + 1014) >> 11;
    unsigned char G6 = (G8 * 253 + 505) >> 10;
    unsigned char B5 = (B8 * 249 + 1014) >> 11;
    return (R5 << 11) | (G6 << 5) | (B5);
}

void bmp_24_to_565(unsigned char *src, unsigned char *dst, int w, int h) {
    int srcl = 3;
    int rownum;
    int colnum;
    int dstl = 2;
    for (rownum = 0; rownum < h; rownum++) {
        unsigned char *oldp, *newp;
        oldp = &src[srcl * rownum * w];
        newp = &dst[dstl * rownum * w];
        for (colnum = 0; colnum < w; colnum++, oldp += srcl, newp += dstl) {
            unsigned char r, g, b;
            r = oldp[0];
            g = oldp[1];
            b = oldp[2];
            (*(uint16_t *) newp) = rgb_to_565(r, g, b);
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
    k2settings->dst_magnification = 1;

    k2settings->dst_userwidth = w;
    k2settings->dst_userwidth_units = UNITS_PIXELS;
    k2settings->dst_userheight = h;
    k2settings->dst_userheight_units = UNITS_PIXELS;
    k2settings->dst_userdpi = dpi;

    masterinfo_init(masterinfo, k2settings);
}

JNIEXPORT jfloat JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_getFontSize(JNIEnv *env, jobject thiz) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    return (jfloat) k2settings->dst_magnification;
}

JNIEXPORT void JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_setFontSize(JNIEnv *env, jobject thiz, jfloat f) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    k2settings->dst_magnification = f;
}

JNIEXPORT jboolean JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_getVerbose(JNIEnv *env, jobject thiz) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    return (jboolean) k2settings->verbose;
}

JNIEXPORT void JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_setVerbose(JNIEnv *env, jobject thiz, jboolean b) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    k2settings->verbose = b;
}

JNIEXPORT jboolean JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_getShowMarkedSource(JNIEnv *env, jobject thiz) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    return (jboolean) k2settings->show_marked_source;
}

JNIEXPORT void JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_setShowMarkedSource(JNIEnv *env, jobject thiz, jboolean b) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    k2settings->show_marked_source = b;
}

JNIEXPORT jboolean JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_getLeftToRight(JNIEnv *env, jobject thiz) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    return (jboolean) k2settings->src_left_to_right;
}

JNIEXPORT void JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_setLeftToRight(JNIEnv *env, jobject thiz, jboolean b) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    k2settings->src_left_to_right = b;
}

JNIEXPORT void JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_load(JNIEnv *env, jobject thiz, jobject bm) {
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

    jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
    jmethodID getDensity = env->GetMethodID(bitmapClass, "getDensity", "()I");

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

    masterinfo_free(masterinfo, k2settings);
    masterinfo_init(masterinfo, k2settings);

    masterinfo->preview_bitmap = 0;
    k2settings->src_dpi = env->CallIntMethod(bm, getDensity);

    bmp_init(src);
    bmp_init(marked);
    bmp_init(srcgrey);

    src->width = info.width;
    src->height = info.height;
    src->bpp = 24;
    bmp_alloc(src);
    switch (info.format) {
        case ANDROID_BITMAP_FORMAT_RGBA_8888:
            bmp_32_to_24(buf, src->data, info.width, info.height);
            break;
        case ANDROID_BITMAP_FORMAT_RGB_565:
            bmp_565_to_24(buf, src->data, info.width, info.height);
            break;
    }

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

    for (int i = 0; i < k2pdfopt->pages.size(); i++) {
        bmp_free(&k2pdfopt->pages[i]->bmp);
        delete k2pdfopt->pages[i];
    }
    k2pdfopt->pages.clear();

    int size_reduction;
    void *ocrwords = 0;
    int flush_output = 1;
    PAGE *page = new PAGE;
    bmp_init(&page->bmp);

    masterinfo->preview_bitmap = 0;
    k2settings->preview_page = 0;

    int bps = 0;
    int bp = 0;
    int dstmar_pixels[4];

    get_dest_margins(dstmar_pixels,k2settings,(double)k2settings->dst_dpi,masterinfo->bmp.width,
                     k2settings->dst_height);
    int l = dstmar_pixels[0];
    int t = dstmar_pixels[1];

    while ((bp = masterinfo_get_next_output_page(masterinfo, k2settings, flush_output, &page->bmp,
                                                 &page->dpi,
                                                 &size_reduction, ocrwords)) > 0) {

        int bpe = bps + bp;
        PAGERECTS *rects = new PAGERECTS;
        for (int i = 0; i < masterinfo->rectmaps.n; i++) {
            WRECTMAP *m = &masterinfo->rectmaps.wrectmap[i];
            POINT2D *dst = &m->coords[1];
            double dsty = dst->y;

            if (bps < dsty && dsty < bpe) {
                dst->x = l + dst->x;
                dst->y = t + dst->y - bps;
                rects->push_back(*m);
            }
        }
        k2pdfopt->rects.push_back(rects);
        bps += bp;
        masterinfo->output_page_count++;
        k2pdfopt->pages.push_back(page);
        page = new PAGE;
        bmp_init(&page->bmp);
    }

    bmp_free(&page->bmp);
    delete page;

    if (k2settings->show_marked_source) {
        if ((ret = AndroidBitmap_lockPixels(env, bm, (void **) &buf)) != 0) {
            env->ThrowNew(env->FindClass("java/lang/RuntimeException"), strerror(ret * -1));
            return;
        }

        switch (info.format) {
            case ANDROID_BITMAP_FORMAT_RGBA_8888:
                bmp_24_to_32(marked->data, buf, marked->width, marked->height);
                break;
            case ANDROID_BITMAP_FORMAT_RGB_565:
                bmp_24_to_565(marked->data, buf, marked->width, marked->height);
                break;
        }

        AndroidBitmap_unlockPixels(env, bm);
    }

    bmp_free(marked);
    bmp_free(srcgrey);
    bmp_free(src);
    fontsize_histogram_free(&fsh);

    masterinfo_free(masterinfo, k2settings);
}

JNIEXPORT jint JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_getCount(JNIEnv *env, jobject thiz) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);
    return (jint) k2pdfopt->pages.size();
}

JNIEXPORT jobject JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_renderPage(JNIEnv *env, jobject thiz, jint page) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    jclass bitmapConfig = env->FindClass("android/graphics/Bitmap$Config");
    jfieldID rgb565FieldID = env->GetStaticFieldID(bitmapConfig, "RGB_565",
                                                   "Landroid/graphics/Bitmap$Config;");
    jobject rgb565Obj = env->GetStaticObjectField(bitmapConfig, rgb565FieldID);

    jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmapMethodID = env->GetStaticMethodID(bitmapClass, "createBitmap",
                                                            "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    jobject bm = 0;

    WILLUSBITMAP *bmp = &k2pdfopt->pages[page]->bmp;

    bm = env->CallStaticObjectMethod(bitmapClass, createBitmapMethodID, bmp->width, bmp->height,
                                     rgb565Obj);

    if (bm == 0)
        return 0; // OutOfMemory exception already pending

    int ret;
    unsigned char *buf;
    if ((ret = AndroidBitmap_lockPixels(env, bm, (void **) &buf)) != 0) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), strerror(ret * -1));
        return 0;
    }

    bmp_24_to_565(bmp->data, buf, bmp->width, bmp->height);

    AndroidBitmap_unlockPixels(env, bm);

    jmethodID setDensity = env->GetMethodID(bitmapClass, "setDensity", "(I)V");
    env->CallVoidMethod(bm, setDensity, (jint) k2pdfopt->pages[page]->dpi);

    return bm;
}

JNIEXPORT void JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_close(JNIEnv *env, jobject thiz) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    if (k2pdfopt != NULL) {
        MASTERINFO *masterinfo = &k2pdfopt->masterinfo;
        K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;
        masterinfo_free(masterinfo, k2settings);

        for (int i = 0; i < k2pdfopt->rects.size(); i++) {
            delete k2pdfopt->rects[i];
        }
        k2pdfopt->rects.clear();

        for (int i = 0; i < k2pdfopt->pages.size(); i++) {
            bmp_free(&k2pdfopt->pages[i]->bmp);
            delete k2pdfopt->pages[i];
        }
        k2pdfopt->pages.clear();

        delete k2pdfopt;

        env->SetLongField(thiz, fid, 0);
    }
}

JNIEXPORT jobject JNICALL
Java_com_github_axet_k2pdfopt_K2PdfOpt_getRectMaps(JNIEnv *env, jobject thiz, jint pageIndex) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "handle", "J");
    k2pdfopt_t k2pdfopt = (k2pdfopt_t) env->GetLongField(thiz, fid);

    K2PDFOPT_SETTINGS *k2settings = &k2pdfopt->k2settings;

    jclass mapClass = env->FindClass("java/util/HashMap");
    jmethodID mapInit = env->GetMethodID(mapClass, "<init>", "()V");
    jobject map = env->NewObject(mapClass, mapInit);
    jmethodID put = env->GetMethodID(mapClass, "put",
                                     "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    jclass rectCls = env->FindClass("android/graphics/Rect");
    jmethodID rectInit = env->GetMethodID(rectCls, "<init>", "(IIII)V");

    PAGERECTS *rects = k2pdfopt->rects[pageIndex];
    for (int i = 0; i < rects->size(); i++) {
        WRECTMAP *m = &(*rects)[i];
        double kx = k2settings->src_dpi / m->srcdpiw;
        double ky = k2settings->src_dpi / m->srcdpih;
        POINT2D *src = &m->coords[0];
        POINT2D *dst = &m->coords[1];
        POINT2D *size = &m->coords[2];
        double srcx = src->x * kx;
        double srcy = src->y * ky;
        double sizex = size->x * kx;
        double sizey = size->y * ky;
        jobject k = env->NewObject(rectCls, rectInit, (int) srcx, (int) srcy,
                                   (int) (srcx + sizex),
                                   (int) (srcy + sizey));
        jobject v = env->NewObject(rectCls, rectInit, (int) dst->x, (int) dst->y,
                                   (int) (dst->x + size->x),
                                   (int) (dst->y + size->y));
        env->CallObjectMethod(map, put, k, v);

        env->DeleteLocalRef(k);
        env->DeleteLocalRef(v);
    }
    return map;
}

}
