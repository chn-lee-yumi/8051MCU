//单片机型号：STC2C5A60S2
//晶振：12MHz
//60000个周期为5ms
#include <reg51.h>
#include <intrins.h>
#include <stdio.h>

/****************************************
**********以下是各种定义**********
****************************************/

//定义ADC的SFR，具体看STC的文档：http://www.stcmcudata.com/datasheet/stc/STC-AD-PDF/STC12C5A60S2.pdf
sfr ADC_CONTR=0xBC;
/*ADC控制寄存器，从高到低依次为
ADC_POWER SPEED1 SPEED0 ADC_FLAG ADC_START CHS2 CHS1 CHS0
ADC_POWER：ADC电源，0关1开
SPEED1 SPEED0：模数转换器转换速度控制，全为1时最快
ADC_FLAG：转换结束标志位（也是中断请求标志位），结束时变1，需手动清0
ADC_START：启动控制位，设1开始
CHS2 CHS1 CHS0：模拟输入通道选择，000~111对应P1.0~P1.7
注：设置ADC_CONTR寄存器后，要过四个NOP才能正确读取到的ADC_CONTR的值
*/
sfr ADC_RES=0xBD;//转换结果寄存器 高
sfr ADC_RESL=0xBE;//转换结果寄存器 低
//ADC_RES存8位结果，ADC_RESL存2位结果。Vin=(ADC_RES[7:0],ADC_RESL[1:0])*Vcc/1024
sfr P1ASF=0x9D;//P1口模拟功能寄存器，相应位置1则P1口相应位能做A/D使用
sbit EADC=IE^5;//ADC中断控制位，1为允许
sfr IPH=0xB7;//中断优先级控制寄存器 高
//sfr IP=0xB8;//中断优先级控制寄存器 低
/*共4个优先级的中断，8位从高到低依次为
PPCA PLVD PADC PS PT1 PX1 PT0 PX0
复位后全为0
*/

//定义LCD1602的引脚
sbit LCD_RS=P0^7;
sbit LCD_RW=P0^6;
sbit LCD_E=P0^5;
#define LCD_DATA P2

//定义LCD的操作函数
void LcdExec();//执行命令
void LcdCommand(unsigned char command);//发送控制指令
void LcdWriteChar(unsigned char datas);//发送字符
void LcdRefreshScreen(unsigned char line1[16],unsigned char line2[16]);//刷新LCD屏幕。
unsigned char LcdMemory[2][16];//显存，两行，一行16个字符。

//定义全局变量
unsigned int test_time_h;//测试进行的时间，单位时。
unsigned int test_time_m;//测试进行的时间，单位分。
unsigned int test_time_s;//测试进行的时间，单位秒。test_time_ms到1000时这个数加1，且test_time_ms清零，刷新LCD屏幕。
unsigned int test_time_ms;//测试进行的时间，单位毫秒，定时器0每中断一次，这个值加5。（12MHz，60000个周期）
unsigned long battery_voltage_mV;//测试电池的电压，单位mV
unsigned long load_current_uA;//负载电流，单位uA
unsigned long battery_capaticy_uAh;//电池容量，单位uAh
unsigned char screen_scene;//屏幕场景
#define TIME_AND_CAPATICY 1 //时间和电池容量
#define TARGET_VOLTAGE_AND_STATUS 2 //目标电压和当前状态
unsigned char tmp_sting[];

/****************************************
**********以下是主函数**********
****************************************/

void main(){
    //初始化ADC
    ADC_CONTR=0xE0;//1110 0000，打开电源，最高速度，选P1.0脚作为输入
    P1ASF=0x01;//0000 0001，P1.0作为转换口
    EA=1;EADC=0;//开启总中断，不开ADC中断
    //初始化定时器0（用于计时）
    ET0=1;//开启定时器0中断
    TMOD=0x01;//定时器0为16位定时计数器
    TH0=60000/255;//设置为60000个周期，12MHz时为5ms             //20180401注：BUG????为啥不是65535-60000？可是实际使用又确实是1s。？？？？？？喵喵喵？？？
    TL0=60000%255;
    //设置中断优先级，定时器0比ADC高
    IPH=0x20;//定时器0优先级为2，ADC优先级为0
    //初始化LCD
	LcdCommand(0x38);   //0011 1000 显示模式设置：16×2显示，5×7点阵，8位数据接口
	LcdCommand(0x0C);   //0000 1100 显示模式设置：无光标
	LcdCommand(0x06);   //0000 0110 输入模式设置：光标右移，字符不移
	LcdCommand(0x01);   //清屏
	//初始化全局变量
	test_time_h=0;
	test_time_m=0;
	test_time_s=0;
	test_time_ms=0;
	battery_capaticy_uAh=0;
	//battery_voltage_mV=0;
	screen_scene=TIME_AND_CAPATICY;
	TR0=1;//启动定时器0
	while(1){
	    //读取电池电压
	    //每秒刷新一次屏幕和读取电压
	    if(test_time_ms==0 ){//&& test_time_s%3==0
	        //启动ADC
	        ADC_CONTR=0xE8;//1110 1000 启动ADC
	        //刷新屏幕
            //LcdRefreshScreen("Time: 000h00m00s","  00000.000mAh  ");
            //LcdRefreshScreen("00.000V 00.000mA","End Voltage:0.9V");
            if(screen_scene==TIME_AND_CAPATICY){
                sprintf(LcdMemory[0],"Time: %03uh%02um%02us",test_time_h,test_time_m,test_time_s);
                sprintf(LcdMemory[1],"  %05lu.%03lumAh  ",(unsigned long)battery_capaticy_uAh/1000,(unsigned long)battery_capaticy_uAh%1000);
                LcdRefreshScreen(LcdMemory[0],LcdMemory[1]);
                screen_scene=TARGET_VOLTAGE_AND_STATUS;
            }else if(screen_scene==TARGET_VOLTAGE_AND_STATUS){
                sprintf(LcdMemory[0],"%02lu.%03luV %02lu.%03lumA",(unsigned long)battery_voltage_mV/1000,(unsigned long)battery_voltage_mV%1000,(unsigned long)load_current_uA/1000,(unsigned long)load_current_uA%1000);
                //sprintf(LcdMemory[1],"  %012lu  ",(unsigned long)battery_voltage_mV);
                LcdRefreshScreen(LcdMemory[0],"End Voltage:0.9V");
                //LcdRefreshScreen(LcdMemory[0],LcdMemory[1]);
                screen_scene=TIME_AND_CAPATICY;
            }
            //读取电压
            battery_voltage_mV=(unsigned long)(ADC_RES*4+ADC_RESL)*5000/1024;
            //计算电流和容量
            load_current_uA=(unsigned long)battery_voltage_mV*1000/27;
            battery_capaticy_uAh+=(unsigned long)load_current_uA/3600;
	    }
	    PCON=0x01;////PCON第0位为IDL，置1后单片机进入空闲模式，可由中断唤醒
	}
}

/****************************************
**********以下是中断函数**********
****************************************/

void Timer0Interrupt() interrupt 1{
    test_time_ms+=5;
    if(test_time_ms==1000){
        test_time_ms=0;
        test_time_s++;
        if(test_time_s==60){
            test_time_s=0;
            test_time_m++;
            if(test_time_m==60){
                test_time_m=0;
                test_time_h++;
            }
        }
    }
    TH0=60000/255;//设置为60000个周期，12MHz时为5ms             //20180401注：BUG????为啥不是65535-60000？可是实际使用又确实是1s。？？？？？？喵喵喵？？？
    TL0=60000%255;
}

/****************************************
**********以下是LCD相关的函数**********
****************************************/

void Delay(int a){
    int b,c;
    for(b=0;b<a;b++){
        for(c=0;c<9;c++);
    }
}

void LcdExec(){//执行命令
    Delay(9);
	LCD_E=1;
	Delay(9);
	LCD_E=0;
	Delay(9);
}

void LcdCommand(unsigned char command){//发送控制指令
	LCD_RS=0;
	LCD_RW=0;
	LCD_DATA=command;
	LcdExec();
}

void LcdWriteChar(unsigned char datas){//发送字符
	LCD_RS=1;
	LCD_RW=0;
	LCD_DATA=datas;
	LcdExec();
}

void LcdRefreshScreen(unsigned char line1[18],unsigned char line2[18]){//刷新LCD屏幕
    unsigned char a;
	LcdCommand(0x80);
	for(a=0;a<17;a++){
		LcdWriteChar(line1[a]);		
	}
	LcdCommand(0xc0);
	for(a=0;a<17;a++){
		LcdWriteChar(line2[a]);		
	}
}
