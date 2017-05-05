// bmp_header.h
// BMP ファイルフォーマットから引用させて頂きました
// http://www.kk.iij4u.or.jp/~kondo/bmp/
//
// 2017/05/04 : takseiさんのご指摘によりintX_tを使った宣言に変更。takseiさんありがとうございました
//              変数の型のサイズの違いによってLinuxの６４ビット版では動作しなかったためです
//              http://marsee101.blog19.fc2.com/blog-entry-3354.html#comment2808
//

#include <stdio.h>
#include <stdint.h>

// BITMAPFILEHEADER 14bytes
typedef struct tagBITMAPFILEHEADER {
	uint16_t bfType; 
	uint32_t bfSize; 
	uint16_t bfReserved1; 
	uint16_t bfReserved2; 
	uint32_t bfOffBits;
} BITMAPFILEHEADER;

// BITMAPINFOHEADER 40bytes
typedef struct tagBITMAPINFOHEADER{
	uint32_t biSize; 
	int32_t biWidth; 
	int32_t biHeight; 
	uint16_t biPlanes; 
	uint16_t biBitCount; 
	uint32_t biCompression; 
	uint32_t biSizeImage; 
	int32_t biXPixPerMeter; 
	int32_t biYPixPerMeter; 
	uint32_t biClrUsed; 
	uint32_t biClrImporant;
} BITMAPINFOHEADER;

typedef struct BMP24bitsFORMAT {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
} BMP24FORMAT;
