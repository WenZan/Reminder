 int val;
 int FSR = 0;//定义FSR压力传感器阻值的输入引脚
void setup() {
  // put your setup code here, to run once:
 

 Serial.begin(9600);
 pinMode(FSR, INPUT);   //设置FSR压力传感器为输入
 digitalWrite(1, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
   val = analogRead(FSR); //读取模拟接口0 的值，即压力传感器传回的阻值，并将其赋给val
    Serial.print("pressure:");  
  Serial.println(val);        
  Serial.println();
  Serial.println();
  delay(500);
}
