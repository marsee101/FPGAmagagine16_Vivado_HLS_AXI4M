// laplacian_filter0.c

#include <stdio.h>
#include <string.h>

#define H_PIXELS    64
#define V_PIXELS    48

#define ALL_PIXEL_VALUE    (H_PIXELS*V_PIXELS)

int laplacian_fil(int x0y0, int x1y0, int x2y0, int x0y1, int x1y1, int x2y1, int x0y2, int x1y2, int x2y2);
int conv_rgb2y(int rgb);

int lap_filter_axim(volatile int *cam_fb, volatile int *lap_fb)
{
    #pragma HLS INTERFACE s_axilite port=return

#pragma HLS INTERFACE m_axi depth=3072 port=cam_fb offset=slave bundle=cam_fb
#pragma HLS INTERFACE m_axi depth=3072 port=lap_fb offset=slave bundle=lap_fb

    int x, y, i, j;
    int lap_fil_val;
    int pix[3][3];

    // RGB値をY（輝度成分）のみに変換し、ラプラシアンフィルタを掛けた。
    Loop0: for (y=0; y<V_PIXELS; y++){
        Loop1: for (x=0; x<H_PIXELS; x++){
            if (y==0 || y==V_PIXELS-1){ // 縦の境界の時の値は0とする
                lap_fil_val = 0;
            }else if (x==0 || x==H_PIXELS-1){ // 横の境界の時も値は0とする
                lap_fil_val = 0;
            }else{
            	Loop2: for (i=0; i<3; i++){
            		Loop3: for (j=0; j<3; j++){
            			pix[i][j] = conv_rgb2y(cam_fb[(y+i-1)*H_PIXELS+(x+j-1)]);
            		}
            	}
                lap_fil_val = laplacian_fil(
                		pix[0][0], pix[0][1], pix[0][2],
						pix[1][0], pix[1][1], pix[1][2],
						pix[2][0], pix[2][1], pix[2][2]);
            }
            // ラプラシアンフィルタ・データの書き込み
            lap_fb[(y*H_PIXELS)+x] = (lap_fil_val<<16)+(lap_fil_val<<8)+lap_fil_val ;
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
