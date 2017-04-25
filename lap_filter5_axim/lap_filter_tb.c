// Testbench of laplacian_filter.c
// lap_filter_tb.c
// BMP�f�[�^���n�[�h�E�F�A�ƃ\�t�g�E�F�A�ŁA���v���V�A���E�t�B���^���|���āA������r����
// m_axi offset=slave version
// 2015/08/26 by marsee
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "bmp_header.h"

int laplacian_fil_soft(int x0y0, int x1y0, int x2y0, int x0y1, int x1y1, int x2y1, int x0y2, int x1y2, int x2y2);
int conv_rgb2y_soft(int rgb);
int lap_filter_axim(volatile int *cam_fb, volatile int *lap_fb);    // hardware
void laplacian_filter_soft(int *cam_fb, int *lap_fb, long width, long height); // software

int main()
{
    int *s, *h;
    long x, y;
    BITMAPFILEHEADER bmpfhr; // BMP�t�@�C���̃t�@�C���w�b�_(for Read)
    BITMAPINFOHEADER bmpihr; // BMP�t�@�C����INFO�w�b�_(for Read)
    FILE *fbmpr, *fbmpw;
    int *rd_bmp, *hw_lapd, *sw_lapd;
    int blue, green, red;
    char blue_c, green_c, red_c;
    struct timeval start_time_hw, end_time_hw;
    struct timeval start_time_sw, end_time_sw;

    if ((fbmpr = fopen("test.bmp", "rb")) == NULL){ // test.bmp ���I�[�v��
        fprintf(stderr, "Can't open test.bmp by binary read mode\n");
        exit(1);
    }
    // bmp�w�b�_�̓ǂݏo��
    fread(&bmpfhr.bfType, sizeof(char), 2, fbmpr);
    fread(&bmpfhr.bfSize, sizeof(long), 1, fbmpr);
    fread(&bmpfhr.bfReserved1, sizeof(short), 1, fbmpr);
    fread(&bmpfhr.bfReserved2, sizeof(short), 1, fbmpr);
    fread(&bmpfhr.bfOffBits, sizeof(long), 1, fbmpr);
    fread(&bmpihr, sizeof(BITMAPINFOHEADER), 1, fbmpr);

    // �s�N�Z�������郁�������A���P�[�g����
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

    // rd_bmp ��BMP�̃s�N�Z�������B���̍ۂɁA�s���t�]����K�v������
    for (y=0; y<bmpihr.biHeight; y++){
        for (x=0; x<bmpihr.biWidth; x++){
            blue = fgetc(fbmpr);
            green = fgetc(fbmpr);
            red = fgetc(fbmpr);
            rd_bmp[((bmpihr.biHeight-1)-y)*bmpihr.biWidth+x] = (blue & 0xff) | ((green & 0xff)<<8) | ((red & 0xff)<<16);
        }
    }
    fclose(fbmpr);

    lap_filter_axim((int *)rd_bmp, (int *)hw_lapd);    // �n�[�h�E�F�A�̃��v���V�A���E�t�B���^
    laplacian_filter_soft(rd_bmp, sw_lapd, bmpihr.biWidth, bmpihr.biHeight);    // �\�t�g�E�F�A�̃��v���V�A���E�t�B���^

    // �n�[�h�E�F�A�ƃ\�t�g�E�F�A�̃��v���V�A���E�t�B���^�̒l�̃`�F�b�N
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

    // �n�[�h�E�F�A�̃��v���V�A���t�B���^�̌��ʂ� test_lap.bmp �֏o�͂���
    if ((fbmpw=fopen("test_lap.bmp", "wb")) == NULL){
        fprintf(stderr, "Can't open test_lap.bmp by binary write mode\n");
        exit(1);
    }
    // BMP�t�@�C���w�b�_�̏�������
    fwrite(&bmpfhr.bfType, sizeof(char), 2, fbmpw);
    fwrite(&bmpfhr.bfSize, sizeof(long), 1, fbmpw);
    fwrite(&bmpfhr.bfReserved1, sizeof(short), 1, fbmpw);
    fwrite(&bmpfhr.bfReserved2, sizeof(short), 1, fbmpw);
    fwrite(&bmpfhr.bfOffBits, sizeof(long), 1, fbmpw);
    fwrite(&bmpihr, sizeof(BITMAPINFOHEADER), 1, fbmpw);

    // RGB �f�[�^�̏������݁A�t���ɂ���
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
    int line_buf[3][800];

    int lap_fil_val;
    int pix, lap;

    int pix_mat[3][3];

    for (int y=0; y<height; y++){
        for (int x=0; x<width; x++){
            pix = cam_fb[y*width+x];

            for (int k=0; k<3; k++){
                for (int m=0; m<2; m++){
                    pix_mat[k][m] = pix_mat[k][m+1];
                }
            }
            pix_mat[0][2] = line_buf[0][x];
            pix_mat[1][2] = line_buf[1][x];

            int y_val = conv_rgb2y(pix);
            pix_mat[2][2] = y_val;

            line_buf[0][x] = line_buf[1][x];    // �s�̓���ւ�
            line_buf[1][x] = y_val;

            lap_fil_val = laplacian_fil(    pix_mat[0][0], pix_mat[0][1], pix_mat[0][2],
                                        pix_mat[1][0], pix_mat[1][1], pix_mat[1][2],
                                        pix_mat[2][0], pix_mat[2][1], pix_mat[2][2]);
            lap = (lap_fil_val<<16)+(lap_fil_val<<8)+lap_fil_val; // RGB�����l������

            if (x<2 || y<2) // �ŏ���2�s�Ƃ��̑��̍s�̍ŏ���2��͖����f�[�^�Ȃ̂�0�Ƃ���
                lap = 0;

            lap_fb[y*width+x] = lap;
        }
    }
}

// RGB����Y�ւ̕ϊ�
// RGB�̃t�H�[�}�b�g�́A{8'd0, R(8bits), G(8bits), B(8bits)}, 1pixel = 32bits
// �P�x�M��Y�݂̂ɕϊ�����B�ϊ����́AY =  0.299R + 0.587G + 0.114B
// "YUV�t�H�[�}�b�g�y�� YUV<->RGB�ϊ�"���Q�l�ɂ����Bhttp://vision.kuee.kyoto-u.ac.jp/~hiroaki/firewire/yuv.html
//�@2013/09/27 : float ���~�߂āA���ׂ�int �ɂ���
int conv_rgb2y_soft(int rgb){
    int r, g, b, y_f;
    int y;

    b = rgb & 0xff;
    g = (rgb>>8) & 0xff;
    r = (rgb>>16) & 0xff;

    y_f = 77*r + 150*g + 29*b; //y_f = 0.299*r + 0.587*g + 0.114*b;�̌W����256�{����
    y = y_f >> 8; // 256�Ŋ���

    return(y);
}

// ���v���V�A���t�B���^
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
