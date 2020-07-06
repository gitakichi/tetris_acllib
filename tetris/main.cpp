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
#define WAIT_STATUS			6
#define FLOATING_STATUS		7

#define TOPWALL				0x1C0E
#define SIDEWALL			0x1002
#define	UNDERWALL			0x1FFE

void KeyboardEvent(int key, int event);
void timerEvent(int id);
int Setup(void);
void putblock(int origin_x,int origin_y,int x, int y,char color);
int blankscan(int x,int y,char mod[][TMP_W]);
void putmodule(int x,int y,char mod[][TMP_W]);
void ext_putmodule(int x,int y,char mod[][TMP_W],char *str);
void copymodule(char from_mod[][TMP_W],char to_mod[][TMP_W]);
void swapmodule(char from_mod[][TMP_W],char to_mod[][TMP_W]);
void randmodule(char to_mod[][TMP_W],int res);
int find_line(char *flash_q);
void blink_line(char *flash_q);
void putview(void);
void ext_putview(void);
void rotfunc(int mode);
void vram2module(int x,int y,char tmp_table[][TMP_W]);
void vram2target(int x,int y,char tmp_table[][TMP_W],int tar_en);
void game2vram(void);
void res_game(void);
int movefunc(int delta_x,int delta_y,char tmp_table[][TMP_W]);
void dropfunc(void);
void CloseEvent(void);
void gameconfig(int key);
void flashfunc(char *flash_q);
void holdfunc(void);
void gameover_func(void);
void acl_printf(int x,int y,char *str1);

//char view_table[24][14];
char **game_table,**view_table,*game_table_base,*view_table_base;
/*
char game_table[24][14] = {
{0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,8,8,8,0,0,0,0,0,0,8,8,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,0,0,0,0,0,0,0,8,0},
{0,8,0,0,0,2,2,0,0,0,0,0,8,0},
{0,8,4,4,2,2,5,7,7,0,5,0,8,0},
{0,8,3,4,4,5,5,1,7,5,5,0,8,0},
{0,8,3,6,6,7,5,1,7,5,5,0,8,0},
{0,8,3,6,6,7,1,1,5,5,5,0,8,0},
{0,8,3,4,1,7,7,7,7,7,1,0,8,0},
{0,8,4,4,1,6,6,7,2,2,1,0,8,0},
{0,8,4,1,1,6,6,2,2,1,1,0,8,0},
{0,8,8,8,8,8,8,8,8,8,8,8,8,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
*/
char tmp_table[TMP_W][TMP_W],hold_table[TMP_W][TMP_W],flash_q[32];
int x,y,hold_status,sum_line,status = 0,score = 0,tar_en = 1,line_n = 0,timerID = 0,div_n,div_i = 0,level;
char blank_table[TMP_W][TMP_W] = {{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0}};

void KeyboardEvent(int key, int event)
{
	static int used = 0;

	if(event == KEY_UP)	used = 0;
	else if(event == KEY_DOWN && used == 0){
		if(status == PLAY_STATUS || status == FLOATING_STATUS){
			if(key == 39)		movefunc(1,0,tmp_table);//right
			else if(key == 37)	movefunc(-1,0,tmp_table);//left	
			else if(key == 40)	movefunc(0,1,tmp_table);//down
			else if(key == 'S')	rotfunc(0);//cw
			else if(key == 'A')	rotfunc(1);//ccw
			else if(key == 38)	dropfunc();//up
			else if(key == 113)	closeWindow();//F2
			else if(key == 'H')	holdfunc();//while flash dont use hold		
			else if(key == 27){
				status = PAUSE_STATUS;//pause
				acl_printf(LEFT+70,200,"Press ESC Start\nPress E Exit");
			}
			else if(key == 112)	tar_en = tar_en ^ 0x01;//F1
		}
		else if(key == 113){//F2
			closeWindow();
		}
		else if(status == PAUSE_STATUS && key == 27){
			status = PLAY_STATUS;//release pause
			putview();
		}
		else if(status == PAUSE_STATUS && key == 'E'){
			gameover_func();
		}
		else if(status == BOOT_STATUS || status == GAMEOVER_STATUS){//start
			gameconfig(key);
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
	static int i = 0;
	static char *qp;

	if(status == BLINK_STATUS){
		if(i == 0){
			blink_line(flash_q);
			i++;
		}
		else if(i == 20){
			status = FLASH_STATUS;
			i = 0;
		}
		else i++;
	}
	else if(status == FLASH_STATUS){
		if(i == 0){
			flashfunc(flash_q);
			i++;
		}
		else if(i == 50){
			status = PLAY_STATUS;
			i = 0;
		}
		else i++;
	}
	else if(status == FLOATING_STATUS){
		if(i == 50){
			i = 0;
			putmodule(x,y,tmp_table);
			if(hold_status == 3) hold_status = 2;
			
			line_n = find_line(flash_q);
			if(line_n == 0)	status = FLASH_STATUS;
			else			status = BLINK_STATUS;
		}
		else i++;
	}
	else if(div_n <= div_i && status == PLAY_STATUS){//continue game
		if(movefunc(0,1,tmp_table) == 1){//detect ground		
			status = FLOATING_STATUS;
		}
		div_i = 0;
	}	
	else	div_i++;
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
	startTimer(timerID, 10);//TBD
	registerCloseEvent(CloseEvent);
	//initConsole ();

	//acl_printf(0,300,"ACLLIB\nTETRIS");
	acl_printf(0,0,"Turn CW:S\nTurn CCW:A\nPause:Esc\nPlay:P\nDrop:Up\nToggle Target:F1\nHold:H\nExit:F2");
	
	res_game();
	game2vram();
	putview();

	gameconfig('0');
	
	return 0;
}


void putblock(int origin_x,int origin_y,int x, int y,char color)
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
	}
	rectangle(origin_x+(x*BLOCK_W),origin_y+(y*BLOCK_W) , origin_x+(x*BLOCK_W)+BLOCK_W, origin_y+(y*BLOCK_W)+BLOCK_W);
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


void putmodule(int x,int y,char mod[][TMP_W])
{
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			if(mod[i][j] != 0){
				game_table[i+y][j+x] = mod[i][j];
			}
		}
	}
}

void ext_putmodule(int x,int y,char mod[][TMP_W],char *str)
{
	acl_printf(x,y+100,str);
	
	beginPaint();
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			putblock(x,y,j,i,mod[i][j]);
		}
	}
	endPaint();
}


void copymodule(char from_mod[][TMP_W],char to_mod[][TMP_W])
{
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			to_mod[i][j] = from_mod[i][j];
		}
	}
}

void swapmodule(char from_mod[][TMP_W],char to_mod[][TMP_W])
{
	char tmp;
	
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			tmp = to_mod[i][j];
			to_mod[i][j] = from_mod[i][j];
			from_mod[i][j] = tmp;
		}
	}
}

void randmodule(char to_mod[][TMP_W],int res)
{
	static char que[4];
	static int k = 0;
	char mod,nxt_mod[TMP_W][TMP_W],str[][16]={"Next Block 1","Next Block 2","Next Block 3"};
	unsigned char table[7][TMP_W]={
		{0x00,0x08,0x0E,0x00,0x00},
		{0x00,0x06,0x0C,0x00,0x00},
		{0x00,0x00,0x1E,0x00,0x00},
		{0x00,0x0C,0x06,0x00,0x00},
		{0x00,0x04,0x0E,0x00,0x00},
		{0x00,0x06,0x06,0x00,0x00},
		{0x00,0x02,0x0E,0x00,0x00}
	};

	if(res == 1){
		que[0] = rand() % 7;
		que[1] = rand() % 7;
		que[2] = rand() % 7;
		que[3] = rand() % 7;
	}
	
	mod = que[k];
	que[k] = rand() % 7;

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

	if(k == 3)	k = 0;
	else k++;

	for(int m = 0;m <= 2;m++){
		for(int i = 0;i < TMP_W;i++){
			if(table[que[0x03&(k+m)]][i] & 0x10)	nxt_mod[i][0] = que[0x03&(k+m)]+1;
			else									nxt_mod[i][0] = 0;

			if(table[que[0x03&(k+m)]][i] & 0x08)	nxt_mod[i][1] = que[0x03&(k+m)]+1;
			else									nxt_mod[i][1] = 0;

			if(table[que[0x03&(k+m)]][i] & 0x04)	nxt_mod[i][2] = que[0x03&(k+m)]+1;
			else									nxt_mod[i][2] = 0;

			if(table[que[0x03&(k+m)]][i] & 0x02)	nxt_mod[i][3] = que[0x03&(k+m)]+1;
			else									nxt_mod[i][3] = 0;

			if(table[que[0x03&(k+m)]][i] & 0x01)	nxt_mod[i][4] = que[0x03&(k+m)]+1;
			else									nxt_mod[i][4] = 0;

			ext_putmodule(560,m*120,nxt_mod,str[m]);
		}
	}
}

int find_line(char *flash_q)
{
	int det = 0,k;
	char *qp = flash_q;

	for(int i = GAME_Y0;i <= GAME_Y1;i++){
		k = 0;
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

void blink_line(char *flash_q)
{
	char *qp = flash_q;
	
	while(*qp != 0){
		for(int j = GAME_X0;j <= GAME_X1;j++){
			view_table[*qp][j] = 9;
		}
		qp++;
	}
	putview();
}


void putview(void)
{
	char str[64];

	beginPaint();
	for(int i = 0;i <= VIEW_Y1;i++){
		for(int j = 0;j <= VIEW_X1;j++){
			putblock(LEFT,0,j,i,view_table[i][j]);
		}
	}
	setPenWidth(2);
	setBrushColor(WHITE);
	setPenColor(WHITE);
	rectangle(LEFT-120,200,LEFT,240);
	endPaint();

	sprintf_s(str,64,"Score: %d     \nLevel: %d     ",score,level);
	acl_printf(LEFT-120,200,str);

}

void ext_putview(void)
{
	char str[64];

	beginPaint();
	for(int i = GAME_Y0;i <= GAME_Y1;i++){
		for(int j = GAME_X0;j <= GAME_X1;j++){
			putblock(LEFT-100,0,j,i,view_table[i][j]);
		}
	}
	setPenWidth(2);
	setBrushColor(WHITE);
	setPenColor(WHITE);
	rectangle(LEFT-120,200,LEFT,240);
	endPaint();

	sprintf_s(str,64,"Score: %d     \nLevel: %d     ",score,level);
	acl_printf(LEFT-120,200,str);

}

void rotfunc(int mode)
{
	char rot_table[TMP_W][TMP_W];
	
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			if(mode == 0)		rot_table[i][j] = tmp_table[4-j][i];//cw
			else if(mode == 1)	rot_table[i][j] = tmp_table[j][4-i];
		}
	}

	if(blankscan(x,y,rot_table) == 0){
		copymodule(rot_table,tmp_table);
		game2vram();
		vram2module(x,y,tmp_table);
		vram2target(x,y,tmp_table,tar_en);
		putview();
	}
}

void vram2module(int x,int y,char tmp_table[][TMP_W])
{
	for(int i = 0;i < TMP_W;i++){
		for(int j = 0;j < TMP_W;j++){
			if(tmp_table[i][j] != 0){
				view_table[i+y][j+x] = tmp_table[i][j];
			}
		}
	}
}

void vram2target(int x,int y,char tmp_table[][TMP_W],int tar_en)
{
	int x_tar = x, y_tar = y;

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
}

void game2vram(void)
{
	for(int i = 0;i <= VIEW_Y1;i++){
		for(int j = 0;j <= VIEW_X1;j++){
			view_table[i][j] = game_table[i][j];
		}
	}
}

void res_game(void)
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
}

int movefunc(int delta_x,int delta_y,char tmp_table[][TMP_W])
{
	int result = blankscan(x+delta_x,y+delta_y,tmp_table);

	if(result == 0){
		if(status == FLOATING_STATUS){
			if(blankscan(x+delta_x,y+1,tmp_table) == 0){
				y++;
			}
		}
		
		x+=delta_x;
		y+=delta_y;
		
		game2vram();//reset vram
		vram2module(x,y,tmp_table);
		vram2target(x,y,tmp_table,tar_en);
		putview();
	}
	return result;
}

void dropfunc(void)
{
	int result = 0;
	
	while(result == 0){
		result = blankscan(x,y+1,tmp_table);

		if(result == 0){
			y++;
			score++;
		}
	}
	status = FLOATING_STATUS;
	game2vram();
	vram2module(x,y,tmp_table);
	//putmodule(x,y,tmp_table);
	putview();

	//if(hold_status == 3) hold_status = 2;
}

void CloseEvent(void)
{
	free(game_table_base);
	free(game_table);

	free(view_table_base);
	free(view_table);
}

void gameconfig(int key)
{
	char str[64];
	
	if(key >= '0' && key <= '9'){
			level = key - '0';
			
			sprintf_s(str,64,"Level %d\nPress P Start",level);
			acl_printf(LEFT+70,300,str);
			
		}
	else if(key == 'P'){
		status = PLAY_STATUS;
		randmodule(tmp_table,1);
		//reset x and y data
		x = 5;
		y = 0;
		score = 0;
		sum_line = 0;
		hold_status = 0;
				
		div_n = 40 - level;

		ext_putmodule(180,0,blank_table,"Hold Block");
		res_game();
		game2vram();//reset vram
		vram2module(x,y,tmp_table);
		vram2target(x,y,tmp_table,tar_en);
		putview();
	}

}
void flashfunc(char *flash_q)
{
	char *qp = flash_q;
	
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
		
	sum_line += line_n;
	if(line_n != 0 && sum_line % 10 == 0){
		level++; 
		if(div_n > 10)		div_n = 40 - level;
	}
	line_n = 0;
	
	while(*qp != 0){
		for(int j = GAME_X0;j <= GAME_X1;j++){
			for(int i = *qp;i > GAME_Y0;i--){
				game_table[i][j] = game_table[i-1][j];
			}
			game_table[2][j] = 0;
		}
		qp++;
	}

	game2vram();
	putview();
	
	/*
	if(line_n == 4){
		for(int i = 0;i <= VIEW_Y1;i++){
			for(int j = 0;j <= VIEW_X1;j++){
				printf("%d,",game_table[i][j]);
			}
			printf("\r\n");
		}
		printf("\r\n");
	}
	*/
	
	randmodule(tmp_table,0);
	//reset x and y data
	x = 5;
	y = 0;
	if(blankscan(x,y,tmp_table) == 1){
		gameover_func();
	}
	timerID = 0;
	div_i = div_n;
}

void holdfunc(void)
{
	if(hold_status == 0){//swap
		copymodule(tmp_table,hold_table);
		randmodule(tmp_table,0);
		hold_status = 2;
	}
	else if(hold_status == 2 && blankscan(x,y,hold_table) == 0){
		swapmodule(tmp_table,hold_table);
		hold_status = 3;
	}
	ext_putmodule(180,0,hold_table,"Hold Block");
}

void gameover_func(void)
{
	putview();
	
	acl_printf(LEFT+70,200,"GAME OVER");
	
	status = GAMEOVER_STATUS; 
	gameconfig('0');
}

void acl_printf(int x,int y,char *str1)
{
	int i = 0;
	char *p1 = str1;
	char str2[32];
	char *p2;

	beginPaint();
	setTextColor(BLACK);
	setTextBkColor(WHITE);
	setTextSize(20);

	while(*p1 != '\0'){
		p2 = str2;
		while(*p1 != '\0' && *p1 != '\n'){
			*p2 = *p1;
			p1++;
			p2++;
		}
		if(*p1 == '\n')	p1++;
		*p2 = '\0';
		paintText(x, y+(i*20), str2);
		i++;
	}
	endPaint();
}