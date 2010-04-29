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
    int row     = 0;
    int rposx,rposy,cposx,cposy;
    //compLogMessage ("output::build", CompLogLevelInfo,
    //                "cell: %d x %d (overlap: %d) | rows: %d, cols: %d",
    //                width, height, overlap, rows, cols);
   
    while(row < rows) {
        col  = cols;
        irow = rows - (row+1);
        while(col--) {
            rposx = col * width;
            rposy = (row+1) * height;
            cposx = (col * width ) - (col * overlap);
            cposy = (irow * height) + (row * overlap);
            //compLogMessage ("\toutput::build", CompLogLevelInfo, "row: %d (^-1: %d), col: %d", row,irow, col);
            if (rposx != cposx || rposy != cposy) {
                glRasterPos2i( rposx, rposy);
                     //compLogMessage ("\toutput::build", CompLogLevelInfo, "rpos %d x %d", col * width, irow*height);
                glCopyPixels( cposx
                            , cposy
                            , width
                            , height
                            , GL_COLOR);

                //compLogMessage ("\toutput::build", CompLogLevelInfo, "rpos: %d/%d, cpos: %d/%d", rposx, rposy, cposx, cposy);
            }
        }
        row++;
    }
    //glRasterPos2i( 0, 2048);
    //glCopyPixels ( 0, 1024, width, height-100, GL_COLOR);
    //glCopyPixels ( 0, 100, width, height, GL_COLOR);

    glLineWidth(4.0);
     glColor3f(.3, .3, .3);

     glBegin(GL_LINES);
        glVertex2i(0, rows*height);
        glVertex2i(cols * width, 0);
     glEnd();

}
