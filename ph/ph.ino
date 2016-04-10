int sensor = A0;

void setup(void)
{
  pinMode(sensor, INPUT);
  Serial.begin(9600);
  while (!Serial);
}
void loop(void)
{
  float pHValue,voltage;
  voltage = analogRead(sensor) * 5.0/1024;
  pHValue = 3.0 * voltage + 1.3;
  Serial.print("Voltage: ");
  Serial.print(voltage, 2);
  Serial.print("\t");
  Serial.print("pH: "); 
  Serial.println(pHValue,2);
  delay(1000);
}
