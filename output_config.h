/* 
 * File:   output_config.h
 * Author: flatline
 *
 * Created on December 20, 2009, 5:11 PM
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
    char* imagepath;
    EdgeblendOutputBlendFunc left;
    EdgeblendOutputBlendFunc top;
    EdgeblendOutputBlendFunc right;
    EdgeblendOutputBlendFunc bottom;
} EdgeblendOutputScreen;

typedef struct _EdgeblendOutputConfig {
    EdgeblendOutputGrid      grid;
    EdgeblendOutputCell      cell;
    EdgeblendOutputScreen*   screens;
} EdgeblendOutputConfig;


#ifdef	__cplusplus
extern "C" {
#endif

    EdgeblendOutputConfig * load_outputconfig(char * filepath, CompScreen *screen);


#ifdef	__cplusplus
}
#endif

#endif	/* _OUTPUT_CONFIG_H */

