#include <Bridge.h>
#include <HttpClient.h>
#include <FileIO.h>
#include <Wire.h>
#include "LowPower.h"

boolean debug = true; //true: Serial works, LowPower disabled
                      //false: Serial doesn't work, LowPower enabled
                      //TODO: find a way for both to work

String ipaddr = "192.168.240.219";

HttpClient client;
String params = "";
String filename = "/mnt/sda1/arduino/www/log.txt";
char path[30];
const float BATTERY_THRESHOLD = 6.00;

int batteryEnable = 0;
//DON'T USE PINS 2 OR 3 (used for I2C)
int tempPin = A0;
int lightPin = A1; float light_resistor = 1000.0;
int phPin = A2;
int batteryPin = A3;
int lininoPin = A4;

/*I2C STUFF*/

//I2C CAPACACITIVE SENSOR COMMANDS
const int SENS_ADDR = 0x39;
const int GET_CAP = 0; //read 2 bytes
const int SET_ADDR = 1;
const int GET_ADDR = 2;
const int MEASURE_LIGHT = 3; //write
const int GET_LIGHT = 4; //read 2 bytes
const int GET_TEMP = 5; //read 2 bytes
const int RESET = 6; //write
const int GET_VERS = 7; //read 1 byte
const int SLEEP = 8; //write, wake up with sending any commands
const int GET_BUSY = 9; //read 1 byte, returns 1 if measurement is in progress

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Waiting a few seconds");
  delay(5000); //not sure if needed to prevent Bridge.begin from hanging?
  Serial.println("Starting Bridge");
  Bridge.begin();
  Serial.println("Starting Filesystem");
  FileSystem.begin();

  pinMode(tempPin, INPUT);
  pinMode(lightPin, INPUT);
  //pinMode(moistPin, INPUT);
  pinMode(phPin, INPUT);
  pinMode(batteryPin, INPUT);
  pinMode(lininoPin, OUTPUT);
  pinMode(batteryEnable, OUTPUT);
  filename.toCharArray(path, sizeof(path));
}

void loop()
{
  //wake up and take readings
  //temp
  Serial.println(path);
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
  int moisture = readI2CRegister16bit(SENS_ADDR, GET_CAP);
  Serial.print(moisture);

  //pH
  float phVoltage = analogRead(phPin) * 5.0/1024;
  float ph = 3.0 * phVoltage + 1.3;
  Serial.print("\tpH: "); 
  Serial.print(ph, 2);

  //TODO: EC
  

  //battery reading
  digitalWrite(batteryEnable, HIGH);
  delay(1); //let the transistor have time to turn on, 1ms should be enough
  float batteryVoltage = analogRead(batteryPin) * (5.0/1024.0) * 2; //multiply by 2 to get back original value before voltage divider
  Serial.print("\tBattery:  ");
  Serial.print(batteryVoltage, 2);
  digitalWrite(batteryEnable, LOW); //shut transistor off

  String lowbat_str = "";
  if (batteryVoltage < BATTERY_THRESHOLD)
  {
    lowbat_str = "true";
  }
  else
  {
    lowbat_str = "false";
  }

  //Create params string
  String time = getTimeStamp();
  String temp_str = String(temp, 2);     
  String brightness_str = String(brightness, 2);
  String moist_str = String(moisture);
  String ph_str = String(ph, 2);
  String battery_str = String(batteryVoltage, 2);
  params = "time=" + time + "&lowbat=" + lowbat_str + "&brightness=" + brightness_str + "&moisture=" + moist_str + "&ph=" + ph_str + "&temp=" + temp_str;

  //Check if data from log needs to be written
  if (FileSystem.exists(path))
  {
    Serial.println("Attempting to flush log.");
    //Flush lines from log and reset log
    File logFile = FileSystem.open(path, FILE_READ);
    //If successful, remove log
    if(flushFile(logFile))
    {
      FileSystem.remove(path);
    }
    else
    {
      Serial.println("Unable to flush log.");
    }
  }

  if (httpPost(params) != 0)
  {
    //If unsuccessful, write data locally to SD card instead
    Serial.println("Connection failed, writing to SD card");
    File logFile = FileSystem.open(path, FILE_APPEND);
    if (logFile)
    {
      logFile.print(params + '\n'); //not using println because we don't want \r
      logFile.close();
      Serial.println(params);
    }
    else
    {
      Serial.println("Failed to write to SD card.");
    }
  }

  //delay and sleep
  if (debug)
  {
    Serial.println("Delay 8s");
    delay(8000);
  }
  else
  {
    Serial.println("Sleepy time!");
    digitalWrite(lininoPin, LOW);  // turns Linino off ~ 50 mA 
    Serial.flush();
    delay(100); //flush is async, so we need to delay for a moment to let it flush the message to USB before we shut the arduino off
                //could probably be shorter
                
    // Enter power down state for 8 s with ADC and BOD module disabled
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); // turns Arduino 32U4 off ~ 15 mA
    digitalWrite(lininoPin, HIGH); // turns Linino back on ~ 225-300 mA
  }
  Serial.println("Finished delay");
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
      Serial.println(line);       
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

//returns 0 on success
int httpPost(String params)
{
  Serial.println("Sending HTTP POST.");
  String httpBody = ""; //"key1=value1" gives a null query, not sure why
  String httpDestination = "http://" + ipaddr + ":8000?" + params;

  int returnCode = client.post(httpDestination, httpBody);
  String response = "";

  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    response += c;
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

/*******************I2C STUFF***********************/

void writeI2CRegister8bit(int addr, int value) {
  Wire.beginTransmission(addr);
  Wire.write(value);
  Wire.endTransmission();
}

unsigned int readI2CRegister16bit(int addr, int reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission();
  delay(20);
  Wire.requestFrom(addr, 2);
  unsigned int t = Wire.read() << 8;
  t = t | Wire.read();
  return t;
}

void findAddress(){
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ){
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    //Serial.print("CURR_ADDRESS: ");
    //Serial.println(address);
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    Serial.print("ERROR: ");
    Serial.println(error);
    if (error == 0){
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
      nDevices++;
    }
    else if (error==4){
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(5000);           // wait 5 seconds for next scan
}
