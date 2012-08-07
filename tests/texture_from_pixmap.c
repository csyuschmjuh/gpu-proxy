/*
 * Mesa 3-D graphics library
 * Version:  7.1
 * 
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gpuprocess_dispatch_private.h>

gpuprocess_dispatch_t dispatch;

static float top, bottom;

static Display *
OpenDisplay(void)
{
   int screen;
   Display *dpy;
   const char *ext;

   dpy = XOpenDisplay(NULL);
   if (!dpy) {
      printf("Couldn't open default display!\n");
      exit(1);
   }

   screen = DefaultScreen(dpy);

   return dpy;
}


static GLXFBConfig
ChoosePixmapFBConfig(Display *display)
{
   int screen = DefaultScreen(display);
   GLXFBConfig *fbconfigs;
   int i, nfbconfigs = 0, value;

   top = 0.0f;
    bottom = 1.0f;
      int attribs[] = {
        GLX_DOUBLEBUFFER, False,
        GLX_BIND_TO_TEXTURE_RGB_EXT, True,
        GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
        GLX_X_RENDERABLE, True,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DEPTH_SIZE, 0,
        0
    };

   fbconfigs = dispatch.ChooseFBConfig(display, screen, attribs, &nfbconfigs);

   return fbconfigs[0];
}


static GLXPixmap
CreatePixmap(Display *dpy, GLXFBConfig config, int w, int h, Pixmap *p)
{
   GLXPixmap gp;
   const int pixmapAttribs[] = {
      GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
      GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
      None
   };
   Window root = RootWindow(dpy, 0);

   *p = XCreatePixmap(dpy, root, w, h, 24);
   gp = dispatch.CreatePixmap(dpy, config, *p, pixmapAttribs);
   fprintf(stderr, "glxpixmap: %ld", gp);

   return gp;
}


static void
DrawPixmapImage(Display *dpy, Pixmap pm, int w, int h)
{
   XGCValues gcvals;
   GC gc;

   gcvals.background = 0;
   gc = XCreateGC(dpy, pm, GCBackground, &gcvals);

   XSetForeground(dpy, gc, 0x0);
   XFillRectangle(dpy, pm, gc, 0, 0, w, h);

   XSetForeground(dpy, gc, 0xff0000);
   XFillRectangle(dpy, pm, gc, 0, 0, 50, 50);

   XSetForeground(dpy, gc, 0x00ff00);
   XFillRectangle(dpy, pm, gc, w - 50, 0, 50, 50);

   XSetForeground(dpy, gc, 0x0000ff);
   XFillRectangle(dpy, pm, gc, 0, h - 50, 50, 50);

   XSetForeground(dpy, gc, 0xffffff);
   XFillRectangle(dpy, pm, gc, h - 50, h - 50, 50, 50);

   XSetForeground(dpy, gc, 0xffff00);
   XSetLineAttributes(dpy, gc, 3, LineSolid, CapButt, JoinBevel);
   XDrawLine(dpy, pm, gc, 0, 0, w, h);
   XDrawLine(dpy, pm, gc, 0, h, w, 0);

   XFreeGC(dpy, gc);
}


static XVisualInfo *
ChooseWindowVisual(Display *dpy)
{
   int screen = DefaultScreen(dpy);
   XVisualInfo *visinfo;
   int attribs[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_DOUBLEBUFFER,
      None
   };

   visinfo = dispatch.ChooseVisual(dpy, screen, attribs);
   if (!visinfo) {
      printf("Unable to find RGB, double-buffered visual\n");
      exit(1);
   }

   return visinfo;
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
BindPixmapTexture(Display *dpy, GLXPixmap gp)
{
   GLuint texture;

   dispatch.GenTextures(1, &texture);
   dispatch.BindTexture(GL_TEXTURE_2D, texture);

   dispatch.BindTexImageEXT(dpy, gp, GLX_FRONT_LEFT_EXT, NULL);

   dispatch.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   dispatch.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   dispatch.Enable(GL_TEXTURE_2D);
}


static void
Resize(Window win, unsigned int width, unsigned int height)
{
   float sz = 1.5;
   dispatch.Viewport(0, 0, width, height);
   dispatch.MatrixMode(GL_PROJECTION);
   dispatch.LoadIdentity();
   dispatch.Ortho(-sz, sz, -sz, sz, -1.0, 1.0);
   dispatch.MatrixMode(GL_MODELVIEW);
}


static void
Redraw(Display *dpy, Window win, float rot)
{
   dispatch.ClearColor(0.25, 0.25, 0.25, 0.0);
   dispatch.Clear(GL_COLOR_BUFFER_BIT);
   dispatch.PushMatrix();
   dispatch.Rotatef(rot, 0, 0, 1);
   dispatch.Rotatef(2.0 * rot, 1, 0, 0);

   dispatch.Begin(GL_QUADS);
   dispatch.TexCoord2d(0.0, bottom);
   dispatch.Vertex2f(-1, -1);
   dispatch.TexCoord2d(1.0, bottom);
   dispatch.Vertex2f( 1, -1);
   dispatch.TexCoord2d(1.0, top);
   dispatch.Vertex2d(1.0, 1.0);
   dispatch.TexCoord2d(0.0, top);
   dispatch.Vertex2f(-1.0, 1.0);
   dispatch.End();

   dispatch.PopMatrix();

   dispatch.SwapBuffers(dpy, win);
}


static void
EventLoop(Display *dpy, Window win)
{
   GLfloat rot = 0.0;
   int anim = 0;
 
   while (1) {
      if (!anim || XPending(dpy) > 0) {
         XEvent event;
         XNextEvent(dpy, &event);

         switch (event.type) {
         case Expose:
            Redraw(dpy, win, rot);
            break;
         case ConfigureNotify:
            Resize(event.xany.window,
                   event.xconfigure.width,
                   event.xconfigure.height);
            break;
         case KeyPress:
            {
               char buf[100];
               KeySym keySym;
               XComposeStatus stat;
               XLookupString(&event.xkey, buf, sizeof(buf), &keySym, &stat);
               if (keySym == XK_Escape) {
                  return; /* exit */
               }
               else if (keySym == XK_r) {
                  rot += 1.0;
                  Redraw(dpy, win, rot);
               }
               else if (keySym == XK_a) {
                  anim = !anim;
               }
               else if (keySym == XK_R) {
                  rot -= 1.0;
                  Redraw(dpy, win, rot);
               }
            }
            break;
         default:
            ; /*no-op*/
         }
      }
      else {
         /* animate */
         rot += 1.0;
         Redraw(dpy, win, rot);
      }
   }
}



int
main(int argc, char *argv[])
{
   Display *dpy;
   GLXFBConfig pixmapConfig;
   XVisualInfo *windowVis;
   GLXPixmap gp;
   Window win;
   GLXContext ctx;
   Pixmap p;

   gpuprocess_dispatch_init(&dispatch);

   dpy = OpenDisplay();

   pixmapConfig = ChoosePixmapFBConfig(dpy);
   windowVis = ChooseWindowVisual(dpy);
   win = CreateWindow(dpy, windowVis, 500, 500, "Texture From Pixmap");

   gp = CreatePixmap(dpy, pixmapConfig, 512, 512, &p);

   DrawPixmapImage(dpy, p, 512, 512);

   ctx = dispatch.CreateContext(dpy, windowVis, NULL, True);
   if (!ctx) {
      printf("Couldn't create GLX context\n");
      exit(1);
   }

   dispatch.MakeCurrent(dpy, win, ctx);

   BindPixmapTexture(dpy, gp);

   EventLoop(dpy, win);

   return 0;
}
