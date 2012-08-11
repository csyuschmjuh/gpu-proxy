noinst_LTLIBRARIES  = libGL.la
libGL_la_SOURCES = \
	src/gpuprocess_dispatch.h \
	src/gpuprocess_dispatch_private.h \
	src/gpuprocess_dispatch.c \
	src/egl_states.h \
	src/gl_states.h \
	src/srv_states.h \
	src/gles2_states.h \
	src/glx_states.h

AM_LDFLAGS = \
	-ldl

if HAS_GL
noinst_PROGRAMS = \
        texture_from_pixmap

texture_from_pixmap_SOURCES = \
	tests/texture_from_pixmap.c

texture_from_pixmap_CFLAGS = \
	$(GPUPROCESS_CFLAGS) \
	-I$(srcdir)/src \
	-I.

texture_from_pixmap_LDFLAGS = \
        -lX11

texture_from_pixmap_LDADD = \
	libGL.la
else
noinst_PROGRAMS = \
	es2tri

es2tri_SOURCES = \
        tests/es2tri.c

es2tri_CFLAGS = \
        $(GPUPROCESS_CFLAGS) \
        -I$(srcdir)/src \
        -I.

es2tri_LDFLAGS = \
        -lX11 -lm

es2tri_LDADD = \
        libGL.la
endif
