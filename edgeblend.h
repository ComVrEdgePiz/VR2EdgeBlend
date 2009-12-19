/* 
 * File:   edgeblend.h
 * Author: flatline
 *
 * Created on December 19, 2009, 2:43 PM
 */

#ifndef _EDGEBLEND_H
#define	_EDGEBLEND_H

#include <math.h>

#include <compiz-core.h>
#include <compiz-mousepoll.h>
#include <X11/Xatom.h>
#include <GL/glu.h>
#include "edgeblend_options.h"
#include "debug.h"


typedef struct _edgeblendDisplay
{
    Atom                edgeblendAtom;
    HandleEventProc     handleEvent;
    int                 screenPrivateIndex;
} edgeblendDisplay;

typedef struct _edgeblendScreen
{
    /* WRAP-Procs */
    PreparePaintScreenProc      preparePaintScreen;
    DonePaintScreenProc         donePaintScreen;
    PaintOutputProc             paintOutput;
    PaintTransformedOutputProc  paintTransformedOutput;
    PaintScreenProc             paintScreen;
    PaintWindowProc             paintWindow;

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



    /* fixes */
    Bool    fixesSupported;
    int     fixesEventBase;
    int     fixesErrorBase;

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

    static int displayPrivateIndex = 0;

#ifdef	__cplusplus
}
#endif

#endif	/* _EDGEBLEND_H */

