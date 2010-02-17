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
*/
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
static void
drawMouseCursor(int x, int y)
{
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
                    drawMouseCursor(ebs->mouseX,ebs->mouseY);
                }
                
                /* 2. Move towards the lower right output of out output-grid and draw then
                 * move from bottom to to and from right to left
                 */
                //move to 2. output (assuming leftof, 2 * 1280x1024)
                //copy overlapping area to the outputdevice
                row     = ebs->outputCfg->grid.rows;
                overlap = ebs->outputCfg->grid.blend;
                while(row--) {
                    col = ebs->outputCfg->grid.cols;
                    while(col--) {
                        if (row == col && row == 0) {
                            //first dev, no need to copy 
                        } else {
                            glRasterPos2i( col * ebs->outputCfg->cell.width
                                         , (row+1) * ebs->outputCfg->cell.height);

                            glCopyPixels( (col * ebs->outputCfg->cell.width) - (col * overlap)
                                        , (row * ebs->outputCfg->cell.height)
                                        , ebs->outputCfg->cell.width
                                        , ebs->outputCfg->cell.height
                                        , GL_COLOR);
                        }
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
//                        glBindTexture(GL_TEXTURE_2D, ebs->textures);
                        glBegin(GL_QUADS);
                            glTexCoord2f(0.0, 0.0);
                            glVertex2f(x, y);
                            glTexCoord2f(1.0, 0.0);
                            glVertex2f(x+128.0, y);
                            glTexCoord2f(0.0, 1.0);
                            glVertex2f(x, y+128.0);
                            glTexCoord2f(1.0, 1.0);
                            glVertex2f(x+128.0, y+128.0);
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

/**
 * Creates blending textures for the painting
 *
 * @param Compscreen         *screen    - Compiz Screen
 *
 */
static void
edgeblendCreateTextures(CompScreen *screen)
{
  //create texturespace (try to create them on graphics ram)
  EDGEBLEND_SCREEN(screen);
  int cols = ebs->outputCfg->grid.cols;
  int rows = ebs->outputCfg->grid.rows;
  int width = ebs->outputCfg->cell.width;
  int height = ebs->outputCfg->cell.height;
  int numScreens = cols*rows;
  int size = width * height * 4; //RGBA texture
  int i,j,n,s;
  GLuint tex[numScreens], c;
  GLuint* pixel;//, r, end;
  EdgeblendOutputScreen scrcfg;
  
  //allocate texture names
  glGenTextures(numScreens, tex);
  
  //store texture names to environment
  ebs->textures = tex;

  //allocate texturespace in ram
  pixel = malloc(size);
  if(!pixel) return ;

  //zeiger auf den wert nach dem letzten pixel (eof)
  //end = pixel+size;
  
  for(s=0;s<numScreens;s++) { //iterate over screens
    //select current texture
    glBindTexture(GL_TEXTURE_2D, tex[s]);
    
    //select modulate to mix texture with color for shading
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); //GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE
    //set bilinear, mipmap, overlap filtering -> do not need as our texture will fit the screen //glTexParameterf();

    scrcfg = ebs->outputCfg->screens[s];
    
    //ebs->outputCfg-> //load different screen definitions
    if(scrcfg.imagepath) {
      //load texture from image
      
      //TODO
    
    } else {//generate texture mask by function
      //put all pixels to max white and opaque
      for(i=0;i<size;i++) pixel[i]=WHITE;//for(r=pixel;r<end;r+=4) *r=0xFFFFFFFF;
      
      //blend defined pixels to black / transparent and/or change some colors (different projectors)
      for(i=0;i<width;i++){//for(r=pixel,i=0;r<end;i++){
        //o=r+height;
        for(j=0;j<height;j++){//for(j=0;r<o;r+=4,j++){ //i and j only define the position i= current width, j= current height (or the other way around)
          //add some cases here to define a mask that makes sense
          // x ist immer die kante, der 0-punkt des koordinatensystems der aktuellen blending func
          // ist immer an dem linken ende der kante (die kante ist die nächste betrachtete)
          // in gl koordinaten wäre für die "top" kante somit der punkt 1.0/1.0 der 0-punkt
          //left x= height-j, y= i
          c = blendFunc(height-j, i, scrcfg.left.a, scrcfg.left.b, scrcfg.left.c);
          //top x= width-i, y= height-j
          c += blendFunc(width-i, height-j, scrcfg.top.a, scrcfg.top.b, scrcfg.top.c);
          //right x=  j, y= width-i 
          c += blendFunc(j, width-i, scrcfg.right.a, scrcfg.right.b, scrcfg.right.c);
          //bottom x= i, y= j
          c += blendFunc(i, j, scrcfg.bottom.a, scrcfg.bottom.b, scrcfg.bottom.c);
          
          c = CLIP_TO_BYTE(c);
          
          n = i*j+j;//current pixel to work with
          
          pixel[n] = pixel[n+1] = pixel[n+2] = c; //reduce only color, not alpha
        }
      }
      
      //push pixel data to texture and texture to gl
      glTexImage2D( GL_TEXTURE_2D    //target
                  , 0                //LOD
                  , GL_RGBA          //internal format
                  , width            //width
                  , height           //height
                  , 0                //no border
                  , GL_RGBA          //use 4 texture components per pixel
                  , GL_UNSIGNED_BYTE //texure components has 1 byte size
                  , pixel            //image data
                  );
    }//end if no image for mask
  } //end screen iterator
  
  free(pixel);
  
  return ; //return nothing, texture names are in the environment
}

/**
 * Creates blending textures for the painting
 *
 * @param Compscreen         *screen    - Compiz Screen
 *
 */
static void
edgeblendCreateTestTexture(CompScreen *screen)
{
  EDGEBLEND_SCREEN(screen);
  GLuint texture;
  GLuint* pixel;
  int size = 128 * 128 * 4; //128x128 RGBA texture
  int i,j,n;
  
  glGenTextures(1, &texture);
  ebs->testtex = texture;
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

  pixel = malloc(size);

  if(!pixel) return ;

  for(i=0;i<size;i++) pixel[i]=255;
  
  for(i=0;i<128;i++){
    for(j=0;j<128;j++){
      n = i*j+j+3; //alpha
      pixel[n] = pixel[n] - (j*2 - 1); 
    }
  }
  
  glTexImage2D( GL_TEXTURE_2D    //target
              , 0                //LOD
              , GL_RGBA          //internal format
              , 128              //width
              , 128              //height
              , 0                //no border
              , GL_RGBA          //use 4 texture components per pixel
              , GL_UNSIGNED_BYTE //texure components has 1 byte size
              , pixel            //image data
              );
  
  free(pixel);
  
  return ;//texture;
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
    if (!filepath) {
        return FALSE;
    }
    ebs->outputCfg = load_outputconfig("/home/vr2/vr2/VR2EdgeBlend/two_head.xml", screen);//filepath);
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
    //fix_XDesktopSize(screen, 2160, screen->height);
    /* disable XCursor and enable mousepolling */
    fix_XCursor(screen, ebs, TRUE);
    /* ensure fullscreenoutput is used to render */
    fix_CompFullscreenOutput(plugin, screen, ebs, TRUE);
    

    /* store private data */
    screen->base.privates[ebd->screenPrivateIndex].ptr = ebs;

//    edgeblendCreateTextures(screen);//ebs->outputConfig, screen);

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
