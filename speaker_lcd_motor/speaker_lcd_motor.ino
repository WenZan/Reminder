int motor = 10; //定义振动提醒的输出引脚为10
int speaker = 4;  //定义语音提醒器的输出引脚为4
int FSR = 0;//定义FSR压力传感器阻值的输入引脚
int val;
unsigned long TIME = 5000; //设定的坐下安全时间
unsigned long starttime; //开始坐下时间点
unsigned long stoptime; //坐下持续时间中间点
unsigned long looptime;//时间差

#include <Wire.h>   
#include "LiquidCrystal_I2C.h"

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup()
{
  Serial.begin(9600);
  pinMode(speaker, OUTPUT); //设置连接蜂鸣器的引脚为输出
  pinMode(FSR, INPUT);   //设置FSR压力传感器为输入
}
void voice_out(void)//提醒声音输出与振动控制函数
{
  digitalWrite(speaker, LOW);
  digitalWrite(speaker,HIGH);
  digitalWrite(motor, LOW);
  analogWrite(motor,255);
  delay(3000);
  digitalWrite(motor, LOW);
}
void loop()
{
  val = analogRead(FSR); //读取模拟接口0 的值，即压力传感器传回的阻值，并将其赋给val
  
  //在端口显示当前压力值（测试所用）
  Serial.print("pressure:");  
  Serial.println(val);        
  Serial.println();
  Serial.println();
  delay(500);
  
  //在LCD屏上显示压力值（测试所用）
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.print("pressure:");
  lcd.print(val);


  if (val < 500) //若用户坐下
  {
    starttime = millis();           //获得坐下时刻
    do {                    //循环直到用户坐下时间达到要求或用户提前起身
      stoptime = millis();          //获得当前时间以获得时间差
      looptime = stoptime - starttime;      //两时间点相减获得时间差
      if (looptime >= TIME) {  //若达到设定时间则发出提醒并退出此次循环
        voice_out();             
        break;
      }
      else val = analogRead(FSR);         //再次获得压力值以判断用户是否离开座位

      //在端口显示当前压力值（测试所用）
      Serial.print("pressure:");                       
      Serial.println(val);
      Serial.print("looptime:");                //在端口显示已坐下时间（测试所用）
      Serial.println(looptime);
      delay(500);                               //降低时间显示的频率（测试所用）
      
      //在lcd屏上显示当前压力值与坐下时间（测试所用）
      lcd.init();  
      lcd.print("pressure:");         
      lcd.print(val);
      lcd.setCursor(0,1);
      lcd.print("looptime:");
      lcd.print(looptime);
    } while (val < 500);                          //若用户仍然在座位上
  }
}

