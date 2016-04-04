int batteryReading = A3;
int enable = 0;

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  pinMode(batteryReading, INPUT);
  pinMode(enable, OUTPUT);
  digitalWrite(enable, HIGH);
}

void loop()
{
  
  delay(100);
  int reading = analogRead(batteryReading);
  Serial.println("analogread:" + String(reading));
  float voltage = (float) reading * (5.0/1024.0);
  Serial.println(String(voltage, 3));
  digitalWrite(enable, LOW);
  delay(1000);
}
