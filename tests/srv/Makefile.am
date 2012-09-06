if HAS_UNIT_TESTS
noinst_PROGRAMS = \
	gpuproxy_test

#FIXME: remove this workaround
rootsrcdir=../..

gpuproxy_test_SOURCES = \
	$(rootsrcdir)/src/dispatch.h \
	$(rootsrcdir)/src/dispatch.c \
	$(rootsrcdir)/src/egl_states.h \
	$(rootsrcdir)/src/egl_server_private.h \
	$(rootsrcdir)/src/egl_server.c \
	$(rootsrcdir)/src/egl_server_helper.c \
	gpuprocess_test.h \
	gpuprocess_test.c \
	tst_egl.c \
	main.c

gpuproxy_test_LDFLAGS = \
	-lX11 -ldl

gpuproxy_test_CFLAGS = \
	-Werror
endif
