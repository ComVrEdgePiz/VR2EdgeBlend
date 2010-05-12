/*
 * File:   tga.h
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
#ifndef _TGA_H
#define	_TGA_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <GL/glu.h>


typedef struct {
   char  idlength; //-> 0
   char  colourmaptype; //-> 0
   char  datatypecode; //-> 2
   short int colourmaporigin; //-> interessiert nicht, 
   short int colourmaplength; //-> brauchen keine color map, wie gif
   char  colourmapdepth;     // -> 0 0 0 0 0
   short int x_origin; //-> 0 0
   short int y_origin; //-> 0 0
   short width; //-> x x
   short height; //-> y y
   char  bitsperpixel; //-> 32
   char  imagedescriptor; //-> 0
} TGAHEADER;

typedef struct {
  TGAHEADER header; //(18 byte)
  unsigned char* data;
} TGA;

void writeTGA(TGA* tga, char* filename);

TGA* readTGA(char* filename);

TGA* texToTGA(GLfloat* tex, int width, int height);

void tgaToTex(TGA* img, GLfloat* tex);

unsigned char* getEmptyRaw(int width, int height);

#ifdef	__cplusplus
}
#endif

#endif	/* _TGA_H */

