/*
 * File:   output.c
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
#include "output.h"
#include <assert.h>
/**
 * Copies the rects from the workarea onto the rects of the outputdevs
 * @PARAM edgeblendScreen   *ebs     - private edgeblend plugin screen data
 */
void buildOutput(edgeblendScreen * ebs)
{
    int irow,col;
    int overlap = ebs->outputCfg->grid.blend;
    int width   = ebs->outputCfg->cell.width;
    int height  = ebs->outputCfg->cell.height;
    int cols    = ebs->outputCfg->grid.cols;
    int rows    = ebs->outputCfg->grid.rows;
    int row     = rows;
    int rposx,rposy,cposx,cposy;
   
    while(row--) {
        col  = cols;
        irow = rows - (row+1);
        while(col--) {
            rposx = col * width;
            rposy = (row+1) * height;
            cposx = (col * width ) - (col * overlap);
            cposy = (irow * height) + (row * overlap);          

            if (rposx != 0 || rposy != height)
            {
                glRasterPos2i( rposx, rposy);
                glCopyPixels( cposx, cposy, width, height, GL_COLOR);
            }
        }
    }
    //glLineWidth(4.0); glColor3f(.3, .3, .3); glBegin(GL_LINES); glVertex2i(0, rows*height); glVertex2i(cols * width, 0); glEnd();
    //lLineWidth(4.0); glColor3f(.9, .3, .9); glBegin(GL_LINES); glVertex2i(0, 0); glVertex2i(cols * width, rows*height); glEnd();

}
