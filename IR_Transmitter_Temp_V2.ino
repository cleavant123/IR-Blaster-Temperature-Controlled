#include "esp_adc_cal.h"
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

#define LM35_Sensor1    10

const int button1Pin = 9;  // the number of the pushbutton pin
const int button2Pin = 8;
const int button3Pin = 7;
const int button4Pin = 6;
const int button5Pin = 5;

int LM35_Raw_Sensor1 = 0;
float LM35_TempC_Sensor1 = 0.0;
float LM35_TempF_Sensor1 = 0.0;
float Voltage = 0.0;

int button1State = 0; // variable to store the state of button 1
int button2State = 0; // variable to store the state of button 2
int button3State = 0; // variable to store the state of button 3
int button4State = 0; // variable to store the state of button 4
int button5State = 0; // variable to store the state of button 5
int currentSpeed = 1;
int RECV_PIN = 11; // IR Receiver connected to Pin 11
int signalsReceived = 0;

unsigned long speedSignal, powerSignal;

IRsend irsend(12); // Use pin 12 to transmit the IR signal

const float TEMP_THRESHOLD = 0.1; // threshold value in degrees Celsius
float lastTemp = 0; // variable to store last temperature reading

bool fan_power = false;
bool finished = false;
bool tempLoop = false;
bool tempMesSent = false;
bool clearMesSent = false;

IRrecv irrecv(RECV_PIN);
decode_results results;

void setup()
{
  Serial.begin(115200);
  pinMode(button1Pin, INPUT); // set button 1 pin as an input
  pinMode(button2Pin, INPUT); // set button 2 pin as an input
  pinMode(button3Pin, INPUT); // set button 3 pin as an input
  pinMode(button4Pin, INPUT); // set button 4 pin as an input
  pinMode(button5Pin, INPUT); // set button 4 pin as an input
  irrecv.enableIRIn();  // Start the receiver
  irsend.begin(); // Start the transmitter
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
  button3State = digitalRead(button3Pin); // read the state of button 3
  button4State = digitalRead(button4Pin); // read the state of button 4
  button5State = digitalRead(button5Pin); // read the state of button 4

  if (button3State == HIGH) {
    Serial.println("IR Receiver activated.");
    IRreceiver();
  }

  if (button4State == HIGH){
    if(!clearMesSent){
      Serial.println("Signals Cleared. ");
      clearMesSent = true;
    }
    clearSignals();
  }

  if (button1State == HIGH) {
    if (!fan_power) {
      // Transmit the command 0xCF331CE using the NEC protocol to turn the fan ON
      irsend.sendNEC(powerSignal);
      Serial.println("Sent command: " + String(powerSignal, HEX) + "(Power On)");
      fan_power = true;
      Serial.println("Fan Power = True");
    } else {
      // Transmit the command 0xCF331CE using the NEC protocol to turn the fan OFF
      irsend.sendNEC(powerSignal);
      Serial.println("Sent command: " + String(powerSignal, HEX) + "(Power Off)");
      fan_power = false;
      Serial.println("Fan Power = False");
    }
    delay(250);
  }
  else{
    if (button2State == HIGH){
      if(!tempLoop){
      tempLoop = true;
        while(tempLoop){
          if(!tempMesSent){
            Serial.println("Temperature Control Activated.");
            tempMesSent = true;
          }
    // check the temperature and set the fan speed accordingly
         if (abs(LM35_TempC_Sensor1 - lastTemp) > TEMP_THRESHOLD && fan_power == true) {
         lastTemp = LM35_TempC_Sensor1; // update last temperature reading
          if(LM35_TempC_Sensor1 < 29) {
            if(currentSpeed == 2){
              irsend.sendNEC(speedSignal); delay(1000); // Speed 3
              Serial.println("Sent command: " + String(speedSignal, HEX) + "(Speed 3)");
              irsend.sendNEC(speedSignal); delay(1000); // Speed 1
              Serial.println("Sent command: " + String(speedSignal, HEX) + "(Speed 1)");
              currentSpeed = 1;
            }
            if(currentSpeed == 3){
              irsend.sendNEC(speedSignal); // Speed 1
              Serial.println("Sent command: " + String(speedSignal, HEX) + "(Speed 1)");
              currentSpeed = 1;
            }

        } else if(LM35_TempC_Sensor1 >= 29.5 && LM35_TempC_Sensor1 < 30.0 ) {
            if(currentSpeed != 2 && currentSpeed != 3){
              irsend.sendNEC(speedSignal); delay(1000); // Speed 2
              Serial.println("Sent command: " + String(speedSignal, HEX) + "(Speed 2)");
              currentSpeed = 2;
            }
            else if(currentSpeed == 3){
              irsend.sendNEC(speedSignal); delay(1000); // Speed 1
              Serial.println("Sent command: " + String(speedSignal, HEX) + "(Speed 1)");
              irsend.sendNEC(speedSignal); delay(1000); // Speed 2
              Serial.println("Sent command: " + String(speedSignal, HEX) + "(Speed 2)");
              currentSpeed = 2;
            }
        } else if(LM35_TempC_Sensor1 >= 30.5) {
            if(currentSpeed != 3){
              irsend.sendNEC(speedSignal);delay(1000); // Speed 3
              Serial.println("Sent command: " + String(speedSignal, HEX) + "(Speed 3)");
              currentSpeed = 3;
            } 
          }
      }
          if(digitalRead(button5Pin) == HIGH){
            Serial.println("Temperature Control Deactivated. ");
            tempLoop = false;
            tempMesSent = false;
            break;
          }
        }
    }
        // Print The Readings
    Serial.print("Temperature = ");
    Serial.print(LM35_TempC_Sensor1);
    Serial.println(" °C ");
    Serial.print("Speed = ");
    Serial.println(currentSpeed);
    delay(400);
    }
  }

}

uint32_t readADC_Cal(int ADC_Raw)
{
  esp_adc_cal_characteristics_t adc_chars;
  
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_13, 1100, &adc_chars);
  return(esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}

void IRreceiver() {
  if(!finished){
    Serial.println("Please send the Power Signal: ");
    while(!finished){
      if (irrecv.decode(&results)) {
        if(!powerSignal){
          Serial.println("Power Signal received. ");
          powerSignal = results.value; // this is the Power Signal code decoded
          signalsReceived++;
          Serial.println("Please send the Speed Signal: ");
      }
        else if(!speedSignal){
          Serial.println("Speed Signal received. ");
          speedSignal = results.value; // this is the Speed Signal code decoded
          signalsReceived++;
      }
      irrecv.resume(); // Receive the next value
    }
    if(isFinished()){
      finished = true;
      Serial.println("Speed Signal: " + String(speedSignal, HEX));
      Serial.println("Power Signal: " + String(powerSignal, HEX));
    }
    delay(100);
  }
}
}

bool isFinished(){
    return signalsReceived == 2;
}

void clearSignals(){
  speedSignal = 0;
  powerSignal = 0;
  signalsReceived = 0;
  finished = false;
  clearMesSent = false;
}
