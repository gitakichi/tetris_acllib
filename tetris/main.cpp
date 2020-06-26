#include <stdio.h>
#include <stdlib.h>
#include "acllib.h"

#define WIDTH		840
#define HEIGHT		480
#define LEFT		280
#define BLOCK_W		20
#define TMP_W		5
#define GAME_X0		2
#define GAME_X1		11
#define GAME_Y0		2
#define GAME_Y1		21
#define VIEW_X1		13
#define VIEW_Y1		23

#define BOOT_STATUS			0
#define PLAY_STATUS			1
#define BLINK_STATUS		2
#define FLASH_STATUS		3
#define PAUSE_STATUS		4
#define GAMEOVER_STATUS		5

#define TOPWALL				0x1C0E
#define SIDEWALL			0x1002
#define	UNDERWALL			0x1FFE


void KeyboardEvent(int key, int event);
void timerEvent(int id);
int Setup(void);
int putblock(int origin_x,int origin_y,int x, int y,char color);
int blankscan(int x,int y,char mod[][TMP_W]);
int putmodule(int x,int y,char mod[][TMP_W]);
int ext_putmodule(int x,int y,char mod[][TMP_W]);
int copymodule(char from_mod[][TMP_W],char to_mod[][TMP_W]);
int swapmodule(char from_mod[][TMP_W],char to_mod[][TMP_W]);
int randmodule(char to_mod[][TMP_W]);
int flash(char *flash_q);
int find_line(char *flash_q);//TBD
int blink_line(char *flash_q);//TBD
int putview(int x0,int y0,int x1,int y1);
int cwfunc(void);
int ccwfunc(void);
int vram2module(int x,int y,char tmp_table[][TMP_W]);
int vram2target(int x,int y,char tmp_table[][TMP_W],int tar_en);
int game2vram(int x0,int y0,int x1,int y1);
int res_game(void);
int movefunc(int delta_x,int delta_y,char tmp_table[][TMP_W]);
int dropfunc(void);

char **game_table,**view_table,*game_table_base,*view_table_base;
char tmp_table[TMP_W][TMP_W],rot_table[TMP_W][TMP_W],hold_table[TMP_W][TMP_W],flash_q[32];
int x = 5,y = 0, status = 0,score = 0,tar_en = 1,line_n = 0,hold_status = 0,timerID = 0;


void KeyboardEvent(int key, int event)
{
	static int used = 0;

	if(event == KEY_UP){
		used = 0;
	}
	
	else if(event == KEY_DOWN && used == 0){
		if(status == PLAY_STATUS){
			if(key == 39)		movefunc(1,0,tmp_table);//right
			else if(key == 37)	movefunc(-1,0,tmp_table);//left	
			else if(key == 40)	movefunc(0,1,tmp_table);//down
			else if(key == 'S')	cwfunc();
			else if(key == 'A')	ccwfunc();
			else if(key == 38){//up
				dropfunc();
				line_n = find_line(flash_q);
				if(line_n == 0)	status = FLASH_STATUS;
				else			status = BLINK_STATUS;
			}
	else if(status == FLASH_STATUS){
			}
			else if(key == 'H'){//while flash dont use hold		
				if(hold_status == 0){
					//swap
					copymodule(tmp_table,hold_table);
					randmodule(tmp_table);
					hold_status = 2;
				}
				else if(hold_status == 2 && blankscan(x,y,hold_table) == 0){
					swapmodule(tmp_table,hold_table);
					hold_status = 3;
				}
					
				//view hold_table
				beginPaint();
				setTextColor(BLACK);
				setTextBkColor(WHITE);
				setTextSize(20);
				paintText(180, 100, "Hold Block");
				
				ext_putmodule(180,0,hold_table);
				endPaint();
			}
			else if(key == 27)	status = PAUSE_STATUS;//pause
			else if(key == 112){//F1
				if(tar_en == 1)	tar_en = 0;
				else			tar_en = 1;
			}
		}
		else if(key == 113){//F2
			free(game_table_base);
			free(game_table);

			free(view_table_base);
			free(view_table);
			
			closeWindow();
		}
		else if(status == PAUSE_STATUS && key == 27){
			status = PLAY_STATUS;//release pause
		}
		else if((status == BOOT_STATUS || status == GAMEOVER_STATUS) && key == 'P'){//start
			status = PLAY_STATUS;
			randmodule(tmp_table);
			//reset x and y data
			x = 5;
			y = 0;
			score = 0;

			res_game();
			game2vram(0,0,VIEW_X1,VIEW_Y1);//reset vram
			vram2module(x,y,tmp_table);
			vram2target(x,y,tmp_table,tar_en);
			putview(0,0,VIEW_X1,VIEW_Y1);
		}
		used = 1;
	}
	
	else if(event == KEY_DOWN && used == 1){
		if(status == PLAY_STATUS){
			if(key == 39)		movefunc(1,0,tmp_table);//right
			else if(key == 37)	movefunc(-1,0,tmp_table);//left	
			else if(key == 40)	movefunc(0,1,tmp_table);//down
		}
	}
}


void timerEvent(int id)
{
	if(status == BLINK_STATUS){
		blink_line(flash_q);
		putview(0,0,VIEW_X1,VIEW_Y1);
		status = FLASH_STATUS;
	}
	else if(status == FLASH_STATUS){
		switch(line_n){
			case 1:
				score += 40;
				break;
			case 2:
				score += 100;
				break;
			case 3:
				score += 300;
				break;
			case 4:
				score += 1200;
				break;
			default:
				break;
		}

		flash(flash_q);
		putview(0,0,VIEW_X1,VIEW_Y1);

		randmodule(tmp_table);
		//reset x and y data
		x = 5;
		y = 0;
		if(blankscan(x,y,tmp_table) == 1)	status = GAMEOVER_STATUS; 
		else								status = PLAY_STATUS;
	}
	else if(status == GAMEOVER_STATUS){
		beginPaint();
		setTextColor(BLACK);
		setTextBkColor(WHITE);
		setTextSize(30);
		paintText(LEFT+70, 200, "GAME OVER");
		endPaint();
	}
	else if(status == PLAY_STATUS){//continue game
		if(movefunc(0,1,tmp_table) == 1){//detect ground		
			line_n = find_line(flash_q);
			if(line_n == 0)	status = FLASH_STATUS;
			else			status = BLINK_STATUS;
			
		}
	}
}

int Setup(void)
{	
	game_table = (char **)malloc(sizeof(char *) * 24);
	game_table_base = (char *)malloc(sizeof(char) * 24 * 14);
	for (int i = 0;i < 24;i++) {
		game_table[i] = game_table_base + i * 14;
	}

	view_table = (char **)malloc(sizeof(char *) * 24);
	view_table_base = (char *)malloc(sizeof(char) * 24 * 14);
	for (int i = 0;i < 24;i++) {
		view_table[i] = view_table_base + i * 14;
	}
	
	initWindow("Tetris", 0, 0, WIDTH, HEIGHT);

	registerKeyboardEvent(KeyboardEvent);
	registerTimerEvent(timerEvent);
	startTimer(timerID, 500);//TBD

	beginPaint();
	setTextColor(BLACK);
	setTextBkColor(WHITE);
	setTextSize(20);
	paintText(0, 0, "Turn CW:S");
	paintText(0, 20, "Turn CCW:A");
	paintText(0, 40, "Pause:Esc");
	paintText(0, 60, "Play:P");
	paintText(0, 80, "Drop:Up");
	paintText(0, 100, "Toggle Target:F1");
	paintText(0, 120, "Hold:H");
	paintText(0, 140, "Exit:F2");
	endPaint();

	res_game();
	game2vram(0,0,VIEW_X1,VIEW_Y1);
	putview(0,0,VIEW_X1,VIEW_Y1);
	
	return 0;
}


int putblock(int origin_x,int origin_y,int x, int y,char color)
{

	setPenWidth(2);
	setPenColor(BLACK);

	switch(color){
	case 1:
		setBrushColor(RGB(0, 123, 204));//blue
		break;
	case 2:
		setBrushColor(GREEN);
		break;
	case 3:
		setBrushColor(CYAN);
		break;
	case 4:
		setBrushColor(RED);
		break;
	case 5:
		setBrushColor(MAGENTA);
		break;
	case 6:
		setBrushColor(YELLOW);
		break;
	case 7:
		setBrushColor(RGB(255, 165, 0));//orange
		break;
	case 8:
		setBrushColor(RGB(118, 118, 118));//gray
		break;
	case 9:
		setBrushColor(WHITE);
		break;
	case 10:
		setBrushColor(BLACK);
		setPenColor(WHITE);
		break;
	default:
		setBrushColor(BLACK);
		break;
	}

	rectangle(origin_x+(x*BLOCK_W),origin_y+(y*BLOCK_W) , origin_x+(x*BLOCK_W)+BLOCK_W, origin_y+(y*BLOCK_W)+BLOCK_W);

	return 0;
}


int blankscan(int x,int y,char mod[][TMP_W])
{
	int result = 0;

	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			if(mod[i][j] != 0 && game_table[i+y][j+x] != 0){
				result = 1;
			}
		}
	}
	return result;
}


int putmodule(int x,int y,char mod[][TMP_W])
{
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			if(mod[i][j] != 0){
				game_table[i+y][j+x] = mod[i][j];
			}
		}
	}

	return 0;
}

int ext_putmodule(int x,int y,char mod[][TMP_W])
{
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			putblock(x,y,j,i,mod[i][j]);
		}
	}

	return 0;
}


int copymodule(char from_mod[][TMP_W],char to_mod[][TMP_W])
{
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			to_mod[i][j] = from_mod[i][j];
		}
	}

	return 0;
}

int swapmodule(char from_mod[][TMP_W],char to_mod[][TMP_W])
{
	char tmp;
	
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			tmp = to_mod[i][j];
			to_mod[i][j] = from_mod[i][j];
			from_mod[i][j] = tmp;
		}
	}

	return 0;
}

int randmodule(char to_mod[][TMP_W])
{
	static int boot = 0;
	static char que[3];
	static char *qp = &que[0];

	char mod;// = rand() % 7;
	
	unsigned char table[7][TMP_W]={
		{0x00,0x08,0x0E,0x00,0x00},
		{0x00,0x06,0x0C,0x00,0x00},
		{0x00,0x00,0x1E,0x00,0x00},
		{0x00,0x0C,0x06,0x00,0x00},
		{0x00,0x04,0x0E,0x00,0x00},
		{0x00,0x06,0x06,0x00,0x00},
		{0x00,0x02,0x0E,0x00,0x00}
	};

	if(boot == 0){
		que[0] = rand() % 7;
		que[1] = rand() % 7;
		que[2] = rand() % 7;

		boot = 1;
	}
	mod = *qp;
	*qp = rand() % 7;
	

	for(int i = 0;i < TMP_W;i++){
		if(table[mod][i] & 0x10)	to_mod[i][0] = mod+1;
		else						to_mod[i][0] = 0;

		if(table[mod][i] & 0x08)	to_mod[i][1] = mod+1;
		else						to_mod[i][1] = 0;

		if(table[mod][i] & 0x04)	to_mod[i][2] = mod+1;
		else						to_mod[i][2] = 0;

		if(table[mod][i] & 0x02)	to_mod[i][3] = mod+1;
		else						to_mod[i][3] = 0;

		if(table[mod][i] & 0x01)	to_mod[i][4] = mod+1;
		else						to_mod[i][4] = 0;
	}		

	if(qp - &que[0] == 2)	qp = &que[0];
	else qp++;

	beginPaint();
	setTextColor(BLACK);
	setTextBkColor(WHITE);
	setTextSize(20);
	paintText(560, 100, "Next Block");
	
	for(int i = 0;i < TMP_W;i++){
		if(table[*qp][i] & 0x10)	putblock(560,0,0,i,(*qp)+1);
		else						putblock(560,0,0,i,0);
	
		if(table[*qp][i] & 0x08)	putblock(560,0,1,i,(*qp)+1);
		else						putblock(560,0,1,i,0);
		
		if(table[*qp][i] & 0x04)	putblock(560,0,2,i,(*qp)+1);
		else						putblock(560,0,2,i,0);
		
		if(table[*qp][i] & 0x02)	putblock(560,0,3,i,(*qp)+1);
		else						putblock(560,0,3,i,0);
		
		if(table[*qp][i] & 0x01)	putblock(560,0,4,i,(*qp)+1);
		else						putblock(560,0,4,i,0);
	}
	endPaint();


	return 0;
}


int flash(char *flash_q)
{	
	char *qp = flash_q;
	
	while(*qp != 0){
		for(int j = GAME_X0;j <= GAME_X1;j++){
			for(int i = *qp;i > GAME_Y0;i--){
				game_table[i][j] = game_table[i-1][j];
			}
			game_table[2][j] = 0;
		}
		qp++;
	}

	return 0;
}

int find_line(char *flash_q)
{
	int det = 0;
	char *qp = flash_q;

	for(int i = GAME_Y0;i <= GAME_Y1;i++){
		int k=0;
		for(int j = GAME_X0;j <= GAME_X1;j++){
			if(game_table[i][j] != 0)	k++;
		}
		if(k == 10){
			det++;//TBD
			*qp = i;
			qp++;
		}
	}
	*qp = 0;

	return det;
}

int blink_line(char *flash_q)
{
	char *qp = flash_q;
	
	beginPaint();
	while(*qp != 0){
		for(int j = GAME_X0;j <= GAME_X1;j++){
			view_table[*qp][j] = 9;
		}
		qp++;
	}
	endPaint();

	return 0;
}


int putview(int x0,int y0,int x1,int y1)
{
	char str[16];

	beginPaint();

	for(int i = y0;i <= y1;i++){
		for(int j = x0;j <= x1;j++){
			putblock(LEFT,0,j,i,view_table[i][j]);
		}
	}

	setTextColor(BLACK);
	setTextBkColor(WHITE);
	setTextSize(30);
	sprintf_s(str,16,"score:%d",score);
	paintText(70, 200, str);		

	endPaint();

	return 0;
}

int cwfunc(void)
{
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			rot_table[i][j] = tmp_table[4-j][i];
		}
	}

	if(blankscan(x,y,rot_table) == 0){
		copymodule(rot_table,tmp_table);
		game2vram(0,0,VIEW_X1,VIEW_Y1);
		vram2module(x,y,tmp_table);
		vram2target(x,y,tmp_table,tar_en);
		putview(0,0,VIEW_X1,VIEW_Y1);
	}
	return 0;
}

int ccwfunc(void)
{
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			rot_table[i][j] = tmp_table[j][4-i];
		}
	}

	if(blankscan(x,y,rot_table) == 0){
		copymodule(rot_table,tmp_table);
		game2vram(0,0,VIEW_X1,VIEW_Y1);
		vram2module(x,y,tmp_table);
		vram2target(x,y,tmp_table,tar_en);
		putview(0,0,VIEW_X1,VIEW_Y1);
	}
	return 0;
}

int vram2module(int x,int y,char tmp_table[][TMP_W])
{
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			if(tmp_table[i][j] != 0){
				view_table[i+y][j+x] = tmp_table[i][j];
			}
		}
	}
	return 0;
}

int vram2target(int x,int y,char tmp_table[][TMP_W],int tar_en)
{
	int x_tar,y_tar;
	
	x_tar = x;
	y_tar = y;

	while(blankscan(x_tar,(y_tar)+1,tmp_table) == 0){
		(y_tar)++;
	}

	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			if((y_tar - y) > 3 && tar_en == 1 && tmp_table[i][j] != 0){
				view_table[i+(y_tar)][j+(x_tar)] = 10;
			}
		}
	}
	
	return 0;
}

int game2vram(int x0,int y0,int x1,int y1)
{
	for(int i = y0;i <= y1;i++){
		for(int j = x0;j <= x1;j++){
			view_table[i][j] = game_table[i][j];
		}
	}

	return 0;
}

int res_game(void)
{
	for(int j = 0;j <= VIEW_X1;j++){
		if(TOPWALL >> j & 0x01){
			game_table[GAME_Y0-1][j] = 8;
		}
		else	game_table[GAME_Y0-1][j] = 0;
	}

	for(int i = GAME_Y0;i <= GAME_Y1;i++){
		for(int j = 0;j <= VIEW_X1;j++){
			if(SIDEWALL >> j & 0x01){
				game_table[i][j] = 8;
			}
			else	game_table[i][j] = 0;
		}
	}

	for(int j = 0;j <= VIEW_X1;j++){
		if(UNDERWALL >> j & 0x01){
			game_table[GAME_Y1+1][j] = 8;
		}
		else	game_table[GAME_Y1+1][j] = 0;
	}

	return 0;
}

int movefunc(int delta_x,int delta_y,char tmp_table[][TMP_W])
{
	int result;

	result = blankscan(x+delta_x,y+delta_y,tmp_table);

	if(result == 0){
		x+=delta_x;
		y+=delta_y;
		
		game2vram(0,0,VIEW_X1,VIEW_Y1);//reset vram
		vram2module(x,y,tmp_table);
		vram2target(x,y,tmp_table,tar_en);
		putview(0,0,VIEW_X1,VIEW_Y1);
	}
	else if(delta_x == 0){
		putmodule(x,y,tmp_table);
		if(hold_status == 3) hold_status = 2;
	}

	return result;
}

int dropfunc(void)
{
	int result = 0;
	
	while(result == 0){
		result = blankscan(x,y+1,tmp_table);

		if(result == 0){
			y++;
			score++;
		}
	}
	game2vram(0,0,VIEW_X1,VIEW_Y1);
	vram2module(x,y,tmp_table);
	putmodule(x,y,tmp_table);
	putview(0,0,VIEW_X1,VIEW_Y1);

	if(hold_status == 3) hold_status = 2;

	return result;
}