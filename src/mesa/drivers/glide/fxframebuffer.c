/*
 * Mesa 3-D graphics library
 * Version:  6.4.2
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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

/* Authors:
 *    Nejc Dolicanin - Updated for Mesa 6.4.2 framebuffer infrastructure
 */

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

/* FX */

#include "fxdrv.h"
#include "fxrenderbuffer.h"
#include "framebuffer.h"
#include "renderbuffer.h"

/**
 * Create and setup the Mesa framebuffer with new renderbuffer infrastructure
 * This replaces the old framebuffer creation in fxapi.c
 */
struct gl_framebuffer *
fxNewFramebuffer(GLcontext *ctx, const GLvisual *visual)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   struct gl_framebuffer *fb;
   struct gl_renderbuffer *frontRb, *backRb, *depthRb = NULL, *stencilRb;
   GLboolean swStencil;

   if (TDFX_DEBUG & VERBOSE_DRIVER)
   {
      fprintf(stderr, "fxNewFramebuffer()\n");
   }

   fb = _mesa_create_framebuffer(visual);
   if (!fb)
   {
      return NULL;
   }

   /* Determine color buffer format based on hardware capabilities */
   GLenum colorFormat;
   switch (fxMesa->colDepth)
   {
   case 15:
      colorFormat = GL_RGBA; /* ARGB1555 */
      break;
   case 16:
      colorFormat = GL_RGB; /* RGB565 */
      break;
   case 32:
      colorFormat = GL_RGBA; /* ARGB8888 */
      break;
   default:
      colorFormat = GL_RGBA;
      break;
   }

   /* Create front color renderbuffer */
   if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
   {
      fprintf(stderr, "Creating front color renderbuffer (format=%s)\n",
              _mesa_lookup_enum_by_nr(colorFormat));
   }
   frontRb = fxNewColorRenderbuffer(ctx, 0, colorFormat);
   if (!frontRb)
   {
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "ERROR: Failed to create front color renderbuffer\n");
      }
      _mesa_destroy_framebuffer(fb);
      return NULL;
   }
   _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, frontRb);
   /* Set up span functions for front color buffer - Call #1 */
   fxSetSpanFunctions(frontRb, visual);
   if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
   {
      fprintf(stderr, "Front color renderbuffer created successfully\n");
   }

   /* Create back color renderbuffer if double buffered */
   if (visual->doubleBufferMode)
   {
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Creating back color renderbuffer (format=%s)\n",
                 _mesa_lookup_enum_by_nr(colorFormat));
      }
      backRb = fxNewColorRenderbuffer(ctx, 0, colorFormat);
      if (!backRb)
      {
         if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
         {
            fprintf(stderr, "ERROR: Failed to create back color renderbuffer\n");
         }
         _mesa_destroy_framebuffer(fb);
         return NULL;
      }
      _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, backRb);
      /* Set up span functions for back color buffer - Call #2 */
      fxSetSpanFunctions(backRb, visual);
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Back color renderbuffer created successfully\n");
      }
   }
   else
   {
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Skipping back color renderbuffer (single buffered)\n");
      }
   }

   /* Create depth renderbuffer if needed */
   if (visual->depthBits > 0)
   {
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Creating depth renderbuffer (%d bits, haveHwStencil=%s)\n",
                 visual->depthBits, fxMesa->haveHwStencil ? "true" : "false");
      }
      depthRb = fxNewDepthRenderbuffer(ctx, 0);
      if (!depthRb)
      {
         if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
         {
            fprintf(stderr, "ERROR: Failed to create depth renderbuffer\n");
         }
         _mesa_destroy_framebuffer(fb);
         return NULL;
      }
      _mesa_add_renderbuffer(fb, BUFFER_DEPTH, depthRb);
      /* Set up span functions for depth buffer - Call #3 */
      fxSetSpanFunctions(depthRb, visual);
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Depth renderbuffer created successfully\n");
      }
   }
   else
   {
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Skipping depth renderbuffer (no depth bits requested)\n");
      }
   }

   /* Create stencil renderbuffer if needed */
   /* swStencil = (visual->stencilBits > 0) && !fxMesa->haveHwStencil; */ /* Nejc: Commented out - unused variable */
   if (visual->stencilBits > 0)
   {
      if (fxMesa->haveHwStencil)
      {
         /* Hardware stencil */
         if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
         {
            fprintf(stderr, "Creating hardware stencil renderbuffer (%d bits)\n",
                    visual->stencilBits);
         }
         stencilRb = fxNewStencilRenderbuffer(ctx, 0);
         if (!stencilRb)
         {
            if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
            {
               fprintf(stderr, "ERROR: Failed to create hardware stencil renderbuffer\n");
            }
            _mesa_destroy_framebuffer(fb);
            return NULL;
         }
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, stencilRb);
         /* Set up span functions for stencil buffer - Call #4 */
         fxSetSpanFunctions(stencilRb, visual);
         if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
         {
            fprintf(stderr, "Hardware stencil renderbuffer created successfully\n");
         }

         /* Additional span function setup for combined depth/stencil context - Call #5 */
         /* This matches the tdfx driver pattern where depth buffer span functions */
         /* are set up again when stencil is also present (shared Z/S buffer) */
         if (depthRb && visual->depthBits > 0)
         {
            if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
            {
               fprintf(stderr, "Re-setting depth span functions for combined depth/stencil\n");
            }
            fxSetSpanFunctions(depthRb, visual);
         }
      }
      else
      {
         /* Software stencil - let Mesa handle it */
         if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
         {
            fprintf(stderr, "Creating software stencil renderbuffer (%d bits)\n",
                    visual->stencilBits);
         }
         _mesa_add_soft_renderbuffers(fb,
                                      GL_FALSE, /* color */
                                      GL_FALSE, /* depth */
                                      GL_TRUE,  /* stencil */
                                      GL_FALSE, /* accum */
                                      GL_FALSE, /* alpha */
                                      GL_FALSE /* aux */);
         if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
         {
            fprintf(stderr, "Software stencil renderbuffer created successfully\n");
         }
      }
   }
   else
   {
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Skipping stencil renderbuffer (no stencil bits requested)\n");
      }
   }

   /* Add software accumulation buffer if needed */
   if (visual->accumRedBits > 0)
   {
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Creating software accumulation buffer (%d bits)\n",
                 visual->accumRedBits);
      }
      _mesa_add_soft_renderbuffers(fb,
                                   GL_FALSE, /* color */
                                   GL_FALSE, /* depth */
                                   GL_FALSE, /* stencil */
                                   GL_TRUE,  /* accum */
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux */);
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Software accumulation buffer created successfully\n");
      }
   }
   else
   {
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Skipping accumulation buffer (no accum bits requested)\n");
      }
   }

   /* Add software alpha buffer if needed and not provided by hardware */
   if (visual->alphaBits > 0 && !fxMesa->haveHwAlpha)
   {
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Creating software alpha buffer (%d bits, haveHwAlpha=%s)\n",
                 visual->alphaBits, fxMesa->haveHwAlpha ? "true" : "false");
      }
      _mesa_add_soft_renderbuffers(fb,
                                   GL_FALSE, /* color */
                                   GL_FALSE, /* depth */
                                   GL_FALSE, /* stencil */
                                   GL_FALSE, /* accum */
                                   GL_TRUE,  /* alpha */
                                   GL_FALSE /* aux */);
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         fprintf(stderr, "Software alpha buffer created successfully\n");
      }
   }
   else
   {
      if (TDFX_DEBUG && VERBOSE_IMMEDIATE)
      {
         if (visual->alphaBits > 0)
         {
            fprintf(stderr, "Skipping software alpha buffer (hardware alpha available)\n");
         }
         else
         {
            fprintf(stderr, "Skipping alpha buffer (no alpha bits requested)\n");
         }
      }
   }

   return fb;
}

/**
 * Update framebuffer size when window is resized
 * This is called from fxMesaUpdateScreenSize()
 */
void fxUpdateFramebufferSize(GLcontext *ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint i;

   if (TDFX_DEBUG & VERBOSE_DRIVER)
   {
      fprintf(stderr, "fxUpdateFramebufferSize(%d x %d)\n",
              fxMesa->width, fxMesa->height);
   }

   /* Update all renderbuffer sizes */
   for (i = 0; i < BUFFER_COUNT; i++)
   {
      struct gl_renderbuffer *rb = fb->Attachment[i].Renderbuffer;
      if (rb)
      {
         rb->Width = fxMesa->width;
         rb->Height = fxMesa->height;
      }
   }

   /* Update framebuffer size */
   fb->Width = fxMesa->width;
   fb->Height = fxMesa->height;
}

/**
 * Set the buffer used for reading
 * This replaces the old buffer selection logic
 */
void fxSetReadBuffer(GLcontext *ctx, GLenum buffer)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (TDFX_DEBUG & VERBOSE_DRIVER)
   {
      fprintf(stderr, "fxSetReadBuffer(%s)\n", _mesa_lookup_enum_by_nr(buffer));
   }

   /* Validate the buffer */
   switch (buffer)
   {
   case GL_FRONT_LEFT:
      fxMesa->currentFB = GR_BUFFER_FRONTBUFFER;
      break;
   case GL_BACK_LEFT:
      if (ctx->Visual.doubleBufferMode)
      {
         fxMesa->currentFB = GR_BUFFER_BACKBUFFER;
      }
      else
      {
         /* Fallback to front buffer if no back buffer */
         fxMesa->currentFB = GR_BUFFER_FRONTBUFFER;
      }
      break;
   default:
      /* For other buffers, let Mesa handle it in software */
      break;
   }
}

/**
 * Set the buffer used for drawing
 * This replaces the old buffer selection logic
 */
void fxSetDrawBuffer(GLcontext *ctx, GLenum buffer)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (TDFX_DEBUG & VERBOSE_DRIVER)
   {
      fprintf(stderr, "fxSetDrawBuffer(%s)\n", _mesa_lookup_enum_by_nr(buffer));
   }

   /* Validate the buffer */
   switch (buffer)
   {
   case GL_FRONT_LEFT:
      fxMesa->currentFB = GR_BUFFER_FRONTBUFFER;
      grRenderBuffer(GR_BUFFER_FRONTBUFFER);
      break;
   case GL_BACK_LEFT:
      if (ctx->Visual.doubleBufferMode)
      {
         fxMesa->currentFB = GR_BUFFER_BACKBUFFER;
         grRenderBuffer(GR_BUFFER_BACKBUFFER);
      }
      else
      {
         /* Fallback to front buffer if no back buffer */
         fxMesa->currentFB = GR_BUFFER_FRONTBUFFER;
         grRenderBuffer(GR_BUFFER_FRONTBUFFER);
      }
      break;
   default:
      /* For other buffers, let Mesa handle it in software */
      break;
   }
}

/**
 * Initialize framebuffer-related driver functions
 * This should be called from fxSetupDDPointers()
 */
void fxInitFramebufferFuncs(struct dd_function_table *functions)
{
   // Mesa 6.4.2 renderbuffer infrastructure uses these function pointers
   functions->ReadBuffer = fxSetReadBuffer;
   functions->DrawBuffer = fxSetDrawBuffer;

   // Mesa 6.3+ framebuffer/renderbuffer functions
   functions->NewFramebuffer = fxNewFramebuffer;
   functions->NewRenderbuffer = fxNewColorRenderbuffer; // Default to color renderbuffer
}
