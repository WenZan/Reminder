int speaker = 9; //定义语音提醒器的输出引脚为9
int FSR = 0;//定义FSR压力传感器阻值的输入引脚
int val;
unsigned long TIME = 5000; //设定的坐下安全时间
unsigned long starttime; //开始坐下时间点
unsigned long stoptime; //坐下持续时间中间点
unsigned long looptime;//时间差

void setup()
{
  Serial.begin(9600);
  pinMode(speaker, OUTPUT); //设置连接蜂鸣器的引脚为输出
  pinMode(FSR, INPUT);   //设置FSR压力传感器为输入
}
void voice_out(void)//提醒声音输出控制函数
{
  digitalWrite(speaker, LOW);
  digitalWrite(speaker, HIGH);
//  delay(5000);
//  digitalWrite(speaker, LOW);
//  delay(500);
//  digitalWrite(speaker, HIGH);
//  delay(2000);
//  digitalWrite(speaker, LOW);
//  delay(500);
//  digitalWrite(speaker, HIGH);
//  delay(2000);
//  digitalWrite(speaker, LOW);
}
void loop()
{
  val = analogRead(FSR); //读取模拟接口0 的值，即压力传感器传回的阻值，并将其赋给val
  Serial.print("pressure:");   //在端口显示当前压力值（测试所用）
  Serial.println(val);        
  Serial.println();
  Serial.println();
  delay(500);
  if (val > 0) //若用户坐下
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
      Serial.print("pressure:");                       //在端口显示当前压力值（测试所用）
      Serial.println(val);
      Serial.print("looptime:");                 //在端口显示已坐下时间（测试所用）
      Serial.println(looptime);
      delay(500);                               //降低时间显示的频率（测试所用）
    } while (val > 0);                          //若用户仍然在座位上
  }
}

