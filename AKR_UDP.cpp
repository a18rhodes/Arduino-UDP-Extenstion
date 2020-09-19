#include "AKR_UDP.h"
// #define DEBUG
#define PWRPROFILE_PIN D6
// #define PWRPROFILE_CONNECT
// #define PWRPROFILE_OTA

int OTA(char *cpCmd)
{
#ifdef PWRPROFILE_OTA
    digitalWrite(PWRPROFILE_PIN, LOW);
#endif
    uint8_t colonIdx = 2;
    uint8_t uiSensorID = -1;
    uint8_t uiCmdLen = 0;
    for (uint8_t i = 0;; i++)
    {
        if (cpCmd[i] == 0 || i > 6)
        {
            uiCmdLen = i;
            break;
        }
    }
    if (cpCmd[0] == 'P' && cpCmd[1] == 'U' && cpCmd[2] == 'S' && cpCmd[3] == 'H')
    {
        unsigned long otastart = millis();
        while (millis() - otastart <= 30000)
        {
            ArduinoOTA.handle();
            delay(100);
            yield();
        }
    }
    else if (cpCmd[0] == 'U' && cpCmd[1] == 'P')
    {
        char *cpSensorID = (char *)malloc(sizeof(char) * uiCmdLen - 1);
        for (uint8_t i = colonIdx + 1, j = 0; i < uiCmdLen; i++, j++)
        {
            cpSensorID[j] = cpCmd[i];
        }
        uiSensorID = atoi(cpSensorID);
    }
#ifdef PWRPROFILE_OTA
    digitalWrite(PWRPROFILE_PIN, HIGH);
#endif
    return uiSensorID;
}

int udpSendPacket(WiFiUDP Udp, IPAddress ip, unsigned int port, String data)
{
    uint8_t len = data.length() + 1;
    char buf[len];
    data.toCharArray(buf, len);
    yield();
    if (!Udp.beginPacket(ip, port))
    {
#ifdef DEBUG
        Serial.println("Could not begin UDP Packet");
#endif
        return 0;
    }
    yield();
    Udp.write(buf, len);
    yield();
    if (!Udp.endPacket())
    {
#ifdef DEBUG
        Serial.println("Could not end UDP Packet");
#endif
        return 0;
    }
    return 1;
}

int updVerifiedSendPacket(WiFiUDP Udp, IPAddress ip, unsigned int port, String data, char caRecvBuf[255], char caCmdBuf[5])
{
    uint8_t len = data.length() + 1;
    char buf[len];
    data.toCharArray(buf, len);
    // This may be causing a missed ack, moving to top, and just know there will be at least several milliseconds used by the transmit
    unsigned long wifiAckStart = millis();
    yield();
    if (!Udp.beginPacket(ip, port))
    {
#ifdef DEBUG
        Serial.println("Could not begin UDP Packet");
#endif
        return 0;
    }
    yield();
    Udp.write(buf, len);
    yield();
    if (!Udp.endPacket())
    {
#ifdef DEBUG
        Serial.println("Could not send UDP Packet");
#endif
        return 0;
    }
    // Wait for ACK
    while (!Udp.parsePacket())
    {
        if (millis() - wifiAckStart > 10000)
        {
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
    if (uiPacketLen > 0)
    {
        for (uint8_t i = uiPacketLen; i >= 0; i--)
        {
            if (caRecvBuf[i] == ':')
            {
                colonIdx = i;
                break;
            }
        }
        for (uint8_t i = colonIdx + 1, j = 0; i < uiPacketLen; i++, j++)
        {
            caCmdBuf[j] = caRecvBuf[i];
            caRecvBuf[i] = 0;
        }
    }
    return 1;
}

float connect(char ssid[], char pass[], const uint8_t mac[], uint8_t *chan, float fTxPwr)
{
#ifdef PWRPROFILE_CONNECT
    digitalWrite(PWRPROFILE_PIN, LOW);
#endif
    // Connect to Wifi.
    randomSeed(millis());
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

    uint8_t i = 0;
    // Not the fastest search, binary would be quicker, but still linear so not a huge concern.
    fTxPwr = (fTxPwr <= 0.5 ? 1.0 : fTxPwr);
    if (random(700) == 88)
    {
        fTxPwr = 1.0;
#ifdef DEBUG
        Serial.println("Re-calibrating");
#endif
    }
    while (WiFi.status() != WL_CONNECTED)
    {
#ifdef DEBUG
        Serial.print("Connection attempt at ");
        Serial.print(fTxPwr);
        Serial.println(" dB");
#endif
        WiFi.setOutputPower(fTxPwr);
        if (*chan)
        {
            WiFi.begin(ssid, pass, (uint32_t)*chan, mac, true);
        }
        else
        {
            WiFi.begin(ssid, pass);
        }
        // Try to connect 10 times
        i = 0;
        while (WiFi.status() != WL_CONNECTED && i++ < 10)
        {
#ifdef DEBUG
            if (i == 0)
            {
                Serial.print("Not connected yet");
            }
            else if (i == 10)
            {
#ifdef DEBUG
                Serial.println();
#endif
            }
            Serial.print(".");
#endif
            delay(500);
        }
// If i==10, then we didn't get a connection, so increment tx and retry
#ifdef DEBUG
        Serial.println();
#endif
        if (i >= 10)
        {
            if (fTxPwr < 20.5)
            {
#ifdef DEBUG
                Serial.println("Increasing TX power");
#endif
                fTxPwr = (fTxPwr <= 19.5 ? fTxPwr += 1 : 20.5);
            }
            else
            {
#ifdef DEBUG
                Serial.println("Couldn't find tx power to connect");
#endif
                fTxPwr = 0;
                break;
            }
            yield();
        }
    }
    if (!*chan)
    {
        mac = WiFi.BSSID();
        *chan = WiFi.channel();
    }
#ifdef DEBUG
    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("Connection took: ");
    Serial.print(i * 500);
    Serial.println(" mS");
    Serial.print("txPwr set to: ");
    Serial.print(fTxPwr);
    Serial.println(" dB");
#endif
    WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
#ifdef PWRPROFILE_CONNECT
    digitalWrite(PWRPROFILE_PIN, HIGH);
#endif
    return fTxPwr;
}
