int val;
void setup()
{
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  digitalWrite(13, LOW);
  delay(3000);
  Serial.println("play,001,$");
}
void loop()
{
  val = Serial.read();
  if (-1 != val)
  {
    if (11 == val)
    {
      digitalWrite(13, HIGH);
    }
    if (10 == val)
    {
      digitalWrite(13, LOW);
    }
  }
}
