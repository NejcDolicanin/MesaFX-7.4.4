/*
 * Mesa 3-D graphics library
 * 
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

/**
 * \file glapi_alias_stubs.c
 * Stub/alias implementations for extension functions that have static_dispatch="false"
 * but are exported in the Windows .def file.
 *
 * These functions forward to their implementation through glGetProcAddress.
 */

#include "../main/glheader.h"
#include "glapi.h"

/* Get the actual function pointer and call it */
#define ALIAS_STUB(name, ret, params, args) \
ret GLAPIENTRY gl##name params { \
    typedef ret (GLAPIENTRY *func_t)params; \
    static func_t func = NULL; \
    if (!func) func = (func_t) _glapi_get_proc_address("gl" #name); \
    if (func) return func args; \
    return (ret)0; \
}

#define ALIAS_STUB_VOID(name, params, args) \
void GLAPIENTRY gl##name params { \
    typedef void (GLAPIENTRY *func_t)params; \
    static func_t func = NULL; \
    if (!func) func = (func_t) _glapi_get_proc_address("gl" #name); \
    if (func) func args; \
}

/* Imaging subset - these ARE implemented but have static_dispatch=false */
/* When using X86 assembly, these are provided by glapi_x86.S, so skip them */
#ifndef USE_X86_ASM
ALIAS_STUB_VOID(ColorTableSGI, (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table), (target, internalformat, width, format, type, table))
ALIAS_STUB_VOID(ColorTableParameterfvSGI, (GLenum target, GLenum pname, const GLfloat *params), (target, pname, params))
ALIAS_STUB_VOID(ColorTableParameterivSGI, (GLenum target, GLenum pname, const GLint *params), (target, pname, params))
ALIAS_STUB_VOID(CopyColorTableSGI, (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width), (target, internalformat, x, y, width))
ALIAS_STUB_VOID(GetColorTableSGI, (GLenum target, GLenum format, GLenum type, GLvoid *table), (target, format, type, table))
ALIAS_STUB_VOID(GetColorTableParameterfvSGI, (GLenum target, GLenum pname, GLfloat *params), (target, pname, params))
ALIAS_STUB_VOID(GetColorTableParameterivSGI, (GLenum target, GLenum pname, GLint *params), (target, pname, params))

ALIAS_STUB_VOID(ColorSubTableEXT, (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data), (target, start, count, format, type, data))
ALIAS_STUB_VOID(CopyColorSubTableEXT, (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width), (target, start, x, y, width))

ALIAS_STUB_VOID(ConvolutionFilter1DEXT, (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image), (target, internalformat, width, format, type, image))
ALIAS_STUB_VOID(ConvolutionFilter2DEXT, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image), (target, internalformat, width, height, format, type, image))
ALIAS_STUB_VOID(ConvolutionParameterfEXT, (GLenum target, GLenum pname, GLfloat param), (target, pname, param))
ALIAS_STUB_VOID(ConvolutionParameterfvEXT, (GLenum target, GLenum pname, const GLfloat *params), (target, pname, params))
ALIAS_STUB_VOID(ConvolutionParameteriEXT, (GLenum target, GLenum pname, GLint param), (target, pname, param))
ALIAS_STUB_VOID(ConvolutionParameterivEXT, (GLenum target, GLenum pname, const GLint *params), (target, pname, params))
ALIAS_STUB_VOID(CopyConvolutionFilter1DEXT, (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width), (target, internalformat, x, y, width))
ALIAS_STUB_VOID(CopyConvolutionFilter2DEXT, (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height), (target, internalformat, x, y, width, height))
ALIAS_STUB_VOID(GetConvolutionFilterEXT, (GLenum target, GLenum format, GLenum type, GLvoid *image), (target, format, type, image))
ALIAS_STUB_VOID(GetConvolutionParameterfvEXT, (GLenum target, GLenum pname, GLfloat *params), (target, pname, params))
ALIAS_STUB_VOID(GetConvolutionParameterivEXT, (GLenum target, GLenum pname, GLint *params), (target, pname, params))
ALIAS_STUB_VOID(SeparableFilter2DEXT, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column), (target, internalformat, width, height, format, type, row, column))
ALIAS_STUB_VOID(GetSeparableFilterEXT, (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span), (target, format, type, row, column, span))

ALIAS_STUB_VOID(HistogramEXT, (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink), (target, width, internalformat, sink))
ALIAS_STUB_VOID(MinmaxEXT, (GLenum target, GLenum internalformat, GLboolean sink), (target, internalformat, sink))
ALIAS_STUB_VOID(ResetHistogramEXT, (GLenum target), (target))
ALIAS_STUB_VOID(ResetMinmaxEXT, (GLenum target), (target))
ALIAS_STUB_VOID(GetHistogramEXT, (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values), (target, reset, format, type, values))
ALIAS_STUB_VOID(GetHistogramParameterfvEXT, (GLenum target, GLenum pname, GLfloat *params), (target, pname, params))
ALIAS_STUB_VOID(GetHistogramParameterivEXT, (GLenum target, GLenum pname, GLint *params), (target, pname, params))
ALIAS_STUB_VOID(GetMinmaxEXT, (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values), (target, reset, format, type, values))
ALIAS_STUB_VOID(GetMinmaxParameterfvEXT, (GLenum target, GLenum pname, GLfloat *params), (target, pname, params))
ALIAS_STUB_VOID(GetMinmaxParameterivEXT, (GLenum target, GLenum pname, GLint *params), (target, pname, params))

/* Blend functions */
ALIAS_STUB_VOID(BlendFuncSeparateINGR, (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha), (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha))

/* Point parameters */
ALIAS_STUB_VOID(PointParameterfSGIS, (GLenum pname, GLfloat param), (pname, param))
ALIAS_STUB_VOID(PointParameterfvSGIS, (GLenum pname, const GLfloat *params), (pname, params))
#endif /* !USE_X86_ASM */

/* Multisample - not provided by assembly, always include */
ALIAS_STUB_VOID(SampleMaskEXT, (GLclampf value, GLboolean invert), (value, invert))
ALIAS_STUB_VOID(SampleMaskSGIS, (GLclampf value, GLboolean invert), (value, invert))
ALIAS_STUB_VOID(SamplePatternEXT, (GLenum pattern), (pattern))
ALIAS_STUB_VOID(SamplePatternSGIS, (GLenum pattern), (pattern))

/* Extensions that are NOT implemented - provide no-op stubs */
ALIAS_STUB_VOID(ActiveStencilFaceEXT, (GLenum face), (face))
ALIAS_STUB_VOID(CullParameterdvEXT, (GLenum pname, GLdouble *params), (pname, params))
ALIAS_STUB_VOID(CullParameterfvEXT, (GLenum pname, GLfloat *params), (pname, params))
ALIAS_STUB_VOID(DeleteFencesNV, (GLsizei n, const GLuint *fences), (n, fences))
ALIAS_STUB_VOID(DepthBoundsEXT, (GLclampd zmin, GLclampd zmax), (zmin, zmax))
ALIAS_STUB_VOID(DetailTexFuncSGIS, (GLenum target, GLsizei n, const GLfloat *points), (target, n, points))
ALIAS_STUB_VOID(FinishFenceNV, (GLuint fence), (fence))
ALIAS_STUB_VOID(FlushRasterSGIX, (void), ())
ALIAS_STUB_VOID(FragmentColorMaterialSGIX, (GLenum face, GLenum mode), (face, mode))
ALIAS_STUB_VOID(FragmentLightModelfSGIX, (GLenum pname, GLfloat param), (pname, param))
ALIAS_STUB_VOID(FragmentLightModelfvSGIX, (GLenum pname, const GLfloat *params), (pname, params))
ALIAS_STUB_VOID(FragmentLightModeliSGIX, (GLenum pname, GLint param), (pname, param))
ALIAS_STUB_VOID(FragmentLightModelivSGIX, (GLenum pname, const GLint *params), (pname, params))
ALIAS_STUB_VOID(FragmentLightfSGIX, (GLenum light, GLenum pname, GLfloat param), (light, pname, param))
ALIAS_STUB_VOID(FragmentLightfvSGIX, (GLenum light, GLenum pname, const GLfloat *params), (light, pname, params))
ALIAS_STUB_VOID(FragmentLightiSGIX, (GLenum light, GLenum pname, GLint param), (light, pname, param))
ALIAS_STUB_VOID(FragmentLightivSGIX, (GLenum light, GLenum pname, const GLint *params), (light, pname, params))
ALIAS_STUB_VOID(FragmentMaterialfSGIX, (GLenum face, GLenum pname, GLfloat param), (face, pname, param))
ALIAS_STUB_VOID(FragmentMaterialfvSGIX, (GLenum face, GLenum pname, const GLfloat *params), (face, pname, params))
ALIAS_STUB_VOID(FragmentMaterialiSGIX, (GLenum face, GLenum pname, GLint param), (face, pname, param))
ALIAS_STUB_VOID(FragmentMaterialivSGIX, (GLenum face, GLenum pname, const GLint *params), (face, pname, params))
ALIAS_STUB_VOID(FrameZoomSGIX, (GLint factor), (factor))
ALIAS_STUB_VOID(GenFencesNV, (GLsizei n, GLuint *fences), (n, fences))
ALIAS_STUB_VOID(GetDetailTexFuncSGIS, (GLenum target, GLfloat *points), (target, points))
ALIAS_STUB_VOID(GetFenceivNV, (GLuint fence, GLenum pname, GLint *params), (fence, pname, params))
ALIAS_STUB_VOID(GetFragmentLightfvSGIX, (GLenum light, GLenum pname, GLfloat *params), (light, pname, params))
ALIAS_STUB_VOID(GetFragmentLightivSGIX, (GLenum light, GLenum pname, GLint *params), (light, pname, params))
ALIAS_STUB_VOID(GetFragmentMaterialfvSGIX, (GLenum face, GLenum pname, GLfloat *params), (face, pname, params))
ALIAS_STUB_VOID(GetFragmentMaterialivSGIX, (GLenum face, GLenum pname, GLint *params), (face, pname, params))
ALIAS_STUB(GetInstrumentsSGIX, GLint, (void), ())
ALIAS_STUB_VOID(GetListParameterfvSGIX, (GLuint list, GLenum pname, GLfloat *params), (list, pname, params))
ALIAS_STUB_VOID(GetListParameterivSGIX, (GLuint list, GLenum pname, GLint *params), (list, pname, params))
ALIAS_STUB_VOID(GetPixelTexGenParameterfvSGIS, (GLenum pname, GLfloat *params), (pname, params))
ALIAS_STUB_VOID(GetPixelTexGenParameterivSGIS, (GLenum pname, GLint *params), (pname, params))
ALIAS_STUB_VOID(GetSharpenTexFuncSGIS, (GLenum target, GLfloat *points), (target, points))
ALIAS_STUB_VOID(GetTexFilterFuncSGIS, (GLenum target, GLenum filter, GLfloat *weights), (target, filter, weights))
ALIAS_STUB_VOID(HintPGI, (GLenum target, GLint mode), (target, mode))
ALIAS_STUB_VOID(IndexFuncEXT, (GLenum func_param, GLclampf ref), (func_param, ref))
ALIAS_STUB_VOID(IndexMaterialEXT, (GLenum face, GLenum mode), (face, mode))
ALIAS_STUB_VOID(InstrumentsBufferSGIX, (GLsizei size, GLint *buffer), (size, buffer))
ALIAS_STUB(IsFenceNV, GLboolean, (GLuint fence), (fence))
ALIAS_STUB_VOID(LightEnviSGIX, (GLenum pname, GLint param), (pname, param))
ALIAS_STUB_VOID(ListParameterfSGIX, (GLuint list, GLenum pname, GLfloat param), (list, pname, param))
ALIAS_STUB_VOID(ListParameterfvSGIX, (GLuint list, GLenum pname, const GLfloat *params), (list, pname, params))
ALIAS_STUB_VOID(ListParameteriSGIX, (GLuint list, GLenum pname, GLint param), (list, pname, param))
ALIAS_STUB_VOID(ListParameterivSGIX, (GLuint list, GLenum pname, const GLint *params), (list, pname, params))
ALIAS_STUB_VOID(MultiModeDrawArraysIBM, (const GLenum *mode, const GLint *first, const GLsizei *count, GLsizei primcount, GLint modestride), (mode, first, count, primcount, modestride))
ALIAS_STUB_VOID(MultiModeDrawElementsIBM, (const GLenum *mode, const GLsizei *count, GLenum type, const GLvoid * const *indices, GLsizei primcount, GLint modestride), (mode, count, type, indices, primcount, modestride))
ALIAS_STUB_VOID(PixelTexGenParameterfSGIS, (GLenum pname, GLfloat param), (pname, param))
ALIAS_STUB_VOID(PixelTexGenParameterfvSGIS, (GLenum pname, const GLfloat *params), (pname, params))
ALIAS_STUB_VOID(PixelTexGenParameteriSGIS, (GLenum pname, GLint param), (pname, param))
ALIAS_STUB_VOID(PixelTexGenParameterivSGIS, (GLenum pname, const GLint *params), (pname, params))
ALIAS_STUB_VOID(PixelTexGenSGIX, (GLenum mode), (mode))
ALIAS_STUB(PollInstrumentsSGIX, GLint, (GLint *marker_p), (marker_p))
ALIAS_STUB_VOID(ReadInstrumentsSGIX, (GLint marker), (marker))
ALIAS_STUB_VOID(ReferencePlaneSGIX, (const GLdouble *equation), (equation))
ALIAS_STUB_VOID(SetFenceNV, (GLuint fence, GLenum condition), (fence, condition))
ALIAS_STUB_VOID(SharpenTexFuncSGIS, (GLenum target, GLsizei n, const GLfloat *points), (target, n, points))
ALIAS_STUB_VOID(SpriteParameterfSGIX, (GLenum pname, GLfloat param), (pname, param))
ALIAS_STUB_VOID(SpriteParameterfvSGIX, (GLenum pname, const GLfloat *params), (pname, params))
ALIAS_STUB_VOID(SpriteParameteriSGIX, (GLenum pname, GLint param), (pname, param))
ALIAS_STUB_VOID(SpriteParameterivSGIX, (GLenum pname, const GLint *params), (pname, params))
ALIAS_STUB_VOID(StartInstrumentsSGIX, (void), ())
ALIAS_STUB_VOID(StopInstrumentsSGIX, (GLint marker), (marker))
ALIAS_STUB_VOID(TagSampleBufferSGIX, (void), ())
ALIAS_STUB_VOID(TbufferMask3DFX, (GLuint mask), (mask))
ALIAS_STUB(TestFenceNV, GLboolean, (GLuint fence), (fence))
ALIAS_STUB_VOID(TexFilterFuncSGIS, (GLenum target, GLenum filter, GLsizei n, const GLfloat *weights), (target, filter, n, weights))
ALIAS_STUB_VOID(TexImage4DSGIS, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const GLvoid *pixels), (target, level, internalformat, width, height, depth, size4d, border, format, type, pixels))
ALIAS_STUB_VOID(TexSubImage4DSGIS, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const GLvoid *pixels), (target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels))
ALIAS_STUB_VOID(VertexWeightPointerEXT, (GLsizei size, GLenum type, GLsizei stride, const GLvoid *pointer), (size, type, stride, pointer))
ALIAS_STUB_VOID(VertexWeightfEXT, (GLfloat weight), (weight))
ALIAS_STUB_VOID(VertexWeightfvEXT, (const GLfloat *weight), (weight))
