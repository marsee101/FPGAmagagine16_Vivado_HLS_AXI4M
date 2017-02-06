// laplacian_filter1.c

#include <stdio.h>
#include <string.h>

#define HORIZONTAL_PIXEL_WIDTH    64
#define VERTICAL_PIXEL_WIDTH    48

#define ALL_PIXEL_VALUE    (HORIZONTAL_PIXEL_WIDTH*VERTICAL_PIXEL_WIDTH)

int laplacian_fil(int x0y0, int x1y0, int x2y0, int x0y1, int x1y1, int x2y1, int x0y2, int x1y2, int x2y2);
int conv_rgb2y(int rgb);

int lap_filter_axim(volatile int *cam_fb, volatile int *lap_fb)
{
    #pragma HLS INTERFACE s_axilite port=return

#pragma HLS INTERFACE m_axi depth=3072 port=cam_fb offset=slave bundle=cam_fb
#pragma HLS INTERFACE m_axi depth=3072 port=lap_fb offset=slave bundle=lap_fb

    int line_buf[3][HORIZONTAL_PIXEL_WIDTH];
    int x, y;
    int lap_fil_val;
    int a, b;
    int fl, sl, tl;

    // RGB値をY（輝度成分）のみに変換し、ラプラシアンフィルタを掛けた。
    Loop0: for (y=0; y<VERTICAL_PIXEL_WIDTH; y++){
        Loop1: for (x=0; x<HORIZONTAL_PIXEL_WIDTH; x++){
            if (y==0 || y==VERTICAL_PIXEL_WIDTH-1){ // 縦の境界の時の値は0とする
                lap_fil_val = 0;
            }else if (x==0 || x==HORIZONTAL_PIXEL_WIDTH-1){ // 横の境界の時も値は0とする
                lap_fil_val = 0;
            }else{
                if (y == 1 && x == 1){ // 最初のラインの最初のピクセルでは2ライン分の画素を読み出す
                    Loop3: for (a=0; a<2; a++){ // 2ライン分
                        Loop4: for (b=0; b<HORIZONTAL_PIXEL_WIDTH; b++){ // ライン
                            line_buf[a][b] = cam_fb[(a*HORIZONTAL_PIXEL_WIDTH)+b];
                            line_buf[a][b] = conv_rgb2y(line_buf[a][b]);
                        }
                    }
                }
                if (x == 1) {    // ラインの最初なので、2つのピクセルを読み込む
                    Loop5: for (b=0; b<2; b++){ // ライン
                        line_buf[(y+1)%3][b] = cam_fb[((y+1)*HORIZONTAL_PIXEL_WIDTH)+b];
                        // (y+1)%3 は、使用済みのラインがに読み込む、y=2 の時 line[0], y=3の時 line[1], y=4の時 line[2]
                        line_buf[(y+1)%3][b] = conv_rgb2y(line_buf[(y+1)%3][b]);
                    }
                }

                // 1つのピクセルを読み込みながらラプラシアン・フィルタを実行する
                line_buf[(y+1)%3][x+1] = cam_fb[((y+1)*HORIZONTAL_PIXEL_WIDTH)+(x+1)];
                // (y+1)%3 は、使用済みのラインがに読み込む、y=2 の時 line[0], y=3の時 line[1], y=4の時 line[2]
                line_buf[(y+1)%3][x+1] = conv_rgb2y(line_buf[(y+1)%3][x+1]);

                fl = (y-1)%3;    // 最初のライン, y=1 012, y=2 120, y=3 201, y=4 012
                sl = y%3;        // 2番めのライン
                tl = (y+1)%3;    // 3番目のライン
                lap_fil_val = laplacian_fil(line_buf[fl][x-1], line_buf[fl][x], line_buf[fl][x+1], line_buf[sl][x-1], line_buf[sl][x], line_buf[sl][x+1], line_buf[tl][x-1], line_buf[tl][x], line_buf[tl][x+1]);
            }
            // ラプラシアンフィルタ・データの書き込み
            lap_fb[(y*HORIZONTAL_PIXEL_WIDTH)+x] = (lap_fil_val<<16)+(lap_fil_val<<8)+lap_fil_val ;
            // printf("x = %d  y = %d", x, y);
        }
     }
     return(0);
}

// RGBからYへの変換
// RGBのフォーマットは、{8'd0, R(8bits), G(8bits), B(8bits)}, 1pixel = 32bits
// 輝度信号Yのみに変換する。変換式は、Y =  0.299R + 0.587G + 0.114B
// "YUVフォーマット及び YUV<->RGB変換"を参考にした。http://vision.kuee.kyoto-u.ac.jp/~hiroaki/firewire/yuv.html
//　2013/09/27 : float を止めて、すべてint にした
int conv_rgb2y(int rgb){
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
int laplacian_fil(int x0y0, int x1y0, int x2y0, int x0y1, int x1y1, int x2y1, int x0y2, int x1y2, int x2y2)
{
    int y;

    y = -x0y0 -x1y0 -x2y0 -x0y1 +8*x1y1 -x2y1 -x0y2 -x1y2 -x2y2;
    if (y<0)
        y = 0;
    else if (y>255)
        y = 255;
    return(y);
}
