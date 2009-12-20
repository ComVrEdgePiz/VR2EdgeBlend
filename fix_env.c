#include <string.h>

#include "fix_env.h"

/**
 * Sets the size of the Desktop of the XDisplay, to get right borders for
 * the mouse
 * @TODO make it work... perhaps since its something about windowhandling it
 *       could be done inside compiz?
 * @TODO store and restore values
 *
 * CompScreen       *screen - Compiz Screen
 * unsigned long    width   - new XDesktop width
 * unsigned long    height  - new XDesktop height
 */
void
fix_XDesktopSize (CompScreen * screen, unsigned long width, unsigned long height)
{
    int geo, work, view;
    unsigned long data[2];
    data[0] = (unsigned long) width;
    data[1] = (unsigned long) height;

    geo = XChangeProperty(screen->display->display, screen->root, screen->display->desktopGeometryAtom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, 2);
    compLogMessage ("edgeblend::fix_env->XDesktop", CompLogLevelInfo," Geo to %d/%d = %d",width, height, geo);

    unsigned long data2[4];
    data2[0] = (unsigned long) 0;
    data2[1] = (unsigned long) 0;
    data2[2] = (unsigned long) width;
    data2[3] = (unsigned long) height;
    work = XChangeProperty(screen->display->display, screen->root, screen->display->workareaAtom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data2, 4);
    compLogMessage ("edgeblend::fix_env->XDesktop", CompLogLevelInfo," Work to %d/%d %d/%d = %d",0,0, width, height, work);
    //compLogMessage ("edgeblend::fix_env->XDesktop", CompLogLevelInfo," Geo = %d, Work = %d",geo, work);

    unsigned long data3[2][2];
    data3[0][0] = 0;
    data3[0][1] = 0;
    data3[1][0] = width;
    data3[1][1] = height;
    view = XChangeProperty(screen->display->display, screen->root, screen->display->desktopViewportAtom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data3, 2);
    compLogMessage ("edgeblend::fix_env->XDesktop", CompLogLevelInfo," View to %d/%d %d/%d = %d", 0, 0, width, height, view);

    screen->workArea.width = width;
    screen->workArea.height = height;

}

/**
 * Forces Compiz to use fullscreenOutput to render
 *
 * @PARAM CompPlugin        *plugin - Compiz Plugin
 * @PARAM CompScreen        *screen - Compiz Screen
 * @PARAM edgeblendScreen   *ebs    - private edgeblend plugin screen data
 * @PARAM BOOL              mode    - on/off
 */
void
fix_CompFullscreenOutput(CompPlugin *plugin, CompScreen * screen, edgeblendScreen * ebs, Bool mode)
{
    if (mode == TRUE) {
        /* switch force independet output */
        CompOptionValue option;
        option = screen->opt[COMP_SCREEN_OPTION_FORCE_INDEPENDENT].value;
        ebs->wasForcedIndependetOutput = option.b;
        option.b = FALSE;
        setScreenOption(plugin, screen, screen->opt[COMP_SCREEN_OPTION_FORCE_INDEPENDENT].name , &option);

        /* switch overlapping output */
        ebs->hadOverlappingOutputs      = screen->hasOverlappingOutputs;
        screen->hasOverlappingOutputs   = TRUE;
        compLogMessage ("edgeblend::fix_env->fullscreenoutput", CompLogLevelInfo," true");
    } else {
        /* reset force independet output */
        CompOptionValue option;
        option.b = ebs->wasForcedIndependetOutput;
        setScreenOption(plugin, screen, screen->opt[COMP_SCREEN_OPTION_FORCE_INDEPENDENT].name , &option);

        /* reset overlapping output */
        screen->hasOverlappingOutputs = ebs->hadOverlappingOutputs;
        compLogMessage ("edgeblend::fix_env->fullscreenoutput", CompLogLevelInfo," false");
    }

}

/**
 * Disables the XCursor, so we can draw our own
 * Todo so we hook into the mousepull-plugin
 *
 * @PARAM CompScreen        *screen - Compiz Screen
 * @PARAM edgeblendScreen   *ebs    - private edgeblend plugin screen data
 * @PARAM Bool              mode    - on/off
 */
void fix_XCursor(CompScreen * screen, edgeblendScreen * ebs, Bool mode)
{
    compLogMessage ("edgeblend::fix_env->hidecursor", CompLogLevelInfo," true????? %d", displayPrivateIndex);

    /* when we can not hide we should not try */
    if (!ebs->fixesSupported) return;

    if (mode == TRUE) {
        /*if (!zs->opt[SOPT_SCALE_MOUSE].value.b) return;
        if (!zs->cursorInfoSelected){
            zs->cursorInfoSelected = TRUE;
            XFixesSelectCursorInput (s->display->display, s->root, XFixesDisplayCursorNotifyMask);
            zoomUpdateCursor (s, &zs->cursor);
        } zs->opt[SOPT_HIDE_ORIGINAL_MOUSE].value.b) ??*/
        
        /* can we? */
        if (ebs->canHideCursor && !ebs->cursorHidden) {
            //poll position handle not nedded since we poll on every draw...
            //ebs->pollHandle = (*ebs->mpFunc->addPositionPolling) (screen, updateMouseInterval);
            ebs->lastChange = time(NULL);
            (*ebs->mpFunc->getCurrentPosition) (screen, &ebs->mouseX, &ebs->mouseY);
            //hide
            ebs->cursorHidden = TRUE;
            XFixesHideCursor (screen->display->display, screen->root);
            compLogMessage ("edgeblend::fix_env->hidecursor", CompLogLevelInfo," true");
        }      
    } else {
        /*if (zs->cursorInfoSelected) {
            zs->cursorInfoSelected = FALSE;
            XFixesSelectCursorInput (s->display->display, s->root, 0);
        }
        if (zs->cursor.isSet) {freeCursor (&zs->cursor);}*/
        /* can we? (since it's hidden we can ;) */
        if (ebs->cursorHidden) {
            //stop polling not needed since we do not use the handle
            //(*ebs->mpFunc->removePositionPolling) (screen, ebs->pollHandle);
            ebs->pollHandle = NULL;
            ebs->mpFunc     = NULL;
            //show XCursor
            ebs->cursorHidden = FALSE;
            XFixesShowCursor (screen->display->display, screen->root);
            compLogMessage ("edgeblend::fix_env->hidecursor", CompLogLevelInfo," false");
        }
    }
}

/**
 * Polls the position of the cursor and saves them inside the ebs (mouseX/Y)
 *
 * @PARAM CompScreen        *screen - Compiz Screen
 * @PARAM edgeblendScreen   *ebs    - private edgeblend plugin screen data
 * @RETURN Bool
 */
Bool fix_CursorPoll(CompScreen * screen, edgeblendScreen * ebs)
{
    //since we have no handle don't check it, it's always NULL...
    if (/*ebs->pollHandle && */ ebs->mpFunc) {
        ebs->lastChange = time(NULL);
	(*ebs->mpFunc->getCurrentPosition) (screen, &ebs->mouseX, &ebs->mouseY);
        return TRUE;
    } else {
        return FALSE;
    }
}


/**
 * Fixes the Workarea of compiz, by calling updateWorkareaForScreen(screen), 
 * by changing the extends of the outputs, after backup
 *
 * @PARAM CompScreen        *screen - Compiz Screen
 * @PARAM edgeblendScreen   *ebs    - private edgeblend plugin screen data
 * @PARAM Bool              mode    - on/off
 */
Bool
fix_CompScreenWorkarea(CompScreen *screen, edgeblendScreen * ebs, Bool mode)
{
    int     i;
    if (mode == TRUE) {
        if (!save_CompScreenWorkArea(screen, ebs)) {
            return FALSE;
        }

        for(i = 0; i < screen->nOutputDev; i++) {
            printOutputdev(&screen->outputDev[i]);
            if (i == 1) { //leafe 0 as it is
                /* @TODO dynamic list */
                screen->outputDev[i].region.extents.x1 = 1280;
                screen->outputDev[i].region.extents.y1 = 0;
                screen->outputDev[i].region.extents.x2 = 1280-200 + 1280;
                screen->outputDev[i].region.extents.y2 = 1024;
            }
            printOutputdev(&screen->outputDev[i]);
        }

        screen->width  = 1280 * 2 - 200;
        screen->height = screen->height;
    } else {
        restore_CompScreenWorkArea(screen, ebs);
    }

    updateWorkareaForScreen(screen);
    return TRUE;
}

/**
 * Saves all data related to the workarea
 *
 * @PARAM CompScreen        *screen - Compiz Screen
 * @PARAM edgeblendScreen   *ebs    - private edgeblend plugin screen data
 * @return Bool
 */
Bool
save_CompScreenWorkArea(CompScreen * screen, edgeblendScreen * ebs)
{
    int i;
    ebs->orginalWorkarea.orginalWidth   = screen->width;
    ebs->orginalWorkarea.orginalHeight  = screen->height;
    ebs->orginalWorkarea.nOutputs       = screen->nOutputDev;
    ebs->orginalWorkarea.outputExtends  = (BOX **) malloc (sizeof(BOX*) * screen->nOutputDev);
    if (!ebs->orginalWorkarea.outputExtends)
        return FALSE;

    for(i = 0; i < screen->nOutputDev; i++) {
        ebs->orginalWorkarea.outputExtends[i] = (Box *) malloc(sizeof(BOX));
        if (!
             (
              ebs->orginalWorkarea.outputExtends[i] &&
              memcpy(ebs->orginalWorkarea.outputExtends[i], &screen->outputDev[i].region.extents, sizeof(BOX))
             )
           )
        {
            i--;
            while(i >= 0) {
                free(ebs->orginalWorkarea.outputExtends[i]);
                i--;
            }
            free(ebs->orginalWorkarea.outputExtends);
            ebs->orginalWorkarea.outputExtends = NULL;
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * restores all data related to the workarea
 *
 * @PARAM CompScreen        *screen - Compiz Screen
 * @PARAM edgeblendScreen   *ebs    - private edgeblend plugin screen data
 */
void
restore_CompScreenWorkArea(CompScreen * screen, edgeblendScreen * ebs)
{
    int i;
    screen->width   = ebs->orginalWorkarea.orginalWidth;
    screen->height  = ebs->orginalWorkarea.orginalHeight;
    if (ebs->orginalWorkarea.outputExtends) {
        for (i=0; i < ebs->orginalWorkarea.nOutputs; i++) {
            printOutputdev(&screen->outputDev[i]);
            if (ebs->orginalWorkarea.outputExtends[i]) {
                memcpy(&screen->outputDev[i].region.extents, ebs->orginalWorkarea.outputExtends[i], sizeof(BOX));
                free(ebs->orginalWorkarea.outputExtends[i]);
            }
            printOutputdev(&screen->outputDev[i]);
        }
    }
    free(ebs->orginalWorkarea.outputExtends);
    ebs->orginalWorkarea.outputExtends = NULL;
}