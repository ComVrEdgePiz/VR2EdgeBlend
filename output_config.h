/*
 * File:   output_config.h
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

#ifndef _OUTPUT_CONFIG_H
#define	_OUTPUT_CONFIG_H

#include <compiz-core.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>

typedef struct _EgdeblendOutputCell {
    int height;
    int width;
} EdgeblendOutputCell;

typedef struct _EdgeblendOutputGrid {
    int rows;
    int cols;
    int blend;
} EdgeblendOutputGrid;

typedef struct _EdgeblendOutputBlendFunc {
    double a,b,c; //a*x^2 + b*x + c
} EdgeblendOutputBlendFunc;

typedef struct _EdgeblendOutputScreen {
    EdgeblendOutputBlendFunc left;
    EdgeblendOutputBlendFunc top;
    EdgeblendOutputBlendFunc right;
    EdgeblendOutputBlendFunc bottom;
} EdgeblendOutputScreen;

typedef struct _EdgeblendOutputConfig {
    EdgeblendOutputGrid      grid;
    EdgeblendOutputCell      cell;
    EdgeblendOutputScreen*   screens;
    char* imagepath;
} EdgeblendOutputConfig;


#ifdef	__cplusplus
extern "C" {
#endif

    EdgeblendOutputConfig * load_outputconfig(char * filepath, CompScreen *screen);


#ifdef	__cplusplus
}
#endif

#endif	/* _OUTPUT_CONFIG_H */

