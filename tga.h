#ifndef _TGA_H
#define	_TGA_H

#ifdef	__cplusplus
extern "C" {
#endif


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
  char* data;
} TGA;

void writeTGA(TGA tga, char* filename);

TGA* readTGA(char* filename);

#ifdef	__cplusplus
}
#endif

#endif	/* _TGA_H */

