// bmp_header.h
// 2015/07/17 by Masaaki Ono
//
// BMP ファイルフォーマットから引用
// http://www.kk.iij4u.or.jp/~kondo/bmp/
//

#include <stdio.h>

#pragma once

#ifndef BMP_HEADER_H_
#define BMP_HEADER_H_
// TODO: プログラムに必要な追加ヘッダーをここで参照してください。
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

#endif /* BMP_HEADER_H_ */
