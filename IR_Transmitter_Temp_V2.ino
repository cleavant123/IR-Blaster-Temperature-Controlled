#include "esp_adc_cal.h"
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

#define LM35_Sensor1    10
const int button1Pin = 9;  // the number of the pushbutton pin
const int button2Pin = 8;

int LM35_Raw_Sensor1 = 0;
float LM35_TempC_Sensor1 = 0.0;
float LM35_TempF_Sensor1 = 0.0;
float Voltage = 0.0;

int button1State = 0; // variable to store the state of button 1
int button2State = 0; // variable to store the state of button 2

IRsend irsend(12); // Use pin 12 to transmit the IR signal

int currentSpeed = 1;

const float TEMP_THRESHOLD = 0.1; // threshold value in degrees Celsius
float lastTemp = 0; // variable to store last temperature reading

bool fan_power = false;

void setup()
{
  Serial.begin(115200);
  pinMode(button1Pin, INPUT); // set button 1 pin as an input
  pinMode(button2Pin, INPUT); // set button 2 pin as an input
  irsend.begin();
}

void loop()
{
  // Read LM35_Sensor1 ADC Pin
  LM35_Raw_Sensor1 = analogRead(LM35_Sensor1);  
  // Calibrate ADC & Get Voltage (in mV)
  Voltage = readADC_Cal(LM35_Raw_Sensor1);
  // TempC = Voltage(mV) / 10
  LM35_TempC_Sensor1 = Voltage / 10;

  // Check the state of the buttons
  button1State = digitalRead(button1Pin); // read the state of button 1
  button2State = digitalRead(button2Pin); // read the state of button 2

  // check the temperature and set the fan speed accordingly
  if (button1State == HIGH) {
    if (!fan_power) {
      // Transmit the command 0xCF331CE using the NEC protocol to turn the fan ON
      irsend.sendNEC(0xCF331CE);
      Serial.println("Sent command: 0xCF331CE (Power On)");
      fan_power = true;
      Serial.println("Fan Power = True");
    } else {
      // Transmit the command 0xCF331CE using the NEC protocol to turn the fan OFF
      irsend.sendNEC(0xCF331CE);
      Serial.println("Sent command: 0xCF331CE (Power Off)");
      fan_power = false;
      Serial.println("Fan Power = False");
    }
    delay(250);
  }
  else if (button2State == HIGH) {
    switch (currentSpeed) {
      case 1:
        irsend.sendNEC(0xCF3619E); 
        Serial.println("Sent command: 0xCF3619E (Speed 1)");
        break;
      case 2:
        irsend.sendNEC(0xCF3619E); 
        Serial.println("Sent command: 0xCF3619E (Speed 2)");
        break;
      case 3:
        irsend.sendNEC(0xCF3619E); 
        Serial.println("Sent command: 0xCF3619E (Speed 3)");
        break;
    }
    currentSpeed++;
    if (currentSpeed > 3) {
      currentSpeed = 1;
    }
    delay(250);
  }
    else {
    if (abs(LM35_TempC_Sensor1 - lastTemp) > TEMP_THRESHOLD && fan_power == true) {
    lastTemp = LM35_TempC_Sensor1; // update last temperature reading
    if(LM35_TempC_Sensor1 < 26) {
        if(currentSpeed != 1){
          irsend.sendNEC(0xCF3619E); delay(1000); // Speed 1
          Serial.println("Sent command: 0xCF3619E (Speed 1)");
          currentSpeed = 1;
    }
    } else if(LM35_TempC_Sensor1 >= 26.5 && LM35_TempC_Sensor1 < 28 ) {
        if(currentSpeed != 2){
          irsend.sendNEC(0xCF3619E); delay(1000); // Speed 2
          Serial.println("Sent command: 0xCF3619E (Speed 2)");
          currentSpeed = 2;
    }
    } else if(LM35_TempC_Sensor1 >= 28.5) {
        if(currentSpeed != 3){
          irsend.sendNEC(0xCF3619E); delay(1000); // Speed 3
          Serial.println("Sent command: 0xCF3619E (Speed 3)");
          currentSpeed = 3;
        } 
      }
  }
  }
      // Print The Readings
  Serial.print("Temperature = ");
  Serial.print(LM35_TempC_Sensor1);
  Serial.println(" °C ");
  delay(400);
  }
  

uint32_t readADC_Cal(int ADC_Raw)
{
  esp_adc_cal_characteristics_t adc_chars;
  
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_13, 1100, &adc_chars);
  return(esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}