# MesaFX-7.4.4
Mesa3d for 3dfx

## Tools needed to build:
(what I used)
- Win10
- MinGw v6.3.0 (4.7 wont work)

## Build instructions
make -f Makefile.mgw clean \
make -f Makefile.mgw realclean <sub><sup>(also unlinks everything)</sup></sub>
<br/><br/>
make -f Makefile.mgw FX=1 <sub><sup>(build with C-dispatch table)</sup></sub> \
make -f Makefile.mgw X86=1 FX=1 <sub><sup>(build with ASM-dispatch table)</sup></sub>

## Additional versions information
make - GNU Make 3.81 \
gcc - MinGW.org GCC-6.3.0-1

## For debugging
In fxdrv.h set:
```
FX_DEBUG = 1
TDFX_DEBUG = 1
```

In fxapi.c you have additional choices which debug prints will be outputed:
```
int TDFX_DEBUG = (0 \
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
```
Then on the system running it, set an environment variable in the terminal:
```
set MESA_DEBUG = 1 (for additional core mesa informations, if any errors)
set MESA_FX_INFO = r (for MesaFx debug output)
```
And run the game from this terminal.

Logfile "Mesa.log" will be created in the folder where the game was ran.

## Readme.3dfx
Legacy Readme.3dfx, in the project, with more informations.
