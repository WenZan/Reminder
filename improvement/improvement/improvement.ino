#include <SPI.h>
//Add the SdFat Libraries
#include <SdFat.h>
#include <SdFatUtil.h>

//and the MP3 Shield Library
#include <SFEMP3Shield.h>

//create and name the library object
SFEMP3Shield MP3player;
SdFat sd;

int motor = 3; //定义振动提醒的输出引脚为3
int FSR = 0;//定义FSR压力传感器阻值的输入引脚
int val;
byte result;

unsigned long TIME = 500; //设定的坐下安全时间
unsigned long TIME1 = 20000; //设定第一次超时时间长度
unsigned long TIME2 = 30000; //设定第二次超时时间长度
unsigned long starttime; //开始坐下时间点
unsigned long currenttime; //坐下持续时间中间点
unsigned long looptime;//时间差
unsigned long pressPoint = 900;   //压力敏感度


void setup()
{
  Serial.begin(115200);
  pinMode(motor, OUTPUT); //设置连接振动模块的引脚为输出
  pinMode(FSR, INPUT);   //设置FSR压力传感器为输入

  //boot up the MP3 Player Shield
  result = MP3player.begin();
  //check result, see readme for error codes.
  if (result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
  }

  Serial.println("OK");
}




void voice_motor(int n)                                                      //提醒声音与振动控制函数
{
  //make sure the MP3player is ready to play music
  MP3player.stopTrack();

  //tell the MP3 Shield to play a track
  MP3player.playTrack(n);
  //check result, see readme for error codes.
  

  if (n <= 2) { //若不是最后一次提醒采用以下振动模式
    //振动三次，每次振动一秒，间隔半秒
    //第一次
    digitalWrite(motor, LOW);
    analogWrite(motor, 255);
    delay(1000);
    digitalWrite(motor, LOW);
    delay(500);
    //第二次
    analogWrite(motor, 255);
    delay(1000);
    digitalWrite(motor, LOW);
    delay(500);
    //第三次
    analogWrite(motor, 255);
    delay(1000);
    digitalWrite(motor, LOW);
  } else {           //最后一次提醒采用一直振动模式，直到用户起身
    analogWrite(motor, 255);
    do {
      val = analogRead(FSR);
    } while (val < pressPoint);
    digitalWrite(motor, LOW);
  }

}


void keepSitting2()                                          //第二次（最后一次）提醒无效子循环2函数keepsitting2()
{
  do {                    //子循环直到用户坐下时间达到要求或用户提前起身
    currenttime = millis();          //获得当前时间以获得时间差
    looptime = currenttime - starttime;      //两时间点相减获得时间差
    if (looptime >= TIME2) {  //若达到超时时间则发出提醒
      voice_motor(4);
      break;
    }
    else val = analogRead(FSR);         //再次获得压力值以判断用户是否离开座位
  } while (val < pressPoint);                          //若用户仍然在座位上
}

void keepSitting1()                                             //第一次提醒无效子循环1函数keepsitting1()
{
  do {                    //子循环直到用户坐下时间达到要求或用户提前起身
    currenttime = millis();          //获得当前时间以获得时间差
    looptime = currenttime - starttime;      //两时间点相减获得时间差
    if (looptime >= TIME1) {  //若达到超时时间则发出提醒
      voice_motor(2);
      keepSitting2();     //用户继续坐着 进入子循环2
      //若用户起身，则退出子循环1
      break;
    }
    else val = analogRead(FSR);         //再次获得压力值以判断用户是否离开座位
  } while (val < pressPoint);                          //若用户仍然在座位上
}





void loop()                                                                             //Loop()
{
  val = analogRead(FSR); //读取模拟接口0 的值，即压力传感器传回的阻值，并将其赋给val
  if (val < pressPoint) //若用户坐下
  {
    starttime = millis();           //获得坐下时刻

    do {                    //循环直到用户坐下时间达到要求或用户提前起身
      currenttime = millis();          //获得当前时间以获得时间差
      looptime = currenttime - starttime;      //两时间点相减获得时间差
      if (looptime >= TIME) {  //若达到设定时间则发出提醒
        voice_motor(1);  
        keepSitting1();   //用户继续坐着 进入子循环1
        //用户已起身，则退出当前循环
        break;
      }
      else val = analogRead(FSR);         //再次获得压力值以判断用户是否离开座位
    } while (val < pressPoint);                          //若用户仍然在座位上
  }
}

