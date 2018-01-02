// laplacian_filter5.c
// 2017/01/10 by marsee

#define HORIZONTAL_PIXEL_WIDTH    64
#define VERTICAL_PIXEL_WIDTH    48
//#define HORIZONTAL_PIXEL_WIDTH    800
//#define VERTICAL_PIXEL_WIDTH    600

#define ALL_PIXEL_VALUE    (HORIZONTAL_PIXEL_WIDTH*VERTICAL_PIXEL_WIDTH)

int laplacian_fil(int x0y0, int x1y0, int x2y0, int x0y1, int x1y1, int x2y1, int x0y2, int x1y2, int x2y2);
int conv_rgb2y(int rgb);

int lap_filter_axim(volatile int cam_fb[ALL_PIXEL_VALUE], volatile int lap_fb[ALL_PIXEL_VALUE])
{
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE m_axi depth=480000 port=lap_fb offset=slave
#pragma HLS INTERFACE m_axi depth=480000 port=cam_fb offset=slave

    int line_buf[3][HORIZONTAL_PIXEL_WIDTH];
#pragma HLS array_partition variable=line_buf block factor=3 dim=1
#pragma HLS resource variable=line_buf core=RAM_2P

    int lap_fil_val;
    int pix, lap;

    int pix_mat[3][3];
#pragma HLS array_partition variable=pix_mat complete

    for (int y=0; y<VERTICAL_PIXEL_WIDTH; y++){
        for (int x=0; x<HORIZONTAL_PIXEL_WIDTH; x++){
#pragma HLS PIPELINE II=1
            pix = cam_fb[y*HORIZONTAL_PIXEL_WIDTH+x];

            for (int k=0; k<3; k++){
                for (int m=0; m<2; m++){
                    pix_mat[k][m] = pix_mat[k][m+1];
                }
            }
            pix_mat[0][2] = line_buf[0][x];
            pix_mat[1][2] = line_buf[1][x];

            int y_val = conv_rgb2y(pix);
            pix_mat[2][2] = y_val;

            line_buf[0][x] = line_buf[1][x];    // 行の入れ替え
            line_buf[1][x] = y_val;

            lap_fil_val = laplacian_fil(    pix_mat[0][0], pix_mat[0][1], pix_mat[0][2],
                                        pix_mat[1][0], pix_mat[1][1], pix_mat[1][2],
                                        pix_mat[2][0], pix_mat[2][1], pix_mat[2][2]);
            lap = (lap_fil_val<<16)+(lap_fil_val<<8)+lap_fil_val; // RGB同じ値を入れる

            if (x<2 || y<2) // 最初の2行とその他の行の最初の2列は無効データなので0とする
                lap = 0;

            lap_fb[y*HORIZONTAL_PIXEL_WIDTH+x] = lap;
        }
    }

    return 0;
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
