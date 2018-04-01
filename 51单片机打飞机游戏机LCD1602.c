#include<reg52.h>
#include<string.h>
#define uchar unsigned char
#define uint unsigned int
sbit en=P3^4;
sbit rs=P3^5;
sbit rw=P3^6;	
sbit dula=P2^6;
sbit wela=P2^7;

#define my_plane '#'
#define my_bullet ')'
#define enemy_small_plane '|'
#define enemy_middle_plane 'I'
#define enemy_large_plane 'H'
#define enemy_boss '8'
#define enemy_bullet '('
#define two_bullets 'O'
#define game_speed 4000
sbit up=P3^1;
sbit down=P3^2;
uchar line[2][100];
uint distance;
uint boss_hp;
int rand_seed;


int rand()
{
	if(line[0][4]==my_bullet) rand_seed++;
	return rand_seed+boss_hp+distance; //这样搞出来BOSS会自动避弹
}

void delay(uint x)
{
	uint a,b;
	for(a=x;a>0;a--)
		for(b=9;b>0;b--); 
}

void exec()
{
	delay(9);
	en=1;
	delay(9);
	en=0;
	delay(9);
}

void write_directive(uchar directive)
{
	//控制指令
	rs=0;
	rw=0;
	P0=directive;
	exec();
}

void write_char(uchar date)
{
	rs=1;
	rw=0;
	P0=date;
	exec();
}

void refresh_screen()
{
	//输出到LCD
	uchar a;
	write_directive(0x80);
	for(a=0;a<16;a++)
	{
		write_char(line[0][a]);		
	}
	write_directive(0xc0);
	for(a=0;a<16;a++)
	{
		write_char(line[1][a]);		
	}
}

void init()
{
	dula=0;
	wela=0;
	write_directive(0x38);   //0011 1000 显示模式设置：16×2显示，5×7点阵，8位数据接口
	write_directive(0x0c);   //0000 1100 显示模式设置：无光标
	write_directive(0x06);   //0000 0110 输入模式设置：光标右移，字符不移
	write_directive(0x01);   //清屏
}

void game_over()
{
	strcpy(line[0],"   GAME OVER!   ");
	strcpy(line[1],"   GAME OVER!   ");
	refresh_screen();
	while(1){
		delay(9999);
		write_directive(0x18); //0001 1000 屏幕内容右移
	}
}

void game_win()
{
	strcpy(line[0],"    YOU WIN!    ");
	strcpy(line[1],"    YOU WIN!    ");
	refresh_screen();
	while(1){
		delay(9999);
		write_directive(0x18); //0001 1000 屏幕内容右移
	}
}




void fire()
{
	//发射子弹
	uchar b;
	for(b=0;b<=1;b++){
		if(line[b][0]==my_plane){
			//判断前面一格有没有飞机
			if(line[b][1]==enemy_small_plane){
				line[b][1]=' ';
			}else if(line[b][1]==enemy_middle_plane || line[b][1]==enemy_large_plane){
				game_over();
			//判断前面一格有没有敌人的子弹
			}else if(line[b][1]==enemy_bullet){
				line[b][1]=two_bullets;
			}else{
				line[b][1]=my_bullet;
			}
		}
	}
}

void move_bullet()
{
	//移动子弹
	uchar a,b;
	//自己的子弹
	for(a=15;a>0;a--){
		for(b=0;b<=1;b++){
			if(line[b][16]==my_bullet) line[b][15]=' ';
			if(line[b][a]==my_bullet){
				if(line[b][a+1]==enemy_small_plane){
					line[b][a+1]=' ';
				}else if(line[b][a+1]==enemy_middle_plane){
					line[b][a+1]=enemy_small_plane;
				}else if(line[b][a+1]==enemy_large_plane){
					line[b][a+1]=enemy_middle_plane;
				}else if(line[b][a+1]==enemy_boss){
					boss_hp--;
					if(boss_hp==0) game_win();
				}else if(line[b][a+1]==enemy_bullet){
					line[b][a+1]=two_bullets;
				}else{
					line[b][a+1]=my_bullet;
				}
				line[b][a]=' ';
			}else if(line[b][a]==two_bullets){
				line[b][a+1]=my_bullet;
				line[b][a]=enemy_bullet;
			}
		}
	}
	//敌人的子弹
	for(a=0;a<100;a++){
		for(b=0;b<=1;b++){
			if(line[b][0]==enemy_bullet) line[b][0]=' ';
			if(line[b][a]==enemy_bullet || line[b][a]==two_bullets){
				if(line[b][a-1]==my_plane) game_over();
				if(line[b][a-1]==my_bullet && line[b][a]==enemy_bullet){
					line[b][a-1]=two_bullets;
					line[b][a]=' ';
				}else if(line[b][a]==two_bullets){
					line[b][a]=my_bullet;
					line[b][a-1]=enemy_bullet;
				}else{
					line[b][a-1]=enemy_bullet;
					line[b][a]=' ';
				}
			}
		}
	}
}

void move_my_plane()
{
	//移动自己的飞机
	if(up==0 && line[1][0]==my_plane){
		rand_seed+=1;
		if(line[0][0]==' '){
			line[0][0]=my_plane;
			line[1][0]=' ';
		}else{
			game_over();
		}
	}else if(down==0 && line[0][0]==my_plane){
		rand_seed+=3;
		if(line[1][0]==' '){
			line[0][0]=' ';
			line[1][0]=my_plane;
		}else{
			game_over();
		}
	}
}

void move_enemy_plane()
{
	//移动敌机
	uchar a,b;
	for(b=0;b<=1;b++){
		a=0;
		if(line[b][a]==enemy_small_plane || line[b][a]==enemy_middle_plane || line[b][a]==enemy_large_plane) line[b][a]=' ';
		for(a=1;a<100;a++){
			if(line[b][a]==enemy_small_plane || line[b][a]==enemy_middle_plane || line[b][a]==enemy_large_plane){
				if(line[b][a-1]==my_plane){
					game_over();
				}else if(line[b][a-1]==my_bullet){
					if(line[b][a]==enemy_small_plane){
						line[b][a-1]=' ';
					}else if(line[b][a]==enemy_middle_plane){
						line[b][a-1]=enemy_small_plane;
					}else if(line[b][a]==enemy_large_plane){
						line[b][a-1]=enemy_middle_plane;
					}else{
						line[b][a]=my_bullet;
					}
				}else{
					line[b][a-1]=line[b][a];
				}
				line[b][a]=' ';
			}
		}
	}
	distance++;
	if(distance==101){
		//BOSS出现
		line[1][15]=enemy_boss;
	}
	if(distance>101 && distance%2==0){
		//移动BOSS
		if(rand()%2==1){
			line[0][15]=enemy_boss;
			line[1][15]=' ';
		}else{
			line[0][15]=' ';
			line[1][15]=enemy_boss;
		}
	}
}


void start_level(level)
{
	uchar fire_count=2;
	distance=0;
	if(level==1)//第一关
	{
		boss_hp=15;
		//下面是地图第一部分
		//strcpy(line[0],"#                                                                                                  ");
		//strcpy(line[1],"                                                                                                   ");
		strcpy(line[0],"#          |   |  |      |||        (I       I III    H I              HHHHHHHHH             H     (");
		strcpy(line[1],"           |   |  |      |           I     I        H   I   I          I         II       HHHHH     ");
		//刷新画面
		refresh_screen();
		//开始游戏
		while(1){
			delay(game_speed);
			move_my_plane();
			move_enemy_plane();
			move_bullet();
			if(fire_count==2){
				fire();//每3回合发射一次子弹
				fire_count=0;
			}
			fire_count++;
			refresh_screen();
			

			/*
			//下面是地图第二部分
			if(distance==99){
				strcpy(line[0],"     H |       |  ||       ||  HHH   |   H   |   |                  I            H    H          II");
				strcpy(line[1],"II    H  |     |      H   |   | | |      ||                     II   I    II   I       I    I    H ");
			}
			*/
		}
	}else{
	//第2345678...关。以后再写
	}
}



void main()
{
	init();
	start_level(1);
}
