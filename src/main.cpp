#include <Arduino.h>
#include <Ultrasonic.h>
#include <TimerOne.h>
#include <SPI.h>  
#include <RTClib.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RH_NRF24.h>
#include <RHReliableDatagram.h>

// Abrevoir toutes les 2h pendant 3s

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2


const int IN_NIV1 = 30;
const int IN_NIV2 = 28;
const int IN_PUM = 36;
const int IN_GAR = 34;
const int IN_CHI = 32;

int modeA = 1;
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
bool overheatPump;

int currentTime;
int lastTime = 0;
int currentLux;
int lastLux;
int currentLev;
int lastLev;
int lastLev_1;
int lastTimeLev;
int flagLev;

volatile unsigned long count = 0; // use volatile for shared variables

RH_NRF24 nrf24(23,22);

Ultrasonic ultrasonic(5, 6);


const int RH_RF24_MAX_MESSAGE_LEN = 25;
int lastTimeTx;


void pump(bool mode){
  if (mode == 1 && !digitalRead(IN_NIV2) && !overheatPump){
    analogWrite(38,255);
    analogWrite(35, 0);
    pum = 1;
    
  }
  else if (!mode){
    analogWrite(38,0);
    analogWrite(35, 255);
    pum = 0;
     if (!autom && !extra){
      overheatPump = 0;
      flagLev = 0;
     }
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

void txData(){

  uint8_t data[RH_RF24_MAX_MESSAGE_LEN];
  sprintf(data,"%d%d%i%d%d%d%d.%03d",digitalRead(IN_NIV2),digitalRead(IN_NIV1),modeA,gar,pum,chi,overheatPump,currentLev);

  if ((currentTime - lastTimeTx ) >= 2000){
    nrf24.send(data, sizeof(data));
    nrf24.waitPacketSent();
    lastTimeTx = currentTime;
    Serial.println((char*)data);
  }

}

void setup() 
{
  Serial.begin(115200);

  digitalWrite(8,HIGH);
  
  pinMode(24,INPUT);
  pinMode(IN_NIV1,INPUT);
  pinMode(IN_NIV2,INPUT);
  pinMode(IN_GAR,INPUT);
  pinMode(IN_CHI, INPUT);
  pinMode(IN_PUM,INPUT);
  pinMode(A0,INPUT);

  Timer1.initialize(1000000);
  Timer1.attachInterrupt(counter); // blinkLED to run every 10 seconds

  if (!nrf24.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
    Serial.println("setRF failed"); 

  nrf24.setChannel(0x90);  

  Serial.println("setup Done") ;
}


void loop()
{  
  currentState = digitalRead(24);
  currentState_1 = digitalRead(IN_GAR);
  currentState_2 = digitalRead(IN_PUM);
  currentState_3 = digitalRead(IN_CHI);
  currentTime = millis();
  currentLev = ultrasonic.read(CM);
  unsigned long copyCount;


  txData();

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

  if ((currentState != lastState && currentState ==1) )
  {
    if (autom){
      autom = 0;
      extra = 0;
      modeA = 3;
      if (!flagArros){
        garden(0);
      }
    }
    else if ((!autom && !extra)){
      extra = 1;
      modeA = 2;
    }
    else if (extra or modeA == 0){
      autom = 1;
      modeA = 1;
    }
  }

  if (autom){ // remplissage auto et arrosage à la tombée de la nuit
    currentLux = analogRead(A0);
    analogWrite(26,255);
    analogWrite(25,0);

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
  else if (extra){ //mode extra : remplissage auto mais pas d'arrosage dans le noir
    analogWrite(25,0);
    analogWrite(26,0);

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
  }

  if (digitalRead(IN_NIV2)){
      pump(0);
      pumping = 0;
  }
  
  if ((currentTime-lastTime) >= 7200000){
    chicken(1);
    delay(3000);
    chicken(0);
    lastTime = currentTime;
  }

  if ((currentState_3 != lastState_3 && currentState_3 ==1))
  {
    if (chi){
      chicken(0);
    }
    else {
      chicken(1);
    }
  }

  if ((currentState_1 != lastState_1 && currentState_1 ==1))
  {
    if (gar){
      garden(0);
    }
    else {
      garden(1);
    }
  }

  if ((currentState_2 != lastState_2 && currentState_2 ==1))
  {
    if (pum){
      pump(0);
    }
    else {
      pump(1);
    }
  }
  if (analogRead(A0) >= 160){
    flagArros = 1;
  }

  if (pum && !gar && !chi){
    if ((currentTime - lastTimeLev) >= 120000) {
      if (currentLev >= lastLev && (abs(currentLev-lastLev)<6)){
        flagLev += 1;
      }
      if (flagLev >= 10){
        pump(0);
        overheatPump = 1;
      }
      lastTimeLev = currentTime;
      lastLev = currentLev;
    }
  }
 
  lastState = currentState; 
  lastState_1 = currentState_1;
  lastState_2 = currentState_2;
  lastState_3 = currentState_3;
  lastLev_1 = currentLev;
  lastLux = analogRead(A0);

}