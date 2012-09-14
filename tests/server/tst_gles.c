#include "tst_gles.h"
#include "gles2_server_tests.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
Display *dpy1 = NULL;
typedef struct
{
   // Handle to a program object
   GLuint programObject;

} UserData;

static void
setup (void)
{
    dpy1 = XOpenDisplay (NULL);
    GPUPROCESS_FAIL_IF (dpy1 == NULL, "XOpenDisplay should work");

}

static void
teardown (void)
{
   XCloseDisplay(dpy1);
}

GLuint LoadShader ( GLenum type, const char *shaderSrc )
{
   GLuint shader;
   GLint compiled;
   GLint get_error_result;
   // Create the shader object
    shader = _gl_create_shader( NULL );
    GPUPROCESS_FAIL_IF (shader, "_gl_create_shader can not be created by an invalid type");

    get_error_result = _gl_get_error();
    GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_ENUM, "_gl_get_error should return GL_INVALID_ENUM");
 
    shader = _gl_create_shader( type );
    GPUPROCESS_FAIL_IF(!shader, "_gl_create_shader FAILED ");

// Load the shader source
    _gl_shader_source( shader, -1, &shaderSrc, NULL );

     get_error_result = _gl_get_error();
     GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE,"_gl_get_error should return GL_INVALID_VALUE");
  
    _gl_shader_source( shader, 1, &shaderSrc, NULL );

   // Compile the shader
    _gl_compile_shader( -1 );
    
    get_error_result = _gl_get_error();
    GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_OPERATION,"_gl_get_error should return GL_INVALID_OPERATION");

   _gl_compile_shader( shader );

   // Check the compile status
   _gl_get_shaderiv( shader, GL_COMPILE_STATUS, &compiled );
    GPUPROCESS_FAIL_UNLESS (!compiled,"_gl_get_shaderiv should return a non zero compiled object");

   if ( !compiled )
   {
      GLint infoLen = 0;

      _gl_get_shaderiv( NULL, GL_INFO_LOG_LENGTH, &infoLen );
     
      GPUPROCESS_FAIL_UNLESS ( infoLen == 0 ,"_gl_get_shaderiv should return zero szie for information log since shader is invalid");
      
      _gl_get_shaderiv( shader, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char* infoLog = malloc (sizeof(char) * infoLen );

        _gl_get_shader_info_log( shader, 0, NULL, infoLog );

        get_error_result = _gl_get_error();
        GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE for invalid max length to _gl_get_shader_info_log");
     
        _gl_get_shader_info_log( shader, infoLen, NULL, infoLog );

         free ( infoLog );
      }

      _gl_delete_shader( NULL );

       get_error_result = _gl_get_error();
       GPUPROCESS_FAIL_UNLESS (get_error_result == GL_INVALID_VALUE, "_gl_get_error should return GL_INVALID_VALUE for an invalid shader");
      
      _gl_delete_shader( shader );
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

   // Load the vertex/fragment shaders
   vertexShader = LoadShader ( GL_VERTEX_SHADER, vShaderStr );
   fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fShaderStr );

   // Create the program object
   programObject = _gl_create_program ( );

   if ( programObject == 0 )
      return 0;

   _gl_attach_shader( programObject, vertexShader );
   _gl_attach_shader( programObject, fragmentShader );

   // Bind vPosition to attribute 0
   _gl_bind_attrib_location( programObject, 0, "vPosition" );

   // Link the program
   _gl_link_program( programObject );
  // Check the link status
   _gl_get_programiv( programObject, GL_LINK_STATUS, &linked );

   if ( !linked )
   {
      GLint infoLen = 0;

      _gl_get_programiv( programObject, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char* infoLog = malloc (sizeof(char) * infoLen );
         _gl_get_program_info_log ( programObject, infoLen, NULL, infoLog );
         free ( infoLog );
      }

      _gl_delete_program( programObject );
      return GL_FALSE;
   }

   // Store the program object
   userData->programObject = programObject;

   _gl_clear_color( 0.0f, 0.0f, 0.0f, 0.0f );
   //UserData *userData = esContext->userData;
   GLfloat vVertices[] = {  0.0f,  0.5f, 0.0f,
                           -0.5f, -0.5f, 0.0f,
                            0.5f, -0.5f, 0.0f };

   // Set the viewport
   _gl_viewport( 0, 0, 200,200);

   // Clear the color buffer
   _gl_clear( GL_COLOR_BUFFER_BIT );

   // Use the program object
   _gl_use_program( userData->programObject );

   // Load the vertex data
   _gl_vertex_attrib_pointer( 0, 3, GL_FLOAT, GL_FALSE, 0, vVertices );
   _gl_enable_vertex_attrib_array ( 0 );

   _gl_draw_arrays( GL_TRIANGLES, 0, 3 );

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
