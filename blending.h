/* 
 * File:   blending.h
 * Author: flatline
 *
 * Created on April 2, 2010, 5:27 PM
 */

#ifndef _BLENDING_H
#define	_BLENDING_H

#include <stdio.h>
#include <string.h>

#include "edgeblend.h"

#include "fix_env.h"

#ifdef	__cplusplus
extern "C" {
#endif

    Bool generateBlendingTexture(edgeblendScreen *screen);
    void blend(edgeblendScreen *screen);
    void freeBlendingTexture(edgeblendScreen *screen);


#ifdef	__cplusplus
}
#endif

#endif	/* _BLENDING_H */

