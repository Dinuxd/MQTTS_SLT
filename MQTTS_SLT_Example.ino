#include "MQTTS_SLT.h"

MQTTS_SLT IIOT_Dev_kit;

Broker TB_Broker0;

String Response;
void setup() {
  Serial.begin(115200);
  while(Serial.available()){
    Serial.read();
  }
  while(!Serial.available());
    if(IIOT_Dev_kit.Init(115200)){
    Serial.println("initialized...");
    while (Serial2.available()) {
    Serial.write( Serial2.read() );
  }  
  }
  else{
    Serial.println("initiallization error"); 
  }

  TB_Broker0.addr          = "test.mosquitto.org";    
  TB_Broker0.port          = "8883";                  
  TB_Broker0.mqttId        = 0;                       
  TB_Broker0.keepalive_time= 300;                     
  TB_Broker0.clean_session = true;                    
  TB_Broker0.Server_type   = 1;                       
}

void loop() {
  if(Serial.available()){
    char x = Serial.read();
    switch (x){
      case '1':
        if(IIOT_Dev_kit.AT_TEST()){
          Serial.println("AT OK."); 
        }
        else{
          Serial.println("AT Error."); 
        }
        break;
      case '2':
        if(IIOT_Dev_kit.PWRDOWN()){
          Serial.println("POWER DOWN OK...");  
        }
        else{
          Serial.println("POWER DOWN Error."); 
        }
        break;
      case '3':
        if(IIOT_Dev_kit.SET_APN("1","IP","airtellive")){
          Serial.println("APN OK...");  
        }
        else{
          Serial.println("APN Error."); 
        }
        break;
      case '4':
        
        if(IIOT_Dev_kit.CSQ(&Response)){   
          Serial.println(Response);  
        }
        else{
          Serial.println("CSQ Error"); 
        }
        break;
      case '5': 
        if(IIOT_Dev_kit.IS_ATTACH()){    
          Serial.println("attched.");  
        }
        else{
          Serial.println("attched Error"); 
        }
        break;
      case '6': 
        if(IIOT_Dev_kit.IS_PACKET_DOMAIN_ATTACH()){    
          Serial.println("packet domain attched.");  
        }
        else{
          Serial.println("attched Error"); 
        }
        break;
      
      case 'S':
      if (IIOT_Dev_kit.MQTT_SETUP(&TB_Broker0,
                              TB_Broker0.addr,
                              TB_Broker0.port)) {
      Serial.println("MQTT Start OK");
      } else {
       Serial.println("MQTT Start Error");
      }
  break;

  case 'U': {
  bool ok = IIOT_Dev_kit.MQTTSUB(&TB_Broker0,
                                 "test/topicmqtts",
                                 0);
  Serial.println(ok ? "Subscribed OK" : "Subscribe failed");
} break;


      case 'C':
        if (IIOT_Dev_kit.MQTT_CONNECT(&TB_Broker0, "620f47c4-1398-44d6-9eac-f70f62c215d0")) {
          Serial.println("Connected to server");
        } else {
          Serial.println("fail to connect");
        }
        break;
      
      case 'D':
        if(IIOT_Dev_kit.MQTT_DISCONNECT(&TB_Broker0)){
           Serial.println("disconnected");  
        }
        else{
          Serial.println("fail to disconnect"); 
        }
        break;
      case 'K':
        if(IIOT_Dev_kit.MQTT_STOP()){
           Serial.println("MQTT STOP");  
        }
        else{
          Serial.println("fail to STOP MQTT"); 
        }
        break;

      case 'P':
{
  bool ok = IIOT_Dev_kit.MQTT_PUB(
    &TB_Broker0,
    "test/topicmqtts",   
    "hello SLT",         
    0,                   
    60,                
    true,            
    false                
  );
  if (ok) {
    Serial.println("Retained MSG sent");
  } else {
    Serial.println("Publish failed");
  }
}
break;

      default:
        break;
    } 
  }
}
