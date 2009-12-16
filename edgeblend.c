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
#include "debug.h"
#include "blend.h"
#include "edge.h"
#include "parser.h"

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
    /* WRAP-Procs */
    PreparePaintScreenProc      preparePaintScreen;
    DonePaintScreenProc         donePaintScreen;
    PaintOutputProc             paintOutput;
    PaintTransformedOutputProc  paintTransformedOutput;
    PaintScreenProc             paintScreen;
}
edgeblendScreen;

typedef struct _backBuffer {
    int x;
    int y;
    int h;
    int w;
    GLubyte * buffer;
} backBuffer;

backBuffer *myBackBuffer;


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


static void
edgeblendPaintScreen (CompScreen   *s,
	     CompOutput   *outputs,
	     int          numOutputs,
	     unsigned int mask)
{
    /*compLogMessage ("edgeblend", CompLogLevelInfo,"painting screen with mask: %d and #Outputs %d",mask,numOutputs);*/
    EDGEBLEND_SCREEN (s);

    UNWRAP (ebs, s, paintScreen);
	(*s->paintScreen) (s, outputs, numOutputs, mask);
    WRAP (ebs, s, paintScreen, edgeblendPaintScreen);
}
//
static void
edgeblendPaintTransformedOutput (CompScreen              *s,
		   const ScreenPaintAttrib *sa,
		   const CompTransform     *transform,
		   Region                  region,
		   CompOutput              *output,
		   unsigned int            mask)
{
    EDGEBLEND_SCREEN (s);
    //compLogMessage ("edgeblend", CompLogLevelInfo,"painting transformedoutput %s (%d) with mask %d", output->name, output->id,mask);
    printOutputdev(output);
    UNWRAP (ebs, s, paintTransformedOutput);
        (*s->paintTransformedOutput) (s, sa, transform, region, output, mask);
    WRAP (ebs, s, paintOutput, paintTransformedOutput);
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

    //compLogMessage ("edgeblend", CompLogLevelInfo,"painting output %s (%d) with mask %d", output->name, output->id,mask);
    //printOutputdev(output);
    Bool status = TRUE;
    float x,y;
    float alpha = 0.5;
    int i = 0;
    GLubyte * buffer;

    unsigned int mymask = PAINT_SCREEN_REGION_MASK | PAINT_SCREEN_TRANSFORMED_MASK | PAINT_SCREEN_FULL_MASK;
    makeScreenCurrent (s);
    UNWRAP (ebs, s, paintOutput);
    status = (*s->paintOutput) (s, sa, transform, region, output, mask);
    WRAP (ebs, s, paintOutput, edgeblendPaintOutput);

    if (output->id != ~0) {
    //compLogMessage ("edgeblend", CompLogLevelInfo,"painting screen %d on output %s (%d)", s->screenNum,output->name, output->id);
    //compLogMessage ("edgeblend", CompLogLevelInfo,"\t %d/%d %d/%d rect ", region->extents.x1, region->extents.y1, region->extents.x2, region->extents.y2);
    printOutputdev(output);
    
    
        transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);
        glLoadMatrixf (sTransform.m);

        if (myBackBuffer->buffer) {
            compLogMessage ("edgeblend", CompLogLevelInfo,"try writing buffers back");
            glRasterPos2i(myBackBuffer->x + 10, myBackBuffer->y+ 10);
            glDrawPixels(myBackBuffer->w-10, myBackBuffer->h -10, GL_RGBA, GL_UNSIGNED_BYTE, myBackBuffer->buffer);
            free (myBackBuffer->buffer);
            myBackBuffer->buffer = NULL;
            compLogMessage ("edgeblend", CompLogLevelInfo,"writing buffers back");
        }

            compLogMessage ("edgeblend", CompLogLevelInfo,"try read buffers");
            buffer = (GLubyte *)malloc (sizeof (GLubyte) * output->width * output->height * 4);
            glReadPixels (output->region.extents.x1, output->region.extents.y1, output->width, output->height,
                  GL_RGBA, GL_UNSIGNED_BYTE,
                  (GLvoid *) buffer);
            myBackBuffer->x = output->region.extents.x1;
            myBackBuffer->y = output->region.extents.y1;
            myBackBuffer->w = output->width;
            myBackBuffer->h = output->height;
            myBackBuffer->buffer = buffer;
            compLogMessage ("edgeblend", CompLogLevelInfo,"read buffers");
        glPushMatrix ();
        glPushAttrib(GL_COLOR_BUFFER_BIT);
            glEnable (GL_BLEND);
            glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            if (output->id == 0) {
                glColor4f (0.5, 0.5, 0.5, 0.75);
            } else {
                glColor4f (0.0, 0.5, 0.5, 0.75);
            }

           
                   x = output->region.extents.x1;
                   y = output->region.extents.y1;

                   //compLogMessage ("edgeblend", CompLogLevelInfo,"painting screen %d output: %d -> x: %f y: %f",s->screenNum,s->outputDev[i].id,x,y);

                    glBegin(GL_QUADS);
                        glVertex2f(x,       y);
                        glVertex2f(x+100.0, y);
                        glVertex2f(x,       y+100.0);
                        glVertex2f(x+100.0, y+100.0);
                    glEnd();


            glDisable(GL_BLEND);
        glPopAttrib();
        glColor4usv (defaultColor);
    glPopMatrix ();
    }
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

/** OPTIONS **/
void edgeblendNotifyCallback(CompDisplay *display, CompOption *opt, EdgeblendDisplayOptions num)
{
    switch (num) {
        case EdgeblendDisplayOptionConfig:
            compLogMessage ("edgeblend", CompLogLevelInfo,"edgeblendNotifyCallback called on option config");
            compLogMessage ("edgeblend", CompLogLevelInfo," %d",load_config(opt->value.s));
            break;
        default:
            compLogMessage ("edgeblend", CompLogLevelInfo,"edgeblendNotifyCallback called on option %d", num);
            break;
    }
}


/******************************************************************************/
/*                                                                            */
/*              Init/Fini - Functions                                         */
/*                                                                            */
/******************************************************************************/
static Bool
edgeblendInitScreen (CompPlugin *plugin,
		     CompScreen *screen)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, 
                    "edgeblendInitScreen called on ScreenNum %d (x: %d y: %d, height: %d, width: %d)",
                    screen->screenNum, screen->x, screen->y, screen->height, screen->width
                   );
    compLogMessage ("edgeblend", CompLogLevelInfo,
                    "edgeblendInitScreen called on ScreenNum %d, is has %d outputdevices (current %d)",
                    screen->screenNum, screen->nOutputDev, screen->currentOutputDev
                   );
    int i = 0;
    while (i < screen->nOutputDev) {
        compLogMessage ("edgeblend", CompLogLevelInfo,
                    "edgeblendInitScreen called on ScreenNum %d, outputdevice %s (%d) box %d/%d %d/%d",
                    screen->screenNum, screen->outputDev[i].name, screen->outputDev[i].id,
                    screen->outputDev[i].region.extents.x1, screen->outputDev[i].region.extents.y1,
                    screen->outputDev[i].region.extents.x2, screen->outputDev[i].region.extents.y2
                   );
        i++;
    }

    EDGEBLEND_DISPLAY (screen->display);

    edgeblendScreen *ebs = (edgeblendScreen *) calloc (1, sizeof (edgeblendScreen) );

    if (!ebs) {
        return FALSE;
    }

    CompOptionValue option;
    option.b = TRUE;
    setScreenOption(plugin, screen, screen->opt[COMP_SCREEN_OPTION_FORCE_INDEPENDENT].name , &option);
    screen->base.privates[ebd->screenPrivateIndex].ptr = ebs;
    compLogMessage ("edgeblend", CompLogLevelInfo, "force ooutputdev drawing");
    //wrap functions
    WRAP (ebs, screen, paintScreen,        edgeblendPaintScreen);
    WRAP (ebs, screen, paintOutput,        edgeblendPaintOutput);
    WRAP (ebs, screen, paintTransformedOutput,        edgeblendPaintTransformedOutput);
    WRAP (ebs, screen, preparePaintScreen, edgeblendPreparePaintScreen);
    WRAP (ebs, screen, donePaintScreen,    edgeblendDonePaintScreen);

    return TRUE;
}


static void
edgeblendFiniScreen (CompPlugin *p,
		  CompScreen *s)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendFiniScreen called");
    EDGEBLEND_SCREEN (s);

    //Restore the original functions
    UNWRAP (ebs, s, paintScreen);
    UNWRAP (ebs, s, paintOutput);
    UNWRAP (ebs, s, paintTransformedOutput);
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
    {/* It's invalid so free memory and return */
	free (ebd);
	return FALSE;
    }

    /* BCOP - Notifies */
    edgeblendSetConfigNotify    (d, &edgeblendNotifyCallback);
    edgeblendSetReloadNotify    (d, &edgeblendNotifyCallback);
    edgeblendSetAutoReloadNotify(d, &edgeblendNotifyCallback);
    edgeblendSetShowAreasNotify (d, &edgeblendNotifyCallback);

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
    
    myBackBuffer = (backBuffer*) malloc (sizeof(backBuffer));
    myBackBuffer->x = 0;
    myBackBuffer->y = 0;
    myBackBuffer->h = 0;
    myBackBuffer->w = 0;
    myBackBuffer->buffer = NULL;

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
    if (myBackBuffer) {
        if (myBackBuffer->buffer) {
            free (myBackBuffer->buffer);
        }
        free(myBackBuffer);
    }
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
