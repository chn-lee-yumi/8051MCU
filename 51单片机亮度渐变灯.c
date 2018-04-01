#include <reg51.h>

sbit LED=P2^0;//采用P2.0口输出，可以任意改
int time;//临时变量，每次中断都加1，达到cycle就修改占空比（亮度）
int ratio;//当前占空比（0~cycle）
int direction;//0为渐亮，1为渐暗。变量类型也可用char或bit。
#define cycle 100 //控制修改亮度的时间间隔（单位为定时器中断次数），也是占空比精度（默认100份，即精度为1%）
#define cycle_2 300 //控制定时器周期（默认1000，即1ms）

void main(){
    LED=0;
    ratio=0;
    direction=0;
    time=0;
    EA=1;//开启总中断
    ET0=1;//开启定时器0中断
    TMOD=0x01;//定时器0为16位定时计数器
    TH0=250;TL0=0;//初始值随便设，中断后会自动设正确的值
    TR0=1;//启动定时器0
    while(1){//坐等中断
        PCON=0x01;////PCON第0位为IDL，置1后单片机进入空闲模式，可由中断唤醒
    }
}

void Timer0Interrupt() interrupt 1{
    if(time>=ratio){
        LED=0;
    }else{
        LED=1;
    }
    time++;
    if(time>cycle) {//换亮度
        time=0;
        if(direction==0){
            ratio++;
            if(ratio>=cycle+10) direction=1;//若想LED最亮时亮久一点，可以增加此处，如+10。
        }else{
            ratio--;
            if(ratio<=0) direction=0;
        }
    }
    TH0=(65535-cycle_2)/255;//设置周期
    TL0=(65535-cycle_2)%255;
}
/*注：
TH0=(65535-1000)/255;
TL0=(65535-1000)%255;
如上的表达式，减数（就是那个1000）不能小于256，否则会有奇怪的bug出现。（LED约5秒闪一次）
坑了我一个多小时！
猜测是编译器的BUG。
*/
//待改进：LED过暗时频闪特别明显，可以当ratio小于一定值时就直接LED=0，继续计算time，当ratio大于一定值，再正常处理。