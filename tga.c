#include "tga.h"
#include <stdlib.h>
#include <stdio.h>

char*
getEmptyRaw(int width, int height) {
  int size = width*height*4, i;
  char* img = malloc(size);

  if (img == NULL) {
    // print error message
    return 0;
  }

  for(i=0; i < size; i++) img[i] = 0;

  return img;
}

TGA*
readTGA (char* filename) {
  TGA tga;
  int bpp, size, i;
  unsigned char p[4];
  char* r;

  FILE* f = fopen(filename, "r");

  if (f == NULL) {
    // print error message, failed to open tga file
    return 0;
  }

  // read header
  tga.header.idlength = fgetc(f);
  tga.header.colourmaptype = fgetc(f);
  tga.header.datatypecode = fgetc(f);
  fread(&tga.header.colourmaporigin, 2, 1, f);
  fread(&tga.header.colourmaplength, 2, 1, f);
  tga.header.colourmapdepth = fgetc(f);
  fread(&tga.header.x_origin, 2, 1, f);
  fread(&tga.header.y_origin, 2, 1, f);
  fread(&tga.header.width, 2, 1, f);
  fread(&tga.header.height, 2, 1, f);
  tga.header.bitsperpixel = fgetc(f);
  tga.header.imagedescriptor = fgetc(f);

  //alloc image space
  tga.data = getEmptyRaw(tga.header.width, tga.header.height);
  
  if ( tga.header.datatypecode != 2 || 
     ( tga.header.bitsperpixel != 24 && tga.header.bitsperpixel != 32 ) ||
       tga.header.colourmaptype != 0 ||
       tga.data == NULL
     ) {
    // cant handle this image
    return 0;
  }

  // skip image id
  fseek(f, tga.header.idlength, SEEK_CUR);

  bpp = tga.header.bitsperpixel / 8;
  size = tga.header.width*tga.header.height;

  for(i=0, r=tga.data; i < size; i++, r+=4) {
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
  
  return &tga;
}

void
writeTGA (TGA img, char* filename) {
  FILE* f = fopen(filename, "w");

  if (f == NULL) {
    // print error message, failed to write tga file
    return ;
  }
 
  //print header
  char buf[] = { '0', '0', '2', '0'
               , '0', '0', '0', '0'
               , '0', '0', '0', '0'};
  fwrite(buf, 1, 12, f);
  //write little endian width/height
  putc((img.header.width & 0x00FF), f);
  putc((img.header.width & 0xFF00) / 256, f);
  putc((img.header.height & 0x00FF), f);
  putc((img.header.height & 0xFF00) / 256, f);
  //write last 2 header bytes
  putc(img.header.bitsperpixel, f);
  putc(0, f);

  //write image data
  fwrite(img.data, 1, sizeof(img.data), f);

  fclose(f);
}
