#include "output.h"
#include <assert.h>

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
   //compLogMessage (" ", CompLogLevelInfo,"");
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
                //compLogMessage (" ", CompLogLevelInfo,"%d  %d/%d %d/%d", glGetError(), rposx, rposy, cposx, cposy);
                glRasterPos2i( rposx, rposy);
                glCopyPixels( cposx, cposy, width, height, GL_COLOR);

            }
        }
    }
    glLineWidth(4.0); glColor3f(.3, .3, .3); glBegin(GL_LINES); glVertex2i(0, rows*height); glVertex2i(cols * width, 0); glEnd();

}
