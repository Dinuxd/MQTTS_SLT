
#include "MQTTS_SLT.h"

bool MQTTS_SLT::Init(unsigned long buad_rate)
{

  pinMode(PWR_PIN, OUTPUT);
  Serial2.begin(buad_rate);

  digitalWrite(PWR_PIN, HIGH);
  delay(600);
  digitalWrite(PWR_PIN, LOW);
  delay(2500);

  if(SENDATCMD("ATE0\r\n", 2000, "OK", "ERROR")!=1){
    return false;
  }
  delay(100);
  return AT_TEST();
}

bool MQTTS_SLT::PWRDOWN()
{
  
  uint8_t answer = SENDATCMD("AT+CPOF\r\n", 2000, "OK", "error");

  if (answer == 1)
  {
    digitalWrite(PWR_PIN, LOW);   
    return true;
  }
  return false;
  
}
bool MQTTS_SLT::CSQ(String *response)
{
  
  uint8_t answer=SEND_AT_CMD_RAW("AT+CSQ\r\n",2000,response);
  return answer;

}


bool MQTTS_SLT::SET_APN(String CID,String PDP_type,String APNNAME)
{
  uint8_t answer = SENDATCMD("AT+CFUN=0\r\n", 2000, "OK", "ERROR");
  if (answer == 1)
  {
    String atCommand = "AT+CGDCONT="+CID+",\""+PDP_type+"\",\""+ APNNAME + "\",\"\",0,0,,,,\r\n";
  
    char charArray[atCommand.length()];
    atCommand.toCharArray(charArray, atCommand.length());
    answer = SENDATCMD(charArray, 4000, "OK", "ERROR");
    if (answer == 1)
    {
      uint8_t answer = SENDATCMD("AT+CFUN=1\r\n", 2000, "OK", "ERROR");
      if (answer == 1)
      {
        return true;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
   
    return false;
  }
}

bool MQTTS_SLT ::IS_ATTACH()
{
  String response;
  uint8_t answer=SEND_AT_CMD_RAW("AT+CREG?\r\n",2000,&response);
  if (answer)
  {
    if((response[7]=='0') && (response[9]=='1')) return true;
  }
  return false;
}

bool MQTTS_SLT ::IS_PACKET_DOMAIN_ATTACH()
{
  String response;
  uint8_t answer=SEND_AT_CMD_RAW("AT+CGATT?\r\n",2000,&response);
  if (answer)
  {
    if(response[8]=='1') return true;
  }
  return false;
}

bool MQTTS_SLT ::GET_IP()
{
  String response;
  uint8_t answer = SEND_AT_CMD_RAW("AT+CGCONTRDP\r\n",2000,&response);

  Serial.println(response);
  if (answer == 1)
  {
    return true;
  }
  else 
  {
    return false;
  }
}

bool MQTTS_SLT::MQTT_SETUP(Broker *broker, String server, String port)
{
  broker->addr = server;
  broker->port = port;


  SENDATCMD("AT+CCERTDELE=\"mosquitto.org.pem\"\r\n",
            20000, "OK", "ERROR");

  SENDATCMD("AT+CMQTTSSLCFG=0,0\r\n",
            2000, "OK", "ERROR");

  {
    char buf[64];
    unsigned len = strlen_P(mosq_ca_cert);
    snprintf(buf, sizeof(buf),
             "AT+CCERTDOWN=\"mosquitto.org.pem\",%u\r\n", len);

 
    if (SENDATCMD(buf, 120000, ">", "ERROR") != 1) return false;

    const char *p = mosq_ca_cert;
    while (*p) { Serial2.write(*p++); }
    Serial2.write("\r\n", 2);

    if (!waitForOK(120000)) return false;
  }


  Serial2.println("AT+CMQTTSTART");
  if (!waitForURC("+CMQTTSTART: 0", 30000)) {
    waitForOK(1000);
  }
  return true;
}


bool MQTTS_SLT::MQTT_STOP()
{
  uint8_t ans = SENDATCMD("AT+CMQTTSTOP\r\n",
                          30000,
                          "+CMQTTSTOP: 0",
                          "ERROR");
  return (ans == 1 || ans == 2);
}


bool MQTTS_SLT ::MQTT_DISCONNECT(Broker *broker)
{
  return MQTT_DISCONNECT(broker, 0);
}

bool MQTTS_SLT::MQTT_DISCONNECT(Broker *broker, uint timeout)
{

  String cmd = "AT+CMQTTDISC=" + String(broker->mqttId);
  if (timeout != 0) cmd += "," + String(timeout);
  cmd += "\r\n";


  Serial.print  (">>> TX: "); Serial.print(cmd);
  Serial2.print (cmd);


  String expectedURC = "+CMQTTDISC:" + String(broker->mqttId) + ",0";
  bool gotURC = false;
  unsigned long t0 = millis();
  while (millis() - t0 < 30000) {
    if (!Serial2.available()) continue;
    String line = Serial2.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    Serial.print("<<< "); Serial.println(line);
    if (line.indexOf(expectedURC) >= 0) {
      gotURC = true;
      break;
    }
    if (line.indexOf("ERROR") >= 0) {
      Serial.println(">>> Received ERROR before URC");
      break;
    }
  }
  if (!gotURC) {
    Serial.println(">>> CMQTTDISC URC not seen, abort");
    return false;
  }
  Serial.println(">>> CMQTTDISC OK");

  String rel = "AT+CMQTTREL=" + String(broker->mqttId) + "\r\n";
  Serial.print  (">>> TX: "); Serial.print(rel);
  Serial2.print (rel);

  t0 = millis();
  while (millis() - t0 < 10000) {
    if (!Serial2.available()) continue;
    String line = Serial2.readStringUntil('\n');
    if (line.indexOf("OK") >= 0) {
      Serial.println(">>> CMQTTREL OK");
      return true;
    }
  }

  Serial.println(">>> CMQTTREL failed");
  return false;
}




bool MQTTS_SLT::MQTT_CONNECT(Broker *broker,
                                String clientid,
                                String Username,
                                String password)
{
  Serial.println(">>> MQTT_CONNECT: start");
  broker->username = Username;
  broker->password = password;


  String cmd1 = "AT+CMQTTACCQ=" + String(broker->mqttId)
                + ",\"" + clientid + "\",1";
  Serial.print  (">>> TX: "); Serial.println(cmd1);
  Serial2.println(cmd1);
 
  unsigned long t0 = millis();
  while (millis() - t0 < 40000) {
    if (!Serial2.available()) continue;
    String line = Serial2.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    Serial.print("<<< "); Serial.println(line);
    if (line.indexOf("OK") >= 0 || line.indexOf("ERROR") >= 0)
      break;
  }


  String cmd2 = "AT+CMQTTCONNECT=" + String(broker->mqttId)
                + ",\"tcp://" + broker->addr + ":8883\","
                + String(broker->keepalive_time) + ","
                + String(broker->clean_session ? 1 : 0);
  Serial.print  (">>> TX: "); Serial.println(cmd2);
  Serial2.println(cmd2);

  String successURC = "+CMQTTCONNECT: " 
                      + String(broker->mqttId) + ",0";
  bool gotOK  = false;
  bool gotURC = false;
  t0 = millis();
  while (millis() - t0 < 40000) {
    if (!Serial2.available()) continue;
    String line = Serial2.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    Serial.print("<<< "); Serial.println(line);

    if (line.indexOf("OK") >= 0) {
      gotOK = true;
      continue;
    }
    if (line.indexOf(successURC) >= 0) {
      Serial.println(">>> Received success URC");
      gotURC = true;
      break;
    }
    if (line.indexOf("ERROR") >= 0) {
      Serial.println(">>> Received ERROR");
      break;
    }
  }

  if (!gotURC) {
    Serial.print(">>> Connect failed ");
    if (!gotOK)  Serial.print("(no OK seen) ");
    if (!gotURC) Serial.print("(no URC seen) ");
    Serial.println();
    Serial.println(">>> Releasing client");
    MQTT_RELESECLIENT(broker);
    return false;
  }

  Serial.println(">>> MQTT_CONNECT: success!");
  return true;
}


bool MQTTS_SLT::MQTT_RELESECLIENT(Broker *broker)
{

  String cmd = "AT+CMQTTREL=" + String(broker->mqttId) + "\r\n";
  return (SENDATCMD(cmd.c_str(), 20000, "OK", "ERROR") == 1);
}

bool MQTTS_SLT::MQTT_CONNECT(Broker *broker, String clientid) {
  return MQTT_CONNECT(broker, clientid, "", "");
}
bool MQTTS_SLT::MQTT_CONNECT(Broker *broker, String clientid, String Username) {
  return MQTT_CONNECT(broker, clientid, Username, "");
}

bool MQTTS_SLT::MQTT_SETTOPIC(Broker *broker, String topic)
{
  uint topic_len = topic.length();

  if((topic_len<1)||(topic_len>1023)) return false;

  
  String atCommand = "AT+CMQTTTOPIC="+String(broker->mqttId)+","+String(topic_len)+"\r\n";
  char charArray[atCommand.length()];
  atCommand.toCharArray(charArray, atCommand.length());

  while(Serial2.available()){
    Serial2.read();
  }

  Serial2.write(charArray);

  bool OK=false;
  uint long previous = millis();
  do{
    if(Serial2.available()){
      char val = Serial2.read();
      if(val == '>'){
          OK = true;
      }
    }
  }while((OK == false) && ((millis()-previous)<3000));
  if(OK==false){
    return false;
  }

  char topicarray[topic.length()];
  topic.toCharArray(topicarray, topic.length()+1);

  String response2;
  bool answer = SEND_AT_CMD_RAW(topicarray, 30000, &response2);

  if(answer && (response2[0]=='O') && (response2[1]=='K')){
    return true;
  }
  else{
    return false;
  }
}

bool MQTTS_SLT::MQTT_PAYLOAD(Broker *broker, String msg){

  uint msg_len = msg.length();
  if((msg_len<1)||(msg_len>10240)) return false;

  String atCommand = "AT+CMQTTPAYLOAD="+String(broker->mqttId)+","+String(msg_len)+"\r\n";
  char charArray[atCommand.length()];
  atCommand.toCharArray(charArray, atCommand.length());

  while(Serial2.available()){
    Serial2.read();
  }

  Serial2.write(charArray); 

  bool OK=false;
  uint long previous = millis();
  do{
    if(Serial2.available()){
      if(Serial2.read() == '>'){
          OK = true;
      }
    }
  }while((OK == false) && ((millis()-previous)<20000));
  if(OK==false){
    return false;
  }
 
  char msgarray[msg_len];
  msg.toCharArray(msgarray, msg_len+1);

  String response2;
  bool answer = SEND_AT_CMD_RAW(msgarray, 30000, &response2);
  
  if(answer && (response2[0]=='O') && (response2[1]=='K')){
    return true;
  }
  else{
    return false;
  }
}

bool MQTTS_SLT::MQTT_PUB(Broker *broker,uint8_t qos, uint pub_timeout, bool retained, bool dup){

  String atCommand = "AT+CMQTTPUB="+String(broker->mqttId)+"," + String(qos) +","+ String(pub_timeout)+","+String(retained)+","+String(dup)+"\r\n";
  char charArray[atCommand.length()];
  atCommand.toCharArray(charArray, atCommand.length());
 
  String response;
  bool answer = SEND_AT_CMD_RAW(charArray, 30000, &response);

  if(answer && (response[0]=='O') && (response[1]=='K')){
    return true;
  }
  else{
    return false;
  }

}

bool MQTTS_SLT::MQTT_PUB(Broker *broker, String topic, String msg)
{
  return MQTT_PUB(broker, topic, msg, 0, 5, 0, 0);
}

bool MQTTS_SLT::MQTT_PUB(Broker *broker, String topic, String msg, uint8_t qos, uint pub_timeout, bool retained, bool dup)
{
  
  if(MQTT_SETTOPIC(broker, topic)==false){
    Serial.println("topic fail");
    return false;
  }

  if(MQTT_PAYLOAD(broker, msg)==false){
    Serial.println("msg fail");
    return false;
  }

  if(MQTT_PUB(broker,qos,pub_timeout,retained,dup)==false)
  {
    Serial.println("pub fail");
     return false;
  }

  return true;
}

bool MQTTS_SLT::MQTTSUB(Broker *broker, const String &topic, uint8_t qos) {

  if (qos > 2) {
    return false;
  }


  String cmd = "AT+CMQTTSUB="
               + String(broker->mqttId) + ","
               + String(topic.length()) + ","
               + String(qos)
               + "\r\n";
  Serial2.print(cmd);


  unsigned long start = millis();
  bool gotPrompt = false;
  while (millis() - start < 5000) {
    if (Serial2.available() && Serial2.read() == '>') {
      gotPrompt = true;
      break;
    }
  }
  if (!gotPrompt) {
    return false;
  }


  Serial2.print(topic);
  Serial2.print("\r\n");

  if (!waitForOK(5000)) {
    return false;
  }

  String urc = "+CMQTTSUB: " + String(broker->mqttId) + ",0";
  if (!waitForURC(urc.c_str(), 5000)) {
    return false;
  }

  return true;
}




uint8_t MQTTS_SLT::MQTTUNSUB(Broker *broker, String topic)
{
  String atCommand = "AT+CMQUNSUB=" + String(broker->mqttId) + "," + topic + "\r\n";
  char charArray[atCommand.length()];
  atCommand.toCharArray(charArray, atCommand.length());
  uint8_t answer = SENDATCMD(charArray, 4000, "OK", "ERROR");
  return answer == 1 ? 0x01 : answer;
}

bool MQTTS_SLT::SEND_AT_CMD_RAW(const char *at_command,
                                  unsigned int timeout,
                                  String *resp)
{
  uint8_t x = 0;
  char response[100] = {0}; 
  unsigned long previous;
  bool buffer_start = false;
  bool buffer_end = false;

  while (Serial2.available() > 0)
    Serial2.read();


  Serial2.write(at_command); 


  bool IS_CGEV=false;
  do{
    previous = millis();
    x=0;
    IS_CGEV=false;

    do
    {
      if (Serial2.available())
      {
        char tem1 = Serial2.read();
        if (tem1 = '\r')
        {
          char tem2 = Serial2.read();
          if (tem1 = '\n')
          {
            buffer_start = true;
          }
        }
      }
      delay(10);
    } while (!buffer_start && ((millis() - previous) < timeout));
    if (!buffer_start)
    {
      return false;
    }

    do
    {
      if (Serial2.available())
      {
        response[x] = Serial2.read();
        x++;
      }
    
      if (x > 1)
      {
        char tempArray[2] = {0};
        tempArray[0] = response[x-2];
        tempArray[1] = response[x-1];
        if (strstr(tempArray, "\r\n") != NULL)
        {
          buffer_end = true;
        }
      }
    } while (!buffer_end && ((millis() - previous) < timeout));
    if (!buffer_end)
    {
      return false;
    }

    /*+CGEV check*/
    if((response[0]=='+')&&(response[1]=='C')&&(response[2]=='G')&&(response[1]=='E')){
      Serial.println("detect +CGEV CMD");
      IS_CGEV = true;
    }
  }while(IS_CGEV);

  char tempStr[100] = {0};
  uint pointer=0;
  for(uint8_t i = 0; i < x - 2; i++) {
    if (response[i] == '\0' || response[i] == '\r' || response[i] == '\n'){
      Serial.println("null, newline detected");
    }else{
      tempStr[pointer]=response[i];
      pointer++;
    }
  }
  *resp = String(tempStr);
  return true;
}


uint8_t MQTTS_SLT::SENDATCMD(const char* at_command, unsigned int timeout, const char *expected_answer1, const char *expected_answer2)
{

  uint8_t x = 0, answer = 0;
  char response[100] = {0};

  unsigned long previous;

  while (Serial2.available() > 0)
    Serial2.read();

  delay(100);


  Serial2.write(at_command); 

  delay(500);

  previous = millis();

  do
  {
   
    if (Serial2.available() != 0)
    {
      response[x] = Serial2.read();
 
      if (expected_answer1 != "")
      {
       
        if (strstr(response, expected_answer1) != NULL)
        {
          answer = 1;
          delay(100);
        }
        else if (expected_answer2 != "")
        {
          if (strstr(response, expected_answer2) != NULL)
          {
            answer = 2;
            delay(100);
          }
        }
      }
      else
      {
      }
      x++;
      delay(10);
    }

  } while ((answer == 0) && ((millis() - previous) < timeout));
 
  return answer;
}

bool MQTTS_SLT::waitForOK(unsigned int timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (Serial2.available()) {
      String line = Serial2.readStringUntil('\n');
      if (line.indexOf("OK") >= 0) return true;
    }
  }
  return false;
}

bool MQTTS_SLT::waitForURC(const char* urc, unsigned int timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (Serial2.available()) {
      String line = Serial2.readStringUntil('\n');
      if (line.indexOf(urc) >= 0) return true;
    }
  }
  return false;
}


bool MQTTS_SLT::AT_TEST()
{
  uint8_t answer = SENDATCMD("AT\r\n", 1000, "OK", "ERROR");
  if (answer == 1)
  {
    return true;
  }
  else
  {
    return false;
  }
}
