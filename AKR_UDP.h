#ifndef AKR_UDP_H
#define AKR_UDP_H

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <String.h>

#define REQUEST "REQ:"
#define PULL "PULL:"
#define UTD "UTD"

int udpSendPacket(WiFiUDP Udp, IPAddress ip, unsigned int port, String data);
int updVerifiedSendPacket(WiFiUDP Udp, IPAddress ip, unsigned int port, String data, char caRecvBuf[255], char caCmdBuf[5]);
float connect(char ssid[], char pass[], const uint8_t mac[], uint8_t chan, float fTxPwr);
int OTA(char * cpCmd);

#endif