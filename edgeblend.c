/*
 * Compiz edgeblending plugin
 *
 * edgeblend.c
 *
 * Copyright : (C) 2009 by Alexander Treptow, Markus Knofe
 * E-mail    : 
 *
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
 *
 */

#include <math.h>

#include <compiz-core.h>
#include <X11/Xatom.h>
#include <GL/glu.h>
#include "edgeblend_options.h"
#include "blend.h"
#include "edge.h"


#define EDGEBLEND_BACKGROUND_DEFAULT ""
#define EDGEBLEND_LOGO_DEFAULT ""

#define GET_EDGEBLEND_DISPLAY(d)                                  \
    ((edgeblendDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define EDGEBLEND_DISPLAY(d)                      \
    edgeblendDisplay *ebd = GET_EDGEBLEND_DISPLAY (d)

#define GET_EDGEBLEND_SCREEN(s, ebd)                                  \
    ((edgeblendScreen *) (s)->base.privates[(ebd)->screenPrivateIndex].ptr)

#define EDGEBLEND_SCREEN(s)                                                      \
    edgeblendScreen *ebs = GET_EDGEBLEND_SCREEN (s, GET_EDGEBLEND_DISPLAY (s->display))


static int displayPrivateIndex = 0;

typedef struct _edgeblendDisplay
{
    Atom edgeblendAtom;
    int screenPrivateIndex;
}
edgeblendDisplay;

typedef struct _edgeblendScreen
{
    PreparePaintScreenProc preparePaintScreen;
    DonePaintScreenProc    donePaintScreen;
    PaintOutputProc        paintOutput;
}
edgeblendScreen;



/**
 * Enables per screen operations on the context,
 * right before drawing
 *
 */
static void
edgeblendPreparePaintScreen (CompScreen *s,
			  int        ms)
{
    EDGEBLEND_SCREEN (s);
    CompDisplay *d = s->display;

    if (edgeblendGetAutoReload(d)) {
        compLogMessage ("edgeblend", CompLogLevelInfo, "auto reload set");
    }


    UNWRAP (ebs, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (ebs, s, preparePaintScreen, edgeblendPreparePaintScreen);

}

static void
edgeblendGetCurrentOutputRect (CompScreen *s,
			    XRectangle *outputRect)
{
    int root_x = 0, root_y = 0;
    int ignore_i;
    unsigned int ignore_ui;
    int output;
    Window ignore_w;

    if (s->nOutputDev == 1)
	output = 0;
    else
    {
	XQueryPointer (s->display->display, s->root, &ignore_w, &ignore_w,
		       &root_x, &root_y, &ignore_i, &ignore_i, &ignore_ui);
	output = outputDeviceForPoint (s, root_x, root_y);
    }

    outputRect->x      = s->outputDev[output].region.extents.x1;
    outputRect->y      = s->outputDev[output].region.extents.y1;
    outputRect->width  = s->outputDev[output].region.extents.x2 -
			 s->outputDev[output].region.extents.x1;
    outputRect->height = s->outputDev[output].region.extents.y2 -
			 s->outputDev[output].region.extents.y1;

}

static Bool
edgeblendPaintOutput (CompScreen              *s,
		   const ScreenPaintAttrib *sa,
		   const CompTransform     *transform,
		   Region                  region,
		   CompOutput              *output,
		   unsigned int            mask)
{
    
    EDGEBLEND_SCREEN (s);
    CompDisplay *d = s->display;
    CompTransform sTransform = *transform;

    Bool status = TRUE;
    float x,y;
    float alpha = 0.5;

    UNWRAP (ebs, s, paintOutput);
    status = (*s->paintOutput) (s, sa, transform, region, output, mask);
    WRAP (ebs, s, paintOutput, edgeblendPaintOutput);

    transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

    glPushMatrix ();       
        glLoadMatrixf (sTransform.m);
        glPushAttrib(GL_COLOR_BUFFER_BIT);
            glEnable (GL_BLEND);
            glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f (0.5, 0.5, 0.5, alpha);

            x = (s->width  - 100) / 2;
            y = (s->height - 100) / 2;

            glBegin(GL_QUADS);
                glVertex2f(x,       y);
                glVertex2f(x+100.0, y);
                glVertex2f(x,       y+100.0);
                glVertex2f(x+100.0, y+100.0);
            glEnd();

        glPopAttrib();
        glColor4usv (defaultColor);
    glPopMatrix ();
    return status;
}

/**
 * Run after drawing the screen, enabled forcing redraws by damaging the screen
 */
static void
edgeblendDonePaintScreen (CompScreen * s)
{
    EDGEBLEND_SCREEN (s);

    if(1) damageScreen (s);

    UNWRAP (ebs, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ebs, s, donePaintScreen, edgeblendDonePaintScreen);
}




static Bool
edgeblendLoadConfig (CompDisplay     *d,
                     CompAction      *ac,
                     CompActionState state,
                     CompOption      *option,
                     int             nOption)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendLoadConfig called");
    CompScreen *s;

    s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption, "root", 0));

    if (s)
    {
	EDGEBLEND_SCREEN (s);
	damageScreen (s);
    }

    return FALSE;
}


/******************************************************************************/
/*                                                                            */
/*              Init/Fini - Functions                                         */
/*                                                                            */
/******************************************************************************/
static Bool
edgeblendInitScreen (CompPlugin *p,
		  CompScreen *s)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, 
                    "edgeblendInitScreen called on ScreenNum %d (x: %d y: %d, height: %d, width: %d)",
                    s->screenNum, s->x, s->y, s->height, s->width
                   );
    EDGEBLEND_DISPLAY (s->display);

    edgeblendScreen *ebs = (edgeblendScreen *) calloc (1, sizeof (edgeblendScreen) );

    if (!ebs) {
        return FALSE;
    }

    s->base.privates[ebd->screenPrivateIndex].ptr = ebs;

    //wrap functions
    WRAP (ebs, s, paintOutput,        edgeblendPaintOutput);
    WRAP (ebs, s, preparePaintScreen, edgeblendPreparePaintScreen);
    WRAP (ebs, s, donePaintScreen,    edgeblendDonePaintScreen);

    return TRUE;
}


static void
edgeblendFiniScreen (CompPlugin *p,
		  CompScreen *s)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendFiniScreen called");
    EDGEBLEND_SCREEN (s);

    //Restore the original functions
    UNWRAP (ebs, s, paintOutput);
    UNWRAP (ebs, s, preparePaintScreen);
    UNWRAP (ebs, s, donePaintScreen);

    //Free the pointer
    free (ebs);
}


static Bool
edgeblendInitDisplay (CompPlugin  *p,
                      CompDisplay *d)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendInitDisplay called on %s",
                    d->displayString);
    edgeblendDisplay *ebd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    /* Generate a edgeblend display struct */
    ebd = (edgeblendDisplay *) malloc (sizeof (edgeblendDisplay) );

    if (!ebd)
	return FALSE;

    /* Allocate a private index */
    ebd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    
    /* Check if its valid */
    if (ebd->screenPrivateIndex < 0)
    {
	/* It's invalid so free memory and return */
	free (ebd);
	return FALSE;
    }

    ebd->edgeblendAtom = XInternAtom (d->display, "_COMPIZ_WM_edgeblend", 0);

    /* add reload hook */
    edgeblendSetReloadInitiate (d, edgeblendLoadConfig);

    d->base.privates[displayPrivateIndex].ptr = ebd;
    return TRUE;
}

static void
edgeblendFiniDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendFiniDisplay called");
    EDGEBLEND_DISPLAY (d);

    /** remove hook */
    edgeblendSetReloadTerminate (d, edgeblendLoadConfig);

    /* Free the private index */
    freeScreenPrivateIndex (d, ebd->screenPrivateIndex);

    /* Free the pointer */
    free (ebd);
}

/**
 * generates an private index for the current display
 */
static Bool
edgeblendInit (CompPlugin *p)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendInit called");
    displayPrivateIndex = allocateDisplayPrivateIndex ();

    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

/**
 * frees private index on display
 */
static void
edgeblendFini (CompPlugin *p)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendFini called");
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}


/** COMPIZ-Plugin-Structure **/
static CompBool
edgeblendInitObject (CompPlugin *p,
		  CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) edgeblendInitDisplay,
	(InitPluginObjectProc) edgeblendInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
edgeblendFiniObject (CompPlugin *p,
		  CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) edgeblendFiniDisplay,
	(FiniPluginObjectProc) edgeblendFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable edgeblendVTable = {
    "edgeblend",
    0,
    edgeblendInit,
    edgeblendFini,
    edgeblendInitObject,
    edgeblendFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &edgeblendVTable;
}
