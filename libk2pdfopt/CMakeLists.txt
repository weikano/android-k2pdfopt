cmake_minimum_required(VERSION 3.4.1)

add_definitions( -DUSE_CMAKE -DK2PDFOPT_KINDLEPDFVIEWER )

add_library( willus

             SHARED

             src/main/cpp/willus/ansi.c
             src/main/cpp/willus/array.c
             src/main/cpp/willus/bmp.c
             src/main/cpp/willus/bmpdjvu.c
             src/main/cpp/willus/bmpmupdf.c
             src/main/cpp/willus/dtcompress.c
             src/main/cpp/willus/filelist.c
             src/main/cpp/willus/fontdata.c
             src/main/cpp/willus/fontrender.c
             src/main/cpp/willus/gslpolyfit.c
             src/main/cpp/willus/linux.c
             src/main/cpp/willus/math.c
             src/main/cpp/willus/mem.c
             src/main/cpp/willus/ocr.c
             src/main/cpp/willus/ocrgocr.c
             src/main/cpp/willus/pdfwrite.c
             src/main/cpp/willus/point2d.c
             src/main/cpp/willus/render.c
             src/main/cpp/willus/strbuf.c
             src/main/cpp/willus/string.c
             src/main/cpp/willus/token.c
             src/main/cpp/willus/wfile.c
             src/main/cpp/willus/wgs.c
             src/main/cpp/willus/wgui.c
             src/main/cpp/willus/willusversion.c
             src/main/cpp/willus/win.c
             src/main/cpp/willus/winbmp.c
             src/main/cpp/willus/wincomdlg.c
             src/main/cpp/willus/winmbox.c
             src/main/cpp/willus/winshell.c
             src/main/cpp/willus/winshellwapi.c
             # src/main/cpp/willus/ocrtess.c
             # src/main/cpp/willus/wleptonica.c
             src/main/cpp/willus/wmupdf.c
             src/main/cpp/willus/wmupdfinfo.c
             src/main/cpp/willus/wpdf.c
             src/main/cpp/willus/wsys.c
             src/main/cpp/willus/wzfile.c
             # src/main/cpp/willus/bmpdjvu.c # DJVU
             # src/main/cpp/willus/bmpmupdf.c src/main/cpp/willus/wmupdf.c # MUPDF
)

include_directories(src/main/cpp/k2pdfopt src/main/cpp/willus src/main/cpp/include_mod )

add_library( k2pdfopt

             SHARED

             src/main/cpp/k2pdfopt/bmpregion.c
             src/main/cpp/k2pdfopt/devprofile.c
             src/main/cpp/k2pdfopt/k2bmp.c
             src/main/cpp/k2pdfopt/k2file.c
             src/main/cpp/k2pdfopt/k2files.c
             src/main/cpp/k2pdfopt/k2gui.c
             src/main/cpp/k2pdfopt/k2gui_cbox.c
             src/main/cpp/k2pdfopt/k2gui_osdep.c
             src/main/cpp/k2pdfopt/k2gui_overlay.c
             src/main/cpp/k2pdfopt/k2mark.c
             src/main/cpp/k2pdfopt/k2master.c
             src/main/cpp/k2pdfopt/k2mem.c
             src/main/cpp/k2pdfopt/k2menu.c
             src/main/cpp/k2pdfopt/k2ocr.c
             src/main/cpp/k2pdfopt/k2parsecmd.c
             src/main/cpp/k2pdfopt/k2proc.c
             src/main/cpp/k2pdfopt/k2publish.c
             src/main/cpp/k2pdfopt/k2settings.c
             src/main/cpp/k2pdfopt/k2settings2cmd.c
             src/main/cpp/k2pdfopt/k2sys.c
             src/main/cpp/k2pdfopt/k2usage.c
             src/main/cpp/k2pdfopt/k2version.c
             src/main/cpp/k2pdfopt/pagelist.c
             src/main/cpp/k2pdfopt/pageregions.c
             src/main/cpp/k2pdfopt/textrows.c
             src/main/cpp/k2pdfopt/textwords.c
             src/main/cpp/k2pdfopt/userinput.c
             src/main/cpp/k2pdfopt/wrapbmp.c )

find_library( log-lib log )

find_library( z-lib z )

target_link_libraries( willus ${log-lib} ${z-lib} )

target_link_libraries( k2pdfopt ${log-lib} ${z-lib} willus )
