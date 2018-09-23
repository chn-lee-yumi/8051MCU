/*
人体感应四段延迟灯控制器
效果：4条LED灯带，晚上感应到人来就按顺序逐条点亮，间隔约1s，当没人活动且过一段时间后（例如8s），按顺序逐条熄灭（顺序同点亮顺序）
引脚定义：
P3.0：人体感应输入（采用HC-SR501，加焊光敏电阻）
P1.0~1.3：LED灯输出（接LED灯带MOS管栅极）
MOS管选用IRF540N，源接LED灯带+，漏接+5V，栅接单片机LED灯输出。IRF540N资料见：https://wenku.baidu.com/view/549f876caf45b307e9719740.html
*/

#include <reg51.h>
#define DELAY 8

int time_50ms = 0;
int time_s = 0;

sbit HAS_HUMAN = P3^0;
sbit LED_0 = P1^0;
sbit LED_1 = P1^1;
sbit LED_2 = P1^2;
sbit LED_3 = P1^3;

char status = 0;//LED灯状态
#define POWEROFF 0
#define START 1
#define HOLD 2
#define END 3

void main() {
    P1=0xff;
    //初始化定时器0（用于计时）
    EA = 1; //开启总中断
    ET0 = 1; //开启定时器0中断
    TMOD = 0x01; //定时器0为16位定时计数器
    while (1) {
        if (HAS_HUMAN) { //如果晚上有人经过
            status = START;
            time_s = 0;
            TR0 = 1; //启动定时器0
            while (status!=POWEROFF) {
                //进入空闲模式，省电
                //PCON = 1;
            }
            TR0 = 0; //关闭定时器0
        }
    }
}

void Timer0Interrupt() interrupt 1 {
    time_50ms++;
    if (time_50ms == 20) {
        time_s++;
        time_50ms = 0;
    }

    //***下面是控制灯的代码
    if (status == START) {//开灯
        switch (time_s) {
            case 1:
                LED_0 = 0;
                break;
            case 2:
                LED_1 = 0;
                break;
            case 3:
                LED_2 = 0;
                break;
            case 4:
                LED_3 = 0;
                status = HOLD;
                break;
        }
    }
    if (status == HOLD){
        while (HAS_HUMAN); //延时直至没有人
        status = END;
        time_s = 0;
    }
    if (status == END && time_s<=DELAY){
        if(HAS_HUMAN) status = HOLD; //如果延迟时间内有人活动，刷新延迟时间
    }
    if (status == END) {//关灯
        switch (time_s) {
            case DELAY+1:
                LED_0 = 1;
                break;
            case DELAY+2:
                LED_1 = 1;
                break;
            case DELAY+3:
                LED_2 = 1;
                break;
            case DELAY+4:
                LED_3 = 1;
                status = POWEROFF;
                break;
        }
    }
    //***控制灯的代码到此结束

    TH0 = (65535 - 60000) / 256; //设置为60000个周期，12MHz时为50ms
    TL0 = (65535 - 60000) % 256;
}