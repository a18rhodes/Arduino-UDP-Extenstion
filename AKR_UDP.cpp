#include "AKR_UDP.h"
// #define DEBUG

void OTA(WiFiUDP Udp, IPAddress ip, unsigned int port, String node, float version, char * cpCmd){
  if(cpCmd[0] != 'P' || cpCmd[1] != 'U' || cpCmd[2] != 'S' || cpCmd[3] != 'H'){
    return;
  }
  unsigned long otastart = millis();
  while(millis() - otastart <= 30000){
      ArduinoOTA.handle();
      delay(100);
      yield();
  }
  return;
}

int udpSendPacket(WiFiUDP Udp, IPAddress ip, unsigned int port, String data){
  uint8_t len = data.length() + 1;
  char buf[len];
  data.toCharArray(buf, len);
  yield();
  if(!Udp.beginPacket(ip, port)){
    #ifdef DEBUG
      Serial.println("Could not begin UDP Packet");
    #endif
    return 0;
  }
  yield();
  Udp.write(buf, len);
  yield();
  if(!Udp.endPacket()){
    #ifdef DEBUG
      Serial.println("Could not end UDP Packet");
    #endif
    return 0;
  }
  return 1;
}

int updVerifiedSendPacket(WiFiUDP Udp, IPAddress ip, unsigned int port, String data, char caRecvBuf[255], char caCmdBuf[5]) {
  uint8_t len = data.length() + 1;
  char buf[len];
  data.toCharArray(buf, len);
  // This may be causing a missed ack, moving to top, and just know there will be at least several milliseconds used by the transmit
  unsigned long wifiAckStart = millis();
  yield();
  if (!Udp.beginPacket(ip, port)) {
#ifdef DEBUG
    Serial.println("Could not begin UDP Packet");
#endif
    return 0;
  }
  yield();
  Udp.write(buf, len);
  yield();
  if (!Udp.endPacket()) {
#ifdef DEBUG
    Serial.println("Could not send UDP Packet");
#endif
    return 0;
  }
  // Wait for ACK
  while (!Udp.parsePacket()) {
    if (millis() - wifiAckStart > 10000) {
#ifdef DEBUG
      Serial.println("Failed to ACK in 10 seconds.");
#endif
      return 0;
    }
    delay(100);
    yield();
  }
#ifdef DEBUG
  Serial.println("ACKED");
#endif
  uint8_t uiPacketLen = Udp.read(caRecvBuf, 255);
  uint8_t colonIdx = uiPacketLen;
  if(uiPacketLen > 0){
      for(uint8_t i = uiPacketLen; i >= 0; i--){
          if (caRecvBuf[i] == ':'){
              colonIdx = i;
              break;
          }
      }
      for(uint8_t i = colonIdx + 1, j = 0; i < uiPacketLen; i++, j++){
        caCmdBuf[j] = caRecvBuf[i];
        caRecvBuf[i] = 0;
      }
  }
  return 1;
}

void connect(char ssid[], char pass[]) {
  // Connect to Wifi.
  #ifdef DEBUG
    Serial.println("-------- WiFi Connecting --------");
    Serial.print("Connecting to ");
    Serial.println(ssid);
  #endif
  // WiFi will be in sleep state upon deepsleep exit, so wake it https://bit.ly/384MFxP
  WiFi.forceSleepWake();
  yield();

  // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);

  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {
    // Check to see if
    if (WiFi.status() == WL_CONNECT_FAILED) {
      delay(10000);
    }

    delay(500);
    #ifdef DEBUG
      Serial.print(".");
    #endif
    // Only try for 15 seconds.
    if (millis() - wifiConnectStart > 15000) {
      #ifdef DEBUG
        Serial.println("Failed to connect to WiFi");
      #endif
      return;
    }
  }
  #ifdef DEBUG
    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  #endif
}
