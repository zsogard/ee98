#include <Bridge.h>
#include <HttpClient.h>
#include <FileIO.h>
#include "LowPower.h"

boolean debug = true; //true: Serial works, LowPower disabled
                      //false: Serial doesn't work, LowPower enabled
                      //TODO: find a way for both to work

String ipaddr = "130.64.161.160";

// Initialize the client library
HttpClient client;
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
  while (!Serial);
  Serial.println("Waiting a few seconds");
  delay(10000); //not sure if needed to prevent Bridge.begin from hanging?
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
  int moisture = analogRead(moistPin);
  Serial.print("\tMoisture: ");
  Serial.println(moisture);

  //Create params string
  String time = getTimeStamp();
  String temp_str = String(temp, 2);     
  String brightness_str = String(brightness);
  String moist_str = String(moisture);
  params = "time=" + time + "&temp=" + temp_str + "&brightness=" + brightness_str + "&moisture=" + moist_str;

  //Check if data from log needs to be written
  if (logDirty)
  {
    Serial.println("Flushing log.");
    //Flush lines from log and reset log
    File logFile = FileSystem.open("/mnt/sd/log.txt", FILE_READ);
    //If successful, remove log
    if(flushFile(logFile))
    {
      FileSystem.remove("/mnt/sd/log.txt");
      logDirty = false;
    }
  }

  if (httpPost(params) != 0)
  {
    //If unsuccessful, write data locally to SD card instead
    Serial.println("Connection failed, writing to SD card");
    File logFile = FileSystem.open("/mnt/sd/log.txt", FILE_APPEND);
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

  Serial.println("Sleepy time!");
  Serial.flush();

  if (debug)
  {
    delay(8000);
  }
  else
  {
    delay(1000);
    // Enter power down state for 8 s with ADC and BOD module disabled
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  Serial.println("We made it!");
}

boolean flushFile(File file)
{
  boolean success = true;
  if (file)
  {
    // Send one line at a time
    String line = "";
    while (file.available() != 0)
    {     
      line = file.readStringUntil('\n');        
      if (line.equals("")) break;        
      //Send HTTP POST
      if (httpPost(line) != 0)
      {
        success = false;
        break;
      }
    }
    file.close();
  }
  else
  {
    Serial.println("Failed to open file.");
    success = false;
  }

  return success;
}

int httpPost(String params)
{
  String httpBody = ""; //"key1=value1" gives a null query, not sure why
  String httpDestination = "http://" + ipaddr + ":8000?" + params;

  int returnCode = client.post(httpDestination, httpBody);

  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
  Serial.flush();

  Serial.println();
  
  return returnCode;
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
