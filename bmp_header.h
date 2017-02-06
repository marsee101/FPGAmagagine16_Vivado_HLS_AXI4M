// bmp_header.h
// BMP ファイルフォーマットから引用させて頂きました
// http://www.kk.iij4u.or.jp/~kondo/bmp/
//

#include <stdio.h>

// BITMAPFILEHEADER 14bytes
typedef struct tagBITMAPFILEHEADER {
  unsigned short bfType;
  unsigned long  bfSize;
  unsigned short bfReserved1;
  unsigned short bfReserved2;
  unsigned long  bfOffBits;
} BITMAPFILEHEADER;

// BITMAPINFOHEADER 40bytes
typedef struct tagBITMAPINFOHEADER{
    unsigned long  biSize;
    long           biWidth;
    long           biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned long  biCompression;
    unsigned long  biSizeImage;
    long           biXPixPerMeter;
    long           biYPixPerMeter;
    unsigned long  biClrUsed;
    unsigned long  biClrImporant;
} BITMAPINFOHEADER;

typedef struct BMP24bitsFORMAT {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
} BMP24FORMAT;
