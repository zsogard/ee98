#include <Wire.h>


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

void setup() {
  Wire.begin();
  Serial.begin(9600);
  pinMode(A0, INPUT);
  Serial.print("I2C Moisture \t");
  Serial.print("I2C Temperature \t");
  Serial.println("Resistive Moisture");
}

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

void loop(){
  Serial.print(readI2CRegister16bit(SENS_ADDR, GET_CAP));
  Serial.print("\t");
  Serial.print(readI2CRegister16bit(SENS_ADDR, GET_TEMP)/float(10));
  Serial.print("\t");
  Serial.println(analogRead(A0));
  delay(500);
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
