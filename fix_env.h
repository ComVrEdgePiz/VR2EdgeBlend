/*
 * File:   fix_env.h
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

#ifndef _FIX_CURSOR_H
#define	_FIX_CURSOR_H

#include "edgeblend.h"
#include <assert.h>
#include <compiz-core.h>
#ifdef	__cplusplus
extern "C" {
#endif


    void fix_XDesktopSize (CompScreen * screen, edgeblendScreen * ebs);
    void fix_CompFullscreenOutput(CompPlugin * plugin, CompScreen * screen, edgeblendScreen * ebs, Bool mode);
    void fix_XCursor(CompScreen * screen, edgeblendScreen * ebs, Bool mode);
    Bool fix_CursorPoll(CompScreen * screen, edgeblendScreen * ebs);
    Bool fix_CompWindowDocks(CompScreen *screen, edgeblendScreen * ebs, Bool mode);
    Bool fix_CompScreenWorkarea(CompScreen *screen, edgeblendScreen * ebs, Bool mode);
    


#ifdef	__cplusplus
}
#endif

#endif	/* _FIX_CURSOR_H */

