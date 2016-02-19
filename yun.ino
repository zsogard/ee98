#include <Bridge.h>
#include <YunClient.h>

//String msg = "POST /text HTTP/1.1\r\nUser-Agent: * \r\nHost: textbelt.com\r\nAccept: */*\r\nContent-Length: 50\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nnumber=5088385800&message=Hey\040Zach\040its\040me,the\040Arduino!\r\n";
/**/

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
  while (!Serial);
  Serial.print("Starting Bridge");
  Bridge.begin();
  pinMode(tempPin, INPUT);
  pinMode(lightPin, INPUT);
  pinMode(moistPin, INPUT);

}

void loop()
{
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
  delay(1000);


 if (client.connect(server, 8000)) {

          Serial.println("connected");

          delay(2500);

          String temp_str =  String(temp, 2);     
          String brightness_str = String(brightness);
          
          //params="number=5088385800&message=";
          //params = "number=6177758794&message=";
          //params += "Hello! The current temperature is: " + temp_str + "C, and the brightness is: " + brightness_str + " lux.";
          params = "temp=" + temp_str + "&brightness=" + brightness_str;

          client.println("POST /test HTTP/1.1");
          //client.println("POST /text HTTP/1.1");

          //client.println("Host: www.textbelt.com");
          client.println("Host: 127.0.0.1");

          client.print("Content-length:");
          client.println(params.length());

          Serial.println(params);

          client.println("Connection: Close");
          client.println("Content-Type: application/x-www-form-urlencoded");
          client.println();
          client.println(params);  

 }
 else
 {
        Serial.println("connection failed");
        delay(1000);
 }

 if(client.connected())
 {

  client.stop();   //disconnect from server
  Serial.println("done");
  while (true); //just send one message

 }



}
