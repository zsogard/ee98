const int tone_pin = 7;
const int rx_high = A2;
const int vx_high = A0;
const int vx_low = A1;


void setup() {
  pinMode(tone_pin, OUTPUT);
  pinMode(rx_high, INPUT);
  pinMode(vx_high, INPUT);
  pinMode(vx_low, INPUT);
  Serial.begin(9600);
}

void loop() {
  double EC_val = getEC();
  delay(5000);
}

double getEC(){
  double R1 = 1000; //100kOhms
  double result_30[16], result_60[16], result_90[16];
  double sum_30 = 0;
  double sum_60 = 0;
  double sum_90 = 0;
  int count_30 = 0;
  int count_60 = 0;
  int count_90 = 0;
  tone(tone_pin, 35);
  delay(4000);
  Serial.println();
  Serial.print("@35 Hz: ");
  for(int i = 0; i < 16; i++){
    int v_result_30 = analogRead(rx_high);
    result_30[i] = map(v_result_30, 0, 1023, 0, 5000)/1000.0;
    Serial.print(v_result_30);
    Serial.print("\t");
    delayMicroseconds(7093);
    //delayMicroseconds(14186);
  }
  Serial.println();
  Serial.print("@60 Hz: ");
  tone(tone_pin, 60);
  delay(3000);
  for(int i = 0; i < 16; i++){
    int v_result_60 = analogRead(rx_high);
    result_60[i] = map(v_result_60, 0, 1023, 0, 5000)/1000.0;
    Serial.print(v_result_60);
    Serial.print("\t");
    delayMicroseconds(4116);
    //delayMicroseconds(8233);
  }
  Serial.println();
  Serial.print("@90 Hz: ");
  tone(tone_pin, 90);
  delay(3000);
  for(int i = 0; i < 16; i++){
    int v_result_90 = analogRead(rx_high);
    result_90[i] = map(v_result_90, 0, 1023, 0, 5000)/1000.0;
    Serial.print(v_result_90);
    Serial.print("\t");
    delayMicroseconds(2728);
    //delayMicroseconds(5456);
  }
  noTone(tone_pin);
  //assume that results are gonna be bipolar square waves
  for(int i = 0; i < 16; i++){
    sum_30 += result_30[i];
    if(result_30[i] > 0)
      count_30++;
    sum_60 += result_60[i];
    if(result_60[i] > 0)
      count_60++;
    sum_90 += result_90[i];
    if(result_90[i] > 0)
      count_90++;
  }
  double v2_30 = sum_30/count_30;
  Serial.println();
  Serial.print("V2: ");
  Serial.print(v2_30);
  Serial.print("\t");
  double v2_60 = sum_60/count_60;
  Serial.print(v2_60);
  Serial.print("\t");
  double v2_90 = sum_90/count_90;
  Serial.print(v2_90);
  Serial.print("\t");  
//assume v1 is +2.5V to -2.5V
  double r2_30 = (v2_30*R1)/2.5;
  double r2_60 = (v2_60*R1)/2.5;
  double r2_90 = (v2_90*R1)/2.5;
  Serial.println();
  Serial.print("R2 Vals: ");
  Serial.print(r2_30);
  Serial.print("\t");
  Serial.print(r2_60);
  Serial.print("\t");
  Serial.print(r2_90);
  Serial.print("\t");
  double EC_30 = 1/r2_30;
  double EC_60 = 1/r2_60;
  double EC_90 = 1/r2_90;
  Serial.println();
  Serial.print("EC Vals: ");
  Serial.print(EC_30, 6);
  Serial.print("\t");
  Serial.print(EC_60, 6);
  Serial.print("\t");
  Serial.print(EC_90, 6);
  Serial.print("\t");
  double avg_EC = (EC_30 + EC_60 + EC_90)/3;
  double avg_freq = (35 + 60 + 90)/3;
  //x-axis is frequency, y-axis is EC
  double sum_num = (EC_30 - avg_EC)*(35 - avg_freq) + (EC_60 - avg_EC)*(60 - avg_freq) + (EC_90 - avg_EC)*(90 - avg_freq);
  double sum_den = pow((35 - avg_freq),2) + pow((60 - avg_freq),2) + pow((90 - avg_freq),2);
  double slope_EC = sum_num / sum_den;
  double EC_0 = avg_EC - slope_EC*avg_freq;
  Serial.print("EC at 0Hz: ");
  Serial.println(EC_0,4);
  return EC_0;
}

