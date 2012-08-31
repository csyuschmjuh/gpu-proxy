if CHECK
noinst_PROGRAMS = \
	gpuprocess_test

#FIXME: remove this workaround
rootsrcdir=../..

gpuprocess_test_SOURCES = \
	$(rootsrcdir)/src/gpuprocess_dispatch.h \
	$(rootsrcdir)/src/gpuprocess_dispatch.c \
	$(rootsrcdir)/src/gpuprocess_egl_states.h \
        $(rootsrcdir)/src/gpuprocess_egl_server_private.h \
	$(rootsrcdir)/src/gpuprocess_egl_server.c \
	$(rootsrcdir)/src/gpuprocess_egl_server_helper.c \
	tst_egl.c \
	main.c

gpuprocess_test_LDFLAGS = \
	-lcheck -lX11 -ldl

gpuprocess_test_CFLAGS = \
	-Werror
endif
