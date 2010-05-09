/*
 * File:   output.h
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

