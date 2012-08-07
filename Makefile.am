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

noinst_PROGRAMS = \
        texture_from_pixmap

texture_from_pixmap_SOURCES = \
	tests/texture_from_pixmap.c

texture_from_pixmap_CFLAGS = \
	-I$(srcdir)/src

texture_from_pixmap_LDFLAGS = \
        -lX11

texture_from_pixmap_LDADD = \
	libGL.la

