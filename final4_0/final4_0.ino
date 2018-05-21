#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <SFEMP3Shield.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

SFEMP3Shield MP3player;
SdFat sd;

int motor = 5;     //定义振动提醒的输出引脚为3
int FSR = A0;      //定义FSR压力传感器阻值的输入引脚
int voiSwi = 4;    //定义扬声器的开关
int val;           //存放临时读取的压力值

unsigned long TIME = 2400000;   //最长坐下时间：40分钟=2400000微秒
unsigned long TIME1 = 2700000;  //第一次超时时间长度：5分钟（数值显示为45分钟）
unsigned long TIME2 = 2760000;  //第二次超时时间长度：1分钟（数值显示为46分钟）


unsigned long starttime;           //开始坐下时间点
unsigned long currenttime;         //当前坐下时间点
unsigned long looptime;            //已坐下时间
unsigned long pressPoint = 1010;   //压力敏感度


void setup()
{
  pinMode(motor, OUTPUT);        //连接振动模块的引脚为输出
  pinMode(FSR, INPUT);           //FSR压力传感器为输入
  pinMode(voiSwi, OUTPUT);       //扬声器引脚为输出
  digitalWrite(voiSwi, HIGH);    //关闭扬声器电源
  
  MP3player.begin();             //初始化MP3模块

  power_timer1_disable();        //关闭不必要的模块
  power_timer2_disable();
  power_twi_disable();
  
  MCUSR &= ~(1<<WDRF);
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  WDTCSR = 1<<WDP0 | 1<<WDP3; 
  WDTCSR |= _BV(WDIE);
  
}

void loop()
{
  wdt_disable();                    //禁止看门狗
  val = analogRead(FSR);            //读取压力值
  if (val < pressPoint)             //用户坐下
  {
    starttime = millis();           //获得坐下时刻

    digitalWrite(motor, HIGH);      //震动一次
    delay(500);
    digitalWrite(motor, LOW);

    int result = 0;
    while (1) {                     //循环直到用户坐下时间达到要求或用户提前起身
      currenttime = millis();       //获得当前时间
      looptime = currenttime - starttime;   //相减得出已坐下时间
      if (looptime >= TIME) {       
        voice_motor(1);             //达到设定时间发出提醒
        keepSitting(1);             //用户继续坐着 进入子循环1
        break;                      //用户已起身，退出当前计时
      }
      else val = analogRead(FSR);      
      if (val > pressPoint) {       //若某时刻用户突然离开座位，例如拿一下书架上的书等
        result = wait();            //若5秒内用户又回到座位上，wait()返回1，继续计时
        if (result == 0)break;      //否则返回0，退出计时
      }
    }
  }
  wdt_enable(9);                     //开启看门狗
  enterSleep();                      //进入睡眠模式
}



//提醒声音与振动控制函数
void voice_motor(int n)                                                      
{
  if (n == 3) {                      //最后一次提醒采用一直振动模式，直到用户起身
    digitalWrite(voiSwi, LOW);       //开启扬声器电源
    MP3player.playTrack(n);          //播放特定音乐

    digitalWrite(motor, HIGH);
    do {
      val = analogRead(FSR);
    } while (val < pressPoint);      
    digitalWrite(motor, LOW);
    unsigned long start = millis();
    unsigned long current;
    do {                              //播放5分钟音乐
      current = millis();
      val = analogRead(FSR);
      if (val < pressPoint) {         //期间用户再次坐下，相当于用户主动要求关闭音乐
        MP3player.stopTrack();        //关音乐
        digitalWrite(voiSwi, HIGH);   //关扬声器
        break;
      }
    } while (current - start < 300000);

  } else {                              //若不是最后一次提醒采用以下振动模式
    //振动三次，每次振动一秒，间隔半秒
    //第一次
    digitalWrite(motor, LOW);
    digitalWrite(motor, HIGH);
    delay(500);
    digitalWrite(motor, LOW);
    delay(200);
    //第二次
    digitalWrite(motor, HIGH);
    delay(500);
    digitalWrite(motor, LOW);
    delay(200);
    //第三次
    digitalWrite(motor, HIGH);
    delay(500);
    digitalWrite(motor, LOW);
  }

}

//提醒无效状态函数keepsitting()
void keepSitting(int mode)                      
{
  int result = 0;
  while (1) {                            
    currenttime = millis();              
    looptime = currenttime - starttime;  
    if ( (1==mode && looptime >= TIME1) || (2==mode && looptime >= TIME2) ) {             
      voice_motor(mode+1);                      
      if(1 == mode) keepSitting(2);     //用户继续坐着 进入状态2
      break;                            //若用户起身，则退出
    }
    else  val = analogRead(FSR);        //再次判断用户是否离开座位
    if (val > pressPoint) {             
      result = wait();                  
      if (result == 0)break;            
    }
  }

}

//防止用户只是突然起身拿个东西等，等待5秒以判断
int wait()                          
{
  unsigned long start = millis();
  unsigned long current;
  do {
    current = millis();
    val = analogRead(FSR);
    if (val < pressPoint) return 1;
  } while (current - start < 5000);
  return 0;
}

//设置睡眠状态
void enterSleep(void)
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   
  sleep_enable();
  sleep_mode();
  sleep_disable(); 
  power_all_enable();
}
