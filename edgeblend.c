/*
 * File:   edgeblend.c
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
#include "edgeblend.h"

#include "fix_env.h"
#include "blending.h"
#include "output.h"
/*
 * @var display plugin-data index
 */
static int displayPrivateIndex = 0;

/**
 * Enables per screen operations on the context, right before drawing,
 *
 * @param CompScreen *screen - Compiz Screen
 * @param int        ms      - msec since last draw
 */
/*
static void
edgeblendPreparePaintScreen (CompScreen *screen, int ms)
{
    EDGEBLEND_SCREEN (screen);
    //CompDisplay *d = screen->display;

    UNWRAP (ebs, screen, preparePaintScreen);
        (*screen->preparePaintScreen) (screen, ms);
    WRAP (ebs, screen, preparePaintScreen, edgeblendPreparePaintScreen);
}
*/

/**
 * Called enables influence the painting of the howl screen
 *
 * @param CompScreen    *screen     - Compiz Screen
 * @param CompOutput    *outputs    - Compiz Output[]
 * @param int           numOutputs  - #outputs
 * @param unsigned int  mask        - draw-flags
 */
/*
static void
edgeblendPaintScreen (CompScreen *screen, CompOutput *outputs,
                      int numOutputs, unsigned int mask)
{
    EDGEBLEND_SCREEN (screen);

    UNWRAP (ebs, screen, paintScreen);
	(*screen->paintScreen) (screen, outputs, numOutputs, mask);
    WRAP (ebs, screen, paintScreen, edgeblendPaintScreen);
}
static Bool
edgeblendPaintWindow (CompWindow              *w,
                   const WindowPaintAttrib *attrib,
                   const CompTransform     *transform,
                   Region                  region,
                   unsigned int            mask)
{
    CompScreen *screen = w->screen;
    Bool status = TRUE;
    EDGEBLEND_SCREEN (screen);

    UNWRAP (ebs, screen, paintWindow);
        status = (*screen->paintWindow) (w, attrib, transform, region, mask);
    WRAP (ebs, screen, paintWindow, edgeblendPaintWindow);

    return status;
}
*/
/**
 * Getting Xorg-Cursor image and store it in gfx-ram:
 * Taken from the zoom-plugin
 * @PARAM edgeblendScreen   *ebs     - private edgeblend plugin screen data
 * @PARAM CompDisplay       *display - Compiz Display
 */
static void
updateMouseCursor(edgeblendScreen *ebs, Display *display)
{
    unsigned char *pixels;
    int i;

    XFixesCursorImage *ci = XFixesGetCursorImage(display);
    /* Hack to avoid changing to an invisible (bugged)cursor image.
     * Example: The animated Firefox cursors.
     */
    if (ci->width <= 1 && ci->height <= 1)
    {
	XFree (ci);
	return;
    }
    
    ebs->cursorWidth    = ci->width;    /* height*/
    ebs->cursorHeight   = ci->height;   /* width*/
    ebs->cursorHotX     = ci->xhot;     /* fix-center-x*/
    ebs->cursorHotY     = ci->yhot;     /* fix-center-y*/

    pixels = malloc(ci->width * ci->height * 4);
    if (!pixels) {
	XFree (ci);
	return;
    } else {
        for (i = 0; i < ci->width * ci->height; i++)
        {
            unsigned long pix = ci->pixels[i];
            pixels[i * 4] = pix & 0xff;
            pixels[(i * 4) + 1] = (pix >> 8) & 0xff;
            pixels[(i * 4) + 2] = (pix >> 16) & 0xff;
            pixels[(i * 4) + 3] = (pix >> 24) & 0xff;
        }

        glEnable (GL_TEXTURE_RECTANGLE_ARB);
            glBindTexture (GL_TEXTURE_RECTANGLE_ARB, ebs->cursorTexture);
            glTexImage2D (GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, ebs->cursorWidth,
                          ebs->cursorHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
            glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
        glDisable (GL_TEXTURE_RECTANGLE_ARB);
        XFree (ci);
        free (pixels);
   }
}

/**
 * Draws a rect for mouse cursor
 * @SEE ezoom-plugin
 * @TODO better done with ./src/paint.c:415:	(*screen->paintCursor) (c, transform, tmpRegion, 0);
 *
 * @PARAM edgeblendScreen   *ebs     - private edgeblend plugin screen data
 * @PARAM CompDisplay       *display - Compiz Display
 */
static void
drawMouseCursor(edgeblendScreen *ebs, Display *display)
{
    int x,y;
    updateMouseCursor(ebs, display);
    x = ebs->mouseX - ebs->cursorHotX;
    y = ebs->mouseY - ebs->cursorHotY;

    glEnable (GL_BLEND);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, ebs->cursorTexture);
    glEnable (GL_TEXTURE_RECTANGLE_ARB);

    glBegin (GL_QUADS);
        glTexCoord2d (0, 0);                                glVertex2f (x, y);
        glTexCoord2d (0, ebs->cursorHeight);                glVertex2f (x, y + ebs->cursorHeight);
        glTexCoord2d (ebs->cursorWidth, ebs->cursorHeight); glVertex2f (x + ebs->cursorWidth, y + ebs->cursorHeight);
        glTexCoord2d (ebs->cursorWidth, 0);                 glVertex2f (x + ebs->cursorWidth, y);
    glEnd ();
    glDisable (GL_BLEND);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
    glDisable (GL_TEXTURE_RECTANGLE_ARB);
    glPopMatrix ();
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
    
    CompTransform sTransform = *transform;

    Bool status = TRUE;
    
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

        /* Save everything*/
        glPushMatrix ();
            glPushAttrib(GL_COLOR_BUFFER_BIT);
                //we want blend everything at the moment since its cool...
                glEnable (GL_BLEND);
                glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                /* 1. We have to draw our replacment Cursor, since the cursor X
                 * provided is out-of-sync when copy buffer arround.
                 */
                //fetch current position of the cursor from mousepoll-plugin
                if (fix_CursorPoll(screen, ebs)) {
                    drawMouseCursor(ebs, screen->display->display);
                }
                glEnable (GL_BLEND);
                /* 2. Move towards the lower right output of out output-grid and draw then
                 * move from bottom to to and from right to left
                 */
                buildOutput(ebs);
                /* 3. assign blending texture
                 */
                blend(ebs);

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
    //burns cpu-time like a champ... need to switch to drawing only what changed
    //or better write an XExtension... perhaps could be used as bachelor-thesis?
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
static void
edgeblendHandleEvent (CompDisplay * display, XEvent * event)
{
    EDGEBLEND_DISPLAY (display);
    CompScreen *screen = display->screens;
    EDGEBLEND_SCREEN (screen);
    int width  = (ebs->outputCfg->cell.width  * ebs->outputCfg->grid.cols) - ((ebs->outputCfg->grid.cols-1) * ebs->outputCfg->grid.blend);
    int height = (ebs->outputCfg->cell.height * ebs->outputCfg->grid.rows) - ((ebs->outputCfg->grid.rows-1) * ebs->outputCfg->grid.blend);
    int oldWidth  = screen->width;
    int oldHeight = screen->height;
  //  EDGEBLEND_SCREEN (screen);

    //since we support only one screen we can assume the firstone in the
    //correct one
    //for (screen = display->screens; screen; screen = screen->next) {
        //see display.c:2701 void warpPointer (CompScreen *s, int dx,int dy);
        if (pointerX > width || pointerY > height || pointerX < 0 || pointerY < 0) {
            screen->width  = width;
            screen->height = height;
            warpPointer(screen, 0,0);
            screen->width  = oldWidth;
            screen->height = oldHeight;
        }
    //}

    /**
     * @better react on Cursor-Change Event
     */
    
    
    UNWRAP (ebd, display, handleEvent);
        (*display->handleEvent) (display, event);
    WRAP (ebd, display, handleEvent, edgeblendHandleEvent);
}

/******************************************************************************/
/* Manage Functions                                                           */
/******************************************************************************/
void
edgeblendDebugOutput (CompDisplay *display);
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
    edgeblendDebugOutput(display);
    if (screen)
    {
	//EDGEBLEND_SCREEN (screen);
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
static void
edgeblendNotifyCallback(CompDisplay *display, CompOption *option, EdgeblendDisplayOptions num)
{
    /* Which Option was changed? */
    switch (num) {
        case EdgeblendDisplayOptionConfig:
            //reload config
            compLogMessage ("edgeblend", CompLogLevelInfo,"edgeblendNotifyCallback called on option config");
            break;
        default:
            //else say something
            edgeblendDebugOutput(display);
            compLogMessage ("edgeblend", CompLogLevelInfo,"edgeblendNotifyCallback called on option %d", num);
            break;
    }
}


/******************************************************************************/
/* Debug - Functions                                                          */
/******************************************************************************/
void
edgeblendDebugOutput (CompDisplay *display)
{
    int i;
    CompWindow *w;
    //EDGEBLEND_DISPLAY (display);
    CompScreen *screen = display->screens;
    //EDGEBLEND_SCREEN (screen);
    compLogMessage ("DISPLAY", CompLogLevelInfo,"-------------------------------------------");
    compLogMessage ("DISPLAY", CompLogLevelInfo,"Screens: %d", display->nScreenInfo);
    for (i=0;i<display->nScreenInfo;i++) {
        compLogMessage ("DISPLAY", CompLogLevelInfo,"XineramaScreen(%d): %d/%d", i, display->screenInfo[i].width, display->screenInfo[i].height);
    }
    compLogMessage ("SCREEN", CompLogLevelInfo,"-------------------------------------------");
    compLogMessage ("SCREEN", CompLogLevelInfo,"Size: %d/%d", screen->width, screen->height);
    compLogMessage ("SCREEN", CompLogLevelInfo,"WorkArea: %d/%d -> %d/%d", screen->workArea.x,screen->workArea.y,screen->workArea.width,screen->workArea.height);
    compLogMessage ("SCREEN", CompLogLevelInfo,"Fullscreen-Output: %d/%d", screen->fullscreenOutput.width, screen->fullscreenOutput.height);
    compLogMessage ("SCREEN", CompLogLevelInfo,"         WorkArea:      %d/%d -> %d/%d", screen->fullscreenOutput.workArea.x,screen->fullscreenOutput.workArea.y,screen->fullscreenOutput.workArea.width,screen->fullscreenOutput.workArea.height);
    compLogMessage ("SCREEN", CompLogLevelInfo,"         RegionExtends: %d/%d -> %d/%d", screen->fullscreenOutput.region.extents.x1,screen->fullscreenOutput.region.extents.y1,screen->fullscreenOutput.region.extents.x2,screen->fullscreenOutput.region.extents.y2);
    compLogMessage ("SCREEN", CompLogLevelInfo,"Outputs: %d", screen->nOutputDev);
    for (i=0; i< screen->nOutputDev; i++) {
        compLogMessage ("SCREEN", CompLogLevelInfo,"Output(%d): %d/%d", i, screen->outputDev[i].width, screen->outputDev[i].height);
        compLogMessage ("SCREEN", CompLogLevelInfo,"         WorkArea:      %d/%d -> %d/%d", screen->outputDev[i].workArea.x,screen->outputDev[i].workArea.y,screen->outputDev[i].workArea.width,screen->outputDev[i].workArea.height);
        compLogMessage ("SCREEN", CompLogLevelInfo,"         RegionExtends: %d/%d -> %d/%d", screen->outputDev[i].region.extents.x1,screen->outputDev[i].region.extents.y1,screen->outputDev[i].region.extents.x2,screen->outputDev[i].region.extents.y2);
    }
    compLogMessage ("SCREEN", CompLogLevelInfo,"Attribt: %d/%d", screen->attrib.width, screen->attrib.height);

    for (w=screen->windows; w; w=w->next) {
        if (w->type == CompWindowTypeDockMask) {
            compLogMessage ("SCREEN", CompLogLevelInfo,"window: x/y=%d/%d h/w=%d/%d", w->serverX, w->serverY, w->serverWidth, w->serverHeight);
            if (w->serverY != 0) {
                //moveWindow(w, 0, -300, TRUE, TRUE);
                //syncWindowPosition(w);
                //result = XMoveWindow (screen->display->display, w->id, w->attrib.x, w->attrib.y);
                //compLogMessage ("SCREEN STRUT", CompLogLevelInfo,"%d", result);
            }
        }

    }


    XConfigureEvent xev;
    xev.event  = screen->root;
    xev.window = screen->root;
    xev.type   = ConfigureNotify;
    xev.x        = 0;
    xev.y	 = 0;
    xev.width	 = 2560 - 3*30;
    xev.height	 = 1024 - 1*30;
    xev.border_width  = None;
    xev.above	      = None;
    xev.override_redirect = FALSE;
    //XSendEvent (screen->display->display, screen->root, FALSE, StructureNotifyMask, (XEvent *) &xev);
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
    //tmp pointer for output config data structure
    char * filepath;
    
    EDGEBLEND_DISPLAY (screen->display);
    //create screen private data
    edgeblendScreen *ebs = (edgeblendScreen *) calloc (1, sizeof (edgeblendScreen) );
    if (!ebs) {
        return FALSE;
    }

    /* ensure we have only one screen for the display else return FALSE;*/
    if (screen->next != 0)
        return FALSE;

    /* load output config */
    filepath     = edgeblendGetConfig(screen->display);
    compLogMessage ("edgeblend", CompLogLevelInfo,"edgeblendNotifyCallback called on option %s", filepath);
    if (!filepath) {
        return FALSE;
    }
    ebs->outputCfg = load_outputconfig(filepath, screen);
    if (!ebs->outputCfg)
        return FALSE;

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

    /* ensure fullscreenoutput is used to render */
    fix_CompFullscreenOutput(plugin, screen, ebs, TRUE);


    //switch ENV
    /* resize XDesktop */
    fix_XDesktopSize(screen, ebs);
    /* fix workarea */
    if (!fix_CompScreenWorkarea(screen, ebs, TRUE)) {
        return FALSE;
    }
    fix_CompWindowDocks(screen, ebs, TRUE);
    
    /* disable XCursor and enable mousepolling */
    fix_XCursor(screen, ebs, TRUE);
  
    //build cursor-texture name
    glGenTextures (1, &ebs->cursorTexture);

   
    /* store private data */
    screen->base.privates[ebd->screenPrivateIndex].ptr = ebs;

    generateBlendingTexture(ebs);
    //edgeblendCreateTextures(screen);//ebs->outputConfig, screen);

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
    //UNWRAP (ebs, screen, paintScreen);
    //UNWRAP (ebs, screen, preparePaintScreen);
    UNWRAP (ebs, screen, paintOutput);
    UNWRAP (ebs, screen, donePaintScreen);
    //UNWRAP (ebs, screen, paintWindow);
    //UNWRAP (ebs, s, paintTransformedOutput);
    



    /* restore compiz fullscreen/outputdevice-based rendering */
    fix_CompFullscreenOutput(plugin, screen, ebs, FALSE);
    /* restore woarkarea */
    fix_CompScreenWorkarea(screen, ebs, FALSE);
    fix_CompWindowDocks(screen, ebs, FALSE);
    /* restore height and width of the X-Desktop */
    fix_XDesktopSize(screen, ebs);
    /* restore XCursor handling and remove mosue polling */
    fix_XCursor(screen, ebs, FALSE);
    
    
    

    //clear edgeblend
    damageScreen (screen);

    if (ebs->outputCfg) {
        free(ebs->outputCfg);
    }

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
    //display->screenInfo->height -= 60;
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

    /* WRAP */
    WRAP (ebd, display, handleEvent, edgeblendHandleEvent); //handle X Events

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
