// laplacian_filter2.c

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
    int fl, sl, tl;

    // �ŏ���2���C���̉�f��ǂݍ���
    Loop1: for (y=0; y<2; y++){ // 2���C����
        Loop2: for (x=0; x<HORIZONTAL_PIXEL_WIDTH; x++){ // ���C��
            line_buf[y][x] = cam_fb[(y*HORIZONTAL_PIXEL_WIDTH)+x];
            line_buf[y][x] = conv_rgb2y(line_buf[y][x]);
        }
    }

    // RGB�l��Y�i�P�x�����j�݂̂ɕϊ����A���v���V�A���t�B���^���|�����B
    Loop3: for (y=2; y<VERTICAL_PIXEL_WIDTH; y++){
    	Loop4: for (x=0; x<2; x++){
    			line_buf[y%3][x] = cam_fb[(y*HORIZONTAL_PIXEL_WIDTH)+x];
    			line_buf[y%3][x] = conv_rgb2y(line_buf[y%3][x]);
    	}
    	lap_fb[((y-1)*HORIZONTAL_PIXEL_WIDTH)] = 0; // ���C���̍ŏ���0����
    	lap_fb[((y-1)*HORIZONTAL_PIXEL_WIDTH)+(HORIZONTAL_PIXEL_WIDTH-1)] = 0; // ���C���̍Ō��0����

        Loop5: for (x=2; x<HORIZONTAL_PIXEL_WIDTH; x++){
			// 1�̃s�N�Z����ǂݍ��݂Ȃ��烉�v���V�A���E�t�B���^�����s����
			line_buf[y%3][x] = cam_fb[(y*HORIZONTAL_PIXEL_WIDTH)+x];
			// y%3 �́A�g�p�ς݂̃��C�����ɓǂݍ��ށAy=2 �̎� line[0], y=3�̎� line[1], y=4�̎� line[2]
			line_buf[y%3][x] = conv_rgb2y(line_buf[y%3][x]);

			fl = (y-2)%3;    // �ŏ��̃��C��, y=2 012, y=3 120, y=4 201, y=5 012
			sl = (y-1)%3;    // 2�Ԃ߂̃��C��
			tl = y%3;   	 // 3�Ԗڂ̃��C��
			lap_fil_val = laplacian_fil(line_buf[fl][x-2], line_buf[fl][x-1], line_buf[fl][x], line_buf[sl][x-2], line_buf[sl][x-1], line_buf[sl][x], line_buf[tl][x-2], line_buf[tl][x-1], line_buf[tl][x]);

			// ���v���V�A���t�B���^�E�f�[�^�̏�������
			lap_fb[((y-1)*HORIZONTAL_PIXEL_WIDTH)+(x-1)] = (lap_fil_val<<16)+(lap_fil_val<<8)+lap_fil_val ;
        }
     }
     Loop7: for (x=0; x<HORIZONTAL_PIXEL_WIDTH; x++){
    	 lap_fb[x] = 0;
    	 lap_fb[HORIZONTAL_PIXEL_WIDTH*(VERTICAL_PIXEL_WIDTH-1)+x] = 0;
     }

     return(0);
}

// RGB����Y�ւ̕ϊ�
// RGB�̃t�H�[�}�b�g�́A{8'd0, R(8bits), G(8bits), B(8bits)}, 1pixel = 32bits
// �P�x�M��Y�݂̂ɕϊ�����B�ϊ����́AY =  0.299R + 0.587G + 0.114B
// "YUV�t�H�[�}�b�g�y�� YUV<->RGB�ϊ�"���Q�l�ɂ����Bhttp://vision.kuee.kyoto-u.ac.jp/~hiroaki/firewire/yuv.html
//�@2013/09/27 : float ���~�߂āA���ׂ�int �ɂ���
int conv_rgb2y(int rgb){
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
