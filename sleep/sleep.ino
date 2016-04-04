#include "LowPower.h"

void setup()
{
  pinMode(A5, OUTPUT);
  pinMode(A0, OUTPUT);
  //delay(10000); //delay to help with reprogramming
}

void loop()  
{  
  digitalWrite(A5, HIGH); // turns Linino on ~ 225-300 mA
  digitalWrite(A0, HIGH); 
  delay(8000);
  
  digitalWrite(A5, LOW);  // turns Linino off ~ 50 mA 
  digitalWrite(A0, LOW);
  delay(8000);
  //turns Arduino off ~ 15 mA!! :)
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
  
}  