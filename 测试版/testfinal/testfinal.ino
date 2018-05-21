#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <SFEMP3Shield.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

SFEMP3Shield MP3player;
SdFat sd;

int motor = 5;  //定义振动提醒的输出引脚为3
int FSR = A0;    //定义FSR压力传感器阻值的输入引脚
int voiSwi = 4;   //定义扬声器的开关
int val;

unsigned long TIME = 3000; //设定的坐下安全时间：3s
unsigned long TIME1 = 6000; //设定第一次超时时间长度：3s
unsigned long TIME2 = 8000; //设定第二次超时时间长度：2s


unsigned long starttime; //开始坐下时间点
unsigned long currenttime; //坐下持续时间中间点
unsigned long looptime;//时间差
unsigned long pressPoint = 1010;   //压力敏感度


void setup()
{
  pinMode(motor, OUTPUT);   //设置连接振动模块的引脚为输出
  pinMode(FSR, INPUT);      //设置FSR压力传感器为输入
  pinMode(voiSwi, OUTPUT);  //设置扬声器引脚为输出
  digitalWrite(voiSwi, HIGH);    //关闭扬声器电源
  
  MP3player.begin();                  //boot up the MP3 Player Shield

  power_timer1_disable();   //close modules which is useless
  power_timer2_disable();
  power_twi_disable();

  /*** Setup the WDT ***/
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
  
  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */
  
  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);

}

void loop()
{
  wdt_disable(); //看门狗禁止
  val = analogRead(FSR);            //读取模拟接口0 的值，即压力传感器传回的阻值，并将其赋给val
  if (val < pressPoint)             //若用户坐下
  {
    starttime = millis();           //获得坐下时刻

    digitalWrite(motor, HIGH);      //motor when first sit down
    delay(500);
    digitalWrite(motor, LOW);

    int result = 0;
    while (1) {                     //循环直到用户坐下时间达到要求或用户提前起身
      currenttime = millis();       //获得当前时间以获得时间差
      looptime = currenttime - starttime;      //两时间点相减获得时间差
      if (looptime >= TIME) {       //若达到设定时间则发出提醒
        voice_motor(1);
        keepSitting1();             //用户继续坐着 进入子循环1
        break;                      //用户已起身，退出当前计时
      }
      else val = analogRead(FSR);         //再次获得压力值以判断用户是否离开座位
      if (val > pressPoint) {             //若某时刻用户突然离开座位，例如拿一下书架上的书等
        result = wait();                  //若5秒内用户又回到座位上，wait()返回1，继续计时
        if (result == 0)break;            //否则返回0，退出计时
      }
    }
  }
  wdt_enable(9);
  enterSleep();
}



void voice_motor(int n)                                                      //提醒声音与振动控制函数
{
  if (n == 3) {                       //最后一次提醒采用一直振动模式，直到用户起身
    digitalWrite(voiSwi, LOW);          //开启扬声器电源
    MP3player.playTrack(n);             //tell the MP3 Shield to play a track

    digitalWrite(motor, HIGH);
    do {
      val = analogRead(FSR);
    } while (val < pressPoint);        //用户起身
    digitalWrite(motor, LOW);
    unsigned long start = millis();
    unsigned long current;
    do {                               //预设5min music播放时间
      current = millis();
      val = analogRead(FSR);
      if (val < pressPoint) {          //期间用户若再次坐下
        MP3player.stopTrack();         //关闭正在播放的语音提醒
        digitalWrite(voiSwi, HIGH);    //关闭扬声器电源
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

void keepSitting1()                      //第一次提醒无效子循环1函数keepsitting1()
{
  int result = 0;
  while (1) {                            //子循环直到用户坐下时间达到要求或用户提前起身
    currenttime = millis();              //获得当前时间以获得时间差
    looptime = currenttime - starttime;  //两时间点相减获得时间差
    if (looptime >= TIME1) {             //若达到超时时间则发出提醒
      voice_motor(2);
      keepSitting2();                    //用户继续坐着 进入子循环2
      break;                             //若用户起身，则退出子循环1
    }
    else val = analogRead(FSR);         //再次获得压力值以判断用户是否离开座位
    if (val > pressPoint) {             //若某时刻用户突然离开座位，例如拿一下书架上的书等
      result = wait();                  //若5秒内用户又回到座位上，wait()返回1，继续计时
      if (result == 0)break;            //否则返回0，退出计时
    }
  }

}


void keepSitting2()                                          //第二次（最后一次）提醒无效子循环2函数keepsitting2()
{
  int result = 0;
  while (1) {                    //子循环直到用户坐下时间达到要求或用户提前起身
    currenttime = millis();          //获得当前时间以获得时间差
    looptime = currenttime - starttime;      //两时间点相减获得时间差
    if (looptime >= TIME2) {  //若达到超时时间则发出提醒
      voice_motor(3);
      break;
    }
    else val = analogRead(FSR);         //再次获得压力值以判断用户是否离开座位
    if (val > pressPoint) {             //若某时刻用户突然离开座位，例如拿一下书架上的书等
      result = wait();                  //若5秒内用户又回到座位上，wait()返回1，继续计时
      if (result == 0)break;            //否则返回0，退出计时
    }
  }
}

int wait()                           //防止用户只是突然起身拿个东西等，等待5秒以判断
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

void enterSleep(void)
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
  sleep_enable();
  
  /* Now enter sleep mode. */
  sleep_mode();
  
  /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */
  
  /* Re-enable the peripherals. */
  power_all_enable();
}
