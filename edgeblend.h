/* 
 * File:   edgeblend.h
 * Author: Markus Knofe, Alexander Treptow
 *
 * Created on May 05, 2010, 2:43 PM
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _EDGEBLEND_H
#define	_EDGEBLEND_H

#include <math.h>

#include <compiz-core.h>
#include <compiz-mousepoll.h>
#include <X11/Xatom.h>
#include <GL/glu.h>

#include "edgeblend_options.h"
#include "output_config.h"

typedef struct _edgeblendDisplay
{
    Atom                edgeblendAtom;
    HandleEventProc     handleEvent;
    int                 screenPrivateIndex;
} edgeblendDisplay;

typedef struct _edgeblendWorkareaFix {
    unsigned int    nOutputs;
    BOX **          outputExtends; // pointer to box pointers...
    int             orginalWidth;
    int             orginalHeight;
} EdgeblendWorkareaFix;

typedef struct _edgeblendScreen
{
    /* WRAP-Procs */
    PreparePaintScreenProc      preparePaintScreen;
    DonePaintScreenProc         donePaintScreen;
    PaintOutputProc             paintOutput;
    PaintTransformedOutputProc  paintTransformedOutput;
    PaintScreenProc             paintScreen;
    PaintWindowProc             paintWindow;

    /* outputconfig */
    EdgeblendOutputConfig *     outputCfg;

    /* workarea */
    EdgeblendWorkareaFix        orginalWorkarea;

    /* fullscreenoutput */
    Bool    hadOverlappingOutputs;
    Bool    wasForcedIndependetOutput;

    /* cursor */
    Bool                    cursorHidden;
    Bool                    canHideCursor;
    MousePollFunc           *mpFunc;
    PositionPollingHandle   pollHandle;
    time_t                  lastChange;
    int                     mouseX;
    int                     mouseY;
    int                     cursorHotX;
    int                     cursorHotY;
    int                     cursorWidth;
    int                     cursorHeight;
    GLuint                  cursorTexture;

    /* fixes */
    Bool    fixesSupported;
    int     fixesEventBase;
    int     fixesErrorBase;

    /* blending textures */
    GLuint* textures;
    GLuint testtex;
} edgeblendScreen;



#define GET_EDGEBLEND_DISPLAY(d)                                  \
    ((edgeblendDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define EDGEBLEND_DISPLAY(d)                      \
    edgeblendDisplay *ebd = GET_EDGEBLEND_DISPLAY (d)

#define GET_EDGEBLEND_SCREEN(s, ebd)                                  \
    ((edgeblendScreen *) (s)->base.privates[(ebd)->screenPrivateIndex].ptr)

#define EDGEBLEND_SCREEN(s)                                                      \
    edgeblendScreen *ebs = GET_EDGEBLEND_SCREEN (s, GET_EDGEBLEND_DISPLAY (s->display))


#ifdef	__cplusplus
extern "C" {
#endif

    

#ifdef	__cplusplus
}
#endif

#endif	/* _EDGEBLEND_H */

