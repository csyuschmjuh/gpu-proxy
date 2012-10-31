#include "test_gles.h"
#include "server.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <EGL/egl.h>

#include <stdio.h>
#include <string.h>

Display *dpy1 = NULL;
EGLDisplay egl_dpy;
EGLContext window_context;
server_t *server;

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
    int count;

    if (! server->dispatch.eglGetConfigAttrib (server, egl_dpy,
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

    buffer_t *first_buffer = malloc (sizeof (buffer_t));
    buffer_create (first_buffer, 512, "command_buffer");
    server = (server_t *) server_new (first_buffer);

    egl_dpy = server->dispatch.eglGetDisplay (server, dpy1);
    printf ("egl display = %p\n", egl_dpy);
    GPUPROCESS_FAIL_IF (egl_dpy == EGL_NO_DISPLAY, "eglGetDisplay failed");

    EGLint major;
    EGLint minor;
    EGLBoolean result;

    major = -1;
    minor = -1;
    result = server->dispatch.eglInitialize (server, egl_dpy, &major, &minor);
    GPUPROCESS_FAIL_IF (!result, "eglInitialize failed");
    GPUPROCESS_FAIL_IF (major < 0 && minor < 0, "eglInitialize returned wrong major and minor values");

    server->dispatch.eglBindAPI (server, EGL_OPENGL_ES_API);

    EGLint config_list_length;
    EGLConfig config;
    EGLint old_config_list_length;
    EGLConfig *config_list = NULL;
    result = server->dispatch.eglGetConfigs (server, egl_dpy, NULL, 0, &config_list_length);
    config_list = (EGLConfig *) malloc (config_list_length * sizeof (EGLConfig));
    old_config_list_length = config_list_length;
    result = server->dispatch.eglGetConfigs (server, egl_dpy, config_list, config_list_length, &config_list_length);
    GPUPROCESS_FAIL_IF (!result, "eglGetConfigs failed");
    GPUPROCESS_FAIL_UNLESS (config_list_length == old_config_list_length, "They should have the same length");

    static const EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
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

    result = server->dispatch.eglChooseConfig (server, egl_dpy, window_attribs, &config, 1, &config_list_length);
    GPUPROCESS_FAIL_IF (!result, "eglChooseConfig failed");

    window_context = server->dispatch.eglCreateContext (server, egl_dpy, config, EGL_NO_CONTEXT, ctx_attribs);
    GPUPROCESS_FAIL_IF (! window_context, "context should be created");

    Window win;
    XVisualInfo *vinfo;

    vinfo = ChooseWindowVisual(dpy1, egl_dpy, config);
    win = CreateWindow(dpy1, vinfo, 400, 400, "gpu_proxy test");

    EGLSurface window_surface = server->dispatch.eglCreateWindowSurface (server, egl_dpy,
                                                                 config,
                                                                 win,NULL);
    GPUPROCESS_FAIL_IF (! window_surface, "surface creation failed");
 
    result = server->dispatch.eglMakeCurrent (server, egl_dpy,
                                window_surface,window_surface,
                                window_context);

 GPUPROCESS_FAIL_IF (result == EGL_FALSE, "eglMakeCurrent failed");

}
static void
teardown (void)
{
    server->dispatch.eglDestroyContext (server, egl_dpy, window_context);
    server->dispatch.eglTerminate (server, egl_dpy);
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

/*    shader = glCreateShader(server,  GL_POINTS );
    GPUPROCESS_FAIL_IF (!shader, "glCreateShader can not be created by an invalid type");

    get_error_result = glGetError(server);
    GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_ENUM, "glGetError should return GL_INVALID_ENUM");
 */   
    shader = server->dispatch.glCreateShader(server, type);
 
    GPUPROCESS_FAIL_IF(!shader, "glCreateShader FAILED ");

    // Load the shader source
    // we must create a copy of source and pass the copy of source
 
    length = strlen (shaderSrc);
    if (length > 0) {
	copySrc = (char **)malloc (sizeof (char));
        copySrc[0] = (char *)malloc (sizeof (char) * (length+1));
        memcpy (copySrc[0], shaderSrc, sizeof (char) * (length+1));
    }
    server->dispatch.glShaderSource(server, shader, 1, (const char **)copySrc, NULL );

     get_error_result = server->dispatch.glGetError(server);
     GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE,"glGetError should return GL_INVALID_VALUE");
  
   // Compile the shader
    server->dispatch.glCompileShader (server,  shader );
    
    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_OPERATION,"glGetError should return GL_INVALID_OPERATION");

   // Check the compile status
    server->dispatch.glGetShaderiv(server,  shader, GL_COMPILE_STATUS, &compiled );
    GPUPROCESS_FAIL_UNLESS (compiled,"glGetShaderiv should return a non zero compiled object");

    if ( !compiled )
   {
      GLint infoLen = 0;

      server->dispatch.glGetShaderiv(server,  shader, GL_INFO_LOG_LENGTH, &infoLen );
     
      GPUPROCESS_FAIL_UNLESS ( infoLen != 0 ,"glGetShaderiv should return zero szie for information log since shader is invalid");
      
      server->dispatch.glGetShaderiv(server,  shader, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char* infoLog = malloc (sizeof(char) * infoLen );

        server->dispatch.glGetShaderInfoLog(server,  shader, 0, NULL, infoLog );

        get_error_result = server->dispatch.glGetError(server);
        GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE for invalid max length to glGetShaderInfoLog");
     
        server->dispatch.glGetShaderInfoLog(server,  shader, infoLen, NULL, infoLog );

//         free ( infoLog );
      }

      server->dispatch.glDeleteShader(server, -1);

       get_error_result = server->dispatch.glGetError(server);
       GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE for an invalid shader");
      
      server->dispatch.glDeleteShader(server,  shader );
      return 0;
   }

    return shader;

}

GPUPROCESS_START_TEST
(test_gl_initloaddraw)
{
    UserData *userData = malloc(sizeof(UserData));
    const char *vShaderStr =
        "attribute vec4 vPosition;    \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = vPosition;  \n"
        "}                            \n";

    const char *fShaderStr =
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
    programObject = server->dispatch.glCreateProgram (server);

    if ( programObject == 0 )
        return;

    GPUPROCESS_FAIL_IF(!programObject,"error creating program object");

    server->dispatch.glAttachShader(server,  programObject, -1 );

    GLint get_error_result = server->dispatch.glGetError(server);

    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_OPERATION, "glGetError should return GL_INVALID_OPERATION");

    server->dispatch.glAttachShader(server,  vertexShader,programObject);

    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_IF(get_error_result == GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE");

    server->dispatch.glAttachShader(server,  programObject, vertexShader );
    server->dispatch.glAttachShader(server,  programObject, fragmentShader );

    // Bind vPosition to attribute 0
    name = (char *)malloc (sizeof(char) * (len+1));
    memcpy (name, "vPosition", len);
    name[len] = 0;
    server->dispatch.glBindAttribLocation(server,  programObject, GL_MAX_VERTEX_ATTRIBS+1, name );

    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_IF(get_error_result != GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE");

    name = (char *)malloc (sizeof(char) * (len+1));
    memcpy (name, "vPosition", len);
    name[len] = 0;
    server->dispatch.glBindAttribLocation(server,  programObject, 0, name );

    // Link the program
    server->dispatch.glLinkProgram(server,  -1 );
    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_IF(get_error_result != GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE");

    server->dispatch.glLinkProgram(server,  vertexShader );
    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_IF(get_error_result == GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE");

    server->dispatch.glLinkProgram(server,  programObject );
    // Check the link status
    server->dispatch.glGetProgramiv(server,  -1, GL_LINK_STATUS, &linked );

    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_IF(get_error_result != GL_INVALID_VALUE, "server->dispatch.glGetError should return GL_INVALID_VALUE");
 
    server->dispatch.glGetProgramiv(server,  programObject, GL_LINK_STATUS, &linked );

    if ( !linked )
   {
      GLint infoLen = 0;

      server->dispatch.glGetProgramiv(server,  programObject, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
        char* infoLog = malloc (sizeof(char) * infoLen );

        server->dispatch.glGetProgramInfoLog(server,  programObject, 0, NULL, infoLog );

        get_error_result = server->dispatch.glGetError(server);

        GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE for invalid max length to glGetProgramInfoLog");

        server->dispatch.glGetProgramInfoLog (server,  programObject, infoLen, NULL, infoLog );

//         free ( infoLog );
      }

      server->dispatch.glDeleteProgram(server,  -1 );
      get_error_result = server->dispatch.glGetError(server);
      GPUPROCESS_FAIL_IF(get_error_result != GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE");

      server->dispatch.glDeleteProgram(server,  programObject );
      return;
   }

   // Store the program object
    userData->programObject = programObject;

    server->dispatch.glClearColor(server,  0.0f, 0.0f, 0.0f, 0.0f );
   //UserData *userData = esContext->userData;
    GLfloat vVertices[] = {  0.0f,  0.5f, 0.0f,
                           -0.5f, -0.5f, 0.0f,
                            0.5f, -0.5f, 0.0f };

   // Set the viewport
    server->dispatch.glViewport(server,  0, 0, -200,200);
    get_error_result = server->dispatch.glGetError(server);

  GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE for negative width or height");
 
    server->dispatch.glViewport(server,  0, 0, 200,200);

   // Clear the color buffer
    server->dispatch.glClear(server,  GL_POINTS );
    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE for GL_POINTS");

    server->dispatch.glClear(server,  GL_COLOR_BUFFER_BIT );

    // Use the program object
    server->dispatch.glUseProgram(server, vertexShader);
    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_OPERATION, "glGetError should return GL_INVALID_VALUE for GL_INVALID_OPERATION");

    server->dispatch.glUseProgram(server,  userData->programObject );

   // Load the vertex data
    server->dispatch.glVertexAttribPointer(server,  0, 5, GL_FLOAT, GL_FALSE, 0, vVertices );
    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE for size > 4");

    server->dispatch.glVertexAttribPointer(server,  0, 3, GL_POINTS, GL_FALSE, 0, vVertices );
    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_ENUM, "glGetError should return GL_INVALID_ENUM for invalid ENUM type GL_POINTS");

    server->dispatch.glVertexAttribPointer(server,  0, 5, GL_FLOAT, GL_FALSE,-10, vVertices );
    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE for negative stride");

    server->dispatch.glVertexAttribPointer(server,  0, 3, GL_FLOAT, GL_FALSE, 0, vVertices );

    server->dispatch.glEnableVertexAttribArray (server,  GL_MAX_VERTEX_ATTRIBS);
    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE for index  >= GL_MAX_VERTEX_ATTRIBS");

    server->dispatch.glEnableVertexAttribArray (server,  0 );
    server->dispatch.glDrawArrays(server,  GL_ARRAY_BUFFER, 0, 3 );
    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_ENUM, "glGetError should return GL_INVALID_ENUM for invalid ENUM type GL_ARRAY_BUFFER");

    server->dispatch.glDrawArrays(server,  GL_TRIANGLES, 0, -3 );
    get_error_result = server->dispatch.glGetError(server);
    GPUPROCESS_FAIL_UNLESS(get_error_result == GL_INVALID_VALUE, "glGetError should return GL_INVALID_VALUE for negative count");

    server->dispatch.glDrawArrays(server,  GL_TRIANGLES, 0, 3 );

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
