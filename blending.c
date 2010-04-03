#include "blending.h"
#include <stdlib.h>

Bool generateBlendingTexture(edgeblendScreen *screen)
{
    int displayWidth, displayHeight, cellWidth, cellHeight;
    int pixelArraySize, blendingSize;
    int i,j,c, arrayPos, posX,posY;
    int needsBlendingLeft,needsBlendingRight;
    int colorspace = 4;
    int output = 0;
    div_t divStruct;
    GLfloat* pixel;
    GLuint texture;

    displayWidth    = screen->orginalWorkarea.orginalWidth;
    displayHeight   = screen->orginalWorkarea.orginalHeight;
    cellWidth       = screen->outputCfg->cell.width;
    cellHeight      = screen->outputCfg->cell.height;
    blendingSize    = screen->outputCfg->grid.blend;
    pixelArraySize  = colorspace * displayWidth * displayHeight;
    pixel           = (GLfloat*) malloc(pixelArraySize * sizeof(GLfloat));

    compLogMessage ("blending->generateBlendingTexture: ", CompLogLevelError,
                    "pixel-array: %d (%d * %d * %d)", pixelArraySize,
                    displayWidth, displayHeight, colorspace);


    if (!pixel) {
        compLogMessage ("blending->generateBlendingTexture: ", CompLogLevelError, "failed to malloc pixel-array of size: ", pixelArraySize);
        return FALSE;
    }
    memset((void*)pixel, 0, pixelArraySize * sizeof(GLfloat));
    
    

    glGenTextures(1, &texture);
    screen->testtex = texture;
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    //forloopings


    for (i=0; i < displayHeight; i++)
    {
        for (j=0; j < displayWidth; j++)
        {
            divStruct = div(j, cellWidth);
            posX = divStruct.rem;
            posY = i % cellHeight;
            //from left to right from top to bottom
            output = divStruct.quot + 0 * divStruct.quot;

            needsBlendingLeft   = screen->outputCfg->screens[output].left.a  && (posX - blendingSize <= 0);
            needsBlendingRight  = screen->outputCfg->screens[output].right.a && (posX + blendingSize >= cellWidth);


            if (needsBlendingLeft) {
                arrayPos = (i * displayWidth + j) * colorspace;
                pixel[arrayPos + 3] = (GLfloat) ((blendingSize-posX) / (GLfloat)blendingSize);
            } else if(needsBlendingRight) {
                arrayPos = (i * displayWidth + j) * colorspace;
                pixel[arrayPos + 3] = (GLfloat) ((blendingSize+posX-cellWidth) / (GLfloat)blendingSize);
            }

        }
    }
    
    glTexImage2D( GL_TEXTURE_RECTANGLE_ARB  //target
              , 0                //LOD
              , GL_RGBA          //internal format
              , displayWidth     //width
              , displayHeight    //height
              , 0                //no border
              , GL_RGBA          //use 4 texture components per pixel
              , GL_FLOAT //texure components has 1 byte size
              , pixel            //image data
              );

    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
    free(pixel);
    
    return FALSE;
}

void blend(edgeblendScreen *screen)
{
    int width, height;
    width  = screen->orginalWorkarea.orginalWidth;
    height = screen->orginalWorkarea.orginalHeight;

        glEnable (GL_BLEND);
        glBindTexture (GL_TEXTURE_RECTANGLE_ARB, screen->testtex);
        glEnable (GL_TEXTURE_RECTANGLE_ARB);

        glBegin (GL_QUADS);
            glTexCoord2d (0, 0);            glVertex2f (0, 0);
            glTexCoord2d (0, height);       glVertex2f (0, height);
            glTexCoord2d (width, height);   glVertex2f (width, height);
            glTexCoord2d (width, 0);        glVertex2f (width, 0);
        glEnd ();

        glDisable (GL_BLEND);
        glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
        glDisable (GL_TEXTURE_RECTANGLE_ARB);
    glPopMatrix();
}

void freeBlendingTexture(edgeblendScreen *screen)
{
    glDeleteTextures(1, &screen->testtex);
}
