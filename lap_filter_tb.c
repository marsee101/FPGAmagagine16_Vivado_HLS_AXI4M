// lap_filter_tb.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmp_header.h"

int laplacian_fil_soft(int x0y0, int x1y0, int x2y0, int x0y1, int x1y1, int x2y1, int x0y2, int x1y2, int x2y2);
int conv_rgb2y_soft(int rgb);
int lap_filter_axim(volatile int *cam_fb, volatile int *lap_fb);    // hardware
void laplacian_filter_soft(int *cam_fb, int *lap_fb, long width, long height); // software

int main()
{
    int *s, *h;
    long x, y;
    BITMAPFILEHEADER bmpfhr; // BMPファイルのファイルヘッダ(for Read)
    BITMAPINFOHEADER bmpihr; // BMPファイルのINFOヘッダ(for Read)
    FILE *fbmpr, *fbmpw;
    int *rd_bmp, *hw_lapd, *sw_lapd;
    int blue, green, red;
    char blue_c, green_c, red_c;

    if ((fbmpr = fopen("test.bmp", "rb")) == NULL){ // test.bmp をオープン
        fprintf(stderr, "Can't open test.bmp by binary read mode\n");
        exit(1);
    }
    // bmpヘッダの読み出し
    fread(&bmpfhr.bfType, sizeof(char), 2, fbmpr);
    fread(&bmpfhr.bfSize, sizeof(long), 1, fbmpr);
    fread(&bmpfhr.bfReserved1, sizeof(short), 1, fbmpr);
    fread(&bmpfhr.bfReserved2, sizeof(short), 1, fbmpr);
    fread(&bmpfhr.bfOffBits, sizeof(long), 1, fbmpr);
    fread(&bmpihr, sizeof(BITMAPINFOHEADER), 1, fbmpr);

    // ピクセルを入れるメモリをアロケートする
    if ((rd_bmp =(int *)malloc(sizeof(int) * (bmpihr.biWidth * bmpihr.biHeight))) == NULL){
        fprintf(stderr, "Can't allocate rd_bmp memory\n");
        exit(1);
    }
    if ((hw_lapd =(int *)malloc(sizeof(int) * (bmpihr.biWidth * bmpihr.biHeight))) == NULL){
        fprintf(stderr, "Can't allocate hw_lapd memory\n");
        exit(1);
    }
    if ((sw_lapd =(int *)malloc(sizeof(int) * (bmpihr.biWidth * bmpihr.biHeight))) == NULL){
        fprintf(stderr, "Can't allocate sw_lapd memory\n");
        exit(1);
    }

    // rd_bmp にBMPのピクセルを代入。その際に、行を逆転する必要がある
    for (y=0; y<bmpihr.biHeight; y++){
        for (x=0; x<bmpihr.biWidth; x++){
            blue = fgetc(fbmpr);
            green = fgetc(fbmpr);
            red = fgetc(fbmpr);
            rd_bmp[((bmpihr.biHeight-1)-y)*bmpihr.biWidth+x] = (blue & 0xff) | ((green & 0xff)<<8) | ((red & 0xff)<<16);
        }
    }
    fclose(fbmpr);

    lap_filter_axim((int *)rd_bmp, (int *)hw_lapd);    // ハードウェアのラプラシアン・フィルタ
    laplacian_filter_soft(rd_bmp, sw_lapd, bmpihr.biWidth, bmpihr.biHeight);    // ソフトウェアのラプラシアン・フィルタ

    // ハードウェアとソフトウェアのラプラシアン・フィルタの値のチェック
    for (y=0, h=hw_lapd, s=sw_lapd; y<bmpihr.biHeight; y++){
        for (x=0; x<bmpihr.biWidth; x++){
            if (*h != *s){
                printf("ERROR HW and SW results mismatch x = %ld, y = %ld, HW = %d, SW = %d\n", x, y, *h, *s);
                return(1);
            } else {
                h++;
                s++;
            }
        }
    }
    printf("Success HW and SW results match\n");

    // ハードウェアのラプラシアンフィルタの結果を temp_lap.bmp へ出力する
    if ((fbmpw=fopen("test_lap.bmp", "wb")) == NULL){
        fprintf(stderr, "Can't open temp_lap.bmp by binary write mode\n");
        exit(1);
    }
    // BMPファイルヘッダの書き込み
    fwrite(&bmpfhr.bfType, sizeof(char), 2, fbmpw);
    fwrite(&bmpfhr.bfSize, sizeof(long), 1, fbmpw);
    fwrite(&bmpfhr.bfReserved1, sizeof(short), 1, fbmpw);
    fwrite(&bmpfhr.bfReserved2, sizeof(short), 1, fbmpw);
    fwrite(&bmpfhr.bfOffBits, sizeof(long), 1, fbmpw);
    fwrite(&bmpihr, sizeof(BITMAPINFOHEADER), 1, fbmpw);

    // RGB データの書き込み、逆順にする
    for (y=0; y<bmpihr.biHeight; y++){
        for (x=0; x<bmpihr.biWidth; x++){
            blue = hw_lapd[((bmpihr.biHeight-1)-y)*bmpihr.biWidth+x] & 0xff;
            green = (hw_lapd[((bmpihr.biHeight-1)-y)*bmpihr.biWidth+x] >> 8) & 0xff;
            red = (hw_lapd[((bmpihr.biHeight-1)-y)*bmpihr.biWidth+x]>>16) & 0xff;

            fputc(blue, fbmpw);
            fputc(green, fbmpw);
            fputc(red, fbmpw);
        }
    }
    fclose(fbmpw);
    free(rd_bmp);
    free(hw_lapd);
    free(sw_lapd);

    return(0);
}

void laplacian_filter_soft(int *cam_fb, int *lap_fb, long width, long height)
{
    int **line_buf;
    int *lap_buf;
    int x, y, i;
    int lap_fil_val;
    int a, b;
    int fl, sl, tl;

    // line_buf の1次元目の配列をアロケートする
    if ((line_buf =(int **)malloc(sizeof(int *) * 3)) == NULL){
        fprintf(stderr, "Can't allocate line_buf[3][]\n");
        exit(1);
    }

    // メモリをアロケートする
    for (i=0; i<3; i++){
        if ((line_buf[i]=(int *)malloc(sizeof(int) * width)) == NULL){
            fprintf(stderr, "Can't allocate line_buf[%d]\n", i);
            exit(1);
        }
    }

    if ((lap_buf=(int *)malloc(sizeof(int) * (width))) == NULL){
        fprintf(stderr, "Can't allocate lap_buf memory\n");
        exit(1);
    }

    // RGB値をY（輝度成分）のみに変換し、ラプラシアンフィルタを掛けた。
    for (y=0; y<height; y++){
        for (x=0; x<width; x++){
            if (y==0 || y==height-1){ // 縦の境界の時の値は0とする
                lap_fil_val = 0;
            }else if (x==0 || x==width-1){ // 横の境界の時も値は0とする
                lap_fil_val = 0;
            }else{
                if (y == 1 && x == 1){ // 最初のラインの最初のピクセルでは2ライン分の画素を読み出す
                    for (a=0; a<2; a++){ // 2ライン分
                        for (b=0; b<width; b++){ // ライン
                            line_buf[a][b] = cam_fb[(a*width)+b];
                            line_buf[a][b] = conv_rgb2y_soft(line_buf[a][b]);
                        }
                    }
                }
                if (x == 1) {    // ラインの最初なので、2つのピクセルを読み込む
                    for (b=0; b<2; b++){ // ライン
                        line_buf[(y+1)%3][b] = cam_fb[((y+1)*width)+b];
                        // (y+1)%3 は、使用済みのラインがに読み込む、y=2 の時 line[0], y=3の時 line[1], y=4の時 line[2]
                        line_buf[(y+1)%3][b] = conv_rgb2y_soft(line_buf[(y+1)%3][b]);
                    }
                }

                // 1つのピクセルを読み込みながらラプラシアン・フィルタを実行する
                line_buf[(y+1)%3][x+1] = cam_fb[((y+1)*width)+(x+1)];
                // (y+1)%3 は、使用済みのラインがに読み込む、y=2 の時 line[0], y=3の時 line[1], y=4の時 line[2]
                line_buf[(y+1)%3][x+1] = conv_rgb2y_soft(line_buf[(y+1)%3][x+1]);

                fl = (y-1)%3;    // 最初のライン, y=1 012, y=2 120, y=3 201, y=4 012
                sl = y%3;        // 2番めのライン
                tl = (y+1)%3;    // 3番目のライン
                lap_fil_val = laplacian_fil_soft(line_buf[fl][x-1], line_buf[fl][x], line_buf[fl][x+1], line_buf[sl][x-1], line_buf[sl][x], line_buf[sl][x+1], line_buf[tl][x-1], line_buf[tl][x], line_buf[tl][x+1]);
            }
            // ラプラシアンフィルタ・データの書き込み
            lap_fb[(y*width)+x] = (lap_fil_val<<16)+(lap_fil_val<<8)+lap_fil_val ;
        }
    }
    free(lap_buf);
    for (i=0; i<3; i++)
        free(line_buf[i]);
    free(line_buf);
}

// RGBからYへの変換
// RGBのフォーマットは、{8'd0, R(8bits), G(8bits), B(8bits)}, 1pixel = 32bits
// 輝度信号Yのみに変換する。変換式は、Y =  0.299R + 0.587G + 0.114B
// "YUVフォーマット及び YUV<->RGB変換"を参考にした。http://vision.kuee.kyoto-u.ac.jp/~hiroaki/firewire/yuv.html
//　2013/09/27 : float を止めて、すべてint にした
int conv_rgb2y_soft(int rgb){
    int r, g, b, y_f;
    int y;

    b = rgb & 0xff;
    g = (rgb>>8) & 0xff;
    r = (rgb>>16) & 0xff;

    y_f = 77*r + 150*g + 29*b; //y_f = 0.299*r + 0.587*g + 0.114*b;の係数に256倍した
    y = y_f >> 8; // 256で割る

    return(y);
}

// ラプラシアンフィルタ
// x0y0 x1y0 x2y0 -1 -1 -1
// x0y1 x1y1 x2y1 -1  8 -1
// x0y2 x1y2 x2y2 -1 -1 -1
int laplacian_fil_soft(int x0y0, int x1y0, int x2y0, int x0y1, int x1y1, int x2y1, int x0y2, int x1y2, int x2y2)
{
    int y;

    y = -x0y0 -x1y0 -x2y0 -x0y1 +8*x1y1 -x2y1 -x0y2 -x1y2 -x2y2;
    if (y<0)
        y = 0;
    else if (y>255)
        y = 255;
    return(y);
}
