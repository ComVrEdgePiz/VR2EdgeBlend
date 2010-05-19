/*
 * File:   blending.c
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
#include "blending.h"
#include "tga.h"
#include <stdlib.h>

/*
#define WHITE 0.0
#define BLACK 1.0
//blend linear between function and mask border
#define BLEND_LINEAR(r, y) ((r<=y)?BLACK:(r<0.9 && r > -0.9)?BLACK:((BLACK*y)/r))
#define INV_BLEND_LINEAR(r, y) (BLACK - BLEND_LINEAR(r, y))
//#define CLIP_TO_BYTE(c) ((c<BLACK)?BLACK:(c>WHITE)?WHITE:c)

//assumes that x-axis is the current spoted border
// and y-axis is at x=0
// returns the value to be substracted from the default color
GLfloat
blendFunct(int x, int y, double a, double b, double c) {
 return
  (a==0.0)
  ?(b==0.0)
    ?(c==0.0)
      ? WHITE
      : INV_BLEND_LINEAR(c, y)//c
    : INV_BLEND_LINEAR(b*x+c, y)//b*x+c
  :(b==0.0)
    ? INV_BLEND_LINEAR(a*x*x+c, y)//a*x^2+c
    : INV_BLEND_LINEAR(a*x*x+b*x+c, y)//a*x^2+b*x+c
  ;
}*/

double
fx(int x, double a, double b, double c) {
 return
  (a==0.0)
  ?(b==0.0)
    ? c
    : b*x + c
  :(b==0.0)
    ? a*x*x + c
    : a*x*x + b*x + c
  ;
}


Bool generateBlendingTexture(edgeblendScreen *screen)
{
    int displayWidth, displayHeight, cellWidth, cellHeight;
    int pixelArraySize;
    int i,j, arrayPos, posX,posY, py;
    int colorspace = 4;
    int output = 0, outputRow;
    div_t divHoriz, divVert;
    GLfloat* pixel;
    GLuint texture;
    double y=0.0;
    EdgeblendOutputScreen cell;
    TGA* tga=0;
    Bool loadFromImage = FALSE;

    displayWidth    = screen->orginalWorkarea.orginalWidth;
    displayHeight   = screen->orginalWorkarea.orginalHeight;
    cellWidth       = screen->outputCfg->cell.width;
    cellHeight      = screen->outputCfg->cell.height;
//    blendingSize    = screen->outputCfg->grid.blend;
    pixelArraySize  = colorspace * displayWidth * displayHeight;
    pixel           = (GLfloat*) malloc(pixelArraySize * sizeof(GLfloat));

    compLogMessage ("blending->generateBlendingTexture: ", CompLogLevelInfo,
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
    
    if(screen->outputCfg->imagepath != NULL){
      FILE* f = fopen(screen->outputCfg->imagepath, "r");
      if(f != NULL){
        fclose(f);
      compLogMessage ("blending->generateBlendingTexture: ", CompLogLevelInfo,
                      "test loaded image file");
        loadFromImage = TRUE;
      }
    }
    
    if (loadFromImage) {
      compLogMessage ("blending->generateBlendingTexture: ", CompLogLevelInfo,
                      "loading image from: %s", screen->outputCfg->imagepath);
    
      tga = readTGA(screen->outputCfg->imagepath);

      if(tga)
        tgaToTex(tga, pixel);
      else // failed to load image
        loadFromImage = FALSE;
    }
    
    if(! loadFromImage) {
      compLogMessage ("blending->generateBlendingTexture: ", CompLogLevelInfo,
                      "generating image");
      //forloopings
      for (i=0; i < displayHeight; i++)
      {
          divVert = div(i, cellHeight);
          posY = divVert.rem;
          outputRow = divVert.quot * screen->outputCfg->grid.cols;
          
          for (j=0; j < displayWidth; j++)
          {
              divHoriz = div(j, cellWidth);
              posX = divHoriz.rem;
              //from left to right from top to bottom
              output = divHoriz.quot + outputRow;
              
              cell = screen->outputCfg->screens[output];

              arrayPos = (i * displayWidth + j) * colorspace;

              // left
              y = fx(posY, cell.left.a, cell.left.b, cell.left.c);
              if((posX - y) < 0) {
                pixel[arrayPos + 3] = (GLfloat) ((y - posX) / y);
              }
//              pixel[arrayPos + 3] = blendFunct(posY, posX, cell.left.a, cell.left.b, cell.left.c);
              // right
              y = fx(posY, cell.right.a, cell.right.b, cell.right.c);
              py = cellWidth - posX;
              if((py - y) < 0) {
                pixel[arrayPos + 3] = (GLfloat) ((y - py) / y);
              }
//              pixel[arrayPos + 3] = blendFunct(posY, cellWidth-posX, cell.right.a, cell.right.b, cell.right.c);

              // top
              y = fx(posX, cell.top.a, cell.top.b, cell.top.c);
              if((posY - y) < 0) {
                pixel[arrayPos + 3] += (GLfloat) ((y - posY) / y);
              }
//              pixel[arrayPos + 3] = blendFunct(posX, posY, cell.top.a, cell.top.b, cell.top.c);
              // bottom
              y = fx(posX, cell.bottom.a, cell.bottom.b, cell.bottom.c);
              py = cellHeight - posY;
              if((py - y) < 0) {
                pixel[arrayPos + 3] += (GLfloat) ((y - py) / y);
              }
              /*if ((posX - cell.left.c) < 0) { // left
                  pixel[arrayPos + 3] = (GLfloat) ((cell.left.c-posX) / (GLfloat)cell.left.c);
              } else if((posX + cell.right.c) >= cellWidth) { // right
                  pixel[arrayPos + 3] = (GLfloat) ((cell.right.c+posX-cellWidth) / (GLfloat)cell.right.c);
              }
              // TODO += is bad blending for grids, find a better one
              if((posY - cell.top.c) < 0) { // top
                  pixel[arrayPos + 3] += (GLfloat) ((cell.top.c-posY) / (GLfloat)cell.top.c);
              } else if((posY + cell.bottom.c) >= cellHeight) { // bottom
                  pixel[arrayPos + 3] += (GLfloat) ((cell.bottom.c+posY-cellHeight) / (GLfloat)cell.bottom.c);
              }*/

          }
      }

      //save texture
      tga = texToTGA(pixel, displayWidth, displayHeight);
      if(tga != NULL)
        writeTGA(tga, screen->outputCfg->imagepath);
        
    }
    
    if(pixel) {
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
    }
    
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
