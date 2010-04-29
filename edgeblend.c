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

#include "fix_env.h"
#include "blending.h"
#include "output.h"

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
*//*
#define WHITE 255
#define BLACK 0
//blend linear between function and mask border
#define BLEND_LINEAR(r, y) ((r<=y)?WHITE:(GLuint)((WHITE*y)/r))
#define INV_BLEND_LINEAR(r, y) (WHITE - BLEND_LINEAR(r, y))
#define CLIP_TO_BYTE(c) ((c<BLACK)?BLACK:(c>WHITE)?WHITE:c)

//assumes that x-axis is the current spoted border
// and y-axis is at x=0
// returns the value to be substracted from the default color
GLuint
blendFunc(int x, int y, double a, double b, double c) {
 return
  (a==0.0)
  ?(b==0.0)
    ?(c==0.0)
      ? WHITE
      : INV_BLEND_LINEAR(c, y)//c
    : INV_BLEND_LINEAR(b*x+c, y)//b*x+c
  :(b==0.0)
    ? INV_BLEND_LINEAR(a*x*x+c, y)//a*x^2+c
    : INV_BLEND_LINEAR(a*x*x+b*x+c, y)//a*x^2+b*x+c
  ;
}*/

/**
 * Getting Xorg-Cursor image and store it in gfx-ram:
 * Taken from the zoom-plugin
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
 * Draws a rect....
 * @SEE ezoom
 *
 * @TODO better done with ./src/paint.c:415:	(*screen->paintCursor) (c, transform, tmpRegion, 0);
 *
 * @param int x - x-Pos
 * @param int y - y-Pos
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

    float x,y;
    int i;

    int row, col, overlap;
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
                 *
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

    //since we support only one screen we can assume the firstone in the
    //correct one
    //for (screen = display->screens; screen; screen = screen->next) {
        //see display.c:2701 void warpPointer (CompScreen *s, int dx,int dy);
        if (pointerX > screen->width || pointerY > screen->height)
            warpPointer(screen, 0,0);
    //}

    /**
     * @TODO react on Cursor-Change Event
     */
    
    
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

    //switch ENV
    /* fix workarea */
    if (!fix_CompScreenWorkarea(screen, ebs, TRUE)) {
        return FALSE;
    }
    /* resize XDesktop */
    fix_XDesktopSize(screen, ebs);
    /* disable XCursor and enable mousepolling */
    fix_XCursor(screen, ebs, TRUE);
    /* ensure fullscreenoutput is used to render */
    fix_CompFullscreenOutput(plugin, screen, ebs, TRUE);

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
