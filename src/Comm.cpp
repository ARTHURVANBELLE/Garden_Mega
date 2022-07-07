#include <Arduino.h>
#include "RTClib.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RH_NRF24.h>
/*
// Singleton instance of the radio driver

char rxMessage[25];

const int RH_RF24_MAX_MESSAGE_LEN = 25;

void rxData(){
    RH_NRF24 nrf24;
    if (nrf24.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_RF24_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (nrf24.recv(buf, &len))
    {
      Serial.print("message: ");
      Serial.println((char*)buf);
      sprintf(rxMessage,"%s",buf);
    }
  }
}

/*
void sendData(char toBeSent)
{
// Send a message
  uint8_t data[] = toBeSent;
  nrf24.send(data, sizeof(data));
  nrf24.waitPacketSent();
}
*/
void setupComm(){
  RH_NRF24 nrf24;
  if (!nrf24.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
    Serial.println("setRF failed");   
  Serial.println("setup Done") ;
}

void communicate(){/*
  RH_NRF24 nrf24;
  if (nrf24.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_RF24_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (nrf24.recv(buf, &len))
    {

      Serial.print("got request: ");
      Serial.println((char*)buf);

      
      // Send a reply
      uint8_t data[] = "And hello back to you";
      nrf24.send(data, sizeof(data));
      nrf24.waitPacketSent();
      Serial.println("Sent a reply");
    }
    else
    {
      Serial.println("recv failed");
    }
  }*/
}

