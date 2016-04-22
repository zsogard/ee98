#include <Bridge.h>
#include <HttpClient.h>
#include <FileIO.h>
#include <Wire.h>
#include "LowPower.h"

boolean debug = true; //true: Serial works, LowPower disabled
//false: Serial doesn't work, LowPower enabled
//TODO: find a way for both to work
boolean quickSleep = true; //true: sleep for 8 seconds
                            //false: sleep for an hour
String ipaddr = "10.3.13.70";

HttpClient client;
String filename = "/mnt/sda1/arduino/www/log.txt";
char path[30];
const float BATTERY_THRESHOLD = 6.00;
String params = "";

//ALL THE PINS
const int tempPin = A0;
//const int tempEnable = 13;
const int lightPin = A1; 
float light_resistor = 5000.0;
const int lightEnable = 12;
const int moistureEnable = 5;
const int phPin = A2;
const int phEnable = 11;
const int batteryPin = A3;
const int batteryEnable = 10;
const int lininoPin = A5;

//EC Pins
const int tone_pin = 7;
const int vx_high = A6; //digital pin 4
const int vx_low = A7; //digital pin 6
//int rx_high = A4; //not needed?
const int ecEnablePMOS = 8;
const int ecEnableNMOS = 9;

//DON'T USE PINS 2 OR 3 (used for I2C)

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
  pinMode(lininoPin, OUTPUT);
  digitalWrite(lininoPin, HIGH); //start linino before we do anything else!
  Serial.begin(9600);
  //while (!Serial);
  Serial.println("Waiting a few seconds");
  delay(10000); //not sure if needed to prevent Bridge.begin from hanging?
  Serial.println("Starting Bridge");
  Bridge.begin();
  Serial.println("Starting Filesystem");
  FileSystem.begin();
  Wire.begin();

  //set pin modes
  pinMode(tempPin, INPUT);
  //pinMode(tempEnable, OUTPUT);
  pinMode(lightPin, INPUT);
  pinMode(lightEnable, OUTPUT);
  pinMode(moistureEnable, OUTPUT);
  pinMode(phPin, INPUT);
  pinMode(phEnable, OUTPUT);
  pinMode(batteryPin, INPUT);
  pinMode(batteryEnable, OUTPUT);
  pinMode(tone_pin, OUTPUT);
  pinMode(vx_high, INPUT);
  pinMode(vx_low, INPUT);
  pinMode(ecEnablePMOS, OUTPUT);
  pinMode(ecEnableNMOS, OUTPUT);

  filename.toCharArray(path, sizeof(path));

  //unsigned long startMillis = millis(); //Can only delay() for 65535 which is slightly too short
  //while (millis() - startMillis < 75000); //So we have to do it this way instead :)
}

void loop()
{
  //wake up and take readings
  //temp
  Serial.println(path);
  digitalWrite(lightEnable, HIGH); delay(1000); //light enable enables both temp and brightness
  //digitalWrite(tempEnable, HIGH); delay(1);
  //float temp_voltage = (analogRead(tempPin) * 5 / 1023.0) * 1000;
  //float temp = (temp_voltage - 500) / 9.3;
  //digitalWrite(tempEnable, LOW);

  //brightness
  
  float light_voltage = analogRead(lightPin) * 5 / 1023.0;
  float light_current = light_voltage / light_resistor * pow(10, 6);
  float brightness = light_current / 2.5;
  digitalWrite(lightEnable, LOW);

  //moisture
  digitalWrite(moistureEnable, HIGH); delay(2000);
  int cap_val = readI2CRegister16bit(SENS_ADDR, GET_CAP);
  double moisture = (cap_val-293.5)/807.2;
  int temp_val = readI2CRegister16bit(SENS_ADDR, GET_TEMP);
  float temp = temp_val / 10.0;
  digitalWrite(moistureEnable, LOW);

  //pH
  digitalWrite(phEnable, HIGH); delay(1000);
  float phVoltage = analogRead(phPin) * 5.0 / 1024;
  float ph = 3.0 * phVoltage + 1.3;
  digitalWrite(phEnable, LOW);

  //EC
  digitalWrite(ecEnablePMOS, LOW);
  digitalWrite(ecEnableNMOS, HIGH);
  delay(1000);
  double ec = getEC();
  digitalWrite(ecEnablePMOS, HIGH);
  digitalWrite(ecEnableNMOS, LOW);


  //battery reading
  digitalWrite(batteryEnable, HIGH);
  delay(1000); //let the transistor have time to turn on, 1ms should be enough
  float batteryVoltage = analogRead(batteryPin) * (5.0 / 1024.0) * 2; //multiply by 2 to get back original value before voltage divider
  digitalWrite(batteryEnable, LOW); //shut transistor off

  String lowbat_str = "";
  if (batteryVoltage < BATTERY_THRESHOLD)
  {
    lowbat_str.concat("&lowbat=true");
  }
  else
  {
    lowbat_str.concat("&lowbat=false");
  }

  //Create params string

  params = "time=";
  params.concat(getTimeStamp());
  //params.concat("test");
  params.concat(lowbat_str);
  params.concat("&brightness=");
  params.concat(String(brightness, 2));
  params.concat("&ec=");
  params.concat(String(ec, 2));
  params.concat("&moisture=");
  params.concat(String(moisture, 2));
  params.concat("&ph=");
  params.concat(String(ph, 2));
  params.concat("&temp=");
  params.concat(String(temp, 2));

  //params = "time=" + time + "&lowbat=" + lowbat_str + "&brightness=" + brightness_str + "&ec=" + ec_str + "&moisture=" + moist_str + "&ph=" + ph_str + "&temp=" + temp_str;

  //Serial.println(params);
  //Check if data from log needs to be written
  /*
  if (FileSystem.exists(path))
  {
  
    Serial.println("Attempting to flush log.");
    //Flush lines from log and reset log
    //If successful, remove log
    if (flushFile())
    {
      FileSystem.remove(path);
    }
    else
    {
      Serial.println("Unable to flush log.");
    }
  }
  */

  if (httpGet() != 0)
  {
    //If unsuccessful, write data locally to SD card instead
    Serial.println("Connection failed, writing to SD card");
    File logFile = FileSystem.open(path, FILE_APPEND);
    if (logFile)
    {
      Serial.println(params);
      params.concat('\n');
      logFile.print(params); //not using println because we don't want \r
      logFile.close();
    }
    else
    {
      Serial.println("Failed to write to SD card.");
    }
  }

  //delay and sleep
  if (debug)
  {
    Serial.println("Delay while awake for 60s");
    delay(60000);
  }
  else
  {
    Serial.println("Shutting Linino off.");
    //digitalWrite(lininoPin, LOW);  // turns Linino off ~ 50 mA

    // Enter power down state for 8 s with ADC and BOD module disabled
    if (quickSleep)
    {
      Serial.println("Sleeping for 8s.");
      Serial.flush();
      delay(1); //flush is async, so we need to delay for a moment to let it flush the message to USB before we shut the arduino off
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); // turns Arduino 32U4 off ~ 15 mA
      delay(8000);
    }
    else
    {
      Serial.println("Sleeping for an hour.");
      Serial.flush();
      delay(1);

      //8s * 450 = 3600s = 1hr
      for (int i = 0; i < 450; i++)
      {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      }
    }
    //digitalWrite(lininoPin, HIGH); // turns Linino back on ~ 225-300 mA
    //need to delay here to let Linino wake up
    //It takes 75s between plugging Yun in and ability to ssh in again
    //so I'll assume that's how long is needed after turning the Linino on
    //before it's ready to make HTTP requests again
    unsigned long startMillis = millis(); //Can only delay() for 65535 which is slightly too short
    while (millis() - startMillis < 75000); //So we have to do it this way instead :)

    Serial.println("Starting Bridge");
    Bridge.begin();
    Serial.println("Starting Filesystem");
    FileSystem.begin();
  }
  Serial.println("Finished delay/sleep");
}

boolean flushFile()
{
  File file = FileSystem.open(path, FILE_READ);
  boolean success = true;
  int lineNum = 0;
  if (file)
  {
    // Send one line at a time
    while (file.available() != 0)
    {
      for (int i = 0; i < lineNum; i++)
      {
        params = file.readStringUntil('\n');
        //while (file.read() != '\n');
      }
      //params = file.readStringUntil('\n');
      file.close();
      Serial.println("Flushing line");
      Serial.println(params);
      if (params.equals("")) break;
      //Send HTTP GET
      if (httpGet() != 0)
      {
        success = false;
        break;
      }
      else
      {
        lineNum++;
      }
      File file = FileSystem.open(path, FILE_READ);
    }
    //file.close();
  }
  else
  {
    Serial.println("Failed to open file.");
    success = false;
  }

  return success;
}

//returns 0 on success
int httpGet()
{
  Serial.println("Sending HTTP GET.");
  String httpDestination = "http://";
  httpDestination.concat(ipaddr);
  httpDestination.concat(":8000?");
  httpDestination.concat(params);

  Serial.println(httpDestination);

  int returnCode = client.get(httpDestination);
  Serial.println(returnCode);
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
  while (time.available() > 0)
  {
    char c = time.read();
    if (c != '\n')
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

void findAddress() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++ ) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    //Serial.print("CURR_ADDRESS: ");
    //Serial.println(address);
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    Serial.print("ERROR: ");
    Serial.println(error);
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(5000);           // wait 5 seconds for next scan
}

/*******************EC STUFF***********************/

const double k_cell = 210.5;
//const double ft[] = {1.163, 1.136, 1.112, 1.087}

double getEC() {
  double R1 = 1000.0; //1kOhms
  double Vin = 2.5;
  double i1 = Vin / R1;
  double vx_30[16], vx_60[16], vx_90[16];
  double sum_vx_30 = 0;
  double sum_vx_60 = 0;
  double sum_vx_90 = 0;
  int count_vx_30 = 0;
  int count_vx_60 = 0;
  int count_vx_90 = 0;
  tone(tone_pin, 35);
  delay(4000);
  //  Serial.println();
  //  Serial.print("@35 Hz: ");
  for (int i = 0; i < 16; i++) {
    int curr_vx_30 = analogRead(vx_high) - analogRead(vx_low);
    vx_30[i] = map(curr_vx_30, 0, 1023, 0, 5000) / 1000.0;
    //    Serial.print(result_30[i],4);
    //    Serial.print("\t");
    //    Serial.println(vx_30[i],4);
    delayMicroseconds(6893);
    //delayMicroseconds(14186);
  }
  //  Serial.println();
  //  Serial.print("@60 Hz: ");
  tone(tone_pin, 60);
  delay(3000);
  for (int i = 0; i < 16; i++) {
    int curr_vx_60 = analogRead(vx_high) - analogRead(vx_low);
    vx_60[i] = map(curr_vx_60, 0, 1023, 0, 5000) / 1000.0;
    //    Serial.print(result_60[i],4);
    //    Serial.print("\t");
    //    Serial.println(vx_60[i],4);
    delayMicroseconds(3916);
    //delayMicroseconds(8233);
  }
  //Serial.println();
  //Serial.print("@90 Hz: ");
  tone(tone_pin, 90);
  delay(3000);
  for (int i = 0; i < 16; i++) {
    int curr_vx_90 = analogRead(vx_high) - analogRead(vx_low);
    vx_90[i] = map(curr_vx_90, 0, 1023, 0, 5000) / 1000.0;
    //    Serial.print("\t");
    //    Serial.println(vx_90[i],4);
    delayMicroseconds(2478);
  }
  noTone(tone_pin);
  //assume that results are gonna be bipolar square waves
  for (int i = 0; i < 16; i++) {
    sum_vx_30 += abs(vx_30[i]);
    count_vx_30++;
    sum_vx_60 += abs(vx_60[i]);
    count_vx_60++;
    sum_vx_90 += abs(vx_90[i]);
    count_vx_90++;
  }
  double v2x_30 = sum_vx_30 / count_vx_30;
  //  Serial.println();
  //  Serial.print("V2: ");
  //  Serial.print("\t");
  //  Serial.println(v2x_30);
  double v2x_60 = sum_vx_60 / count_vx_60;
  //  Serial.print("\t");
  //  Serial.println(v2x_60);
  double v2x_90 = sum_vx_90 / count_vx_90;
  //  Serial.print("\t");
  //  Serial.println(v2x_90);
  //assume v1 is +2.5V to -2.5V
  double r2x_30 = v2x_30 / i1;
  double r2x_60 = v2x_60 / i1;
  double r2x_90 = v2x_90 / i1;
  //  Serial.println();
  //  Serial.print("R2x Vals: ");
  //  Serial.print("\t");
  //  Serial.print(r2x_30);
  //  Serial.print("\t");
  //  Serial.print(r2x_60);
  //  Serial.print("\t");
  //  Serial.print(r2x_90);
  //  Serial.print("\t");
  double ECx_30 = 1 / r2x_30;
  double ECx_60 = 1 / r2x_60;
  double ECx_90 = 1 / r2x_90;
  //  Serial.println();
  //  Serial.print("ECx Vals: ");
  //  Serial.print("\t");
  //  Serial.print(ECx_30, 6);
  //  Serial.print("\t");
  //  Serial.print(ECx_60, 6);
  //  Serial.print("\t");
  //  Serial.print(ECx_90, 6);
  //  Serial.print("\t");
  double avg_ECx = (ECx_30 + ECx_60 + ECx_90) / 3;
  double avg_freq = (35 + 60 + 90) / 3;
  //x-axis is frequency, y-axis is EC
  double sum_numx = (ECx_30 - avg_ECx) * (35 - avg_freq) + (ECx_60 - avg_ECx) * (60 - avg_freq) + (ECx_90 - avg_ECx) * (90 - avg_freq);
  double sum_denx = pow((35 - avg_freq), 2) + pow((60 - avg_freq), 2) + pow((90 - avg_freq), 2);
  double slope_ECx = sum_numx / sum_denx;
  double ECx_0 = avg_ECx - slope_ECx * avg_freq;
  //  Serial.print("ECx at 0Hz: ");
  //  Serial.println(ECx_0,4);
  double EC_noT = k_cell * ECx_0;
  return EC_noT;
}


