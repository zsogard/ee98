#include <Bridge.h>
#include <YunClient.h>
#include "LowPower.h"

//IPAddress server(104,28,21,107);
IPAddress server(10,3,14,64);

YunClient client;

String params ="";

int tempPin = A0;
int lightPin = A1;
float light_resistor = 1000.0;
int moistPin = A2;

void setup()
{
  Serial.begin(9600);
  //while (!Serial);
  Serial.print("Starting Bridge");
  Bridge.begin();
  pinMode(tempPin, INPUT);
  pinMode(lightPin, INPUT);
  pinMode(moistPin, INPUT);

}

void loop()
{
  // Enter power down state for 8 s with ADC and BOD module disabled
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

  //wake up and take readings
  float temp_voltage = (analogRead(tempPin)*5/1023.0)*1000;
  float temp = (temp_voltage-500)/9.3;
  Serial.print("Temperature: ");
  Serial.print(temp, 2);
  Serial.print(" C");

  float light_voltage = analogRead(lightPin)*5/1023.0;
  float light_current = light_voltage/light_resistor * pow(10,6);
  float brightness = light_current / 2.5;
  Serial.print("\tBrightness: ");
  Serial.print(brightness, 2);
  Serial.print(" lux");

  int moist_val = analogRead(moistPin);
  Serial.print("\tMoisture: ");
  Serial.println(moist_val);

  //connect to server
  if (client.connect(server, 8000))
  {

    Serial.println("connected");
    String temp_str =  String(temp, 2);     
    String brightness_str = String(brightness);
    params = "temp=" + temp_str + "&brightness=" + brightness_str;

    client.println("POST /test HTTP/1.1");
    client.println("Host: thinkpad.local");
    client.print("Content-length:");
    client.println(params.length());
    client.println("Connection: Close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println();
    client.println(params);  

  }
  else
  {
    Serial.println("connection failed");
  }

  if(client.connected())
  {
    client.stop();   //disconnect from server
  }
}
