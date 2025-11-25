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
 *    Nejc Dolicanin - Updated for Mesa 6.4.2 renderbuffer infrastructure
 */

// #ifndef FXRENDERBUFFER_H
// #define FXRENDERBUFFER_H

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

/* FX */

#include "mtypes.h"

/* Function prototypes */
extern struct gl_renderbuffer *
fxNewColorRenderbuffer(GLcontext *ctx, GLuint name, GLenum internalFormat);

extern struct gl_renderbuffer *
fxNewDepthRenderbuffer(GLcontext *ctx, GLuint name);

extern struct gl_renderbuffer *
fxNewStencilRenderbuffer(GLcontext *ctx, GLuint name);

extern void
fxSetSpanFunctions(struct gl_renderbuffer *rb, const GLvisual *vis);

/* Span function declarations */
extern void fxReadRGBASpan_ARGB8888(GLcontext *ctx, struct gl_renderbuffer *rb,
                                    GLuint count, GLint x, GLint y, void *values);
extern void fxReadRGBAPixels_ARGB8888(GLcontext *ctx, struct gl_renderbuffer *rb,
                                      GLuint count, const GLint x[], const GLint y[],
                                      void *values);

extern void fxWriteRGBASpan_ARGB8888(GLcontext *ctx, struct gl_renderbuffer *rb,
                                     GLuint count, GLint x, GLint y, const void *values,
                                     const GLubyte *mask);
extern void fxWriteRGBSpan_ARGB8888(GLcontext *ctx, struct gl_renderbuffer *rb,
                                    GLuint count, GLint x, GLint y, const void *values,
                                    const GLubyte *mask);
extern void fxWriteMonoRGBASpan_ARGB8888(GLcontext *ctx, struct gl_renderbuffer *rb,
                                         GLuint count, GLint x, GLint y, const void *value,
                                         const GLubyte *mask);
extern void fxWriteRGBAPixels_ARGB8888(GLcontext *ctx, struct gl_renderbuffer *rb,
                                       GLuint count, const GLint x[], const GLint y[],
                                       const void *values, const GLubyte *mask);
extern void fxWriteMonoRGBAPixels_ARGB8888(GLcontext *ctx, struct gl_renderbuffer *rb,
                                           GLuint count, const GLint x[], const GLint y[],
                                           const void *value, const GLubyte *mask);

extern void fxReadRGBASpan_RGB565(GLcontext *ctx, struct gl_renderbuffer *rb,
                                  GLuint count, GLint x, GLint y, void *values);
extern void fxReadRGBAPixels_RGB565(GLcontext *ctx, struct gl_renderbuffer *rb,
                                    GLuint count, const GLint x[], const GLint y[],
                                    void *values);
extern void fxWriteRGBASpan_RGB565(GLcontext *ctx, struct gl_renderbuffer *rb,
                                   GLuint count, GLint x, GLint y, const void *values,
                                   const GLubyte *mask);
extern void fxWriteRGBSpan_RGB565(GLcontext *ctx, struct gl_renderbuffer *rb,
                                  GLuint count, GLint x, GLint y, const void *values,
                                  const GLubyte *mask);
extern void fxWriteMonoRGBASpan_RGB565(GLcontext *ctx, struct gl_renderbuffer *rb,
                                       GLuint count, GLint x, GLint y, const void *value,
                                       const GLubyte *mask);
extern void fxWriteRGBAPixels_RGB565(GLcontext *ctx, struct gl_renderbuffer *rb,
                                     GLuint count, const GLint x[], const GLint y[],
                                     const void *values, const GLubyte *mask);
extern void fxWriteMonoRGBAPixels_RGB565(GLcontext *ctx, struct gl_renderbuffer *rb,
                                         GLuint count, const GLint x[], const GLint y[],
                                         const void *value, const GLubyte *mask);
extern void fxReadRGBASpan_ARGB1555(GLcontext *ctx, struct gl_renderbuffer *rb,
                                    GLuint count, GLint x, GLint y, void *values);
extern void fxReadRGBAPixels_ARGB1555(GLcontext *ctx, struct gl_renderbuffer *rb,
                                      GLuint count, const GLint x[], const GLint y[],
                                      void *values);
extern void fxWriteRGBASpan_ARGB1555(GLcontext *ctx, struct gl_renderbuffer *rb,
                                     GLuint count, GLint x, GLint y, const void *values,
                                     const GLubyte *mask);
extern void fxWriteRGBSpan_ARGB1555(GLcontext *ctx, struct gl_renderbuffer *rb,
                                    GLuint count, GLint x, GLint y, const void *values,
                                    const GLubyte *mask);
extern void fxWriteMonoRGBASpan_ARGB1555(GLcontext *ctx, struct gl_renderbuffer *rb,
                                         GLuint count, GLint x, GLint y, const void *value,
                                         const GLubyte *mask);
extern void fxWriteRGBAPixels_ARGB1555(GLcontext *ctx, struct gl_renderbuffer *rb,
                                       GLuint count, const GLint x[], const GLint y[],
                                       const void *values, const GLubyte *mask);
extern void fxWriteMonoRGBAPixels_ARGB1555(GLcontext *ctx, struct gl_renderbuffer *rb,
                                           GLuint count, const GLint x[], const GLint y[],
                                           const void *value, const GLubyte *mask);
extern void fxReadDepthSpan_Z24(GLcontext *ctx, struct gl_renderbuffer *rb,
                                GLuint count, GLint x, GLint y, void *values);
extern void fxReadDepthPixels_Z24(GLcontext *ctx, struct gl_renderbuffer *rb,
                                  GLuint count, const GLint x[], const GLint y[],
                                  void *values);
extern void fxWriteDepthSpan_Z24(GLcontext *ctx, struct gl_renderbuffer *rb,
                                 GLuint count, GLint x, GLint y, const void *values,
                                 const GLubyte *mask);
extern void fxWriteMonoDepthSpan_Z24(GLcontext *ctx, struct gl_renderbuffer *rb,
                                     GLuint count, GLint x, GLint y, const void *value,
                                     const GLubyte *mask);
extern void fxWriteDepthPixels_Z24(GLcontext *ctx, struct gl_renderbuffer *rb,
                                   GLuint count, const GLint x[], const GLint y[],
                                   const void *values, const GLubyte *mask);
extern void fxWriteMonoDepthPixels_Z24(GLcontext *ctx, struct gl_renderbuffer *rb,
                                       GLuint count, const GLint x[], const GLint y[],
                                       const void *value, const GLubyte *mask);
extern void fxReadDepthSpan_Z16(GLcontext *ctx, struct gl_renderbuffer *rb,
                                GLuint count, GLint x, GLint y, void *values);
extern void fxReadDepthPixels_Z16(GLcontext *ctx, struct gl_renderbuffer *rb,
                                  GLuint count, const GLint x[], const GLint y[],
                                  void *values);
extern void fxWriteDepthSpan_Z16(GLcontext *ctx, struct gl_renderbuffer *rb,
                                 GLuint count, GLint x, GLint y, const void *values,
                                 const GLubyte *mask);
extern void fxWriteMonoDepthSpan_Z16(GLcontext *ctx, struct gl_renderbuffer *rb,
                                     GLuint count, GLint x, GLint y, const void *value,
                                     const GLubyte *mask);
extern void fxWriteDepthPixels_Z16(GLcontext *ctx, struct gl_renderbuffer *rb,
                                   GLuint count, const GLint x[], const GLint y[],
                                   const void *values, const GLubyte *mask);
extern void fxWriteMonoDepthPixels_Z16(GLcontext *ctx, struct gl_renderbuffer *rb,
                                       GLuint count, const GLint x[], const GLint y[],
                                       const void *value, const GLubyte *mask);
extern void fxReadStencilSpan(GLcontext *ctx, struct gl_renderbuffer *rb,
                              GLuint count, GLint x, GLint y, void *values);
extern void fxReadStencilPixels(GLcontext *ctx, struct gl_renderbuffer *rb,
                                GLuint count, const GLint x[], const GLint y[],
                                void *values);
extern void fxWriteStencilSpan(GLcontext *ctx, struct gl_renderbuffer *rb,
                               GLuint count, GLint x, GLint y, const void *values,
                               const GLubyte *mask);
extern void fxWriteMonoStencilSpan(GLcontext *ctx, struct gl_renderbuffer *rb,
                                   GLuint count, GLint x, GLint y, const void *value,
                                   const GLubyte *mask);
extern void fxWriteStencilPixels(GLcontext *ctx, struct gl_renderbuffer *rb,
                                 GLuint count, const GLint x[], const GLint y[],
                                 const void *values, const GLubyte *mask);
extern void fxWriteMonoStencilPixels(GLcontext *ctx, struct gl_renderbuffer *rb,
                                     GLuint count, const GLint x[], const GLint y[],
                                     const void *value, const GLubyte *mask);

// #endif /* FXRENDERBUFFER_H */
