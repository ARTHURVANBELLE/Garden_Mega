#include <Arduino.h>
#include "RTClib.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RH_NRF24.h>


char rxData();
void setupComm();
void communicate();
void sendData(char toBeSent);