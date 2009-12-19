/* 
 * File:   fix_cursor.h
 * Author: flatline
 *
 * Created on December 19, 2009, 2:34 PM
 */

#ifndef _FIX_CURSOR_H
#define	_FIX_CURSOR_H

#include "edgeblend.h"

#ifdef	__cplusplus
extern "C" {
#endif


    void fix_XDesktopSize (CompScreen * screen, unsigned long width, unsigned long height);
    void fix_CompFullscreenOutput(CompPlugin * plugin, CompScreen * screen, edgeblendScreen * ebs, Bool mode);
    void fix_XCursor(CompScreen * screen, edgeblendScreen * ebs, Bool mode);
    Bool fix_CursorPoll(CompScreen * screen, edgeblendScreen * ebs);

    


#ifdef	__cplusplus
}
#endif

#endif	/* _FIX_CURSOR_H */

