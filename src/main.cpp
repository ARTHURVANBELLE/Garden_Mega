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

RF24 radio(23, 22); // CE, CSN
const byte addresses [][6] = {"00001", "00002"};  //Setting the two addresses. One for transmitting and one for receiving
char messagerx [30];
char messagetx[30];

const int IN_NIV1 = 3;
const int IN_NIV2 = 4;
const int IN_PUM = 36;
const int IN_GAR = 34;
const int IN_CHI = 32;

const int R_pump = 38;
const int R_garden = 42;
const int R_chicken = 40;

const int L_au = 25;
const int L_m = 26;
const int L_niv1 = 29;
const int L_niv2 = 27; 

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
int levTime = 0;
int lastTime = 0;
int currentLux;
int lastLux;
int currentLev;
int lastLev;
int lastLev_1;
int lastTimeLev;
int flagLev;
int lastSent = 0;

volatile unsigned long count = 0; // use volatile for shared variables

Ultrasonic ultrasonic(5, 6);

int lastTimeTx;


void pump(bool mode){
  if (mode == 1 && !digitalRead(IN_NIV2) && !overheatPump){
    digitalWrite(R_pump,1);
    pum = 1;
    
  }
  else if (!mode){
    digitalWrite(R_pump,0);
    pum = 0;
     if (!autom && !extra){
      overheatPump = 0;
      flagLev = 0;
     }
  }
}

void chicken(bool mode){
  if (mode){
    digitalWrite(R_chicken, 1);
    chi = 1;
  }
  else if (!mode){
    digitalWrite(R_chicken,0);
    chi = 0;
  }
  
}

void garden(bool mode){
  if (mode){
    digitalWrite(R_garden,1);
    gar = 1;
  }
  else if (!mode){
    digitalWrite(R_garden,0);
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

void txData()
{
  radio.stopListening();
  sprintf(messagetx,"A%d%d%i%d%d%d%d%03dB",digitalRead(IN_NIV2),digitalRead(IN_NIV1),modeA,gar,pum,chi,overheatPump,currentLev);
  Serial.println(messagetx);
  radio.write(&messagetx, sizeof(messagetx));
  delay(20);
}

void rxdata()
{
  radio.startListening();                            //This sets the module as receiver
  if (radio.available())
  {                                                     //Looking for incoming data
    Serial.println("RX");
    radio.read(&messagerx, sizeof(messagerx));           //Reading the data
    delay(20);
    //sprintf(messageTx,"%d-%d-%d-%d",oldstate_c, oldstate_g, oldstate_m, oldstate_p);
    chicken(messagerx[0] - '0');
    garden(messagerx[2] - '0');
    modeA = messagerx[4] - '0';
    pump(messagerx[6] - '0');
  }
}
  

void setup() 
{
  digitalWrite(IN_NIV1,HIGH);
  Serial.begin(115200);

  pinMode(24,INPUT);
  pinMode(IN_NIV1,INPUT);
  pinMode(IN_NIV2,INPUT);
  pinMode(IN_GAR,INPUT);
  pinMode(IN_CHI, INPUT);
  pinMode(IN_PUM,INPUT);
  pinMode(A0,INPUT);

  pinMode(R_chicken, OUTPUT);
  pinMode(R_garden,OUTPUT);
  pinMode(R_pump,OUTPUT);
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(counter);          // counter to run every 1 seconds
  radio.begin();
  radio.openWritingPipe(addresses[1]);     //Setting the address at which we will send the data
  radio.openReadingPipe(1, addresses[0]);  //Setting the address at which we will receive the data
  radio.setPALevel(RF24_PA_HIGH);          //You can set it as minimum or maximum depending on the distance between the transmitter and receiver. 
  radio.startListening();                  //This sets the module as receiver
  radio.setChannel(90);  

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

  if ((currentTime - levTime) >= 5000){   //get water level every 5s
    currentLev = ultrasonic.read(CM);
    levTime = currentTime;
  }

  rxdata();                               //receive data from HMI

  if ((currentTime - lastSent) >= 3500){  //Send data every 3.5s
    txData();
    lastSent = currentTime;
  }
  

  if (digitalRead(IN_NIV1)){
    digitalWrite(L_niv1,1);
  }
  else{
    digitalWrite(L_niv1,0);
  }

  if (digitalRead(IN_NIV2)){
    digitalWrite(L_niv2,1);
  }
  else{
    digitalWrite(L_niv2,0);
  }

  if ((currentState != lastState && currentState ==1) )
  {
    if (autom){         //mode Auto 
      autom = 0;
      extra = 0;
      modeA = 3;
      if (!flagArros){
        garden(0);
      }
    }
    else if ((!autom && !extra)){    //
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
    digitalWrite(L_au,1);
    digitalWrite(L_m,0);

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

    if (currentLux <= 120){
      noInterrupts();
      copyCount = count;
      interrupts();
      garden(1);
  
    }
    noInterrupts();
    if (flagArros && count == copyCount){
      garden(0);
      flagArros = 0;
    }
    interrupts();
  }
  
  else if (!autom && !extra){
    digitalWrite(L_au,0);
    digitalWrite(L_m,1);
  }
  else if (extra){ //mode extra : remplissage auto mais pas d'arrosage dans le noir
    digitalWrite(L_au,1);
    digitalWrite(L_m,1);

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