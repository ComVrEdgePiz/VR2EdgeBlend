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
    int geo, work;
    unsigned long data[2];
    data[0] = (unsigned long) width;
    data[1] = (unsigned long) height;
    geo = XChangeProperty(screen->display->display, screen->root, screen->display->desktopGeometryAtom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, 2);

    unsigned long data2[4];
    data2[0] = (unsigned long) 0;
    data2[1] = (unsigned long) 0;
    data2[2] = (unsigned long) width;
    data2[3] = (unsigned long) height;
    work = XChangeProperty(screen->display->display, screen->root, screen->display->workareaAtom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data2, 4);

    compLogMessage ("edgeblend::fix_env->XDesktop", CompLogLevelInfo," Geo = %d, Work = %d",geo, work);
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
