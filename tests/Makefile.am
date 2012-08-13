AM_LDFLAGS = \
	-ldl

if HAS_GL
noinst_PROGRAMS = \
        texture_from_pixmap

texture_from_pixmap_SOURCES = \
	texture_from_pixmap.c

texture_from_pixmap_CFLAGS = \
	$(GPUPROCESS_CFLAGS) \
	-I$(top_srcdir)/src \
	-I$(srcdir).

texture_from_pixmap_LDFLAGS = \
        -lX11

texture_from_pixmap_LDADD = \
	$(top_builddir)/src/libGL.la
	libGL.la
else
noinst_PROGRAMS = \
	es2tri

es2tri_SOURCES = \
        tests/es2tri.c

es2tri_CFLAGS = \
        $(GPUPROCESS_CFLAGS) \
        -I$(srcdir)/src \
        -I$(srcdir).

es2tri_LDFLAGS = \
        -lX11 -lm

es2tri_LDADD = \
	$(top_builddir)/src/libGL.la
endif
