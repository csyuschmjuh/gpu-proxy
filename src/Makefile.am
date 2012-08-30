lib_LTLIBRARIES = libGPUProcess.la

libGPUProcess_la_CFLAGS = \
	-Igenerated

libGPUProcess_la_SOURCES = \
	gpuprocess_compiler_private.h \
	gpuprocess_dispatch_private.h \
	gpuprocess_dispatch.h \
	gpuprocess_dispatch.c \
	gpuprocess_gl_server_private.h \
	gpuprocess_gl_states.h \
	gpuprocess_thread_private.h \
	gpuprocess_types_private.h

if HAS_GL
libGPUProcess_la_SOURCES += \
	gpuprocess_glx_states.h
endif

if HAS_GLES2
libGPUProcess_la_SOURCES += \
	gpuprocess_egl_api.c \
	gpuprocess_egl_server.c \
	gpuprocess_egl_server_private.h \
	gpuprocess_egl_server_helper.c \
	gpuprocess_egl_states.h \
	gpuprocess_gles2_api.c \
	gpuprocess_gles2_server.c \
	gpuprocess_gles2_states.h
endif

libGPUProcess_la_LDFLAGS = -version-info 0:0:0

AM_LDFLAGS = \
	-ldl

all-local: libGPUProcess.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR);
	ln -f .libs/libGPUProcess.so.0.0.0 $(top_builddir)/$(LIB_DIR)/libGPUProcess.so
if HAS_GL
	ln -f $(top_builddir)/$(LIB_DIR)/libGPUProcess.so $(top_builddir)/$(LIB_DIR)/libGL.so
else
	ln -sf $(top_builddir)/$(LIB_DIR)/libGPUProcess.so $(top_builddir)/$(LIB_DIR)/libEGL.so
	ln -sf $(top_builddir)/$(LIB_DIR)/libGPUProcess.so $(top_builddir)/$(LIB_DIR)/libGLESv2.so
endif
