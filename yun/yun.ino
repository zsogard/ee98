#include <Bridge.h>
#include <YunClient.h>
#include <FileIO.h>
#include "LowPower.h"

IPAddress server(10,3,14,64);
#define PORT 8000

YunClient client;
String params = "";
String filename = "/mnt/sd/log.txt";
char path[16];

int tempPin = A0;
int lightPin = A1;
float light_resistor = 1000.0;
int moistPin = A2;
boolean logDirty = false;

void setup()
{
  Serial.begin(9600);
  //while (!Serial);
  Serial.println("Starting Bridge");
  Bridge.begin();
  Serial.println("Starting Filesystem");
  FileSystem.begin();

  pinMode(tempPin, INPUT);
  pinMode(lightPin, INPUT);
  pinMode(moistPin, INPUT);
  filename.toCharArray(path, sizeof(path));
}

void loop()
{
  // Enter power down state for 8 s with ADC and BOD module disabled
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

  //wake up and take readings
  //temp
  float temp_voltage = (analogRead(tempPin)*5/1023.0)*1000;
  float temp = (temp_voltage-500)/9.3;
  Serial.print("Temperature: ");
  Serial.print(temp, 2);
  Serial.print(" C");

  //brightness
  float light_voltage = analogRead(lightPin)*5/1023.0;
  float light_current = light_voltage/light_resistor * pow(10,6);
  float brightness = light_current / 2.5;
  Serial.print("\tBrightness: ");
  Serial.print(brightness, 2);
  Serial.print(" lux");

  //moisture
  int moist_val = analogRead(moistPin);
  Serial.print("\tMoisture: ");
  Serial.println(moist_val);

  //Create params string
  String time = getTimeStamp();
  String temp_str = String(temp, 2);     
  String brightness_str = String(brightness);
  params = "time=" + time + "&temp=" + temp_str + "&brightness=" + brightness_str;

  //connect to server
  if (client.connect(server, PORT))
  {
    Serial.println("Successfully connected");
    
    //Check if data from log needs to be written
    if (logDirty)
    {
      //Flush lines from log and reset log
      File logFile = FileSystem.open(path, FILE_READ);
      flushFile(logFile, client);
      FileSystem.remove(path);
      logDirty = false;
    }

    //Create and send HTTP POST
    client.println("POST /test HTTP/1.1");
    client.println("Host: thinkpad.local");
    client.print("Content-length:");
    client.println(params.length());
    client.println("Connection: Close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println();
    client.println(params);
    client.stop(); //disconnect from server
  }
  else
  {
    //Write data locally to SD card instead
    Serial.println("Connection failed, writing to SD card");
    File logFile = FileSystem.open(path, FILE_APPEND);
    if (logFile)
    {
      logFile.println(params);
      logFile.close();
      logDirty = true;
      Serial.println(params);
    }
    else
    {
      Serial.println("Failed to write to SD card.");
    }
  }
}

void flushFile(File file, YunClient client)
{ 
  if (file)
  {
    // Send one line at a time
    String params = "";
    while (file.available() != 0)
    {     
      params = file.readStringUntil('\n');        
      if (params == "") break;        
      //Send HTTP POST
      client.println("POST /test HTTP/1.1");
      client.println("Host: thinkpad.local");
      client.print("Content-length:");
      client.println(params.length());
      client.println("Connection: Close");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println();
      client.println(params);
      Serial.println(params);
    }
    file.close();
  }
  else
  {
    Serial.println("Failed to open file.");
  }
}

String getTimeStamp()
{
  String result;
  Process time;
  time.begin("date");
  time.addParameter("+%D-%T");  
  time.run(); 

  while(time.available()>0)
  {
    char c = time.read();
    if(c != '\n')
      result += c;
  }

  return result;
}
