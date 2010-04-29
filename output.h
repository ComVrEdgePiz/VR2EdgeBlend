/* 
 * File:   output.h
 * Author: flatline
 *
 * Created on April 29, 2010, 12:47 PM
 */

#ifndef _OUTPUT_H
#define	_OUTPUT_H

#include <stdio.h>
#include <string.h>

#include "edgeblend.h"

#include "fix_env.h"

#ifdef	__cplusplus
extern "C" {
#endif

    void buildOutput(edgeblendScreen * ebs);

#ifdef	__cplusplus
}
#endif

#endif	/* _OUTPUT_H */

