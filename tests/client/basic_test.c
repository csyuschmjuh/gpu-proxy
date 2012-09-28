#include "basic_test.h"
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

static void
test_basic_egl (void)
{
    GPUPROCESS_ASSERT (dlsym (NULL, "eglGetError"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglGetDisplay"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglInitialize"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglTerminate"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglQueryString"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglGetConfigs"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglChooseConfig"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglGetConfigAttrib"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglCreateWindowSurface"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglCreatePbufferSurface"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglCreatePixmapSurface"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglDestroySurface"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglQuerySurface"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglBindAPI"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglQueryAPI"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglWaitClient"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglReleaseThread"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglCreatePbufferFromClientBuffer"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglSurfaceAttrib"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglBindTexImage"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglReleaseTexImage"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglSwapInterval"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglCreateContext"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglDestroyContext"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglMakeCurrent"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglGetCurrentContext"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglGetCurrentSurface"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglGetCurrentDisplay"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglQueryContext"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglWaitGL"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglWaitNative"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglSwapBuffers"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglCopyBuffers"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglGetProcAddress"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglLockSurfaceKHR"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglUnlockSurfaceKHR"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglCreateImageKHR"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglDestroyImageKHR"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglCreateSyncKHR"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglDestroySyncKHR"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglClientWaitSyncKHR"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglSignalSyncKHR"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglGetSyncAttribKHR"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglCreateFenceSyncNV"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglDestroySyncNV"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglFenceNV"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglClientWaitSyncNV"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglSignalSyncNV"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglGetSyncAttribNV"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglCreatePixmapSurfaceHI"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglCreateDRMImageMESA"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglExportDRMImageMESA"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglPostSubBufferNV"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglQuerySurfacePointerANGLE"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglMapImageSEC"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglUnmapImageSEC"));
    GPUPROCESS_ASSERT (dlsym (NULL, "eglGetImageAttribSEC"));
}

static void
test_basic_gles (void)
{
}

void
add_basic_testcases (gpuprocess_suite_t *suite)
{
    gpuprocess_testcase_t *basic = gpuprocess_testcase_create ("basic");
    gpuprocess_testcase_add_test (basic, test_basic_egl);
    gpuprocess_testcase_add_test (basic, test_basic_gles);
    gpuprocess_suite_add_testcase (suite, basic);
}
