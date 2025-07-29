#include <Arduino.h>
#include "MQTTS_SLT.h"

// ——— CHANGE YOUR CA CERT HERE ———
// Just replace everything between BEGIN/END CERTIFICATE
const char mosq_ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL
BQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwG
A1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAU
BgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hv
by5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UE
BhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTES
MBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVp
dHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJ
KoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPg
UZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FW
Te3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEA
s06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH
3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxo
E6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNT
MFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV
6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL
BQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC
6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf
+pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wK
sMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839
LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJE
m/XriWr/Cq4h/JfB7NTsezVslgkBaoU=
-----END CERTIFICATE-----
)EOF";


MQTTS_SLT IIOT_Dev_kit;
Broker    TB_Broker0;
String    Response;


void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  delay(100);

  // Broker config (once)
  TB_Broker0.addr           = "test.mosquitto.org";
  TB_Broker0.port           = "8883";
  TB_Broker0.mqttId         = 0;
  TB_Broker0.keepalive_time = 300;
  TB_Broker0.clean_session  = true;
  TB_Broker0.Server_type    = 1;

  Serial.println(F("Ready. Press 'S' to Subscribe, 'P' to Publish."));
}

void loop() {
  
  while (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    line.trim();
    if (line.length()) Serial.println(line);
  }


  if (!Serial.available()) return;
  char cmd = Serial.read();


  auto bringUp = [&]() -> bool {
    // power-on + echo off
    if (!IIOT_Dev_kit.Init(115200)) {
      Serial.println("Init failed");
      return false;
    }
 
    if (!IIOT_Dev_kit.SET_APN("1","IP","yourAPN")) {
      Serial.println("APN failed");
      return false;
    }

    IIOT_Dev_kit.CSQ(&Response);
    Serial.println(Response);

  
    if (!IIOT_Dev_kit.IS_ATTACH()) {
      Serial.println("Warning: CREG attach failed");
    } else {
      Serial.println("CREG attached");
    }

    if (!IIOT_Dev_kit.IS_PACKET_DOMAIN_ATTACH()) {
      Serial.println("Warning: CGATT attach failed");
    } else {
      Serial.println("CGATT attached");
    }

    return true;
  };


  if (cmd == 'S') {
    if (!bringUp()) return;

    if (!IIOT_Dev_kit.MQTT_SETUP(&TB_Broker0, TB_Broker0.addr, TB_Broker0.port)) {
      Serial.println("MQTT setup failed");
      return;
    }
    if (!IIOT_Dev_kit.MQTT_CONNECT(&TB_Broker0, "myClientID")) {
      Serial.println("MQTT connect failed");
      return;
    }

    if (IIOT_Dev_kit.MQTTSUB(&TB_Broker0, "test/topicmqtts", 0)) {
      Serial.println("Subscribed OK");
    } else {
      Serial.println("Subscribe failed");
    }
  }

  else if (cmd == 'P') {
    if (!bringUp()) return;

    if (!IIOT_Dev_kit.MQTT_SETUP(&TB_Broker0, TB_Broker0.addr, TB_Broker0.port)) {
      Serial.println("MQTT setup failed");
      return;
    }
    if (!IIOT_Dev_kit.MQTT_CONNECT(&TB_Broker0, "myClientID")) {
      Serial.println("MQTT connect failed");
      return;
    }

    bool ok = IIOT_Dev_kit.MQTT_PUB(
      &TB_Broker0,
      "test/topicmqtts", 
      "hello SLT",         
      0,                
      60,                 
      true,                
      false                
    );
    Serial.println(ok ? "Published OK" : "Publish failed");
  }
}
