#include "test_gles.h"
#include "gles2_server_tests.h"
#include "egl_server_tests.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <EGL/egl.h>

#include <stdio.h>
#include <string.h>

Display *dpy1 = NULL;
EGLDisplay egl_dpy;
EGLContext window_context;
typedef struct
{
   // Handle to a program object
   GLuint programObject;

} UserData;

static XVisualInfo *
ChooseWindowVisual(Display *dpy, EGLDisplay egl_dpy, EGLConfig cfg)
{
    XVisualInfo template, *vinfo;
    EGLint id;
    unsigned long mask;
    int screen = DefaultScreen(dpy);
    Window root_win = RootWindow(dpy, screen);
    int count;

    if (! _egl_get_config_attrib (NULL, egl_dpy,
                                  cfg, EGL_NATIVE_VISUAL_ID, &id)) {
        printf ("eglGetConfigAttrib() failed\n");
    }

    template.visualid = id;
    vinfo = XGetVisualInfo (dpy, VisualIDMask, &template, &count);
    if (count != 1) {
        printf ("XGetVisualInfo() failed\n");
    }

    return vinfo;
}

static Window
CreateWindow(Display *dpy, XVisualInfo *visinfo,
             int width, int height, const char *name)
{
    int screen = DefaultScreen(dpy);
    Window win;
    XSetWindowAttributes attr;
    unsigned long mask;
    Window root;

    root = RootWindow(dpy, screen);

    /* window attributes */
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
    attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    win = XCreateWindow(dpy, root, 0, 0, width, height,
                        0, visinfo->depth, InputOutput,
                        visinfo->visual, mask, &attr);
    if (win) {
        XSizeHints sizehints;
        sizehints.width  = width;
        sizehints.height = height;
        sizehints.flags = USSize;
        XSetNormalHints(dpy, win, &sizehints);
        XSetStandardProperties(dpy, win, name, name,
                               None, (char **)NULL, 0, &sizehints);

        XMapWindow(dpy, win);
    }
    return win;
}

static void
setup (void)
{

    dpy1 = XOpenDisplay (NULL);
    GPUPROCESS_FAIL_IF (dpy1 == NULL, "XOpenDisplay should work");

 //EGL Display 
    egl_dpy = _egl_get_display (NULL, dpy1);
    printf ("egl display = %p\n", egl_dpy);
    GPUPROCESS_FAIL_IF (egl_dpy == EGL_NO_DISPLAY, "_egl_get_display failed");

//EGL Initialization
    EGLint major;
    EGLint minor;
    EGLBoolean result;

    major = -1;
    minor = -1;
    result = _egl_initialize (NULL, egl_dpy, &major, &minor);
    GPUPROCESS_FAIL_IF (!result, "_egl_initialize failed");
    GPUPROCESS_FAIL_IF (major < 0 && minor < 0, "_egl_initialize returned wrong major and minor values");

//EGL Bind API
    _egl_bind_api (NULL, EGL_OPENGL_ES_API);

//EGL Get Config     
    EGLint config_list_length;
    EGLConfig config;
    EGLint old_config_list_length;
    EGLConfig *config_list = NULL;
    EGLint get_error_result;
    result = _egl_get_configs (NULL, egl_dpy, NULL, 0, &config_list_length);
    config_list = (EGLConfig *) malloc (config_list_length * sizeof (EGLConfig));
    old_config_list_length = config_list_length;
    result = _egl_get_configs (NULL, egl_dpy, config_list, config_list_length, &config_list_length);
    GPUPROCESS_FAIL_IF (!result, "_egl_get_configs failed");
    GPUPROCESS_FAIL_UNLESS (config_list_length == old_config_list_length, "They should have the same length");
//    free (config_list);

//EGL Surface and Context creation

    static const EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    static const EGLint pbuffer_attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_NONE
    };
    static const EGLint window_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_NONE
    };

    result = _egl_choose_config (NULL, egl_dpy, window_attribs, &config, 1, &config_list_length);
    GPUPROCESS_FAIL_IF (!result, "_egl_choose_config failed");
    
    window_context = _egl_create_context (NULL, egl_dpy, config, EGL_NO_CONTEXT, ctx_attribs);
    GPUPROCESS_FAIL_IF (! window_context, "context should be created");
    
    Window win;
    XVisualInfo *vinfo; 
   
    vinfo = ChooseWindowVisual(dpy1, egl_dpy, config);
    win = CreateWindow(dpy1, vinfo, 400, 400, "gpu_proxy test");

    EGLSurface window_surface = _egl_create_window_surface (NULL, egl_dpy,
                                                                 config,
                                                                 win,NULL);
    GPUPROCESS_FAIL_IF (! window_surface, "surface creation failed");
 
    result = _egl_make_current (NULL, egl_dpy,
                                window_surface,window_surface,
                                window_context);

 GPUPROCESS_FAIL_IF (result == EGL_FALSE, "_egl_make_current failed");

}
static void
teardown (void)
{
 _egl_destroy_context (NULL, egl_dpy, window_context);
 _egl_terminate (NULL, egl_dpy);
  XCloseDisplay(dpy1);
}

GLuint LoadShader ( GLenum type, const char *shaderSrc )
{
   GLuint shader;
   GLint compiled;
   GLint get_error_result;
   char **copySrc = NULL;
   int length = 0;
   // Create the shader object

/*    shader = _gl_create_shader(NULL,  GL_POINTS );
    GPUPROCESS_FAIL_IF (!shader, "_gl_create_shader can not be created by an invalid type");

    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_ENUM, "_gl_get_error should return GL_INVALID_ENUM");
 */   
    shader = _gl_create_shader(NULL, type);
 
    GPUPROCESS_FAIL_IF(!shader, "_gl_create_shader FAILED ");

    // Load the shader source
    // we must create a copy of source and pass the copy of source
 
   length = strlen (shaderSrc);
    if (length > 0) {
	copySrc = (char **)malloc (sizeof (char));
        copySrc[0] = (char *)malloc (sizeof (char) * (length+1));
        memcpy (copySrc[0], shaderSrc, sizeof (char) * (length+1));
    }
    _gl_shader_source( NULL, shader, 1, (const char **)copySrc, NULL );

     get_error_result = _gl_get_error(NULL);
     GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE,"_gl_get_error should return GL_INVALID_VALUE");
  
   // Compile the shader
    _gl_compile_shader(NULL,  shader );
    
    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_OPERATION,"_gl_get_error should return GL_INVALID_OPERATION");

   // Check the compile status
   _gl_get_shaderiv(NULL,  shader, GL_COMPILE_STATUS, &compiled );
    GPUPROCESS_FAIL_UNLESS (compiled,"_gl_get_shaderiv should return a non zero compiled object");

   if ( !compiled )
   {
      GLint infoLen = 0;

      _gl_get_shaderiv(NULL,  shader, GL_INFO_LOG_LENGTH, &infoLen );
     
      GPUPROCESS_FAIL_UNLESS ( infoLen != 0 ,"_gl_get_shaderiv should return zero szie for information log since shader is invalid");
      
      _gl_get_shaderiv(NULL,  shader, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char* infoLog = malloc (sizeof(char) * infoLen );

        _gl_get_shader_info_log(NULL,  shader, 0, NULL, infoLog );

        get_error_result = _gl_get_error(NULL);
        GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE for invalid max length to _gl_get_shader_info_log");
     
        _gl_get_shader_info_log(NULL,  shader, infoLen, NULL, infoLog );

//         free ( infoLog );
      }

      _gl_delete_shader(NULL, -1);

       get_error_result = _gl_get_error(NULL);
       GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE for an invalid shader");
      
      _gl_delete_shader(NULL,  shader );
      return 0;
   }

   return shader;

}

GPUPROCESS_START_TEST
(test_gl_initloaddraw)
{
   UserData *userData = malloc(sizeof(UserData));
   GLbyte vShaderStr[] =
      "attribute vec4 vPosition;    \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = vPosition;  \n"
      "}                            \n";

   GLbyte fShaderStr[] =
      "precision mediump float;\n"\
      "void main()                                  \n"
      "{                                            \n"
      "  gl_FragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );\n"
      "}                                            \n";

   GLuint vertexShader;
   GLuint fragmentShader;
   GLuint programObject;
   GLint linked;
   char *name;
   int len = strlen ("vPosition");

   // Load the vertex/fragment shaders
   vertexShader = LoadShader ( GL_VERTEX_SHADER, vShaderStr );
   fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fShaderStr );

   // Create the program object
   programObject = _gl_create_program (NULL);

   if ( programObject == 0 )
      return;

   GPUPROCESS_FAIL_IF(!programObject,"error creating program object");
  
   _gl_attach_shader(NULL,  programObject, -1 );
   
    GLint get_error_result = _gl_get_error(NULL);

   GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_OPERATION, "_gl_get_error should return GL_INVALID_OPERATION");
  
   _gl_attach_shader(NULL,  vertexShader,programObject);

    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_IF(get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE");

   _gl_attach_shader(NULL,  programObject, vertexShader );
   _gl_attach_shader(NULL,  programObject, fragmentShader );
   
// Bind vPosition to attribute 0
    name = (char *)malloc (sizeof(char) * (len+1));
    memcpy (name, "vPosition", len);
    name[len] = 0;
   _gl_bind_attrib_location(NULL,  programObject, GL_MAX_VERTEX_ATTRIBS+1, name );

    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_IF(get_error_result != GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE");
  
    name = (char *)malloc (sizeof(char) * (len+1));
    memcpy (name, "vPosition", len);
    name[len] = 0;
    _gl_bind_attrib_location(NULL,  programObject, 0, name );

   // Link the program
   _gl_link_program(NULL,  -1 );
    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_IF(get_error_result != GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE");
   
   _gl_link_program(NULL,  vertexShader );
    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_IF(get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE");

   _gl_link_program(NULL,  programObject );
  // Check the link status
   _gl_get_programiv(NULL,  -1, GL_LINK_STATUS, &linked );

    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_IF(get_error_result != GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE");
 
   _gl_get_programiv(NULL,  programObject, GL_LINK_STATUS, &linked );

   if ( !linked )
   {
      GLint infoLen = 0;

      _gl_get_programiv(NULL,  programObject, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char* infoLog = malloc (sizeof(char) * infoLen );
 
        _gl_get_program_info_log(NULL,  programObject, 0, NULL, infoLog );

        get_error_result = _gl_get_error(NULL);

        GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE for invalid max length to _gl_get_program_info_log");

        _gl_get_program_info_log (NULL,  programObject, infoLen, NULL, infoLog );

//         free ( infoLog );
      }

      _gl_delete_program(NULL,  -1 );
      get_error_result = _gl_get_error(NULL);
      GPUPROCESS_FAIL_IF(get_error_result != GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE");

      _gl_delete_program(NULL,  programObject );
      return;
   }

   // Store the program object
   userData->programObject = programObject;

   _gl_clear_color(NULL,  0.0f, 0.0f, 0.0f, 0.0f );
   //UserData *userData = esContext->userData;
   GLfloat vVertices[] = {  0.0f,  0.5f, 0.0f,
                           -0.5f, -0.5f, 0.0f,
                            0.5f, -0.5f, 0.0f };

   // Set the viewport
   _gl_viewport(NULL,  0, 0, -200,200);
   get_error_result = _gl_get_error(NULL);

  GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE for negative width or height");
 
   _gl_viewport(NULL,  0, 0, 200,200);

   // Clear the color buffer
   _gl_clear(NULL,  GL_POINTS );
    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE for GL_POINTS");

   _gl_clear(NULL,  GL_COLOR_BUFFER_BIT );

   // Use the program object
   _gl_use_program(NULL, vertexShader );
    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_IF(get_error_result == GL_INVALID_OPERATION, "_gl_get_error should return GL_INVALID_VALUE for GL_INVALID_OPERATION");
 
   _gl_use_program(NULL,  userData->programObject );

   // Load the vertex data
   _gl_vertex_attrib_pointer(NULL,  0, 5, GL_FLOAT, GL_FALSE, 0, vVertices );
    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE for size > 4");
  
   _gl_vertex_attrib_pointer(NULL,  0, 3, GL_POINTS, GL_FALSE, 0, vVertices );
    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_ENUM, "_gl_get_error should return GL_INVALID_ENUM for invalid ENUM type GL_POINTS");
   
   _gl_vertex_attrib_pointer(NULL,  0, 5, GL_FLOAT, GL_FALSE,-10, vVertices );
    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE for negative stride");
   
   _gl_vertex_attrib_pointer(NULL,  0, 3, GL_FLOAT, GL_FALSE, 0, vVertices );
   
   _gl_enable_vertex_attrib_array (NULL,  GL_MAX_VERTEX_ATTRIBS);
    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE for index  >= GL_MAX_VERTEX_ATTRIBS");
   
   _gl_enable_vertex_attrib_array (NULL,  0 );
   _gl_draw_arrays(NULL,  GL_ARRAY_BUFFER, 0, 3 );
    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_ENUM, "_gl_get_error should return GL_INVALID_ENUM for invalid ENUM type GL_ARRAY_BUFFER");

   _gl_draw_arrays(NULL,  GL_TRIANGLES, 0, -3 );
    get_error_result = _gl_get_error(NULL);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE for negative count");

   _gl_draw_arrays(NULL,  GL_TRIANGLES, 0, 3 );

}
GPUPROCESS_END_TEST

gpuprocess_suite_t *
gles_testsuite_create(void)
{
    gpuprocess_suite_t *s;
    gpuprocess_testcase_t *tc = NULL;

    tc = gpuprocess_testcase_create("gles");
    gpuprocess_testcase_add_fixture (tc, setup, teardown);
    gpuprocess_testcase_add_test (tc, test_gl_initloaddraw);
    s = gpuprocess_suite_create ("gles");
    gpuprocess_suite_add_testcase (s, tc);

    return s;
}
