/*
 * File:   tga.c
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
#include "tga.h"
#include <stdlib.h>
#include <stdio.h>

unsigned char*
getEmptyRaw(int width, int height) {
  int size = width*height*4, i;
  unsigned char* img = malloc(size);

  if (img == NULL) {
    // print error message
    return 0;
  }

  for(i=0; i < size; i++) img[i] = 0;

  return img;
}

TGA*
readTGA (char* filename) {
  TGA* tga=0;
  int bpp, size, i;
  unsigned char p[4];
  unsigned char* r;

  FILE* f = fopen(filename, "r");
  tga=malloc(sizeof(TGA));
  if (f == NULL || tga == NULL) {
    // print error message, failed to open tga file
    return 0;
  }

  // read header
  tga->header.idlength = fgetc(f);
  tga->header.colourmaptype = fgetc(f);
  tga->header.datatypecode = fgetc(f);
  fread(&tga->header.colourmaporigin, 2, 1, f);
  fread(&tga->header.colourmaplength, 2, 1, f);
  tga->header.colourmapdepth = fgetc(f);
  fread(&tga->header.x_origin, 2, 1, f);
  fread(&tga->header.y_origin, 2, 1, f);
  fread(&tga->header.width, 2, 1, f);
  fread(&tga->header.height, 2, 1, f);
  tga->header.bitsperpixel = fgetc(f);
  tga->header.imagedescriptor = fgetc(f);

  //alloc image space
  tga->data = getEmptyRaw(tga->header.width, tga->header.height);
  
  if ( tga->header.datatypecode != 2 || 
     ( tga->header.bitsperpixel != 24 && tga->header.bitsperpixel != 32 ) ||
       tga->header.colourmaptype != 0 ||
       tga->data == NULL
     ) {
    // cant handle this image
    return 0;
  }

  // skip image id
  fseek(f, tga->header.idlength, SEEK_CUR);

  bpp = tga->header.bitsperpixel / 8;
  size = tga->header.width*tga->header.height;

  for(i=0, r=tga->data; i < size; i++, r+=4) {
    if (fread(p, 1, bpp, f) != bpp) {
      // unexpected eof
      return 0;
    }

    r[0] = p[2];
    r[1] = p[1];
    r[2] = p[0];
    if (bpp == 4) {
      r[3] = p[3];
    } else { // == 3
      r[3] = 0;
    }
  }

  fclose(f);
  
  return tga;
}

void
writeTGA (TGA* img, char* filename) {
  FILE* f = fopen(filename, "wb");

  if (f == NULL) {
    // print error message, failed to write tga file
    return ;
  }
 
  //print header
  unsigned char buf[] = { 0, 0, 2, 0 //uncompressed data, with no color table
                        , 0, 0, 0, 0
                        , 0, 0, 0, 0};
  fwrite(buf, 1, 12, f);
  //write little endian width/height
  putc((img->header.width & 0x00FF), f);
  putc((img->header.width & 0xFF00) / 256, f);
  putc((img->header.height & 0x00FF), f);
  putc((img->header.height & 0xFF00) / 256, f);
  //write last 2 header bytes
  putc(32, f); // should be 32
  putc(8, f); // 8 alpha bits

  //write image data
  fwrite(img->data, 1, img->header.width * img->header.height * 4, f);

  fclose(f);
  if(img){
    if(img->data) free(img->data);
    free(img);
  }
}

TGA*
texToTGA(GLfloat* tex, int width, int height) {
  TGA* img = malloc(sizeof(TGA));
  int size = width*height*4, i;
  
  if(img==NULL) return 0;
  
  img->data = malloc(size);
  
  img->header.width = width;
  img->header.height = height;
  img->header.bitsperpixel = 4;
  
  if(img->data ==NULL) return 0;

#define FTOS(f) ((int) (f*255.0))
#define CLIP(i) ((i<0)?0:(i>255)?255:i)
  for(i=0; i < size; i+=4){
    img->data[i]   = CLIP(FTOS(tex[i+2]));
    img->data[i+1] = CLIP(FTOS(tex[i+1]));
    img->data[i+2] = CLIP(FTOS(tex[i]));
    img->data[i+3] = CLIP(FTOS(tex[i+3]));
  }
 
  return img;
}

void
tgaToTex(TGA* img, GLfloat* tex) {
  if (img && tex) {
    int size = img->header.width * img->header.height * 4;
    int i;
    for(i=0; i < size; i++) {
      tex[i] = (GLfloat) (((double)img->data[i]) / 255.0);
    }    
  }
}
