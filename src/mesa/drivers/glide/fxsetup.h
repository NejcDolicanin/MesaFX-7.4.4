/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 *    David Bucciarelli
 *    Brian Paul
 *    Daryll Strauss
 *    Keith Whitwell
 *    Daniel Borca
 *    Hiroshi Morii
 */

/* fxsetup.c - 3Dfx VooDoo rendering mode setup functions */
/* This code belongs to fxsetup.c, but I didn't want to clutter
 * the original code with Napalm specifics, in order to keep things
 * clear -- especially for backward compatibility. I should have
 * put it into another .c file, but I didn't want to export so many
 * things...
 * The point is, Napalm uses a different technique for texture env.
 * SST1 Single texturing:
 *      setup standard grTexCombine
 *      fiddle with grColorCombine/grAlphaCombine
 * SST1 Multi texturing:
 *      fiddle with grTexCombine/grColorCombine/grAlphaCombine
 * Napalm Single texturing:
 *      setup standard grColorCombineExt/grAlphaCombineExt
 *      fiddle with grTexColorCombine/grTexAlphaCombine
 * Napalm Multi texturing:
 *      setup standard grColorCombineExt/grAlphaCombineExt
 *      fiddle with grTexColorCombine/grTexAlphaCombine
 */

/* Compatibility aliases for EXT/ARB token variants */
#ifndef GL_SUBTRACT_EXT
#define GL_SUBTRACT_EXT GL_SUBTRACT
#endif
#ifndef GL_ADD_SIGNED_EXT
#define GL_ADD_SIGNED_EXT GL_ADD_SIGNED
#endif
#ifndef GL_INTERPOLATE_EXT
#define GL_INTERPOLATE_EXT GL_INTERPOLATE
#endif

/*
 * These macros are used below when handling COMBINE_EXT.
 */
#define TEXENV_OPERAND_INVERTED(operand)                            \
  (((operand) == GL_ONE_MINUS_SRC_ALPHA)                            \
   || ((operand) == GL_ONE_MINUS_SRC_COLOR))
#define TEXENV_OPERAND_ALPHA(operand)                               \
  (((operand) == GL_SRC_ALPHA) || ((operand) == GL_ONE_MINUS_SRC_ALPHA))
#define TEXENV_SETUP_ARG_A(param, source, operand, iteratedAlpha)   \
    switch (source) {                                               \
    case GL_TEXTURE:                                                \
        param = GR_CMBX_LOCAL_TEXTURE_ALPHA;                        \
        break;                                                      \
    case GL_CONSTANT_EXT:                                           \
        param = GR_CMBX_TMU_CALPHA;                                 \
        break;                                                      \
    case GL_PRIMARY_COLOR_EXT:                                      \
        param = GR_CMBX_ITALPHA;                                    \
        break;                                                      \
    case GL_PREVIOUS_EXT:                                           \
        param = iteratedAlpha;                                      \
        break;                                                      \
    default:                                                        \
       /*                                                           \
        * This is here just to keep from getting                    \
        * compiler warnings.                                        \
        */                                                          \
        param = GR_CMBX_ZERO;                                       \
        break;                                                      \
    }

#define TEXENV_SETUP_ARG_RGB(param, source, operand, iteratedColor, iteratedAlpha) \
    if (!TEXENV_OPERAND_ALPHA(operand)) {                           \
        switch (source) {                                           \
        case GL_TEXTURE:                                            \
            param = GR_CMBX_LOCAL_TEXTURE_RGB;                      \
            break;                                                  \
        case GL_CONSTANT_EXT:                                       \
            param = GR_CMBX_TMU_CCOLOR;                             \
            break;                                                  \
        case GL_PRIMARY_COLOR_EXT:                                  \
            param = GR_CMBX_ITRGB;                                  \
            break;                                                  \
        case GL_PREVIOUS_EXT:                                       \
            param = iteratedColor;                                  \
            break;                                                  \
        default:                                                    \
           /*                                                       \
            * This is here just to keep from getting                \
            * compiler warnings.                                    \
            */                                                      \
            param = GR_CMBX_ZERO;                                   \
            break;                                                  \
        }                                                           \
    } else {                                                        \
        switch (source) {                                           \
        case GL_TEXTURE:                                            \
            param = GR_CMBX_LOCAL_TEXTURE_ALPHA;                    \
            break;                                                  \
        case GL_CONSTANT_EXT:                                       \
            param = GR_CMBX_TMU_CALPHA;                             \
            break;                                                  \
        case GL_PRIMARY_COLOR_EXT:                                  \
            param = GR_CMBX_ITALPHA;                                \
            break;                                                  \
        case GL_PREVIOUS_EXT:                                       \
            param = iteratedAlpha;                                  \
            break;                                                  \
        default:                                                    \
           /*                                                       \
            * This is here just to keep from getting                \
            * compiler warnings.                                    \
            */                                                      \
            param = GR_CMBX_ZERO;                                   \
            break;                                                  \
        }                                                           \
    }

#define TEXENV_SETUP_MODE_RGB(param, operand)                       \
    switch (operand) {                                              \
    case GL_SRC_COLOR:                                              \
    case GL_SRC_ALPHA:                                              \
        param = GR_FUNC_MODE_X;                                     \
        break;                                                      \
    case GL_ONE_MINUS_SRC_ALPHA:                                    \
    case GL_ONE_MINUS_SRC_COLOR:                                    \
        param = GR_FUNC_MODE_ONE_MINUS_X;                           \
        break;                                                      \
    default:                                                        \
        param = GR_FUNC_MODE_ZERO;                                  \
        break;                                                      \
    }

#define TEXENV_SETUP_MODE_A(param, operand)                         \
    switch (operand) {                                              \
    case GL_SRC_ALPHA:                                              \
        param = GR_FUNC_MODE_X;                                     \
        break;                                                      \
    case GL_ONE_MINUS_SRC_ALPHA:                                    \
        param = GR_FUNC_MODE_ONE_MINUS_X;                           \
        break;                                                      \
    default:                                                        \
        param = GR_FUNC_MODE_ZERO;                                  \
        break;                                                      \
    }

static void
fxSetupTextureEnvNapalm_NoLock(GLcontext * ctx, GLuint textureset, GLuint tmu, GLboolean iterated)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[textureset];
   struct tdfx_combine_alpha_ext alphaComb;
   struct tdfx_combine_color_ext colorComb;
   const GLfloat *envColor = texUnit->EnvColor;
   GrCombineLocal_t localc, locala; /* fragmentColor/Alpha */
   GLint ifmt;
   tfxTexInfo *ti;
   struct gl_texture_object *tObj = texUnit->_Current;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTextureEnvNapalm_NoLock(unit %u, TMU %u, iterated %d)\n",
                      textureset, tmu, iterated);
   }

   ti = fxTMGetTexInfo(tObj);

   ifmt = ti->baseLevelInternalFormat;

   if (iterated) {
      /* we don't have upstream TMU */
      locala = GR_CMBX_ITALPHA;
      localc = GR_CMBX_ITRGB;
   } else {
      /* we have upstream TMU */
      locala = GR_CMBX_OTHER_TEXTURE_ALPHA;
      localc = GR_CMBX_OTHER_TEXTURE_RGB;
   }

   alphaComb.InvertD = FXFALSE;
   alphaComb.Shift   = 0;
   alphaComb.Invert  = FXFALSE;
   colorComb.InvertD = FXFALSE;
   colorComb.Shift   = 0;
   colorComb.Invert  = FXFALSE;

   switch (texUnit->EnvMode) {
   case GL_DECAL:
      alphaComb.SourceA = locala;
      alphaComb.ModeA   = GR_FUNC_MODE_X;
      alphaComb.SourceB = GR_CMBX_ZERO;
      alphaComb.ModeB   = GR_FUNC_MODE_X;
      alphaComb.SourceC = GR_CMBX_ZERO;
      alphaComb.InvertC = FXTRUE;
      alphaComb.SourceD = GR_CMBX_ZERO;

      colorComb.SourceA = GR_CMBX_LOCAL_TEXTURE_RGB;
      colorComb.ModeA   = GR_FUNC_MODE_X;
      colorComb.SourceB = localc;
      colorComb.ModeB   = GR_FUNC_MODE_NEGATIVE_X;
      colorComb.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
      colorComb.InvertC = FXFALSE;
      colorComb.SourceD = GR_CMBX_B;
      break;
   case GL_MODULATE:
      if (ifmt == GL_LUMINANCE || ifmt == GL_RGB) {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      } else {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.InvertC = FXFALSE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      }

      if (ifmt == GL_ALPHA) {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      } else {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_LOCAL_TEXTURE_RGB;
         colorComb.InvertC = FXFALSE;
         colorComb.SourceD = GR_CMBX_ZERO;
      }
      break;
   case GL_BLEND:
      if (ifmt == GL_INTENSITY) {
         alphaComb.SourceA = GR_CMBX_TMU_CALPHA;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = locala;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.InvertC = FXFALSE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      } else {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.InvertC = FXFALSE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      }

      if (ifmt == GL_ALPHA) {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      } else {
         colorComb.SourceA = GR_CMBX_TMU_CCOLOR;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = localc;
         colorComb.ModeB   = GR_FUNC_MODE_NEGATIVE_X;
         colorComb.SourceC = GR_CMBX_LOCAL_TEXTURE_RGB;
         colorComb.InvertC = FXFALSE;
         colorComb.SourceD = GR_CMBX_B;
      }

      fxMesa->Glide.grConstantColorValueExt(tmu,
         (((GLuint)(envColor[0] * 255.0f))      ) |
         (((GLuint)(envColor[1] * 255.0f)) <<  8) |
         (((GLuint)(envColor[2] * 255.0f)) << 16) |
         (((GLuint)(envColor[3] * 255.0f)) << 24));
      break;
   case GL_REPLACE:
      if (ifmt == GL_LUMINANCE || ifmt == GL_RGB) {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      } else {
         alphaComb.SourceA = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      }

      if (ifmt == GL_ALPHA) {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      } else {
         colorComb.SourceA = GR_CMBX_LOCAL_TEXTURE_RGB;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      }
      break;
   case GL_ADD:
      if (ifmt == GL_LUMINANCE || ifmt == GL_RGB) {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      } else if (ifmt == GL_INTENSITY) {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      } else {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.InvertC = FXFALSE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      }

      if (ifmt == GL_ALPHA) {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      } else {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_LOCAL_TEXTURE_RGB;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      }
      break;
    /* COMBINE_EXT */
    case GL_COMBINE_EXT:
      /* XXX todo - INCOMPLETE!!! */
      if (TDFX_DEBUG & (VERBOSE_DRIVER | VERBOSE_TEXTURE)) {
#if 1
         fprintf(stderr, "COMBINE_EXT: %s + %s\n",
	      _mesa_lookup_enum_by_nr(texUnit->Combine.ModeRGB),
	      _mesa_lookup_enum_by_nr(texUnit->Combine.ModeA));
#else
         fprintf(stderr, "Texture Unit %d\n", textureset);
         fprintf(stderr, "  GL_TEXTURE_ENV_MODE = %s\n", _mesa_lookup_enum_by_nr(texUnit->EnvMode));
         fprintf(stderr, "  GL_COMBINE_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.ModeRGB));
         fprintf(stderr, "  GL_COMBINE_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.ModeA));
         fprintf(stderr, "  GL_SOURCE0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[0]));
         fprintf(stderr, "  GL_SOURCE1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[1]));
         fprintf(stderr, "  GL_SOURCE2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[2]));
         fprintf(stderr, "  GL_SOURCE0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[0]));
         fprintf(stderr, "  GL_SOURCE1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[1]));
         fprintf(stderr, "  GL_SOURCE2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[2]));
         fprintf(stderr, "  GL_OPERAND0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[0]));
         fprintf(stderr, "  GL_OPERAND1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[1]));
         fprintf(stderr, "  GL_OPERAND2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[2]));
         fprintf(stderr, "  GL_OPERAND0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[0]));
         fprintf(stderr, "  GL_OPERAND1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[1]));
         fprintf(stderr, "  GL_OPERAND2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[2]));
         fprintf(stderr, "  GL_RGB_SCALE = %d\n", 1 << texUnit->Combine.ScaleShiftRGB);
         fprintf(stderr, "  GL_ALPHA_SCALE = %d\n", 1 << texUnit->Combine.ScaleShiftA);
         fprintf(stderr, "  GL_TEXTURE_ENV_COLOR = (%f, %f, %f, %f)\n", envColor[0], envColor[1], envColor[2], envColor[3]);
#endif
      }

      alphaComb.Shift   = texUnit->Combine.ScaleShiftA;
      colorComb.Shift   = texUnit->Combine.ScaleShiftRGB;

      switch (texUnit->Combine.ModeRGB) {
             case GL_MODULATE:
                  /* Arg0 * Arg1 == (A + 0) * C + 0 */
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceA,
                                       texUnit->Combine.SourceRGB[0],
                                       texUnit->Combine.OperandRGB[0],
                                       localc, locala);
                  TEXENV_SETUP_MODE_RGB(colorComb.ModeA,
                                        texUnit->Combine.OperandRGB[0]);
                  colorComb.SourceB = GR_CMBX_ZERO;
                  colorComb.ModeB   = GR_FUNC_MODE_ZERO;
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceC,
                                       texUnit->Combine.SourceRGB[1],
                                       texUnit->Combine.OperandRGB[1],
                                       localc, locala);
                  colorComb.InvertC = TEXENV_OPERAND_INVERTED(
                                       texUnit->Combine.OperandRGB[1]);
                  colorComb.SourceD = GR_CMBX_ZERO;
                  break;
             case GL_REPLACE:
                  /* Arg0 == (A + 0) * 1 + 0 */
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceA,
                                       texUnit->Combine.SourceRGB[0],
                                       texUnit->Combine.OperandRGB[0],
                                       localc, locala);
                  TEXENV_SETUP_MODE_RGB(colorComb.ModeA,
                                        texUnit->Combine.OperandRGB[0]);
                  colorComb.SourceB = GR_CMBX_ZERO;
                  colorComb.ModeB   = GR_FUNC_MODE_ZERO;
                  colorComb.SourceC = GR_CMBX_ZERO;
                  colorComb.InvertC = FXTRUE;
                  colorComb.SourceD = GR_CMBX_ZERO;
                  break;
             case GL_ADD:
                  /* Arg0 + Arg1 = (A + B) * 1 + 0 */
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceA,
                                       texUnit->Combine.SourceRGB[0],
                                       texUnit->Combine.OperandRGB[0],
                                       localc, locala);
                  TEXENV_SETUP_MODE_RGB(colorComb.ModeA,
                                        texUnit->Combine.OperandRGB[0]);
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceB,
                                       texUnit->Combine.SourceRGB[1],
                                       texUnit->Combine.OperandRGB[1],
                                       localc, locala);
                  TEXENV_SETUP_MODE_RGB(colorComb.ModeB,
                                        texUnit->Combine.OperandRGB[1]);
                  colorComb.SourceC = GR_CMBX_ZERO;
                  colorComb.InvertC = FXTRUE;
                  colorComb.SourceD = GR_CMBX_ZERO;
                  break;
               case GL_ADD_SIGNED_EXT:
         /* Approximate as ADD (ignoring -0.5 bias due to hardware limits) */
         TEXENV_SETUP_ARG_RGB(colorComb.SourceA,
                              texUnit->Combine.SourceRGB[0],
                              texUnit->Combine.OperandRGB[0],
                              localc, locala);
         TEXENV_SETUP_MODE_RGB(colorComb.ModeA,
                               texUnit->Combine.OperandRGB[0]);
         TEXENV_SETUP_ARG_RGB(colorComb.SourceB,
                              texUnit->Combine.SourceRGB[1],
                              texUnit->Combine.OperandRGB[1],
                              localc, locala);
         TEXENV_SETUP_MODE_RGB(colorComb.ModeB,
                               texUnit->Combine.OperandRGB[1]);
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
         break;
      case GL_SUBTRACT_EXT:
         /* Arg0 - Arg1 = (A + (-B)) * 1 + 0 */
         TEXENV_SETUP_ARG_RGB(colorComb.SourceA,
                              texUnit->Combine.SourceRGB[0],
                              texUnit->Combine.OperandRGB[0],
                              localc, locala);
         TEXENV_SETUP_MODE_RGB(colorComb.ModeA,
                               texUnit->Combine.OperandRGB[0]);
         TEXENV_SETUP_ARG_RGB(colorComb.SourceB,
                              texUnit->Combine.SourceRGB[1],
                              texUnit->Combine.OperandRGB[1],
                              localc, locala);
         colorComb.ModeB = GR_FUNC_MODE_NEGATIVE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
         break;
             case GL_INTERPOLATE_EXT:
                  /* Arg0 * Arg2 + Arg1 * (1 - Arg2) == (A - B) * C + B */
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceA,
                                       texUnit->Combine.SourceRGB[0],
                                       texUnit->Combine.OperandRGB[0],
                                       localc, locala);
                  TEXENV_SETUP_MODE_RGB(colorComb.ModeA,
                                        texUnit->Combine.OperandRGB[0]);
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceB,
                                       texUnit->Combine.SourceRGB[1],
                                       texUnit->Combine.OperandRGB[1],
                                       localc, locala);
                  /* Implement (A - B) by using NEGATIVE_X on B */
                  colorComb.ModeB = GR_FUNC_MODE_NEGATIVE_X;
                  /*
                   * The Source/Operand for the C value must
                   * specify some kind of alpha value.
                   */
                  TEXENV_SETUP_ARG_A(colorComb.SourceC,
                                     texUnit->Combine.SourceRGB[2],
                                     texUnit->Combine.OperandRGB[2],
                                     locala);
                  colorComb.InvertC = FXFALSE;
                  /* D = B */
                  colorComb.SourceD = GR_CMBX_B;
                  colorComb.InvertD = FXFALSE;
                  break;
             default:
                  fprintf(stderr, "COMBINE_EXT_color: %s\n",
                                  _mesa_lookup_enum_by_nr(texUnit->Combine.ModeRGB));
      }

      switch (texUnit->Combine.ModeA) {
             case GL_MODULATE:
                  /* Arg0 * Arg1 == (A + 0) * C + 0 */
                  TEXENV_SETUP_ARG_A(alphaComb.SourceA,
                                     texUnit->Combine.SourceA[0],
                                     texUnit->Combine.OperandA[0],
                                     locala);
                  TEXENV_SETUP_MODE_A(alphaComb.ModeA,
                                      texUnit->Combine.OperandA[0]);
                  alphaComb.SourceB = GR_CMBX_ZERO;
                  alphaComb.ModeB   = GR_FUNC_MODE_ZERO;
                  TEXENV_SETUP_ARG_A(alphaComb.SourceC,
                                     texUnit->Combine.SourceA[1],
                                     texUnit->Combine.OperandA[1],
                                     locala);
                  alphaComb.InvertC = TEXENV_OPERAND_INVERTED(
                                       texUnit->Combine.OperandA[1]);
                  alphaComb.SourceD = GR_CMBX_ZERO;
                  break;
             case GL_REPLACE:
                 /* Arg0 == (A + 0) * 1 + 0 */
                  TEXENV_SETUP_ARG_A(alphaComb.SourceA,
                                     texUnit->Combine.SourceA[0],
                                     texUnit->Combine.OperandA[0],
                                     locala);
                  TEXENV_SETUP_MODE_A(alphaComb.ModeA,
                                      texUnit->Combine.OperandA[0]);
                  alphaComb.SourceB = GR_CMBX_ZERO;
                  alphaComb.ModeB   = GR_FUNC_MODE_ZERO;
                  alphaComb.SourceC = GR_CMBX_ZERO;
                  alphaComb.InvertC = FXTRUE;
                  alphaComb.SourceD = GR_CMBX_ZERO;
                  break;
             case GL_ADD:
                  /* Arg0 + Arg1 = (A + B) * 1 + 0 */
                  TEXENV_SETUP_ARG_A(alphaComb.SourceA,
                                     texUnit->Combine.SourceA[0],
                                     texUnit->Combine.OperandA[0],
                                     locala);
                  TEXENV_SETUP_MODE_A(alphaComb.ModeA,
                                      texUnit->Combine.OperandA[0]);
                  TEXENV_SETUP_ARG_A(alphaComb.SourceB,
                                     texUnit->Combine.SourceA[1],
                                     texUnit->Combine.OperandA[1],
                                     locala);
                  TEXENV_SETUP_MODE_A(alphaComb.ModeB,
                                      texUnit->Combine.OperandA[1]);
                  alphaComb.SourceC = GR_CMBX_ZERO;
                  alphaComb.InvertC = FXTRUE;
                  alphaComb.SourceD = GR_CMBX_ZERO;
                  break;
             case GL_ADD_SIGNED_EXT:
         /* Approximate as ADD for alpha (ignoring -0.5 bias) */
         TEXENV_SETUP_ARG_A(alphaComb.SourceA,
                            texUnit->Combine.SourceA[0],
                            texUnit->Combine.OperandA[0],
                            locala);
         TEXENV_SETUP_MODE_A(alphaComb.ModeA,
                             texUnit->Combine.OperandA[0]);
         TEXENV_SETUP_ARG_A(alphaComb.SourceB,
                            texUnit->Combine.SourceA[1],
                            texUnit->Combine.OperandA[1],
                            locala);
         TEXENV_SETUP_MODE_A(alphaComb.ModeB,
                             texUnit->Combine.OperandA[1]);
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
         break;
      case GL_SUBTRACT_EXT:
         /* Arg0 - Arg1 = (A + (-B)) * 1 + 0 */
         TEXENV_SETUP_ARG_A(alphaComb.SourceA,
                            texUnit->Combine.SourceA[0],
                            texUnit->Combine.OperandA[0],
                            locala);
         TEXENV_SETUP_MODE_A(alphaComb.ModeA,
                             texUnit->Combine.OperandA[0]);
         TEXENV_SETUP_ARG_A(alphaComb.SourceB,
                            texUnit->Combine.SourceA[1],
                            texUnit->Combine.OperandA[1],
                            locala);
         alphaComb.ModeB = GR_FUNC_MODE_NEGATIVE_X;
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
         break;
      case GL_INTERPOLATE_EXT:
         /* Arg0 * Arg2 + Arg1 * (1 - Arg2) == (A - B) * C + B == (A - B) * C + D */
         TEXENV_SETUP_ARG_A(alphaComb.SourceA,
                            texUnit->Combine.SourceA[0],
                            texUnit->Combine.OperandA[0],
                            locala);
         TEXENV_SETUP_MODE_A(alphaComb.ModeA,
                             texUnit->Combine.OperandA[0]);
         TEXENV_SETUP_ARG_A(alphaComb.SourceB,
                            texUnit->Combine.SourceA[1],
                            texUnit->Combine.OperandA[1],
                            locala);
         /* Implement (A - B) by using NEGATIVE_X on B */
         alphaComb.ModeB = GR_FUNC_MODE_NEGATIVE_X;
         /* C must be some alpha channel */
         TEXENV_SETUP_ARG_A(alphaComb.SourceC,
                            texUnit->Combine.SourceA[2],
                            texUnit->Combine.OperandA[2],
                            locala);
         alphaComb.InvertC = FXFALSE;
         /* D = B */
         TEXENV_SETUP_ARG_A(alphaComb.SourceD,
                            texUnit->Combine.SourceA[1],
                            texUnit->Combine.OperandA[1],
                            locala);
         alphaComb.InvertD = FXFALSE;
         break;
             default:
                  fprintf(stderr, "COMBINE_EXT_alpha: %s\n",
                                  _mesa_lookup_enum_by_nr(texUnit->Combine.ModeA));
      }

      fxMesa->Glide.grConstantColorValueExt(tmu,
         (((GLuint)(envColor[0] * 255.0f))      ) |
         (((GLuint)(envColor[1] * 255.0f)) <<  8) |
         (((GLuint)(envColor[2] * 255.0f)) << 16) |
         (((GLuint)(envColor[3] * 255.0f)) << 24));
      break;

   default:
      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "fxSetupTextureEnvNapalm_NoLock: %x Texture.EnvMode not yet supported\n",
		 texUnit->EnvMode);
      }
      return;
   }

   /* On Napalm we simply put the color combine unit into passthrough mode
    * and do everything we need with the texture combine units. */
   fxMesa->Glide.grColorCombineExt(GR_CMBX_TEXTURE_RGB,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   FXTRUE,
                                   GR_CMBX_ZERO,
                                   FXFALSE,
                                   0,
                                   FXFALSE);
   fxMesa->Glide.grAlphaCombineExt(GR_CMBX_TEXTURE_ALPHA,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   FXTRUE,
                                   GR_CMBX_ZERO,
                                   FXFALSE,
                                   0,
                                   FXFALSE);

   fxMesa->Glide.grTexAlphaCombineExt(tmu,
                                      alphaComb.SourceA,
                                      alphaComb.ModeA,
                                      alphaComb.SourceB,
                                      alphaComb.ModeB,
                                      alphaComb.SourceC,
                                      alphaComb.InvertC,
                                      alphaComb.SourceD,
                                      alphaComb.InvertD,
                                      alphaComb.Shift,
                                      alphaComb.Invert);
   fxMesa->Glide.grTexColorCombineExt(tmu,
                                      colorComb.SourceA,
                                      colorComb.ModeA,
                                      colorComb.SourceB,
                                      colorComb.ModeB,
                                      colorComb.SourceC,
                                      colorComb.InvertC,
                                      colorComb.SourceD,
                                      colorComb.InvertD,
                                      colorComb.Shift,
                                      colorComb.Invert);
}


/************************* Single Texture Set ***************************/

static void
fxSelectSingleTMUSrcNapalm_NoLock(fxMesaContext fxMesa, GLint tmu, FxBool LODblend)
{
   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSelectSingleTMUSrcNapalm_NoLock(%d, %d)\n", tmu, LODblend);
   }

   if (LODblend) {
      /* XXX todo - GR_CMBX_LOD_FRAC? */
      fxMesa->tmuSrc = FX_TMU_SPLIT;
   }
   else {
      if (tmu != FX_TMU1) {
         /* disable tex1 */
         if (fxMesa->haveTwoTMUs) {
            fxMesa->Glide.grTexAlphaCombineExt(FX_TMU1,
                                               GR_CMBX_ZERO,
                                               GR_FUNC_MODE_ZERO,
                                               GR_CMBX_ZERO,
                                               GR_FUNC_MODE_ZERO,
                                               GR_CMBX_ZERO,
                                               FXTRUE,
                                               GR_CMBX_ZERO,
                                               FXFALSE,
                                               0,
                                               FXFALSE);
            fxMesa->Glide.grTexColorCombineExt(FX_TMU1,
                                               GR_CMBX_ZERO,
                                               GR_FUNC_MODE_ZERO,
                                               GR_CMBX_ZERO,
                                               GR_FUNC_MODE_ZERO,
                                               GR_CMBX_ZERO,
                                               FXTRUE,
                                               GR_CMBX_ZERO,
                                               FXFALSE,
                                               0,
                                               FXFALSE);
         }

	 fxMesa->tmuSrc = FX_TMU0;
      }
      else {
#if 1
         grTexCombine(GR_TMU0,
                      GR_COMBINE_FUNCTION_BLEND,
                      GR_COMBINE_FACTOR_ONE,
                      GR_COMBINE_FUNCTION_BLEND,
                      GR_COMBINE_FACTOR_ONE,
                      FXFALSE,
                      FXFALSE);
#else
         /* [dBorca] why, oh why? doesn't work! stupid Glide? */
         fxMesa->Glide.grTexAlphaCombineExt(FX_TMU0,
                                            GR_CMBX_OTHER_TEXTURE_ALPHA,
                                            GR_FUNC_MODE_X,
                                            GR_CMBX_ZERO,
                                            GR_FUNC_MODE_X,
                                            GR_CMBX_ZERO,
                                            FXTRUE,
                                            GR_CMBX_ZERO,
                                            FXFALSE,
                                            0,
                                            FXFALSE);
         fxMesa->Glide.grTexColorCombineExt(FX_TMU0,
                                            GR_CMBX_OTHER_TEXTURE_RGB,
                                            GR_FUNC_MODE_X,
                                            GR_CMBX_ZERO,
                                            GR_FUNC_MODE_X,
                                            GR_CMBX_ZERO,
                                            FXTRUE,
                                            GR_CMBX_ZERO,
                                            FXFALSE,
                                            0,
                                            FXFALSE);
#endif

	 fxMesa->tmuSrc = FX_TMU1;
      }
   }
}

/* Nejc: pass-1 (even) setup for two-pass trilinear on a genuine dual-texture
 * draw (base + lightmap in one draw call).
 *
 * (Single-texture draws don't come here: they use the classic split-TMU
 * single-pass trilinear path instead - see fxSetupTexture_NoLock. The
 * two-pass scheme exists for the dual case only, where both TMUs are taken
 * and there is no capacity left for a split.)
 *
 * The mechanism, verified against Glide h5 source and the VSA-100 CSIM:
 *  - grTexMipMapMode(..., FXTRUE) sets SST_TRILINEAR. LOD selection then
 *    snaps to the even (this pass) or odd (SST_LOD_ODD, second pass) member
 *    of the {floor(lod), floor(lod)+1} mip pair. The texture stays a single
 *    normal GR_MIPMAPLEVELMASK_BOTH download - no extra memory.
 *  - the classic texture combine with a LOD_FRACTION factor scales the texel
 *    by the parity-adjusted blend weight of that member. Its
 *    ONE_MINUS_LOD_FRACTION twin is Glide's documented signature for the odd
 *    pass and automatically raises SST_LOD_ODD (grep for "Hack to enable
 *    TWO-PASS Trilinear" in Glide's gtex.c).
 * Pass 1 renders texel_even * weight_even, modulated by the lightmap on the
 * downstream TMU; fxMultipass_Trilinear (fxtris.c) then re-rasterizes the
 * same batch with the odd-pass combine and additive blending. Because the
 * modulate distributes over the sum, the result is exactly
 * lightmap * trilinear(base).
 *
 * While SST_TRILINEAR is set the hardware parity-inverts the TMU combine
 * factor per pixel (CSIM: TREX.C/CCU.C), which would corrupt any normal
 * formula like modulate-by-texture - so the base's env is applied at the
 * chip combine instead: the TMU outputs the weighted texel, the chip does
 * MODULATE/REPLACE (the only env modes fxTrilinearEligible admits). Classic
 * chip-wide grColor/AlphaCombine is safe on Napalm: Glide switches the chip
 * out of extended-combine mode in the classic entry points and back in the
 * Ext ones.
 *
 * The base MUST sit on the upstream TMU: with SST_TRILINEAR set, a TMU's
 * combine factor is parity-inverted per pixel, so only the LOD_FRACTION
 * weight formula is safe there - a downstream "modulate by other" never is.
 * fxTrilinearEligible therefore reports which GL unit holds the base
 * (Quake2-family engines bind it on unit 0, Quake3 binds the lightmap first
 * and the base on unit 1), and this function places the textures by ROLE:
 * lightmap on downstream TMU0, base on upstream TMU1
 * (fxSetupDoubleTMU_NoLock's tObj0/tObj1 contract, incl. evicting swapped
 * residencies). fxChooseVertexState routes each unit's texcoords to the
 * matching TMU - crossed mapping for base-on-unit-0, straight-through for
 * base-on-unit-1. fxTrilinearEligible only admits the second GL stage
 * (unit 1) in GL_MODULATE, which is what the downstream combine implements
 * regardless of which texture it holds.
 */
static void
fxSetupTrilinearPass1DualNapalm_NoLock(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint baseUnit = fxMesa->trilinearBaseUnit;
   struct gl_texture_object *baseObj = ctx->Texture.Unit[baseUnit]._Current;
   struct gl_texture_object *lmObj = ctx->Texture.Unit[1 - baseUnit]._Current;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTrilinearPass1DualNapalm_NoLock(base unit %d)\n",
                      baseUnit);
   }

   /* place by role: lightmap -> downstream TMU0, base -> upstream TMU1 */
   fxSetupDoubleTMU_NoLock(fxMesa, lmObj, baseObj);

   /* Combines first, grTexMipMapMode(..., FXTRUE) last - same UMA-rule
    * ordering constraint as the single-texture variant (Glide's combine
    * entry points would force-raise SST_LOD_ODD on TMU1 otherwise). Both
    * TMUs' SST_TRILINEAR bits are clear right now: fxSetupDoubleTMU_NoLock
    * just programmed grTexMipMapMode(..., FXFALSE) on both. */

   /* upstream TMU1: RGB = base texel * weight(even member). The weight
    * lives on the RGB side only - Glide's SST_LOD_ODD signature checks the
    * RGB combine bits, and the framebuffer sum uses (ONE, ONE) so source
    * alpha is free to carry the REAL texture alpha instead: FUNCTION_LOCAL
    * is F-independent and thus safe under the trilinear parity-invert.
    * Real alpha in both passes makes alpha test work exactly (SoF runs its
    * lightmapped world with alpha test enabled). */
   grTexCombine(FX_TMU1,
                GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL,
                GR_COMBINE_FACTOR_LOD_FRACTION,
                GR_COMBINE_FUNCTION_LOCAL,
                GR_COMBINE_FACTOR_NONE,
                FXFALSE, FXFALSE);

   /* downstream TMU0: RGB modulate by the lightmap; alpha = baseA * lmA
    * (the GL MODULATE alpha formula; lmA is 255 for RGB lightmaps) */
   grTexCombine(FX_TMU0,
                GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL,
                GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL,
                FXFALSE, FXFALSE);

   /* even pass: trilinear LOD mode on the base TMU only */
   grTexMipMapMode(FX_TMU1, GR_MIPMAP_NEAREST, FXTRUE);

   /* The vertex color enters the chain through the FIRST GL stage (unit 0):
    * if its env is MODULATE the chip multiplies by iterated color, if
    * REPLACE nothing does - correct for both unit orientations, since the
    * whole chain is one commutative product. (GL_ALPHA formats are excluded
    * by fxTrilinearEligible.) */
   if (ctx->Texture.Unit[0].EnvMode == GL_MODULATE) {
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_LOCAL,
                     GR_COMBINE_LOCAL_ITERATED,
                     GR_COMBINE_OTHER_TEXTURE,
                     FXFALSE);
   } else {
      /* GL_REPLACE */
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     GR_COMBINE_LOCAL_ITERATED,
                     GR_COMBINE_OTHER_TEXTURE,
                     FXFALSE);
   }
   /* real GL alpha at the chip (blending is excluded, but alpha TEST needs
    * it): the TMU chain already delivers texA_base * texA_lm; the chip
    * multiplies in the iterated alpha - except for a first stage in
    * GL_REPLACE with an alpha-carrying format, where GL drops the fragment
    * alpha. ifmt 0 (not yet derived, e.g. compressed uploads) counts as
    * alpha-carrying, which keeps alpha-tested cutouts correct. */
   {
      tfxTexInfo *tiU0 = fxTMGetTexInfo(ctx->Texture.Unit[0]._Current);
      GLint ifmt0 = tiU0->baseLevelInternalFormat;
      GLboolean fmt0NoAlpha = (ifmt0 == GL_RGB) || (ifmt0 == GL_LUMINANCE);

      if ((ctx->Texture.Unit[0].EnvMode == GL_REPLACE) && !fmt0NoAlpha) {
         grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                        GR_COMBINE_FACTOR_ONE,
                        GR_COMBINE_LOCAL_ITERATED,
                        GR_COMBINE_OTHER_TEXTURE,
                        FXFALSE);
      } else {
         grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                        GR_COMBINE_FACTOR_LOCAL,
                        GR_COMBINE_LOCAL_ITERATED,
                        GR_COMBINE_OTHER_TEXTURE,
                        FXFALSE);
      }
   }

   /* dual-texture vertex hints, as in the normal double-TMU path */
   fxMesa->stw_hint_state |= GR_STWHINT_ST_DIFF_TMU1;
   FX_grHints_NoLock(GR_HINT_STWHINT, fxMesa->stw_hint_state);

   /* bypassed bookkeeping - force full re-derivation on the next draw */
   fxMesa->tmuSrc = FX_TMU_NONE;
   fxMesa->lastUnitsMode = FX_UM_NONE;
   fxMesa->lastCombineTex[0] = NULL;
   fxMesa->lastCombineTex[1] = NULL;

   fxMesa->trilinearTmu = FX_TMU1;	/* arms the odd pass */
}

static void
fxSetupTextureSingleTMUNapalm_NoLock(GLcontext * ctx, GLuint textureset)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLuint unitsmode;
   tfxTexInfo *ti;
   struct gl_texture_object *tObj = ctx->Texture.Unit[textureset]._Current;
   int tmu;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTextureSingleTMUNapalm_NoLock(%d)\n", textureset);
   }

   ti = fxTMGetTexInfo(tObj);

   fxTexValidate(ctx, tObj);

   fxSetupSingleTMU_NoLock(fxMesa, tObj);

   if (ti->whichTMU == FX_TMU_BOTH)
      tmu = FX_TMU0;
   else
      tmu = ti->whichTMU;

   /* Nejc: never pass LODblend here - trilinear-wanting single-texture
    * draws are routed to the classic split path by fxSetupTexture_NoLock;
    * if one still lands here (env mode the classic path can't express) it
    * renders plain bilinear, and the Napalm LODblend combine is an
    * unimplemented stub anyway */
   if (fxMesa->tmuSrc != tmu)
      fxSelectSingleTMUSrcNapalm_NoLock(fxMesa, tmu, FXFALSE);

   if (textureset == 0 || !fxMesa->haveTwoTMUs)
      unitsmode = fxGetTexSetConfiguration(ctx, tObj, NULL);
   else
      unitsmode = fxGetTexSetConfiguration(ctx, NULL, tObj);

   /* Safe skip: only when both combine mode and bound texture match */
   if (fxMesa->lastUnitsMode == unitsmode &&
       fxMesa->lastCombineTex[textureset] == tObj)
   {
      return;
   }

   /* Update guard keys */
   fxMesa->lastUnitsMode = unitsmode;
   fxMesa->lastCombineTex[textureset] = tObj;

   fxMesa->stw_hint_state = 0;
   FX_grHints_NoLock(GR_HINT_STWHINT, 0);

   if (TDFX_DEBUG & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
      fprintf(stderr, "fxSetupTextureSingleTMUNapalm_NoLock: envmode is %s\n",
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[textureset].EnvMode));

   /* [dBorca] Hack alert:
    * what if we're in split mode? (LODBlend)
    * also should we update BOTH TMUs in FX_TMU_BOTH mode?
    */
   fxSetupTextureEnvNapalm_NoLock(ctx, textureset, tmu, GL_TRUE);
}


/************************* Double Texture Set ***************************/

static void
fxSetupTextureDoubleTMUNapalm_NoLock(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti0, *ti1;
   struct gl_texture_object *tObj0 = ctx->Texture.Unit[1]._Current;
   struct gl_texture_object *tObj1 = ctx->Texture.Unit[0]._Current;
   GLuint unitsmode;
   int tmu0 = 0, tmu1 = 1;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTextureDoubleTMUNapalm_NoLock(...)\n");
   }

   ti0 = fxTMGetTexInfo(tObj0);
   fxTexValidate(ctx, tObj0);

   ti1 = fxTMGetTexInfo(tObj1);
   fxTexValidate(ctx, tObj1);

   /* Nejc: trilinear-eligible dual draws take the dedicated two-pass setup;
    * it makes its own, orientation-aware fxSetupDoubleTMU_NoLock call */
   if (fxMesa->trilinearActive) {
      fxSetupTrilinearPass1DualNapalm_NoLock(ctx);
      return;
   }

   fxSetupDoubleTMU_NoLock(fxMesa, tObj0, tObj1);

   unitsmode = fxGetTexSetConfiguration(ctx, tObj0, tObj1);

   /* Safe skip: only when both combine mode and bound texture match */
   if (fxMesa->lastUnitsMode == unitsmode &&
       fxMesa->lastCombineTex[0] == tObj0 &&
       fxMesa->lastCombineTex[1] == tObj1)
   {
      return;
   }

   /* Update guard keys */
   fxMesa->lastUnitsMode = unitsmode;
   fxMesa->lastCombineTex[0] = tObj0;
   fxMesa->lastCombineTex[1] = tObj1;

   fxMesa->stw_hint_state |= GR_STWHINT_ST_DIFF_TMU1;
   FX_grHints_NoLock(GR_HINT_STWHINT, fxMesa->stw_hint_state);

   if (TDFX_DEBUG & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
      fprintf(stderr, "fxSetupTextureDoubleTMUNapalm_NoLock: envmode is %s/%s\n",
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[0].EnvMode),
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[1].EnvMode));


   if ((ti0->whichTMU == FX_TMU1) || (ti1->whichTMU == FX_TMU0)) {
      tmu0 = 1;
      tmu1 = 0;
   }
   fxMesa->tmuSrc = FX_TMU_BOTH;

   /* OpenGL vs Glide texture pipeline */
   fxSetupTextureEnvNapalm_NoLock(ctx, 0, 1, GL_TRUE);
   fxSetupTextureEnvNapalm_NoLock(ctx, 1, 0, GL_FALSE);
}

/************************* No Texture ***************************/

static void
fxSetupTextureNoneNapalm_NoLock(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTextureNoneNapalm_NoLock(...)\n");
   }

   /* the combiner formula is: (A + B) * C + D
   **
   ** a = tc_otherselect
   ** a_mode = tc_invert_other
   ** b = tc_localselect
   ** b_mode = tc_invert_local
   ** c = (tc_mselect, tc_mselect_7)
   ** d = (tc_add_clocal, tc_add_alocal)
   ** shift = tc_outshift
   ** invert = tc_invert_output
   */

   fxMesa->Glide.grColorCombineExt(GR_CMBX_ITRGB,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   GR_FUNC_MODE_ZERO,
                                   GR_CMBX_ZERO,
                                   FXTRUE,
                                   GR_CMBX_ZERO,
                                   FXFALSE,
                                   0,
                                   FXFALSE);
   fxMesa->Glide.grAlphaCombineExt(GR_CMBX_ITALPHA,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   GR_FUNC_MODE_ZERO,
                                   GR_CMBX_ZERO,
                                   FXTRUE,
                                   GR_CMBX_ZERO,
                                   FXFALSE,
                                   0,
                                   FXFALSE);

   /* Reset on full TMU flush*/
   fxMesa->lastCombineTex[0] = fxMesa->lastCombineTex[1] = NULL;
   fxMesa->lastUnitsMode = FX_UM_NONE;
}