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
#include "edgeblend.h"

#include "parser.h"

#include "fix_env.h"


/**
 * Enables per screen operations on the context, right before drawing,
 *
 * @param CompScreen *screen - Compiz Screen
 * @param int        ms      - msec since last draw
 */
static void
edgeblendPreparePaintScreen (CompScreen *screen, int ms)
{
    EDGEBLEND_SCREEN (screen);
    CompDisplay *d = screen->display; 

    UNWRAP (ebs, screen, preparePaintScreen);
        (*screen->preparePaintScreen) (screen, ms);
    WRAP (ebs, screen, preparePaintScreen, edgeblendPreparePaintScreen);
}

/**
 * Called enables influence the painting of the howl screen
 *
 * @param CompScreen    *screen     - Compiz Screen
 * @param CompOutput    *outputs    - Compiz Output[]
 * @param int           numOutputs  - #outputs
 * @param unsigned int  mask        - draw-flags
 */
static void
edgeblendPaintScreen (CompScreen *screen, CompOutput *outputs,
                      int numOutputs, unsigned int mask)
{
    EDGEBLEND_SCREEN (screen);

    UNWRAP (ebs, screen, paintScreen);
	(*screen->paintScreen) (screen, outputs, numOutputs, mask);
    WRAP (ebs, screen, paintScreen, edgeblendPaintScreen);
}



/**
 * Draws a rect....
 * @TODO Draw the right Cursor
 * @SEE ezoom
 *
 * @TODO better done with ./src/paint.c:415:	(*screen->paintCursor) (c, transform, tmpRegion, 0);
 *
 * @param int x - x-Pos
 * @param int y - y-Pos
 */
void drawMouse(int x, int y) {
    glColor4f (1.0, 0.0, 0.0, 0.85);
    glBegin(GL_QUADS);
        glVertex2i(x, y);
        glVertex2i(x+10, y);
        glVertex2i(x, y+10);
        glVertex2i(x+10, y+10);
    glEnd();
}

/**
 * Draws the actual output
 *
 * @param CompScreen            *screen                 - Compiz Screen
 * @param ScreenPaintAttrib     *screenAttrib (const)   - Screen Attributs
 * @param Region                region (const)          - Region to draw
 * @param CompOutput            *output                 - Compiz Output
 * @param unsigned int          mask                    - draw-flags
 * @return Bool
 */
static Bool
edgeblendPaintOutput (CompScreen              *screen,
		   const ScreenPaintAttrib *sa,
		   const CompTransform     *transform,
		   Region                  region,
		   CompOutput              *output,
		   unsigned int            mask)
{
    EDGEBLEND_SCREEN (screen);
    
    CompDisplay *d = screen->display;

    CompTransform sTransform = *transform;

    Bool status = TRUE;

    float x,y;
    int i;

    /* ensure right textures */
    makeScreenCurrent (screen);

    /* lets draw everybody to the framebuffer */
    UNWRAP (ebs, screen, paintOutput);
        status = (*screen->paintOutput) (screen, sa, transform, region, output, mask);
    WRAP (ebs, screen, paintOutput, edgeblendPaintOutput);
    /* and here we go ;) */

    /* transform to right viewport */
    transformToScreenSpace (screen, output, -DEFAULT_Z_CAMERA, &sTransform);
    glLoadMatrixf (sTransform.m);

    /* Save current RasterPosition */
    glPushAttrib(GL_CURRENT_BIT);

    /* 1. Move towards the lower right output of out output-grid and draw then
     * move from bottom to to and from right to left
     * @TODO - do like above
     */
        //move to 2. output (assuming leftof, 2 * 1280x1024)
        //copy overlapping area to the outputdevice
        glRasterPos2i(1280, screen->height);
        glCopyPixels(1080, 0, 1280, screen->height, GL_COLOR);


        /* Save every thing*/
        glPushMatrix ();
            glPushAttrib(GL_COLOR_BUFFER_BIT);
                //we want blend everything at the moment since its cool...
                glEnable (GL_BLEND);
                glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                /* 2. We have to draw our replacment Cursor, since the cursor X
                 * provided is out-of-sync when copy buffer arround.
                 */
                //fetch current position of the cursor from mousepoll-plugin
                if (fix_CursorPoll(screen, ebs)) {
                    x = ebs->mouseX;
                    y = ebs->mouseY;
               /* @TODO  needed clipping when at the edge of outputdevices*/
                    // != left upper output then draw cursor:
                    if (x > 1080) {
                        /* if in overlapping area draw two cursors */
                        if (x < 1280) {
                            drawMouse(x,y);
                            drawMouse(x + 200,y);
                        }
                        /* else draw cursor here he belongs */
                        else {
                            drawMouse(x + 200,y);
                        }
                    }
                    /* draw cursor on the current position */
                    else {
                        drawMouse(x,y);
                    }
                }
                
                /*
                 * 3. Loop over all outputdevices and assign blending, as
                 * configurated
                 * @TODO
                 */
                for (i = 0; i < screen->nOutputDev; i++) {
                    if (i == 0) {
                        glColor4f (0.5, 0.5, 0.5, 0.75);
                    } else {
                        glColor4f (0.0, 0.5, 0.5, 0.75);
                    }
                       x = screen->outputDev[i].region.extents.x1;
                       y = screen->outputDev[i].region.extents.y1;

                        glBegin(GL_QUADS);
                            glVertex2f(x, y);
                            glVertex2f(x+100.0, y);
                            glVertex2f(x, y+100.0);
                            glVertex2f(x+100.0, y+100.0);
                        glEnd();
                }
                /* restore opengl state */
                
                glDisable(GL_BLEND);
            glPopAttrib();
        glPopMatrix ();
    glPopAttrib();
    
    return status;
}

/**
 * Run after drawing the screen, enabled forcing redraws by damaging the screen
 * 
 * @param CompScreen    *screen - Compiz Screen
 */
static void
edgeblendDonePaintScreen (CompScreen * screen)
{
    EDGEBLEND_SCREEN (screen);

    //let's damage the screen since we have to redraw the howl screen....
    damageScreen (screen);

    UNWRAP (ebs, screen, donePaintScreen);
        (*screen->donePaintScreen) (screen);
    WRAP (ebs, screen, donePaintScreen, edgeblendDonePaintScreen);
}

/**
 * React on XEvents, used to keep the pointer inside the workarea
 *
 * @param CompDisplay   *display    - Compiz Display
 * @param XEvent        *event      - Xserver Event
 */
void
edgeblendHandleEvent (CompDisplay * display, XEvent * event)
{  
    EDGEBLEND_DISPLAY(display);
    CompScreen *screen = display->screens;

    //since we support only one screen we can assume the firstone in the
    //correct one
    //for (screen = display->screens; screen; screen = screen->next) {
        //see display.c:2701 void warpPointer (CompScreen *s, int dx,int dy);
        if (pointerX > screen->width || pointerY > screen->height)
            warpPointer(screen, 0,0);
    //}
    
    UNWRAP (ebd, display, handleEvent);
        (*display->handleEvent) (display, event);
    WRAP (ebd, display, handleEvent, edgeblendHandleEvent);
}

/******************************************************************************/
/* Manage Functions                                                           */
/******************************************************************************/
/**
 * Called wenn the "Reload"-shortcut is pressed
 *
 * @param CompDisplay       *display    - 
 * @param CompAction        *action     -
 * @param CompActionState   state       -
 * @param CompOption        *option     -
 * @param int               nOption     -
 * @return Bool
 */
static Bool
edgeblendLoadConfig (CompDisplay     *display,
                     CompAction      *action ,
                     CompActionState  state  ,
                     CompOption      *option ,
                     int              nOption)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendLoadConfig called");
    CompScreen *screen;

    screen = findScreenAtDisplay (display, getIntOptionNamed (option, nOption, "root", 0));

    if (screen)
    {
	EDGEBLEND_SCREEN (screen);
	damageScreen (screen);
    }

    return FALSE;
}

/**
 * Called when an option in the CCSM is changed
 *
 * @param CompDisplay               *display    - Compiz Display
 * @param CompOption                *option     - Compiz Option
 * @param EdgeblendDisplayOptions    num        - Option number see xml/bcop
 *
 */
void edgeblendNotifyCallback(CompDisplay *display, CompOption *option, EdgeblendDisplayOptions num)
{
    /* Which Option was cahnged? */
    switch (num) {
        case EdgeblendDisplayOptionConfig:
            //reload config
            compLogMessage ("edgeblend", CompLogLevelInfo,"edgeblendNotifyCallback called on option config");
            compLogMessage ("edgeblend", CompLogLevelInfo," %d",load_config(option->value.s));
            break;
        default:
            //else say something
            compLogMessage ("edgeblend", CompLogLevelInfo,"edgeblendNotifyCallback called on option %d", num);
            break;
    }
}


/******************************************************************************/
/* Init/Fini - Functions                                                      */
/******************************************************************************/
/**
 * Initialize a screen, since edgeblending is only supported on Bigscreen
 * enviroments, like Nvidia's TwinView, we only have one screen per display.
 *
 * @param CompPlugin    *plugin - Compiz Plugin
 * @param CompScreen    *screen - Compiz Screen
 * @return Bool
 */
static Bool
edgeblendInitScreen (CompPlugin *plugin, CompScreen *screen)
{
    //for version checking
    int major, minor;
    //for mousepolling plugin index
    int index;

    EDGEBLEND_DISPLAY (screen->display);
    //create screen private data
    edgeblendScreen *ebs = (edgeblendScreen *) calloc (1, sizeof (edgeblendScreen) );
    if (!ebs) {
        return FALSE;
    }

    /* @TODO ensure we have only one screen for the display else return FALSE;*/

    /* ensure we can disable the XCursor */
    ebs->fixesSupported = XFixesQueryExtension(screen->display->display, &ebs->fixesEventBase, &ebs->fixesErrorBase);
    if (!ebs->fixesSupported)
        return FALSE;

    /* ensure we can disable the XCursor */
    XFixesQueryVersion(screen->display->display, &major, &minor);
    ebs->canHideCursor = (major >= 4) ? TRUE : FALSE;
    if (!ebs->canHideCursor)
        return FALSE;
    
    ebs->cursorHidden = FALSE;
    
    if (!checkPluginABI ("mousepoll", MOUSEPOLL_ABIVERSION))
	return FALSE;
    
    if (!getPluginDisplayIndex (screen->display, "mousepoll", &index))
	return FALSE;

    /* grep mousepolling plugin function */
    ebs->mpFunc = screen->display->base.privates[index].ptr;

    ebs->orginalWorkarea.outputExtends = NULL;

    //switch ENV
    /* fix workarea */
    if (!fix_CompScreenWorkarea(screen, ebs,TRUE)) {
        return FALSE;
    }
    /* resize XDesktop */
    //fix_XDesktopSize(screen, 2160, screen->height);
    /* disable XCursor and enable mousepolling */
    fix_XCursor(screen, ebs, TRUE);
    /* ensure fullscreenoutput is used to render */
    fix_CompFullscreenOutput(plugin, screen, ebs, TRUE);
    


    /* store private data */
    screen->base.privates[ebd->screenPrivateIndex].ptr = ebs;
    
    //wrap functions
    //WRAP (ebs, screen, paintScreen,    edgeblendPaintScreen);
    //WRAP (ebs, screen, preparePaintScreen, edgeblendPreparePaintScreen);
    WRAP (ebs, screen, paintOutput,     edgeblendPaintOutput);      //we must hook into drawing...
    WRAP (ebs, screen, donePaintScreen, edgeblendDonePaintScreen);  //we must damage the screen every time
    //WRAP (ebs, screen, paintWindow,    edgeblendPaintWindow);

    //damage screen to init edgeblend
    damageScreen (screen);
    return TRUE;
}

/**
 * Finalizes a screen.
 *
 * @param CompPlugin    *plugin - Compiz Plugin
 * @param CompScreen    *screen - Compiz Screen
 */
static void
edgeblendFiniScreen (CompPlugin *plugin, CompScreen *screen)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendFiniScreen called");
    EDGEBLEND_SCREEN (screen);

    //Restore the original functions
    //UNWRAP (ebs, s, paintScreen);
    //UNWRAP (ebs, screen, preparePaintScreen);
    UNWRAP (ebs, screen, paintOutput);
    UNWRAP (ebs, screen, donePaintScreen);
    //UNWRAP (ebs, s, paintWindow);
    //UNWRAP (ebs, s, paintTransformedOutput);
    


    /* restore woarkarea */
    fix_CompScreenWorkarea(screen, ebs, FALSE);
    /* restore height and width of the X-Desktop */
  //  fix_XDesktopSize(screen, 2560, screen->height);
    /* restore XCursor handling and remove mosue polling */
    fix_XCursor(screen, ebs, FALSE);
    /* restore compiz fullscreen/outputdevice-based rendering */
    fix_CompFullscreenOutput(plugin, screen, ebs, FALSE);
    

    //clear edgeblend
    damageScreen (screen);
    //Free the pointer
    free (ebs);
}

/**
 * Initialize a Display
 *
 * @param CompPlugin    *plugin     - Compiz Plugin
 * @param CompDisplay   *display    - Compiz Display
 * @return Bool
 */
static Bool
edgeblendInitDisplay (CompPlugin  *plugin, CompDisplay *display)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendInitDisplay called on %s",
                    display->displayString);

    edgeblendDisplay *ebd;

    //check Application Binary Interface Version
    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    /* Generate a edgeblend display struct */
    ebd = (edgeblendDisplay *) malloc (sizeof (edgeblendDisplay) );
    if (!ebd)
	return FALSE;

    /* Allocate a private index */
    ebd->screenPrivateIndex = allocateScreenPrivateIndex (display);
    
    /* Check if its valid */
    if (ebd->screenPrivateIndex < 0)
    {/* It's invalid so free memory and return */
	free (ebd);
	return FALSE;
    }

    /* BCOP - Notify-Hooks */
    edgeblendSetConfigNotify    (display, &edgeblendNotifyCallback);
    edgeblendSetReloadNotify    (display, &edgeblendNotifyCallback);
    edgeblendSetAutoreloadNotify(display, &edgeblendNotifyCallback);
    edgeblendSetShowareasNotify (display, &edgeblendNotifyCallback);

    /* WRAP */
    WRAP (ebd, display, handleEvent, edgeblendHandleEvent); //handle X Events

    /* add reload hook 
     * @TODO -  build real handler for this, since it must do more then load the config,
     *          like resize XDesktop, update cursor...
     */
    edgeblendSetReloadInitiate (display, edgeblendLoadConfig);

    display->base.privates[displayPrivateIndex].ptr = ebd;
    return TRUE;
}

/**
 * Finalizes a Display  -> cleaning
 *
 * @param CompPlugin    *plugin     - Compiz Plugin
 * @param CompDisplay   *display    - Compiz Display
 */
static void
edgeblendFiniDisplay (CompPlugin  *plugin, CompDisplay *display)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendFiniDisplay called");
    EDGEBLEND_DISPLAY (display);

    /** remove hook */
    edgeblendSetReloadTerminate (display, edgeblendLoadConfig);

    /* UNWRAP */
    UNWRAP (ebd, display, handleEvent);
    
    /* Free the private index */
    freeScreenPrivateIndex (display, ebd->screenPrivateIndex);

    /* Free the pointer */
    free (ebd);
}

/**
 * Initializes the plugin
 * Generates an private index for the current display
 *
 * @param CompPlugin *plugin - Compiz Plugin
 * @return Bool
 */
static Bool
edgeblendInit (CompPlugin *plugin)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendInit called");

    //get index on display
    displayPrivateIndex = allocateDisplayPrivateIndex ();

    //no inex no plugin
    if (displayPrivateIndex < 0)
	return FALSE;
    
    return TRUE;
}

/**
 * Finalizes the plugin -> cleaning
 *
 * @param CompPlugin *plugin - Compiz Plugin
 */
static void
edgeblendFini (CompPlugin *p)
{
    compLogMessage ("edgeblend", CompLogLevelInfo, "edgeblendFini called");

    //cleanup display index, if any
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}

/******************************************************************************/
/* COMPIZ-Plugin-Structure                                                    */
/******************************************************************************/
/**
 * Provides an array with the init-functions
 *
 * @param CompPlugin    *plugin - Compiz Plugin
 * @param CompObject    *object - Compiz Object
 * @return CompizBool
 */
static CompBool
edgeblendInitObject (CompPlugin *plugin, CompObject *object)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) edgeblendInitDisplay,
	(InitPluginObjectProc) edgeblendInitScreen
    };

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), TRUE, (plugin, object));
}

/**
 * Provides an array with the fini-functions
 *
 * @param CompPlugin    *plugin - Compiz Plugin
 * @param CompObject    *object - Compiz Object
 */
static void
edgeblendFiniObject (CompPlugin *plugin, CompObject *object)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) edgeblendFiniDisplay,
	(FiniPluginObjectProc) edgeblendFiniScreen
    };

    DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), (plugin, object));
}

/**
 * @var CompPluginVTable    edgeblendVTable - InitStruct
 */
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

/**
 * Compiz Plugin load function
 *
 * @return CompPluginVTable*
 */
CompPluginVTable *
getCompPluginInfo (void)
{
    return &edgeblendVTable;
}
