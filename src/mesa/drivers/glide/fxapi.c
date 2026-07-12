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


/* fxapi.c - public interface to FX/Mesa functions (fxmesa.h) */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)
#include "fxdrv.h"
#ifdef _WIN32
#include "fxutil.h"
#endif

#include "drivers/common/driverfuncs.h"
#include "main/framebuffer.h"

#ifndef TDFX_DEBUG
int TDFX_DEBUG = (0
/*		  | VERBOSE_VARRAY */
/*		  | VERBOSE_TEXTURE */
/*		  | VERBOSE_IMMEDIATE */
/*		  | VERBOSE_PIPELINE */
/*		  | VERBOSE_DRIVER */
/*		  | VERBOSE_STATE */
/*		  | VERBOSE_API */
/*		  | VERBOSE_DISPLAY_LIST */
/*		  | VERBOSE_LIGHTING */
/*		  | VERBOSE_PRIMS */
/*		  | VERBOSE_VERTS */
   );
#endif

static fxMesaContext fxMesaCurrentCtx = NULL;

/*
 * Status of 3Dfx hardware initialization
 */

static int glbGlideInitialized = 0;
static int glb3DfxPresent = 0;
static int glbTotNumCtx = 0;

static GrHwConfiguration glbHWConfig;
static int glbCurrentBoard = 0;

/* Nejc: nonzero from the moment an alt-tab deactivation is detected until
 * fxMesaRestoreGlideContext has reopened the glide window. While set, no
 * draw/clear may reach glide: glide3x's own lost-context guard in the
 * retail (asm) draw path is broken and crashes with a breakpoint fault
 * when a triangle batch comes in after the lost flag is set mid-frame.
 * Checked in fxRunPipeline (all TNL rendering) and fxDDClear. */
volatile int glbGlideLost = 0;


#if defined(__WIN32__)
static int
cleangraphics(void)
{
   glbTotNumCtx = 1;
   fxMesaDestroyContext(fxMesaCurrentCtx);

   return 0;
}
#elif defined(__linux__)
static void
cleangraphics(void)
{
   glbTotNumCtx = 1;
   fxMesaDestroyContext(fxMesaCurrentCtx);
}

static void
cleangraphics_handler(int s)
{
   fprintf(stderr, "fxmesa: ERROR: received a not handled signal %d\n", s);

   cleangraphics();
/*    abort(); */
   exit(1);
}
#endif


/*
 * Query 3Dfx hardware presence/kind
 */
static GLboolean GLAPIENTRY fxQueryHardware (void)
{
 if (TDFX_DEBUG & VERBOSE_DRIVER) {
    fprintf(stderr, "fxQueryHardware()\n");
 }

 if (!glbGlideInitialized) {
    grGlideInit();
    glb3DfxPresent = FX_grSstQueryHardware(&glbHWConfig);

    glbGlideInitialized = 1;

#if defined(__WIN32__)
    _onexit((_onexit_t) cleangraphics);
#elif defined(__linux__)
    /* Only register handler if environment variable is not defined. */
    if (!getenv("MESA_FX_NO_SIGNALS")) {
       atexit(cleangraphics);
    }
#endif
 }

 return glb3DfxPresent;
}


/*
 * Select the Voodoo board to use when creating
 * a new context.
 */
GLint GLAPIENTRY fxMesaSelectCurrentBoard (int n)
{
   fxQueryHardware();

   if ((n < 0) || (n >= glbHWConfig.num_sst))
      return -1;

   return glbHWConfig.SSTs[glbCurrentBoard = n].type;
}


fxMesaContext GLAPIENTRY fxMesaGetCurrentContext (void)
{
 return fxMesaCurrentCtx;
}


void GLAPIENTRY fxGetScreenGeometry (GLint *w, GLint *h)
{
 GLint width = 0;
 GLint height = 0;
 
 if (fxMesaCurrentCtx != NULL) {
    width = fxMesaCurrentCtx->screen_width;
    height = fxMesaCurrentCtx->screen_height;
 }

 if (w != NULL) {
    *w = width;
 }
 if (h != NULL) {
    *h = height;
 }
}


/*
 * The 3Dfx Global Palette extension for GLQuake.
 * More a trick than a real extesion, use the shared global
 * palette extension. 
 */
extern void GLAPIENTRY gl3DfxSetPaletteEXT(GLuint * pal);	/* silence warning */
void GLAPIENTRY
gl3DfxSetPaletteEXT(GLuint * pal)
{
   fxMesaContext fxMesa = fxMesaCurrentCtx;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      int i;

      fprintf(stderr, "gl3DfxSetPaletteEXT(...)\n");

      for (i = 0; i < 256; i++) {
	 fprintf(stderr, "\t%x\n", pal[i]);
      }
   }

   if (fxMesa) {
      fxMesa->haveGlobalPaletteTexture = 1;

      /* Nejc: keep a copy so the palette can be re-downloaded after the
       * Glide context is restored (alt-tab) */
      memcpy(&fxMesa->glbPalette, pal, sizeof(fxMesa->glbPalette));

      grTexDownloadTable(GR_TEXTABLE_PALETTE, (GuTexPalette *) pal);
   }
}


static GrScreenResolution_t fxBestResolution (int width, int height)
{
 static int resolutions[][3] = {
        { GR_RESOLUTION_320x200,    320,  200 },
        { GR_RESOLUTION_320x240,    320,  240 },
        { GR_RESOLUTION_400x256,    400,  256 },
        { GR_RESOLUTION_512x384,    512,  384 },
        { GR_RESOLUTION_640x200,    640,  200 },
        { GR_RESOLUTION_640x350,    640,  350 },
        { GR_RESOLUTION_640x400,    640,  400 },
        { GR_RESOLUTION_640x480,    640,  480 },
        { GR_RESOLUTION_800x600,    800,  600 },
        { GR_RESOLUTION_960x720,    960,  720 },
        { GR_RESOLUTION_856x480,    856,  480 },
        { GR_RESOLUTION_512x256,    512,  256 },
        { GR_RESOLUTION_1024x768,  1024,  768 },
        { GR_RESOLUTION_1280x1024, 1280, 1024 },
        { GR_RESOLUTION_1600x1200, 1600, 1200 },
        { GR_RESOLUTION_400x300,    400,  300 },
        { GR_RESOLUTION_1152x864,  1152,  864 },
        { GR_RESOLUTION_1280x960,  1280,  960 },
        { GR_RESOLUTION_1600x1024, 1600, 1024 },
        { GR_RESOLUTION_1792x1344, 1792, 1344 },
        { GR_RESOLUTION_1856x1392, 1856, 1392 },
        { GR_RESOLUTION_1920x1440, 1920, 1440 },
        { GR_RESOLUTION_2048x1536, 2048, 1536 },
        { GR_RESOLUTION_2048x2048, 2048, 2048 },
        /* nd Extended */
        {GR_RESOLUTION_1280x720, 1280, 720},
        {GR_RESOLUTION_1280x800, 1280, 800},
        {GR_RESOLUTION_1360x768, 1360, 768},
        {GR_RESOLUTION_1440x900, 1440, 900},
        {GR_RESOLUTION_1600x900, 1600, 900},
        {GR_RESOLUTION_1680x720, 1680, 720},
        {GR_RESOLUTION_1680x1050, 1680, 1050},
        {GR_RESOLUTION_1792x768, 1792, 768},
        {GR_RESOLUTION_1920x800, 1920, 800},
        {GR_RESOLUTION_1920x1080, 1920, 1080},
        {GR_RESOLUTION_1920x1200, 1920, 1200},
        {GR_RESOLUTION_1960x840, 1960, 840},
        /* This one added so we have an unreachable max, if more come */
        {GR_RESOLUTION_3840x2160, 3840, 2160}};
 

 int i, size;
 int lastvalidres = GR_RESOLUTION_640x480;
 int min = 3840 * 2160; // max is GR_RESOLUTION_3840x2160
 GrResolution resTemplate = {
              GR_QUERY_ANY,
              GR_QUERY_ANY,
              2 /*GR_QUERY_ANY */,
              GR_QUERY_ANY
 };
 GrResolution *presSupported;

 fxQueryHardware();

 size = grQueryResolutions(&resTemplate, NULL);
 presSupported = malloc(size);
        
 size /= sizeof(GrResolution);
 grQueryResolutions(&resTemplate, presSupported);

 for (i = 0; i < size; i++) {
     int r = presSupported[i].resolution;
     if ((width <= resolutions[r][1]) && (height <= resolutions[r][2])) {
        if (min > (resolutions[r][1] * resolutions[r][2])) {
           min = resolutions[r][1] * resolutions[r][2];
           lastvalidres = r;
        }
     }
 }

 free(presSupported);

 return resolutions[lastvalidres][0];
}


fxMesaContext GLAPIENTRY
fxMesaCreateBestContext(GLuint win, GLint width, GLint height,
			const GLint attribList[])
{
 int res = fxBestResolution(width, height);

 if (res == -1) {
    return NULL;
 }

/* Get best Refresh rate*/
   GrScreenRefresh_t refresh = GR_REFRESH_60Hz;    /* Default refresh */
   int refreshFromReg = ReadRefreshFromRegistry(); /* Or env variable */

   // Convert to GrScreenRefresh_t
   if (refreshFromReg > 0)
   {
      switch (refreshFromReg)
      {
      case 60:
         refresh = GR_REFRESH_60Hz;
         break;
      case 70:
         refresh = GR_REFRESH_70Hz;
         break;
      case 72:
         refresh = GR_REFRESH_72Hz;
         break;
      case 75:
         refresh = GR_REFRESH_75Hz;
         break;
      case 80:
         refresh = GR_REFRESH_80Hz;
         break;
      case 85:
         refresh = GR_REFRESH_85Hz;
         break;
      case 90:
         refresh = GR_REFRESH_90Hz;
         break;
      case 100:
         refresh = GR_REFRESH_100Hz;
         break;
      case 120:
         refresh = GR_REFRESH_120Hz;
         break;
      case 144:
         refresh = GR_REFRESH_144Hz;
         break;
      default:
         /* Unknown refresh, ignore override */
         break;
      }
   }

   return fxMesaCreateContext(win, res, refresh, attribList);
 /* return fxMesaCreateContext(win, res, GR_REFRESH_60Hz, attribList); */
}


/*
 * Create a new FX/Mesa context and return a handle to it.
 */
fxMesaContext GLAPIENTRY
fxMesaCreateContext(GLuint win,
		    GrScreenResolution_t res,
		    GrScreenRefresh_t ref, const GLint attribList[])
{
 fxMesaContext fxMesa = NULL;
 GLcontext *ctx = NULL, *shareCtx = NULL;
 struct dd_function_table functions;

 int i;
 const char *str;
 int sliaa, numSLI, samplesPerChip;
 struct SstCard_St *voodoo;
 struct tdfx_glide *Glide;

 GLboolean aux;
 GLboolean doubleBuffer;
 GLuint colDepth;
 GLuint depthSize, alphaSize, stencilSize, accumSize;
 GLuint redBits, greenBits, blueBits, alphaBits;
 GrPixelFormat_t pixFmt;
   
 if (TDFX_DEBUG & VERBOSE_DRIVER) {
    fprintf(stderr, "fxMesaCreateContext(...)\n");
 }

 /* Okay, first process the user flags */
 aux = GL_FALSE;
 doubleBuffer = GL_FALSE;
 colDepth = 16;
 depthSize = alphaSize = stencilSize = accumSize = 0;

 i = 0;
 while (attribList[i] != FXMESA_NONE) {
       switch (attribList[i]) {
              case FXMESA_COLORDEPTH:
	           colDepth = attribList[++i];
	           break;
              case FXMESA_DOUBLEBUFFER:
	           doubleBuffer = GL_TRUE;
	           break;
              case FXMESA_ALPHA_SIZE:
	           if ((alphaSize = attribList[++i])) {
	              aux = GL_TRUE;
                   }
	           break;
              case FXMESA_DEPTH_SIZE:
	           if ((depthSize = attribList[++i])) {
	              aux = GL_TRUE;
                   }
	           break;
              case FXMESA_STENCIL_SIZE:
	           stencilSize = attribList[++i];
	           break;
              case FXMESA_ACCUM_SIZE:
	           accumSize = attribList[++i];
	           break;
              /* XXX ugly hack here for sharing display lists */
              case FXMESA_SHARE_CONTEXT:
                   shareCtx = (GLcontext *)attribList[++i];
	           break;
              default:
                   fprintf(stderr, "fxMesaCreateContext: ERROR: wrong parameter (%d) passed\n", attribList[i]);
	           return NULL;
       }
       i++;
 }

 if (!fxQueryHardware()) {
    str = "no Voodoo hardware!";
    goto errorhandler;
 }

 grSstSelect(glbCurrentBoard);
 grEnable(GR_OPENGL_MODE_EXT); /* Pass to glide we are OpenGL */
 voodoo = &glbHWConfig.SSTs[glbCurrentBoard];

 fxMesa = (fxMesaContext)CALLOC_STRUCT(tfxMesaContext);
 if (!fxMesa) {
    str = "private context";
    goto errorhandler;
 }

 if (getenv("MESA_FX_INFO")) {
    fxMesa->verbose = GL_TRUE;
 }

 fxMesa->type = voodoo->type;
 fxMesa->HavePalExt = voodoo->HavePalExt && !getenv("MESA_FX_IGNORE_PALEXT");
 fxMesa->HavePixExt = voodoo->HavePixExt && !getenv("MESA_FX_IGNORE_PIXEXT");
 fxMesa->HaveTexFmt = voodoo->HaveTexFmt && !getenv("MESA_FX_IGNORE_TEXFMT");
 fxMesa->HaveCmbExt = voodoo->HaveCmbExt && !getenv("MESA_FX_IGNORE_CMBEXT");
 fxMesa->HaveMirExt = voodoo->HaveMirExt && !getenv("MESA_FX_IGNORE_MIREXT");
 fxMesa->HaveTexUma = voodoo->HaveTexUma && !getenv("MESA_FX_IGNORE_TEXUMA");
 fxMesa->Glide = glbHWConfig.Glide;
 Glide = &fxMesa->Glide;
 fxMesa->HaveTexus2 = Glide->txImgQuantize &&
                      Glide->txMipQuantize &&
                      Glide->txPalToNcc && !getenv("MESA_FX_IGNORE_TEXUS2");

/* Defaults enabled: keep textures resident on invalidate */
fxMesa->keepResidentOnInvalidate = GL_TRUE;

/* Nejc 16bit Textures override from 3dfx tools */
   if (fxGetRegistryOrEnvironmentString("FX_MESA_FORCE_16BPP_TEXTURES") != NULL)
   {
         fxMesa->HaveTexFmt = GL_FALSE;
   }

   /* Nejc 16bit pixel format override - force at context creation, safety */
   if (fxGetRegistryOrEnvironmentString("FX_MESA_FORCE_16BPP_PIX") != NULL)
   {
      int boardType = fxMesaSelectCurrentBoard(0);
      if (boardType == GR_SSTTYPE_Voodoo5 || boardType == GR_SSTTYPE_Voodoo4)
      {
         colDepth = 16;
         depthSize = 16;
      }
   }

/* HSR will be initialized after window is opened and dimensions are known */
 fxMesa->hsrEnabled = GL_FALSE;
#ifdef _WIN32
 if ((getenv("MESA_FX_NO_HSR") == NULL) && DetectQuake3Engine()) {
    fxMesa->hsrEnabled = GL_TRUE;
 }
#endif
 fxMesa->hsrTileDepth = NULL;

 /* Determine if we need vertex swapping, RGB order and SLI/AA */
 sliaa = 0;
 switch (fxMesa->type) {
        case GR_SSTTYPE_VOODOO:
        case GR_SSTTYPE_SST96:
        case GR_SSTTYPE_Banshee:
             fxMesa->bgrOrder = GL_TRUE;
             fxMesa->snapVertices = (getenv("MESA_FX_NOSNAP") == NULL);
             break;
        case GR_SSTTYPE_Voodoo2:
             fxMesa->bgrOrder = GL_TRUE;
             fxMesa->snapVertices = GL_FALSE;
             break;
        case GR_SSTTYPE_Voodoo4:
        case GR_SSTTYPE_Voodoo5:
             /* number of SLI units and AA Samples per chip */
             if ((str = Glide->grGetRegistryOrEnvironmentStringExt("SSTH3_SLI_AA_CONFIGURATION")) != NULL) {
                sliaa = atoi(str);
             }
             fxMesa->bgrOrder = GL_FALSE;     /* correct for Voodoo 4/5 */
             fxMesa->snapVertices = GL_FALSE;
             break;
        case GR_SSTTYPE_Voodoo3:
        default:
             fxMesa->bgrOrder = GL_FALSE;
             fxMesa->snapVertices = GL_FALSE;
             break;
 }
 /* XXX todo - Add the old SLI/AA settings for Napalm. */
 switch(voodoo->numChips) {
 case 4: /* 4 chips */
   switch(sliaa) {
   case 8: /* 8 Sample AA */
     numSLI         = 1;
     samplesPerChip = 2;
     break;
   case 7: /* 4 Sample AA */
     numSLI         = 1;
     samplesPerChip = 1;
     break;
   case 6: /* 2 Sample AA */
     numSLI         = 2;
     samplesPerChip = 1;
     break;
   default:
     numSLI         = 4;
     samplesPerChip = 1;
   }
   break;
 case 2: /* 2 chips */
   switch(sliaa) {
   case 4: /* 4 Sample AA */
     numSLI         = 1;
     samplesPerChip = 2;
     break;
   case 3: /* 2 Sample AA */
     numSLI         = 1;
     samplesPerChip = 1;
     break;
   default:
     numSLI         = 2;
     samplesPerChip = 1;
   }
   break;
 default: /* 1 chip */
   switch(sliaa) {
   case 1: /* 2 Sample AA */
     numSLI         = 1;
     samplesPerChip = 2;
     break;
   default:
     numSLI         = 1;
     samplesPerChip = 1;
   }
 }

 fxMesa->fsaa = samplesPerChip * voodoo->numChips / numSLI; /* 1:noFSAA, 2:2xFSAA, 4:4xFSAA, 8:8xFSAA */

 switch (fxMesa->colDepth = colDepth) {
   case 15:
     redBits   = 5;
     greenBits = 5;
     blueBits  = 5;
     alphaBits = depthSize ? 1 : 8;
     switch(fxMesa->fsaa) {
       case 8:
         pixFmt = GR_PIXFMT_AA_8_ARGB_1555;
         break;
       case 4:
         pixFmt = GR_PIXFMT_AA_4_ARGB_1555;
         break;
       case 2:
         pixFmt = GR_PIXFMT_AA_2_ARGB_1555;
         break;
       default:
         pixFmt = GR_PIXFMT_ARGB_1555;
     }
     break;
   case 16:
     redBits   = 5;
     greenBits = 6;
     blueBits  = 5;
     alphaBits = depthSize ? 0 : 8;
     switch(fxMesa->fsaa) {
       case 8:
         pixFmt = GR_PIXFMT_AA_8_RGB_565;
         break;
       case 4:
         pixFmt = GR_PIXFMT_AA_4_RGB_565;
         break;
       case 2:
         pixFmt = GR_PIXFMT_AA_2_RGB_565;
         break;
       default:
         pixFmt = GR_PIXFMT_RGB_565;
     }
     break;
   case 24:
     fxMesa->colDepth = 32;
   case 32:
     redBits   = 8;
     greenBits = 8;
     blueBits  = 8;
     alphaBits = 8;
     switch(fxMesa->fsaa) {
       case 8:
         pixFmt = GR_PIXFMT_AA_8_ARGB_8888;
         break;
       case 4:
         pixFmt = GR_PIXFMT_AA_4_ARGB_8888;
         break;
       case 2:
         pixFmt = GR_PIXFMT_AA_2_ARGB_8888;
         break;
       default:
         pixFmt = GR_PIXFMT_ARGB_8888;
     }
     break;
   default:
     str = "pixelFormat";
     goto errorhandler;
 }

 /* Tips:
  * 1. we don't bother setting/checking AUX for stencil, because we'll decide
  *    later whether we have HW stencil, based on depth buffer (thus AUX is
  *    properly set)
  * 2. when both DEPTH and ALPHA are enabled, depth should win. However, it is
  *    not clear whether 15bpp and 32bpp require AUX alpha buffer. Furthermore,
  *    alpha buffering is required only if destination alpha is used in alpha
  *    blending; alpha blending modes that do not use destination alpha can be
  *    used w/o alpha buffer.
  * 3. `alphaBits' is what we can provide
  *    `alphaSize' is what app requests
  *    if we cannot provide enough bits for alpha buffer, we should fallback to
  *    SW alpha. However, setting `alphaBits' to `alphaSize' might confuse some
  *    of the span functions...
  */

 fxMesa->haveHwAlpha = GL_FALSE;
 if (alphaSize && (alphaSize <= alphaBits)) {
    alphaSize = alphaBits;
    fxMesa->haveHwAlpha = GL_TRUE;
 }

 fxMesa->haveHwStencil = (fxMesa->HavePixExt && stencilSize && depthSize == 24);

 fxMesa->haveZBuffer = depthSize > 0;
 fxMesa->haveDoubleBuffer = doubleBuffer;
 fxMesa->haveGlobalPaletteTexture = GL_FALSE;
 fxMesa->board = glbCurrentBoard;

 fxMesa->haveTwoTMUs = (voodoo->nTexelfx > 1);

 if ((str = Glide->grGetRegistryOrEnvironmentStringExt("FX_GLIDE_NUM_TMU"))) {
    if (atoi(str) <= 1) {
       fxMesa->haveTwoTMUs = GL_FALSE;
    }
 }

 if ((str = Glide->grGetRegistryOrEnvironmentStringExt("FX_GLIDE_SWAPPENDINGCOUNT"))) {
    fxMesa->maxPendingSwapBuffers = atoi(str);
    if (fxMesa->maxPendingSwapBuffers > 6) {
       fxMesa->maxPendingSwapBuffers = 6;
    } else if (fxMesa->maxPendingSwapBuffers < 0) {
       fxMesa->maxPendingSwapBuffers = 0;
    }
 } else {
    fxMesa->maxPendingSwapBuffers = 2;
 }

 if ((str = Glide->grGetRegistryOrEnvironmentStringExt("FX_GLIDE_SWAPINTERVAL"))) {
    fxMesa->swapInterval = atoi(str);
 } else {
    fxMesa->swapInterval = 0;
 }

 BEGIN_BOARD_LOCK();
 if (fxMesa->HavePixExt) {
    fxMesa->glideContext = Glide->grSstWinOpenExt((FxU32)win, res, ref,
                                                  GR_COLORFORMAT_ABGR, GR_ORIGIN_LOWER_LEFT,
                                                  pixFmt,
                                                  2, aux);
 } else if (pixFmt == GR_PIXFMT_RGB_565) {
    fxMesa->glideContext = grSstWinOpen((FxU32)win, res, ref,
                                        GR_COLORFORMAT_ABGR, GR_ORIGIN_LOWER_LEFT,
                                        2, aux);
 } else {
    fxMesa->glideContext = 0;
 }
 END_BOARD_LOCK();
 if (!fxMesa->glideContext) {
    str = "grSstWinOpen";
    goto errorhandler;
 }

 /* Nejc: save the open parameters for fxMesaRestoreGlideContext (alt-tab) */
 fxMesa->openWin = (FxU32) win;
 fxMesa->openRes = res;
 fxMesa->openRef = ref;
 fxMesa->openPixFmt = pixFmt;
 fxMesa->openAux = aux;

 /* a fresh grSstWinOpen just cleared glide's lost-context flag */
 glbGlideLost = 0;

   /* screen */
   fxMesa->screen_width = FX_grSstScreenWidth();
   fxMesa->screen_height = FX_grSstScreenHeight();

   /* window inside screen */
   fxMesa->width = fxMesa->screen_width;
   fxMesa->height = fxMesa->screen_height;

   /* scissor inside window */
   fxMesa->clipMinX = 0;
   fxMesa->clipMaxX = fxMesa->width;
   fxMesa->clipMinY = 0;
   fxMesa->clipMaxY = fxMesa->height;

   /* Initialize HSR (Hidden Surface Removal) now that we have screen dimensions */
   if (fxMesa->hsrEnabled) {
      fxMesa->hsrTileSize = 16;  /* 16x16 pixel tiles for coarse depth testing */
      fxMesa->hsrTilesX = (fxMesa->width + fxMesa->hsrTileSize - 1) / fxMesa->hsrTileSize;
      fxMesa->hsrTilesY = (fxMesa->height + fxMesa->hsrTileSize - 1) / fxMesa->hsrTileSize;
      fxMesa->hsrTileDepth = (GLfloat *)CALLOC(fxMesa->hsrTilesX * fxMesa->hsrTilesY * sizeof(GLfloat));
      if (fxMesa->hsrTileDepth) {
         /* Initialize all tiles to far plane (1.0 in normalized depth) */
         GLuint i;
         for (i = 0; i < fxMesa->hsrTilesX * fxMesa->hsrTilesY; i++) {
            fxMesa->hsrTileDepth[i] = 1.0f;
         }
         fprintf(stderr, "Voodoo HSR: Enabled (%dx%d tiles, %dx%d resolution)\n",
                 fxMesa->hsrTilesX, fxMesa->hsrTilesY,
                 fxMesa->width, fxMesa->height);
      } else {
         fxMesa->hsrEnabled = GL_FALSE;  /* Disable if allocation failed */
         fprintf(stderr, "Voodoo HSR: Disabled (allocation failed)\n");
      }
   } else {
#ifdef _WIN32
      if (getenv("MESA_FX_NO_HSR") != NULL) {
         fprintf(stderr, "Voodoo HSR: Disabled (MESA_FX_NO_HSR set)\n");
      } else {
         fprintf(stderr, "Voodoo HSR: Disabled (non-Quake3 engine)\n");
      }
#else
      fprintf(stderr, "Voodoo HSR: Disabled (non-Win32 build)\n");
#endif
   }

   if (fxMesa->verbose) {
      FxI32 tmuRam, fbRam;

      /* Not that it matters, but tmuRam and fbRam change after grSstWinOpen. */
      tmuRam = voodoo->tmuConfig[GR_TMU0].tmuRam;
      fbRam  = voodoo->fbRam;
      BEGIN_BOARD_LOCK();
      grGet(GR_MEMORY_TMU, 4, &tmuRam);
      grGet(GR_MEMORY_FB, 4, &fbRam);
      END_BOARD_LOCK();

      fprintf(stderr, "Voodoo Using Glide %s\n", grGetString(GR_VERSION));
      fprintf(stderr, "Voodoo Board: %d/%d, %s, %d GPU\n",
                      fxMesa->board + 1,
                      glbHWConfig.num_sst,
                      grGetString(GR_HARDWARE),
                      voodoo->numChips);
      fprintf(stderr, "Voodoo Memory: FB = %ld, TM = %d x %ld\n",
                      fbRam,
                      voodoo->nTexelfx,
                      tmuRam);
      fprintf(stderr, "Voodoo Screen: %dx%d:%d %s, %svertex snapping\n",
	              fxMesa->screen_width,
                      fxMesa->screen_height,
                      colDepth,
                      fxMesa->bgrOrder ? "BGR" : "RGB",
                      fxMesa->snapVertices ? "" : "no ");
   }

  sprintf(fxMesa->rendererString, "Mesa %s v0.74 %s%s",
          grGetString(GR_RENDERER),
          grGetString(GR_HARDWARE),
          ((fxMesa->type < GR_SSTTYPE_Voodoo4) && (voodoo->numChips > 1)) ? " SLI" : "");

   fxMesa->glVis = _mesa_create_visual(GL_TRUE,		/* RGB mode */
				       doubleBuffer,
				       GL_FALSE,	/* stereo */
				       redBits,		/* RGBA.R bits */
				       greenBits,	/* RGBA.G bits */
				       blueBits,	/* RGBA.B bits */
				       alphaSize,	/* RGBA.A bits */
				       0,		/* index bits */
				       depthSize,	/* depth_size */
				       stencilSize,	/* stencil_size */
				       accumSize,
				       accumSize,
				       accumSize,
				       alphaSize ? accumSize : 0,
                                       1);
   if (!fxMesa->glVis) {
      str = "_mesa_create_visual";
      goto errorhandler;
   }

   if (TDFX_DEBUG & VERBOSE_DRIVER)
   {
      fprintf(stderr, "Visual created successfully, about to init driver functions...\n");
   }

   /* 
   * Init default driver functions then plug in our tdfx-specific functions
   */
   _mesa_init_driver_functions(&functions);
    // Nejc added
   fxSetupDDPointers_PreContext(&functions, fxMesa);
   fxInitTextureFuncs(&functions);

   ctx = fxMesa->glCtx = _mesa_create_context(fxMesa->glVis, shareCtx,
					      &functions, (void *) fxMesa);
   if (!ctx) {
      str = "_mesa_create_context";
      goto errorhandler;
   }

   /* Nejc CRITICAL: Link the fxMesa context to the Mesa context so FX_CONTEXT() macro works. This links the Mesa GL context to the driver-specific context */
   ctx->DriverCtx = fxMesa;

if (TDFX_DEBUG & VERBOSE_DRIVER)
   {
      fprintf(stderr, "Mesa context created successfully. About to call fxDDInitFxMesaContext...\n");
   }

   if (!fxDDInitFxMesaContext(fxMesa)) {
      str = "fxDDInitFxMesaContext";
      goto errorhandler;
   }

   /* Create framebuffer using Mesa 6.4.2 renderbuffer infrastructure */
   fxMesa->glBuffer = fxNewFramebuffer(ctx, fxMesa->glVis);
   if (!fxMesa->glBuffer) {
      str = "fxNewFramebuffer";
      goto errorhandler;
   }

   if (TDFX_DEBUG & VERBOSE_DRIVER)
   {
      fprintf(stderr, "fxNewFramebuffer completed successfully\n");
   }

   glbTotNumCtx++;
   return fxMesa;

errorhandler:
 if (fxMesa) {
    if (fxMesa->glideContext) {
       grSstWinClose(fxMesa->glideContext);
       fxMesa->glideContext = 0;
    }

    if (fxMesa->state) {
       FREE(fxMesa->state);
    }
    if (fxMesa->fogTable) {
       FREE(fxMesa->fogTable);
    }
    if (fxMesa->glBuffer) {
       _mesa_unreference_framebuffer(&fxMesa->glBuffer);
    }
    if (fxMesa->glVis) {
       _mesa_destroy_visual(fxMesa->glVis);
    }
    if (fxMesa->glCtx) {
       _mesa_destroy_context(fxMesa->glCtx);
    }
    FREE(fxMesa);
 }

 fprintf(stderr, "fxMesaCreateContext: ERROR: %s\n", str);
 return NULL;
}

/*
 * Function to set the new window size in the context (mainly for the Voodoo Rush)
 */
void GLAPIENTRY
fxMesaUpdateScreenSize(fxMesaContext fxMesa)
{
   fxMesa->width = FX_grSstScreenWidth();
   fxMesa->height = FX_grSstScreenHeight();

   /*Nejc - Update framebuffer size */
   if (fxMesa->glCtx)
   {
      fxUpdateFramebufferSize(fxMesa->glCtx);
   }
}

/*
 * Nejc: Alt-tab recovery ("lazy restore").
 *
 * When the game window is deactivated (alt-tab), glide3x sets its internal
 * lost-context flag - via its own window hook (WM_ACTIVATEAPP FALSE /
 * minimize, see _XPAltTabProc in the glide sources) and, on Win9x, via the
 * context DWORD shared with the display driver. From that moment every
 * glide call is silently ignored, and nothing ever clears the flag except
 * a new grSstWinOpen. Glide never recovers by itself.
 *
 * So when the game comes back we close the dead window and open a fresh
 * one with the parameters saved at context creation - exactly the same
 * grSstWinClose/grSstWinOpen plumbing the (working) in-game resolution
 * change path uses - and then re-apply all hardware state ourselves,
 * because unlike a real mode change the game does not know anything
 * happened and will not re-issue its GL setup. Texture memory content
 * does not survive either, so resident textures are re-downloaded.
 *
 * grSstWinClose on a lost context is safe: glide detects the lost state,
 * skips the FIFO flush and only resets the video / unhooks its winproc.
 *
 * Returns GL_FALSE if the re-open failed (window may still be
 * mid-transition); the caller may retry on a later frame.
 */
GLboolean GLAPIENTRY
fxMesaRestoreGlideContext(fxMesaContext fxMesa)
{
   struct tdfx_glide *Glide;
   GrContext_t newGlideCtx;

   if (!fxMesa)
      return GL_FALSE;

   /* windowed contexts don't own the display and never lose it;
    * report success so the caller disarms */
   if (fxMesa->openRes == GR_RESOLUTION_NONE)
      return GL_TRUE;

   Glide = &fxMesa->Glide;

   BEGIN_BOARD_LOCK();

   /* the old window is dead (lost context); close it first so open/close
    * stay balanced inside glide, like on an in-game mode change */
   if (fxMesa->glideContext) {
      grSstWinClose(fxMesa->glideContext);
      fxMesa->glideContext = 0;
   }

   grSstSelect(fxMesa->board);
   grEnable(GR_OPENGL_MODE_EXT);

   if (fxMesa->HavePixExt) {
      newGlideCtx = Glide->grSstWinOpenExt(fxMesa->openWin,
                                           fxMesa->openRes, fxMesa->openRef,
                                           GR_COLORFORMAT_ABGR, GR_ORIGIN_LOWER_LEFT,
                                           fxMesa->openPixFmt,
                                           2, fxMesa->openAux);
   } else {
      newGlideCtx = grSstWinOpen(fxMesa->openWin,
                                 fxMesa->openRes, fxMesa->openRef,
                                 GR_COLORFORMAT_ABGR, GR_ORIGIN_LOWER_LEFT,
                                 2, fxMesa->openAux);
   }
   END_BOARD_LOCK();

   if (!newGlideCtx) {
      return GL_FALSE;
   }
   fxMesa->glideContext = newGlideCtx;

   /* the fresh grSstWinOpen cleared glide's lost flag; rendering (and the
    * state/texture restore below) may reach the hardware again */
   glbGlideLost = 0;

   /* NOTE: deliberately no grGlideSetState() here. The state mirror in
    * fxMesa->state is only captured on context switches; single-context
    * games never populate it, and replaying a stale/zeroed mirror would
    * program garbage. grSstWinOpen leaves glide at defaults - the same
    * baseline a fresh context starts from - and the full revalidation
    * forced below re-emits everything from Mesa's state. */

   /* re-apply the hardware-facing setup normally done in
    * fxDDInitFxMesaContext */
   FX_setupGrVertexLayout();
   fxMesa->stw_hint_state = 0;

   if (fxMesa->colDepth == 32) {
      Glide->grColorMaskExt(FXTRUE, FXTRUE, FXTRUE, fxMesa->haveHwAlpha);
   } else {
      grColorMask(FXTRUE, fxMesa->haveHwAlpha);
   }

   grRenderBuffer(fxMesa->currentFB);

   if (fxMesa->haveZBuffer) {
      grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
   }

   if (!fxMesa->bgrOrder) {
      grLfbWriteColorFormat(GR_COLORFORMAT_ABGR);
   }

   if (Glide->grSetNumPendingBuffers != NULL) {
      Glide->grSetNumPendingBuffers(fxMesa->maxPendingSwapBuffers);
   }

   if (fxMesa->HaveTexUma) {
      grEnable(GR_TEXTURE_UMA_EXT);
   }

   if (fxMesa->glCtx) {
      grDitherMode(fxMesa->glCtx->Color.DitherFlag ? GR_DITHER_4x4
                                                   : GR_DITHER_DISABLE);
   }

   /* the global palette (GLQuake) lives in texture hardware; re-download */
   if (fxMesa->haveGlobalPaletteTexture) {
      grTexDownloadTable(GR_TEXTABLE_PALETTE, &fxMesa->glbPalette);
   }

   /* pick up the (possibly changed) screen size */
   fxMesaUpdateScreenSize(fxMesa);

   /* re-size the HSR tile cache if the resolution changed */
   if (fxMesa->hsrEnabled && fxMesa->hsrTileDepth) {
      GLuint tilesX = (fxMesa->width + fxMesa->hsrTileSize - 1) / fxMesa->hsrTileSize;
      GLuint tilesY = (fxMesa->height + fxMesa->hsrTileSize - 1) / fxMesa->hsrTileSize;
      if (tilesX != fxMesa->hsrTilesX || tilesY != fxMesa->hsrTilesY) {
         GLfloat *newTiles = (GLfloat *) CALLOC(tilesX * tilesY * sizeof(GLfloat));
         if (newTiles) {
            GLuint i;
            FREE(fxMesa->hsrTileDepth);
            fxMesa->hsrTileDepth = newTiles;
            fxMesa->hsrTilesX = tilesX;
            fxMesa->hsrTilesY = tilesY;
            for (i = 0; i < tilesX * tilesY; i++) {
               fxMesa->hsrTileDepth[i] = 1.0f;
            }
         } else {
            FREE(fxMesa->hsrTileDepth);
            fxMesa->hsrTileDepth = NULL;
            fxMesa->hsrEnabled = GL_FALSE;
         }
      }
   }

   /* texture memory content did not survive: re-download the textures
    * that are currently bound and mark everything else as swapped out
    * so it gets re-uploaded on demand */
   fxTMRestoreTextures_NoLock(fxMesa);

   /* reset the combine guards so the texture units get re-programmed */
   fxMesa->lastCombineTex[0] = NULL;
   fxMesa->lastCombineTex[1] = NULL;
   fxMesa->lastUnitsMode = FX_UM_NONE;
   fxMesa->tmuSrc = FX_TMU_NONE;

   /* re-activate the Mesa context and force a full state re-validation */
   if (fxMesa->glCtx) {
      _mesa_make_current(fxMesa->glCtx, fxMesa->glBuffer, fxMesa->glBuffer);
      fxSetupDDPointers(fxMesa->glCtx);
   }
   fxMesa->new_state = _NEW_ALL;
   if (!fxMesa->haveHwStencil) {
      fxMesa->new_state &= ~FX_NEW_STENCIL;
   }
   fxMesa->new_gl_state = ~0;

   return GL_TRUE;
}

/*
 * Destroy the given FX/Mesa context.
 */
void GLAPIENTRY
fxMesaDestroyContext(fxMesaContext fxMesa)
{
   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxMesaDestroyContext(...)\n");
   }

   if (!fxMesa)
      return;

   if (fxMesa->verbose) {
      fprintf(stderr, "Misc Stats:\n");
      fprintf(stderr, "  # swap buffer: %u\n", fxMesa->stats.swapBuffer);

      if (!fxMesa->stats.swapBuffer)
	 fxMesa->stats.swapBuffer = 1;

      fprintf(stderr, "Textures Stats:\n");
      fprintf(stderr, "  Free texture memory on TMU0: %d\n",
	      fxMesa->freeTexMem[FX_TMU0]);
      if (fxMesa->haveTwoTMUs)
	 fprintf(stderr, "  Free texture memory on TMU1: %d\n",
		 fxMesa->freeTexMem[FX_TMU1]);
      fprintf(stderr, "  # request to TMM to upload a texture objects: %u\n",
	      fxMesa->stats.reqTexUpload);
      fprintf(stderr,
	      "  # request to TMM to upload a texture objects per swapbuffer: %.2f\n",
	      fxMesa->stats.reqTexUpload / (float) fxMesa->stats.swapBuffer);
      fprintf(stderr, "  # texture objects uploaded: %u\n",
	      fxMesa->stats.texUpload);
      fprintf(stderr, "  # texture objects uploaded per swapbuffer: %.2f\n",
	      fxMesa->stats.texUpload / (float) fxMesa->stats.swapBuffer);
      fprintf(stderr, "  # MBs uploaded to texture memory: %.2f\n",
	      fxMesa->stats.memTexUpload / (float) (1 << 20));
      fprintf(stderr,
	      "  # MBs uploaded to texture memory per swapbuffer: %.2f\n",
	      (fxMesa->stats.memTexUpload /
	       (float) fxMesa->stats.swapBuffer) / (float) (1 << 20));

      /* HSR Statistics */
      if (fxMesa->hsrEnabled && fxMesa->stats.hsrTrianglesTotal > 0) {
         fprintf(stderr, "HSR Stats:\n");
         fprintf(stderr, "  # triangles submitted: %u\n", fxMesa->stats.hsrTrianglesTotal);
         fprintf(stderr, "  # triangles rejected by HSR: %u (%.1f%%)\n",
                 fxMesa->stats.hsrTrianglesRejected,
                 100.0f * fxMesa->stats.hsrTrianglesRejected / (float)fxMesa->stats.hsrTrianglesTotal);
         fprintf(stderr, "  # tile tests: %u\n", fxMesa->stats.hsrTileTests);
         fprintf(stderr, "  # tile updates: %u\n", fxMesa->stats.hsrTileUpdates);
      }
   }

   glbTotNumCtx--;

   if (!glbTotNumCtx && getenv("MESA_FX_INFO")) {
      GrSstPerfStats_t st;

      FX_grSstPerfStats(&st);

      fprintf(stderr, "Pixels Stats:\n");
      fprintf(stderr, "  # pixels processed (minus buffer clears): %u\n",
              (unsigned) st.pixelsIn);
      fprintf(stderr, "  # pixels not drawn due to chroma key test failure: %u\n",
              (unsigned) st.chromaFail);
      fprintf(stderr, "  # pixels not drawn due to depth test failure: %u\n",
              (unsigned) st.zFuncFail);
      fprintf(stderr,
              "  # pixels not drawn due to alpha test failure: %u\n",
              (unsigned) st.aFuncFail);
      fprintf(stderr, "  # pixels drawn (including buffer clears and LFB writes): %u\n",
              (unsigned) st.pixelsOut);
   }

   /* close the hardware first,
    * so we can debug atexit problems (memory leaks, etc).
    */
   grSstWinClose(fxMesa->glideContext);
   fxCloseHardware();

   fxDDDestroyFxMesaContext(fxMesa); /* must be before _mesa_destroy_context */
   _mesa_destroy_visual(fxMesa->glVis);
   _mesa_destroy_context(fxMesa->glCtx);
   _mesa_unreference_framebuffer(&fxMesa->glBuffer);
   fxTMClose(fxMesa); /* must be after _mesa_destroy_context */

   /* Free HSR tile cache */
   if (fxMesa->hsrTileDepth) {
      FREE(fxMesa->hsrTileDepth);
      fxMesa->hsrTileDepth = NULL;
   }

   FREE(fxMesa);

   if (fxMesa == fxMesaCurrentCtx)
      fxMesaCurrentCtx = NULL;
}


/*
 * Make the specified FX/Mesa context the current one.
 */
void GLAPIENTRY
fxMesaMakeCurrent(fxMesaContext fxMesa)
{
   if (!fxMesa) {
      _mesa_make_current(NULL, NULL, NULL);
      fxMesaCurrentCtx = NULL;

      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "fxMesaMakeCurrent(NULL)\n");
      }

      return;
   }

   /* if this context is already the current one, we can return early */
   if (fxMesaCurrentCtx == fxMesa
       && fxMesaCurrentCtx->glCtx == _mesa_get_current_context()) {
      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "fxMesaMakeCurrent(NOP)\n");
      }

      return;
   }

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxMesaMakeCurrent(...)\n");
   }

   if (fxMesaCurrentCtx)
      grGlideGetState((GrState *) fxMesaCurrentCtx->state);

   fxMesaCurrentCtx = fxMesa;

   grSstSelect(fxMesa->board);
   grGlideSetState((GrState *) fxMesa->state);

   _mesa_make_current(fxMesa->glCtx, fxMesa->glBuffer, fxMesa->glBuffer);

   fxSetupDDPointers(fxMesa->glCtx);

    /* Nejc - The first time we call MakeCurrent we set the initial viewport size */
   if (fxMesa->glCtx->Viewport.Width == 0)
   {
      _mesa_set_viewport(fxMesa->glCtx, 0, 0, fxMesa->width, fxMesa->height);
   }
}


/*
 * Swap front/back buffers for current context if double buffered.
 */
void GLAPIENTRY
fxMesaSwapBuffers(void)
{
   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxMesaSwapBuffers()\n");
   }

   if (fxMesaCurrentCtx) {
      /* Nejc: glide window closed (failed alt-tab restore) - do nothing */
      if (fxMesaCurrentCtx->glideContext == 0)
         return;

      _mesa_notifySwapBuffers(fxMesaCurrentCtx->glCtx);

      if (fxMesaCurrentCtx->haveDoubleBuffer) {

	 grBufferSwap(fxMesaCurrentCtx->swapInterval);

#if 0
	 /*
	  * Don't allow swap buffer commands to build up!
	  */
	 while (FX_grGetInteger(GR_PENDING_BUFFERSWAPS) >
		fxMesaCurrentCtx->maxPendingSwapBuffers)
	    /* The driver is able to sleep when waiting for the completation
	       of multiple swapbuffer operations instead of wasting
	       CPU time (NOTE: you must uncomment the following line in the
	       in order to enable this option) */
	    /* usleep(10000); */
	    ;
#endif

	 fxMesaCurrentCtx->stats.swapBuffer++;
      }
      /* Increment frame counter */
      fxMesaCurrentCtx->frame_no++;

      /* Reset combine guard here too */
      fxMesaCurrentCtx->lastCombineTex[0] = NULL;
      fxMesaCurrentCtx->lastCombineTex[1] = NULL;
      fxMesaCurrentCtx->lastUnitsMode = FX_UM_NONE;

      /* Clear HSR tile cache for new frame */
      if (fxMesaCurrentCtx->hsrEnabled && fxMesaCurrentCtx->hsrTileDepth) {
         fxHSRClear(fxMesaCurrentCtx);
      }
   }
}


/*
 * Shutdown Glide library
 */
void GLAPIENTRY
fxCloseHardware(void)
{
   if (glbGlideInitialized) {
      if (glbTotNumCtx == 0) {
    grDisable(GR_OPENGL_MODE_EXT); /* Added for openGl */
	 grGlideShutdown();
	 glbGlideInitialized = 0;
      }
   }
}


#else


/*
 * Need this to provide at least one external definition.
 */
extern int gl_fx_dummy_function_api(void);
int
gl_fx_dummy_function_api(void)
{
   return 0;
}

#endif /* FX */
