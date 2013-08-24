AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/openfimg \
	$(PTHREADSTUBS_CFLAGS) \
	-I$(top_srcdir)/include/drm

libdrm_openfimg_la_LTLIBRARIES = libdrm_openfimg.la
libdrm_openfimg_ladir = $(libdir)
libdrm_openfimg_la_LDFLAGS = -version-number 1:0:0 -no-undefined
libdrm_openfimg_la_LIBADD = ../libdrm.la @PTHREADSTUBS_LIBS@

libdrm_openfimg_la_SOURCES = \
	openfimg_device.c \
	openfimg_pipe.c \
	openfimg_priv.h \
	openfimg_ringbuffer.c \
	openfimg_bo.c \
	list.h \
	exynos_drm.h

libdrm_openfimgcommonincludedir = ${includedir}/openfimg
libdrm_openfimgcommoninclude_HEADERS = openfimg_drmif.h openfimg_ringbuffer.h

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdrm_openfimg.pc
