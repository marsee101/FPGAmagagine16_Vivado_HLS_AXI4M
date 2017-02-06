// laplacian_filter4.c

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
#pragma HLS array_partition variable=line_buf block factor=3 dim=1
#pragma HLS resource variable=line_buf core=RAM_2P

    int lap_buf[HORIZONTAL_PIXEL_WIDTH];
    int x, y;
    int lap_fil_val;
    int fl, sl, tl;
    int line_sel;
    int prev[3],current[3],next[3];    // 0->1ライン目, 1->2ライン目, 2->3ライン目, prev->1pixel前, current->現在, next->次pixel
#pragma HLS array_partition variable=prev complete dim=0
#pragma HLS array_partition variable=current complete dim=0
#pragma HLS array_partition variable=next complete dim=0

    // 最初に 2 行分 Read する
    Loop0: for (y=0; y<2; y++)
        memcpy(line_buf[y], (const int*)&cam_fb[y*(HORIZONTAL_PIXEL_WIDTH)], HORIZONTAL_PIXEL_WIDTH*sizeof(int));

    // RGB値をY（輝度成分）のみに変換し、ラプラシアンフィルタを掛けた。
    Loop1: for (y=2, line_sel=1; y<VERTICAL_PIXEL_WIDTH; y++){
        // 最初のライン, y=1 012, y=2 120, y=3 201, y=4 012
        switch(line_sel){
            case 1 :
                fl = 0; sl = 1; tl = 2;
                break;
            case 2 :
                fl = 1; sl = 2; tl = 0;
                break;
            case 3 :
                fl = 2; sl = 0; tl = 1;
                break;
            default :
                fl = 0; sl = 1; tl = 2;
        }

        // 最初のラインではないので、1ラインだけ読み込む。すでに他の2ラインは読み込まれている
        memcpy(line_buf[tl], (const int*)&cam_fb[y*(HORIZONTAL_PIXEL_WIDTH)], HORIZONTAL_PIXEL_WIDTH*sizeof(int));
        current[0] = conv_rgb2y(line_buf[fl][0]);
        current[1] = conv_rgb2y(line_buf[sl][0]);
        current[2] = conv_rgb2y(line_buf[tl][0]);
        next[0] = conv_rgb2y(line_buf[fl][1]);
        next[1] = conv_rgb2y(line_buf[sl][1]);
        next[2] = conv_rgb2y(line_buf[tl][1]);

        lap_buf[0] = 0;
        lap_buf[HORIZONTAL_PIXEL_WIDTH-1] = 0;

        Loop2: for (x = 1; x < HORIZONTAL_PIXEL_WIDTH-1; x++){
#pragma HLS PIPELINE II=1
            prev[0] = current[0];
            current[0] = next[0];
            next[0] = conv_rgb2y(line_buf[fl][x+1]);

            prev[1] = current[1];
            current[1] = next[1];
            next[1] = conv_rgb2y(line_buf[sl][x+1]);

            prev[2] = current[2];
            current[2] = next[2];
            next[2] = conv_rgb2y(line_buf[tl][x+1]);

            lap_fil_val = laplacian_fil(prev[0], current[0], next[0],
                                        prev[1], current[1], next[1],
                                        prev[2], current[2], next[2]);

            lap_buf[x] = (lap_fil_val<<16)+(lap_fil_val<<8)+lap_fil_val; // RGB同じ値を入れる
        }
    	memcpy((int*)&lap_fb[(y-1)*(HORIZONTAL_PIXEL_WIDTH)], (const int*)lap_buf, HORIZONTAL_PIXEL_WIDTH*sizeof(int));

        line_sel++;
        if (line_sel > 3){
            line_sel = 1;
        }
    }

    Loop3: for (x=0; x<HORIZONTAL_PIXEL_WIDTH; x++)
#pragma HLS PIPELINE II=1
        lap_buf[x] = 0;

    memcpy(&((int *)lap_fb)[0], (const int*)lap_buf, HORIZONTAL_PIXEL_WIDTH*sizeof(int)); // 最初の行を 0 クリア
    memcpy(&((int *)lap_fb)[(VERTICAL_PIXEL_WIDTH-1)*(HORIZONTAL_PIXEL_WIDTH)], (const int*)lap_buf, HORIZONTAL_PIXEL_WIDTH*sizeof(int)); // 最後の行を 0 クリア

    return(0);
}

// RGBからYへの変換
// RGBのフォーマットは、{8'd0, R(8bits), G(8bits), B(8bits)}, 1pixel = 32bits
// 輝度信号Yのみに変換する。変換式は、Y =  0.299R + 0.587G + 0.114B
// "YUVフォーマット及び YUV<->RGB変換"を参考にした。http://vision.kuee.kyoto-u.ac.jp/~hiroaki/firewire/yuv.html
//　2013/09/27 : line[sl]oat を止めて、すべてint にした
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
