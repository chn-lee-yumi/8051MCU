# include <AT89X52.h>
# include <intrins.h>
//void SEG7Test();//数码管测试
/*
一、设计任务
基于单片机控制，设计制作一个十字路口的交通灯系统，系统由 12 只 LED 组成，
分成东、南、西、北向四组，每组均含红、黄、绿 3 只 LED 指示灯。
二、设计要求
1．基本要求
(1) 各向绿灯和红灯点亮时间各为 10 秒；各向当绿灯亮 10 秒后，黄灯闪烁 5 次
后亮红灯。（20 分）
(2) 当东向和西向的绿灯（或红灯）同步点亮时，南向和北向的红灯（或绿灯）
必须同步点亮。（20 分）
(3) 12 只 LED 在电路板上按东南西北四个方向分布，依红黄绿顺序排列。（10 分）
2．发挥部分
(1) 各方向增加七段数码管，当绿灯（或红灯）点亮的最后几秒显示倒计时计数
值。（20 分）
(2) 可设置红绿灯的点亮时间（0-99 秒）及其倒计时计数时间。（20 分）
(3) 可在白天模式和夜间模式切换，白天模式的工作逻辑请参考基本要求，夜间
模式的工作逻辑为：各方向的黄灯每秒点亮一次，其他灯不亮。（10 分）
*/

/*
电路引脚说明：
P3的低四位为红绿灯控制(P3_0和P3_1为东西向，P3_0和P3_1为南北向，00为不亮，01为红灯，10为黄灯，11为绿灯)，
P3_4和P3_5为东西/南北向数码管选择(0为选通)，
P3_6和P3_7为AB按钮输入。
P1用于发送BCD码给数码管，低四位为个位数的管，高四位为十位数的管。
*/

void TimeCounterManage();//时间计数器处理函数
void ButtonManage();//按键处理函数
void TrafficLightDisplay();//红绿灯显示函数
void InitTrafficLight();//初始化红绿灯状态
void Seg7Display(unsigned char direction,unsigned char value);//BCD7段数码管显示函数
void Seg7Flash(unsigned char seg);//数码管闪烁函数
void ReadConfig();//读取红绿灯配置
void SaveConfig();//保存红绿灯配置

#define MACHINE_CYCLE_PER_MS 100 //一毫秒经过的机器周期数。如果是一个机器周期为1us，那么这里的值为1000。
#define SEG7 P1 //用SEG7表示P1端口。P1用于发送BCD码给数码管。

//变量用数组，代码会更简洁，但是现在没有时间改了。

unsigned char traffic_light_N_S;//南北向红绿灯，0为全不亮，1为红灯，2为黄灯，3为绿灯
unsigned char traffic_light_E_W;//东西向红绿灯
#define BLACK 0
#define RED 1
#define YELLOW 2
#define GREEN 3

unsigned char seg_7_N_S;//南北向数码管，数值为00~99
unsigned char seg_7_E_W;//东西向数码管
#define EAST_WEST 0
#define NORTH_SOUTH 1
#define HIDE_DISPLAY 255

#define BUTTON_A ~P3_6 //A键
#define BUTTON_B ~P3_7 //B键
unsigned char button_a_last=0; //A键历史状态
unsigned char button_b_last=0; //B键历史状态
unsigned char button_ab_last=0; //AB键历史状态

#define DAY_MODE 0 //白天模式
#define NIGHT_MODE 1 //夜间模式
#define TIME_SET_MODE 2 //点亮时间设置模式
#define GREEN_COUNTDOWN_SET_MODE 3 //倒计时设置模式
#define RED_COUNTDOWN_SET_MODE 4 //倒计时设置模式
unsigned char mode=DAY_MODE; //模式，默认白天模式

unsigned char time_N_S;//南北向绿灯时间，范围01-94（因为有5秒黄灯，红灯显示最多99）
unsigned char time_E_W;//东西向绿灯时间
unsigned char green_countdown_N_S;//南北向倒计时时间
unsigned char red_countdown_N_S;
unsigned char green_countdown_E_W;
unsigned char red_countdown_E_W;

unsigned char cursor;//光标，指向个位或者十位
#define ONES_N_S 0 //南北向个位
#define TENS_N_S 1 //南北向十位
#define ONES_E_W 2 //东西向个位
#define TENS_E_W 3 //东西向十位

unsigned char time_counter;//时间计数器，每次中断会加1。

unsigned char current_time_N_S;//南北向当前灯剩余时间
unsigned char current_time_E_W;//东西向当前灯剩余时间

//下面是STC12C5A60S2的EEPROM操作代码，参考官网文档
sfr IAP_DATA = 0xC2;//数据寄存器，读写的时候数据都放在这
sfr IAP_ADDRH = 0xC3;//地址高八位
sfr IAP_ADDRL = 0xC4;//地址低八位    IAP操作完成后，地址寄存器内容不变
sfr IAP_CMD = 0xC5;//命令寄存器，只有最后两位有用。00为无操作，01为读，10位编程，11为擦除
sfr IAP_TRIG= 0xC6;//IAPEN=1时，先对IAP_TRIG写入0x5A,再写入0xA5，IAP命令才生效
sfr IAP_CONTR = 0xC7;//控制寄存器，从高位到低位分别为：IAPEN SWBS SWRST CMD_FAIL - WT2 WT1 WT0
                     //IAPEN为IAP功能允许位，为0时禁止IAP操作，为1时允许。WT210为CPU等待时间，根据系统时钟设置，官网文档建议12MHz设置为011。其它位这里用不到，全给0。
//固件版本在v7.1及以上的芯片的EEPROM : 2048字节(0000H-07FFH)
//固件版本低于v7.1的芯片的EEPROM     : 1024字节(0000H-03FFH)
//这里使用STC12C5A60S2的固件版本号: 7.1.4I，所以有四个扇区可用
unsigned char IAPReadByte(unsigned char addrh,unsigned char addrl){
    //读字节
    unsigned char dat;
    IAP_CMD=0x01;
    IAP_ADDRL=addrl;
    IAP_ADDRH=addrh;
    IAP_TRIG=0x5a;
    IAP_TRIG=0xa5;
    _nop_();
    dat=IAP_DATA;
    return dat;
}
void IAPProgramByte(unsigned char addrh,unsigned char addrl,unsigned char dat){
    //编程字节
    IAP_CMD=0x02;
    IAP_ADDRL=addrl;
    IAP_ADDRH=addrh;
    IAP_DATA=dat;
    IAP_TRIG=0x5a;
    IAP_TRIG=0xa5;
    _nop_();
}
void IAPEraseSector(unsigned char addrh,unsigned char addrl){
    //擦除扇区，擦除地址所在扇区的所有字节
    IAP_CMD=0x03;
    IAP_ADDRL=addrl;
    IAP_ADDRH=addrh;
    IAP_TRIG=0x5a;
    IAP_TRIG=0xa5;
    _nop_();
}
//EEPROM操作代码到此结束
/************EEPROM空间分配***********
第一扇区（000H-1FFH） 红绿灯设置
000H time_N_S
001H time_E_W
002H green_countdown_N_S
003H red_countdown_N_S
004H green_countdown_E_W
005H red_countdown_E_W
*************************************/

void main(){
    //SEG7Test();
    //初始化定时器
    TMOD=0x01;//定时器0为16位定时计数器，用于按键检测
    ET0=0;//不开中断，采用查询法
    TH0=(65535-10000)/255;//按键抖动时间一般为5ms～10ms，这里使用10ms
    TL0=(65535-10000)%255;
    TR0=1;//启动定时器
    //初始化IAP，读取配置
    IAP_CONTR=0x83;
    ReadConfig();
    //初始化红绿灯状态
    InitTrafficLight();
    //循环检测按键和时间
    while(1){
        if(TF0==1){
            time_counter++;
            //时间计数器处理
            if(time_counter==50||time_counter==100) TimeCounterManage();
            //按键处理
            ButtonManage();
            //重置定时器
            TL0=(65535-10000)%255;
            TH0=(65535-10000)/255;
            TF0=0;
        }
    }
}

void TimeCounterManage(){//时间计数器处理函数
    //先判断模式
    switch(mode){
        case DAY_MODE:
            if(time_counter==100){//1秒
                //然后判断南北向红绿灯状态
                if(traffic_light_N_S==GREEN){
                    current_time_N_S--;
                    if(current_time_N_S==0){//换黄灯
                        traffic_light_N_S=YELLOW;
                        TrafficLightDisplay();
                        current_time_N_S=5;//黄灯5秒
                        Seg7Display(NORTH_SOUTH,HIDE_DISPLAY);//数码管不显示
                    }else if(current_time_N_S<=green_countdown_N_S){//到了倒计时时间
                        Seg7Display(NORTH_SOUTH,current_time_N_S);//显示倒计时时间
                    }
                }else if(traffic_light_N_S==BLACK){
                    current_time_N_S--;
                    traffic_light_N_S=YELLOW;
                    TrafficLightDisplay();
                    if(current_time_N_S==0){//换红灯
                        traffic_light_N_S=RED;
                        TrafficLightDisplay();
                        current_time_N_S=time_E_W+5;//对面绿灯时间+黄灯5秒
                        if(current_time_N_S<=red_countdown_N_S){
                            Seg7Display(NORTH_SOUTH,current_time_N_S); 
                        }else{
                            Seg7Display(NORTH_SOUTH,HIDE_DISPLAY);
                        }
                    }
                }else if(traffic_light_N_S==RED){
                    current_time_N_S--;
                    if(current_time_N_S==0){//换绿灯
                        traffic_light_N_S=GREEN;
                        TrafficLightDisplay();
                        current_time_N_S=time_N_S;
                        if(current_time_N_S<=green_countdown_N_S){
                            Seg7Display(NORTH_SOUTH,current_time_N_S); 
                        }else{
                            Seg7Display(NORTH_SOUTH,HIDE_DISPLAY);
                        }
                    }else if(current_time_N_S<=red_countdown_N_S){//到了倒计时时间
                        Seg7Display(NORTH_SOUTH,current_time_N_S);//显示倒计时时间
                    }
                }
                //然后判断东西向红绿灯状态
                if(traffic_light_E_W==GREEN){
                    current_time_E_W--;
                    if(current_time_E_W==0){//换黄灯
                        traffic_light_E_W=YELLOW;
                        TrafficLightDisplay();
                        current_time_E_W=5;//黄灯5秒
                        Seg7Display(EAST_WEST,HIDE_DISPLAY);//数码管不显示
                    }else if(current_time_E_W<=green_countdown_E_W){//到了倒计时时间
                        Seg7Display(EAST_WEST,current_time_E_W);//显示倒计时时间
                    }
                }else if(traffic_light_E_W==BLACK){
                    current_time_E_W--;
                    traffic_light_E_W=YELLOW;
                    TrafficLightDisplay();
                    if(current_time_E_W==0){//换红灯
                        traffic_light_E_W=RED;
                        TrafficLightDisplay();
                        current_time_E_W=time_N_S+5;
                        if(current_time_E_W<=red_countdown_E_W){
                            Seg7Display(EAST_WEST,current_time_E_W); 
                        }else{
                            Seg7Display(EAST_WEST,HIDE_DISPLAY);
                        }
                    }
                }else if(traffic_light_E_W==RED){
                    current_time_E_W--;
                    if(current_time_E_W==0){//换绿灯
                        traffic_light_E_W=GREEN;
                        TrafficLightDisplay();
                        current_time_E_W=time_E_W;
                        if(current_time_E_W<=green_countdown_E_W){
                            Seg7Display(EAST_WEST,current_time_E_W); 
                        }else{
                            Seg7Display(EAST_WEST,HIDE_DISPLAY);
                        }
                    }else if(current_time_E_W<=red_countdown_E_W){//到了倒计时时间
                        Seg7Display(EAST_WEST,current_time_E_W);//显示倒计时时间
                    }
                }
                time_counter=0;
            }
            if(time_counter==50){//0.5秒，控制黄灯闪烁
                if(traffic_light_N_S==YELLOW){
                    traffic_light_N_S=BLACK;
                    TrafficLightDisplay();
                }
                if(traffic_light_E_W==YELLOW){
                    traffic_light_E_W=BLACK;
                    TrafficLightDisplay();
                }
            }
            break;
        case NIGHT_MODE:
            //黄灯闪烁
            if(time_counter==50){//0.5秒，控制黄灯闪烁
                if(traffic_light_N_S==YELLOW){
                    traffic_light_N_S=BLACK;
                    traffic_light_E_W=BLACK;
                }else{
                    traffic_light_N_S=YELLOW;
                    traffic_light_E_W=YELLOW;
                }
            }
            TrafficLightDisplay();
            time_counter=0;
            break;
        case TIME_SET_MODE:
            //光标闪烁
            if(time_counter==50){
                if(cursor==ONES_N_S||cursor==TENS_N_S){//根据前面写的代码，每次输出完之后SEG7都是EAST_WEST方向的，因此如果NORTH_SOUTH要闪烁就要再输出一次
                    Seg7Display(NORTH_SOUTH,time_N_S);
                }
                Seg7Flash(cursor);
            }
            if(time_counter==100){
                Seg7Display(NORTH_SOUTH,time_N_S);
                Seg7Display(EAST_WEST,time_E_W);
                time_counter=0;
            }
            break;
        case GREEN_COUNTDOWN_SET_MODE:
            //光标闪烁
            if(time_counter==50){
                if(cursor==ONES_N_S||cursor==TENS_N_S){//根据前面写的代码，每次输出完之后SEG7都是EAST_WEST方向的，因此如果NORTH_SOUTH要闪烁就要再输出一次
                    Seg7Display(NORTH_SOUTH,green_countdown_N_S);
                }
                Seg7Flash(cursor);
            }
            if(time_counter==100){
                Seg7Display(NORTH_SOUTH,green_countdown_N_S);
                Seg7Display(EAST_WEST,green_countdown_E_W);
                time_counter=0;
            }
            break;
        case RED_COUNTDOWN_SET_MODE:
            //光标闪烁
            if(time_counter==50){
                if(cursor==ONES_N_S||cursor==TENS_N_S){//根据前面写的代码，每次输出完之后SEG7都是EAST_WEST方向的，因此如果NORTH_SOUTH要闪烁就要再输出一次
                    Seg7Display(NORTH_SOUTH,red_countdown_N_S);
                }
                Seg7Flash(cursor);
            }
            if(time_counter==100){
                Seg7Display(NORTH_SOUTH,red_countdown_N_S);
                Seg7Display(EAST_WEST,red_countdown_E_W);
                time_counter=0;
            }
            break;
    }
}

void ButtonManage(){//按键处理函数
    /*发挥部分思路
        电路有A、B两个按键。
        在白天或夜间模式下，按A键或B键，会在白天和夜间模式下切换。(松开按键才切换)
        在白天模式下，同时按下AB键，进入点亮时间设置模式。
        在点亮时间设置模式下，按A键依次选择：南北方向个位、南北方向十位、东西方向个位、东西方向十位。被选中的那位数码管会闪。（BCD码给1111，数码管就会不显示）
        在点亮时间设置模式下，按B键，会给当前位+1，满10变0。
        在点亮时间设置模式下，同时按下AB键，转到绿灯倒计时设置模式。
        在绿灯倒计时设置模式下，按A键依次选择：东西方向十位、东西方向个位、南北方向十位、南北方向个位。被选中的那位数码管会闪。（BCD码给1111，数码管就会不显示）
        在绿灯倒计时设置模式下，按B键，会给当前位+1，满10变0。
        在绿灯倒计时设置模式下，同时按下AB键，转到红灯倒计时设置模式。
        在红灯倒计时设置模式下，A、B键作用同绿灯倒计时。
        在红灯倒计时设置模式下，同时按下AB键，保存设置并回到白天模式。
    */
    //按键响应
    if(button_ab_last==1 && BUTTON_A==0 && BUTTON_B==0 ){//AB键同时按下
        switch(mode){
            case DAY_MODE:
                mode=TIME_SET_MODE;
                cursor=ONES_N_S;
                Seg7Display(NORTH_SOUTH,time_N_S);//显示当前点亮时间
                Seg7Display(EAST_WEST,time_E_W);
                traffic_light_N_S=BLACK;
                traffic_light_E_W=BLACK;
                TrafficLightDisplay();
                break;
            case TIME_SET_MODE:
                mode=GREEN_COUNTDOWN_SET_MODE;
                cursor=ONES_N_S;
                Seg7Display(NORTH_SOUTH,green_countdown_N_S);//显示当前倒计时时间
                Seg7Display(EAST_WEST,green_countdown_E_W);
                break;
            case GREEN_COUNTDOWN_SET_MODE:
                mode=RED_COUNTDOWN_SET_MODE;
                cursor=ONES_N_S;
                Seg7Display(NORTH_SOUTH,red_countdown_N_S);//显示当前倒计时时间
                Seg7Display(EAST_WEST,red_countdown_E_W);
                break;    
            case RED_COUNTDOWN_SET_MODE:
                mode=DAY_MODE;
                SaveConfig();//把设置写入到EEPROM
                InitTrafficLight();
                break;
        }
        button_ab_last=0;
    }else if(button_a_last==1 && BUTTON_A==0 && button_ab_last==0){//按下A键，松开时
        switch(mode){
            case DAY_MODE:
                mode=NIGHT_MODE;
                Seg7Display(NORTH_SOUTH,HIDE_DISPLAY);
                Seg7Display(EAST_WEST,HIDE_DISPLAY);
                break;
            case NIGHT_MODE:
                mode=DAY_MODE;
                InitTrafficLight();
                break;
            case TIME_SET_MODE://设置时间
                switch(cursor){
                    case ONES_N_S:
                        time_N_S+=1;
                        if(time_N_S%10==0) time_N_S-=10;
                        if(time_N_S==0) time_N_S=1;
                        break;
                    case TENS_N_S:
                        time_N_S+=10;
                        if(time_N_S>=100) time_N_S-=100;
                        if(time_N_S==0) time_N_S=10;
                        break;
                    case ONES_E_W:
                        time_E_W+=1;
                        if(time_E_W%10==0) time_E_W-=10;
                        if(time_E_W==0) time_E_W=1;
                        break;
                    case TENS_E_W:
                        time_E_W+=10;
                        if(time_E_W>=100) time_E_W-=100;
                        if(time_E_W==0) time_E_W=10;
                        break;
                }
                Seg7Display(NORTH_SOUTH,time_N_S);
                Seg7Display(EAST_WEST,time_E_W);
                break;
            case GREEN_COUNTDOWN_SET_MODE:
                switch(cursor){
                    case ONES_N_S:
                        green_countdown_N_S+=1;
                        if(green_countdown_N_S%10==0) green_countdown_N_S-=10;
                        break;
                    case TENS_N_S:
                        green_countdown_N_S+=10;
                        if(green_countdown_N_S>=100) green_countdown_N_S-=100;
                        break;
                    case ONES_E_W:
                        green_countdown_E_W+=1;
                        if(green_countdown_E_W%10==0) green_countdown_E_W-=10;
                        break;
                    case TENS_E_W:
                        green_countdown_E_W+=10;
                        if(green_countdown_E_W>=100) green_countdown_E_W-=100;
                        break;
                }
                Seg7Display(NORTH_SOUTH,green_countdown_N_S);
                Seg7Display(EAST_WEST,green_countdown_E_W);
                break;
            case RED_COUNTDOWN_SET_MODE:
                switch(cursor){
                    case ONES_N_S:
                        red_countdown_N_S+=1;
                        if(red_countdown_N_S%10==0) red_countdown_N_S-=10;
                        break;
                    case TENS_N_S:
                        red_countdown_N_S+=10;
                        if(red_countdown_N_S>=100) red_countdown_N_S-=100;
                        break;
                    case ONES_E_W:
                        red_countdown_E_W+=1;
                        if(red_countdown_E_W%10==0) red_countdown_E_W-=10;
                        break;
                    case TENS_E_W:
                        red_countdown_E_W+=10;
                        if(red_countdown_E_W>=100) red_countdown_E_W-=100;
                        break;
                }
                Seg7Display(NORTH_SOUTH,red_countdown_N_S);
                Seg7Display(EAST_WEST,red_countdown_E_W);
                break;
        }
    }else if(button_b_last==1 && BUTTON_B==0 && button_ab_last==0){//按下B键，松开时
        switch(mode){
            case DAY_MODE://
                mode=NIGHT_MODE;
                Seg7Display(NORTH_SOUTH,HIDE_DISPLAY);
                Seg7Display(EAST_WEST,HIDE_DISPLAY);
                break;
            case NIGHT_MODE:
                mode=DAY_MODE;
                InitTrafficLight();
                break;
            case TIME_SET_MODE://移动光标（移动选中的数码管）
                if(cursor==TENS_E_W){
                    cursor=ONES_N_S;
                }else{
                    cursor++;
                }
                Seg7Display(NORTH_SOUTH,time_N_S);
                Seg7Display(EAST_WEST,time_E_W);
                break;
            case GREEN_COUNTDOWN_SET_MODE:
                if(cursor==TENS_E_W){
                    cursor=ONES_N_S;
                }else{
                    cursor++;
                }
                Seg7Display(NORTH_SOUTH,green_countdown_N_S);
                Seg7Display(EAST_WEST,green_countdown_E_W);
                break;
            case RED_COUNTDOWN_SET_MODE:
                if(cursor==TENS_E_W){
                    cursor=ONES_N_S;
                }else{
                    cursor++;
                }
                Seg7Display(NORTH_SOUTH,red_countdown_N_S);
                Seg7Display(EAST_WEST,red_countdown_E_W);
        }
    }
    //按键历史状态记录
    button_a_last=BUTTON_A;
    button_b_last=BUTTON_B;
    if(BUTTON_A==1 && BUTTON_B==1) button_ab_last=1;
}

void InitTrafficLight(){//初始化红绿灯状态
    traffic_light_N_S=GREEN;
    traffic_light_E_W=RED;
    TrafficLightDisplay();
    current_time_N_S=time_N_S;
    current_time_E_W=time_N_S+5;//对面绿灯时间+黄灯5秒
    if(current_time_N_S<=green_countdown_N_S){
        Seg7Display(NORTH_SOUTH,current_time_N_S);
    }else{
        Seg7Display(NORTH_SOUTH,HIDE_DISPLAY);
    }
    if(current_time_E_W<=red_countdown_E_W){
        Seg7Display(EAST_WEST,current_time_E_W);
    }else{
        Seg7Display(EAST_WEST,HIDE_DISPLAY);
    }
}

void TrafficLightDisplay(){//红绿灯显示函数
    //P3的低四位为红绿灯控制，P3_4和P3_5为东西/南北向数码管选择，P3_6和P3_7为AB按钮输入。
    //保持P3_4、P3_5不变，P3_6、P3_7写入1（这两个口用于读取按钮输入的），,然后修改低四位的红绿灯状态
    //南北向使用P3_2和P3_3，东西向使用P3_0和P3_1
    P3=(P3&0x30|0xc0)|(traffic_light_N_S<<2|traffic_light_E_W);
}

void Seg7Display(unsigned char direction,unsigned char value){//BCD7段数码管显示函数。direction为0表示东西向，为1表示南北向。value表示数值。
    //P1用于发送BCD码给数码管，低四位为个位数的管，高四位为十位数的管。
    //先送BCD码，然后选通数码管，然后等待几微秒，然后取消选通。
    unsigned char ones,tens;//个位、十位
    if(value==HIDE_DISPLAY){
        SEG7=0xff;//隐藏显示
    }else{
        tens=value/10;
        ones=value%10;
        SEG7=tens<<4|ones;
    }
    if(direction==EAST_WEST){
        P3_4=0;
        P3_4=1;
    }else if(direction==NORTH_SOUTH){
        P3_5=0;
        P3_5=1;
    }
}

void Seg7Flash(unsigned char seg){//数码管闪烁函数。作用为关闭单个数码管。seg为数码管位置。
    if(seg==ONES_N_S||seg==ONES_E_W){
        SEG7=SEG7|0x0f;
    }
    if(seg==TENS_N_S||seg==TENS_E_W){
        SEG7=SEG7|0xf0;
    }
    if(seg==ONES_N_S||seg==TENS_N_S){
        P3_5=0;
        P3_5=1;
    }
    if(seg==ONES_E_W||seg==TENS_E_W){
        P3_4=0;
        P3_4=1;
    }
}

void ReadConfig(){//读取红绿灯配置
    time_N_S=IAPReadByte(0x00,0x00);
    time_E_W=IAPReadByte(0x00,0x01);
    green_countdown_N_S=IAPReadByte(0x00,0x02);
    red_countdown_N_S=IAPReadByte(0x00,0x03);
    green_countdown_E_W=IAPReadByte(0x00,0x04);
    red_countdown_E_W=IAPReadByte(0x00,0x05);
    //如果没数据或者数据损坏了
    if(time_N_S>99 || time_N_S==0){
        time_N_S=10;
        time_E_W=10;
        green_countdown_N_S=5;
        red_countdown_N_S=10;
        green_countdown_E_W=5;
        red_countdown_E_W=10;
        SaveConfig();
    }
}

void SaveConfig(){//保存红绿灯配置
    ///*STC12C5A60S2
    IAPEraseSector(0x00,0x00);//先擦再写
    IAPProgramByte(0x00,0x00,time_N_S);
    IAPProgramByte(0x00,0x01,time_E_W);
    IAPProgramByte(0x00,0x02,green_countdown_N_S);
    IAPProgramByte(0x00,0x03,red_countdown_N_S);
    IAPProgramByte(0x00,0x04,green_countdown_E_W);
    IAPProgramByte(0x00,0x05,red_countdown_E_W);
    //*/
    /*//STC89C52
    IAPEraseSector(0x20,0x00);//先擦再写
    IAPProgramByte(0x20,0x00,time_N_S);
    IAPProgramByte(0x20,0x01,time_E_W);
    IAPProgramByte(0x20,0x02,green_countdown_N_S);
    IAPProgramByte(0x20,0x03,red_countdown_N_S);
    IAPProgramByte(0x20,0x04,green_countdown_E_W);
    IAPProgramByte(0x20,0x05,red_countdown_E_W);
    */
}
/*
void SEG7Test(){//数码管测试
    char tmp_1;
    unsigned long tmp_2=0;
    Seg7Display(NORTH_SOUTH,HIDE_DISPLAY);
    Seg7Display(EAST_WEST,HIDE_DISPLAY);
    for(tmp_1=0;tmp_1<21;tmp_1++){
        Seg7Display(NORTH_SOUTH,tmp_1);
        while(tmp_2<1000*1000) tmp_2++;
        tmp_2=0;
    }
    for(tmp_1=0;tmp_1<21;tmp_1++){
        Seg7Display(EAST_WEST,tmp_1);
        while(tmp_2<1000*1000) tmp_2++;
        tmp_2=0;
    }
}*/