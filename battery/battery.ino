int batteryReading = A3;
int enable = 0;

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  pinMode(batteryReading, INPUT);
  pinMode(enable, OUTPUT);
}

void loop()
{
  digitalWrite(enable, HIGH);
  delay(1);
  int reading = analogRead(batteryReading);
  float voltage = (float) reading * (5.0/1024.0) * 2;
  Serial.println(String(voltage, 3));
  digitalWrite(enable, LOW);
  delay(1000);
}
