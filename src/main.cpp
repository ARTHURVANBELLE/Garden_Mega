#include <Arduino.h>
#include <Ultrasonic.h>
#include <TimerOne.h>
#include <SPI.h>  
#include <RTClib.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RH_NRF24.h>

// Abrevoir toutes les 2h pendant 3s

const int IN_NIV1 = 30;
const int IN_NIV2 = 28;
const int IN_PUM = 36;
const int IN_GAR = 34;
const int IN_CHI = 32;

int modeA = 0;
bool autom = 1;
bool extra;
bool gar;
bool pum;
bool chi;
bool lastState;
bool currentState;
bool lastState_1;
bool currentState_1;
bool lastState_2;
bool currentState_2;
bool lastState_3;
bool currentState_3;
bool pumping;
bool flagArros;
bool flagRX;

int currentTime;
int lastTime = 0;
int currentLux;
int lastLux;

volatile unsigned long count = 0; // use volatile for shared variables

Ultrasonic ultrasonic(2, 3);

RH_NRF24 nrf24(23,22);


/*
RF24 myRadio (22, 23);
byte addresses[][6] = {"0"};
*/
struct package {
  int mode;
  bool chi;
  bool pum;
  bool gar;
  bool niv1;
  bool niv2;
  int lev;
};


typedef struct package Package;
Package dataRecieve;
Package dataTransmit;

char rec[15];
char rxMessage[25];
const int RH_RF24_MAX_MESSAGE_LEN = 25;

void pump(bool mode){
  if (mode == 1 && !digitalRead(IN_NIV2)){
    analogWrite(38,255);
    analogWrite(35, 0);
    pum = 1;
    
  }
  else if (!mode){
    analogWrite(38,0);
    analogWrite(35, 255);
    pum = 0;
  }
}

void chicken(bool mode){
  if (mode){
    analogWrite(39,255);
    analogWrite(31,0);
    chi = 1;
  }
  else if (!mode){
    analogWrite(39,0);
    analogWrite(31,255);
    chi = 0;
  }
  
}

void garden(bool mode){
  if (mode){
    analogWrite(40,255);
    analogWrite(33,0);
    gar = 1;
  }
  else if (!mode){
    analogWrite(33,255);
    analogWrite(40,0);
    gar = 0;
  }
  
}

void counter(void)
{
  count = count + 1;
  if (count >= 60){
    count = 0;
  }
  
}

void setup() 
{
  Serial.begin(115200);
  
  pinMode(24,INPUT);
  pinMode(IN_NIV1,INPUT);
  pinMode(IN_NIV2,INPUT);
  pinMode(IN_GAR,INPUT);
  pinMode(IN_CHI, INPUT);
  pinMode(IN_PUM,INPUT);
  pinMode(A0,INPUT);

  Timer1.initialize(1000000);
  Timer1.attachInterrupt(counter); // blinkLED to run every 1 second

  if (!nrf24.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
    Serial.println("setRF failed");   

  Serial.println("setup Done") ;
}


void loop()
{  
  currentState = digitalRead(24);
  currentState_1 = digitalRead(IN_GAR);
  currentState_2 = digitalRead(IN_PUM);
  currentState_3 = digitalRead(IN_CHI);
  currentTime = millis();
  unsigned long copyCount;
  flagRX = 0;

  if (nrf24.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_RF24_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (nrf24.recv(buf, &len))
    {
      Serial.println("message: ");
      Serial.println((char*)buf);
      sprintf(rxMessage,"%s",buf);

      uint8_t data[RH_RF24_MAX_MESSAGE_LEN];
      sprintf(data,"%i%d%d%d",modeA,gar,pum,chi);
      nrf24.send(data, sizeof(data));
      nrf24.waitPacketSent();
      Serial.print("Sent a reply");

      Serial.print((rxMessage[0]));
      Serial.print((rxMessage[1]));
      Serial.print((rxMessage[2]));
      Serial.println((rxMessage[3]));

      Serial.println(atoi(rxMessage));
      int proto = atoi(rxMessage);

      if (proto >= 2000){
        Serial.println("MODE EXTRA RX");
        proto -= 2000;
        modeA = 2;
      }
      else if (proto >= 1000){
        Serial.println("MODE MANU RX");
        proto -= 1000;
        modeA = 1;
      }
      else{
        Serial.println("MODE AUTO RX");
        modeA = 0;
      }

      if (proto >= 100){
        gar =0;
        proto -= 100;
      }
      else{
        gar =1;
      }   

      if (proto >= 10){
        pum = 0;
        proto -= 10;
      }  
      else{
        pum = 1;
      }

      if (proto == 1){
        chi = 0;
      }
      else{
        chi = 1;
      }
      
      flagRX = 1;
    }
  }


  if (digitalRead(IN_NIV1)){
    analogWrite(29,0);
  }
  else{
    analogWrite(29,255);
  }

  if (digitalRead(IN_NIV2)){
    analogWrite(27,0);
  }
  else{
    analogWrite(27,255);
  }

  if ((currentState != lastState && currentState ==1) or flagRX)
  {
    if (autom or modeA==1){
      autom = 0;
      extra = 0;
      if (!flagArros){
        garden(0);
      }
    }
    else if ((!autom && !extra) or modeA == 2){
      extra = 1;
    }
    else if (extra or modeA == 0){
      autom = 1;
    }
  }

  if (autom){
    currentLux = analogRead(A0);
    analogWrite(26,255);
    analogWrite(25,0);
    //Serial.println(currentLux);

    if ((digitalRead(IN_NIV1)==0 && (digitalRead(IN_NIV2)==0))){
      pump(1);
      pumping = 1;
    }
    else if ((!digitalRead(IN_NIV2) && (pumping ==1))){
      pump(1);
    }
    else if (digitalRead(IN_NIV2)){
      pumping = 0;
      pump(0);
    }

    if (currentLux <= 120 && flagArros){
      noInterrupts();
      copyCount = count;
      interrupts();
      garden(1);
      delay(1000);
    }
    noInterrupts();
    if (flagArros && count == copyCount){
      garden(0);
      flagArros = 0;
    }
    interrupts();
  }
  
  else if (!autom && !extra){
    analogWrite(25,255);
    analogWrite(26,0);
  }
  else if (extra){
    analogWrite(25,0);
    analogWrite(26,0);
  }

  if (digitalRead(IN_NIV2)){
      pump(0);
      pumping = 0;
    }

  
  //Serial.print(ultrasonic.read());

  if ((currentTime-lastTime) >= 7200000){
    chicken(1);
    delay(3000);
    chicken(0);
    lastTime = currentTime;
  }

  if ((currentState_3 != lastState_3 && currentState_3 ==1) or flagRX)
  {
    if (chi){
      chicken(0);
      chi = 0;
    }
    else {
      chicken(1);
      chi = 1;
    }
  }

  if ((currentState_1 != lastState_1 && currentState_1 ==1)or flagRX)
  {
    if (gar){
      garden(0);
      gar = 0;
    }
    else {
      garden(1);
      gar = 1;
    }
  }

  if ((currentState_2 != lastState_2 && currentState_2 ==1)or flagRX)
  {
    if (pum){
      pump(0);
      pum = 0;
    }
    else {
      pump(1);
      pum = 1;
    }
  }
  if (analogRead(A0) >= 160){
    flagArros = 1;
  }
  
  lastState = currentState; 
  lastState_1 = currentState_1;
  lastState_2 = currentState_2;
  lastState_3 = currentState_3;
  lastLux = analogRead(A0);

}