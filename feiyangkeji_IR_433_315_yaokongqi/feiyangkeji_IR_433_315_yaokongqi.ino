#include "defines.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
/* 依赖 PubSubClient 2.4.0 */
#include <PubSubClient.h>
#include <WiFiClientSecureBearSSL.h>  
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "AccessHandler.h"
#include <ArduinoStreamParser.h>
#include <Ticker.h>
#include "315_Handler.h"
#include "433_Handler.h"
#include "IR_handler.h"
#include "feiyangkeji.h"
#include "ConfigHandler.h"
#include "OneButton.h"

typedef struct{
  String cmd;
  String uid;
  String topic;
  String msg;
}yangMsgParam;


#if (defined(YYXBC_WEBCONFIG))
  #include "wificfg.h"
#endif

using namespace std;

//authkey
String auth = "wFWdxtsAQfBqjSZD";
//WIFI名称，区分大小写，不要写错
String ssid = "panzujiMi10";
//String ssid = "Xiaomi_10EE";
//WIFI密码
String pswd = "moto1984";




//webUrl
String weburl = "http://feiyang.sooncore.com";
///https://iot.xn2.net/api/app/device/login?auth=VjwCuaHbfemZPTAk

const int YYXBC_BUTTON_NUM   = 10;


//MQTT客户端相关初始化，默认即可
static WiFiClient TCPclient;
static unsigned long lastMs = 0;
//static unsigned long lasttimestamp = 0;
static bool preMQttConnected = false;
static WiFiClient espClient;
static PubSubClient  client(espClient);

Ticker change_rf_ticker; // change receiver

//函数原形： OneButton(const int pin, const boolean activeLow = true, const bool pullupActive = true);
/**
 * Initialize the OneButton library.
 * @param pin The pin to be used for input from a momentary button.
 * @param activeLow Set to true when the input level is LOW when the button is pressed, Default is true.
 * @param pullupActive Activate the internal pullup when available. Default is true.
 */

OneButton button(YYXBC_BUTTON_RESET,true);

//TCP初始化连接
void startMQttClient();

void buttonx_callback( const int& index, const String & state ,const String& from) ;

bool SendMsgUI( String cmd,int errCode);
bool SendMsgBtnUI(String btnId, String json );
bool SendMsgAudioBoxUI( String btnId,  String state);

/***
 * 继电器高电平触发时，YYXBC_HIGH = 1，YYXBC_LOW  = 0
 * 继电器低电平触发时，YYXBC_HIGH = 0，YYXBC_LOW  = 1
 */
int YYXBC_HIGH = 1 ;
int YYXBC_LOW  = 0 ;

bool oState[10] = {YYXBC_LOW,YYXBC_LOW,YYXBC_LOW,YYXBC_LOW,YYXBC_LOW,YYXBC_LOW,YYXBC_LOW,YYXBC_LOW,YYXBC_LOW,YYXBC_LOW};

 String getStateByIndex(int index)
{
  String  state  = "";
  if(YYXBC_HIGH == oState[index] ){
    state = BLINKER_CMD_ON;
  }else{
    state = BLINKER_CMD_OFF;
  }
  return state;
}

typedef enum
{
    RF315,
    RF433
} RFTYPE;

RFTYPE rf_receiver;

static void changeReceiver(void)
{
    static bool flag = false;
    flag = !flag;
    rf_receiver = flag ? RF315 : RF433;
//    Serial.printf("changeReceiver flag：%d \n", rf_receiver);
}

//是否处理联网状态
bool isNetConnected(){return (WiFi.status() == WL_CONNECTED);}

int BtnState2Int(String state){
  int ret = YYXBC_BTN_STATE_DEFAULT;        
  if(state == BLINKER_CMD_ON){
     ret = YYXBC_BTN_STATE_ON;
  }else if(state == BLINKER_CMD_OFF){
    ret = YYXBC_BTN_STATE_OFF;
  }else if(state == BLINKER_CMD_TAP){
    ret = YYXBC_BTN_STATE_TAP;
  }
  return ret;
}

void buttonx_callback( const int& index, const String & state ) {
    Serial.printf("get button state: %s\n", state.c_str());
    Serial.printf("index：%d \n", index);
    BlinkerButton* pBlinkerButton = FeiYangJeki::instance().getButton(index);
    if(pBlinkerButton){
        String cmd  = pBlinkerButton->cmd();
        if(cmd == YYXBC_CMD_BUTTON_CHANGE_STATE_REQ){
          if (state == BLINKER_CMD_ON) {
            Serial.println("Toggle on!");
            oState[index] = YYXBC_HIGH;

            
            #if(defined(YYXBC_WITH_315))
              IR315Handler::instance().SendIRMsg(index,BtnState2Int(state));
            #endif  

 
            IR433Handler::instance().SendIRMsg(index,BtnState2Int(state));
            IRHwaiHandler::instance().SendIRMsg(index,BtnState2Int(state));

            pBlinkerButton->set_cmd(YYXBC_CMD_BUTTON_CHANGE_STATE_RES);
            if (client.connected())  SendMsgBtnUI(FeiYangJeki::instance().getBtnNameByIndex(index).c_str(),pBlinkerButton->print("on"));
      
      
          }
          else if (state == BLINKER_CMD_OFF) {
            Serial.println("Toggle off!");
            
            oState[index] = YYXBC_LOW;

            IR433Handler::instance().SendIRMsg(index,BtnState2Int(state));

                        
            #if(defined(YYXBC_WITH_315))
              IR315Handler::instance().SendIRMsg(index,BtnState2Int(state));
            #endif  
            
          
            IRHwaiHandler::instance().SendIRMsg(index,BtnState2Int(state));
            
            pBlinkerButton->set_cmd(YYXBC_CMD_BUTTON_CHANGE_STATE_RES);
            if (client.connected())  SendMsgBtnUI(FeiYangJeki::instance().getBtnNameByIndex(index).c_str(),pBlinkerButton->print("off"));
          }
          else if (state == BLINKER_CMD_TAP) {
            Serial.println("Toggle tap!");
            
            oState[index] = !oState[index];
            
                        
            #if(defined(YYXBC_WITH_315))
              IR315Handler::instance().SendIRMsg(index,BtnState2Int(state));
            #endif  
            
            IR433Handler::instance().SendIRMsg(index,BtnState2Int(state));
            IRHwaiHandler::instance().SendIRMsg(index,BtnState2Int(state));
            
            int tempstate =  oState[index];
            pBlinkerButton->set_cmd(YYXBC_CMD_BUTTON_CHANGE_STATE_RES);
            if(YYXBC_HIGH == tempstate ){
                if (client.connected())  SendMsgBtnUI(FeiYangJeki::instance().getBtnNameByIndex(index).c_str(),pBlinkerButton->print("on"));
             }else{
                if (client.connected())  SendMsgBtnUI(FeiYangJeki::instance().getBtnNameByIndex(index).c_str(),pBlinkerButton->print("off"));
             }
          }
      
        }//if(cmd == YYXBC_CMD_BUTTON_CHANGE_STATE_REQ){
        else if(cmd ==YYXBC_CMD_BUTTON_QUERY_STATE_REQ){

            int tempstate =  oState[index];
            pBlinkerButton->set_cmd(YYXBC_CMD_BUTTON_CHANGE_STATE_RES);
            
           if(YYXBC_HIGH == tempstate ){
              if (client.connected())  SendMsgBtnUI(FeiYangJeki::instance().getBtnNameByIndex(index).c_str(),pBlinkerButton->print("on"));
           }else{
              if (client.connected())  SendMsgBtnUI(FeiYangJeki::instance().getBtnNameByIndex(index).c_str(),pBlinkerButton->print("off"));
          }

      }//else if(cmd ==YYXBC_CMD_BUTTON_QUERY_STATE_REQ){
      else if(cmd == YYXBC_CMD_BUTTON_BEGIN_DEBUG_REQ){
        Serial.println("YYXBC_CMD_BUTTON_BEGIN_DEBUG_REQ!");

        int ret = BtnState2Int(state);

                          
      #if(defined(YYXBC_WITH_315))
        IR315Handler::instance().setIsDebug(true);
        IR315Handler::instance().setIndex(index);
        IR315Handler::instance().setDebugState(ret);
      #endif  

            
        IR433Handler::instance().setIsDebug(true);
        IR433Handler::instance().setIndex(index);
        IR433Handler::instance().setDebugState(ret);

        IRHwaiHandler::instance().setIsDebug(true);
        IRHwaiHandler::instance().setIndex(index);
        IRHwaiHandler::instance().setDebugState(ret);
        
        pBlinkerButton->set_cmd(YYXBC_CMD_BUTTON_BEGIN_DEBUG_RES);
        pBlinkerButton->set_text("对码中。。");
        pBlinkerButton->set_state(getStateByIndex(index));
        if (client.connected())  SendMsgBtnUI(FeiYangJeki::instance().getBtnNameByIndex(index).c_str(),pBlinkerButton->print());
        
      }//end YYXBC_CMD_BUTTON_BEGIN_DEBUG_REQ
      else if(cmd == YYXBC_CMD_BUTTON_SAVE_DEBUG_REQ){
        Serial.println("YYXBC_CMD_BUTTON_SAVE_DEBUG_REQ!");


        IRHwaiHandler::instance().bindRICodeToBlinkerButton(index,BtnState2Int(state));
        IR433Handler::instance().bindRICodeToBlinkerButton(index,BtnState2Int(state));
        
     #if(defined(YYXBC_WITH_315))
        IR315Handler::instance().bindRICodeToBlinkerButton(index,BtnState2Int(state));
     #endif
              
        IR433Handler::instance().save433Config();
        
   #if(defined(YYXBC_WITH_315))
        IR315Handler::instance().save315Config();
   #endif
        
        IRHwaiHandler::instance().saveHwaiConfig();

   #if(defined(YYXBC_WITH_315))
        IR315Handler::instance().setIsDebug(false);
        IR315Handler::instance().setIndex(index);
        IR315Handler::instance().resetCurrentData();
        IR315Handler::instance().setDebugState(YYXBC_BTN_STATE_DEFAULT);
   #endif
            
        IR433Handler::instance().setIsDebug(false);
        IR433Handler::instance().setIndex(index);
        IR433Handler::instance().resetCurrentData();
        IR433Handler::instance().setDebugState(YYXBC_BTN_STATE_DEFAULT);

        IRHwaiHandler::instance().setIsDebug(false);
        IRHwaiHandler::instance().setIndex(index);
        IRHwaiHandler::instance().resetCurrentData();
        IRHwaiHandler::instance().setDebugState(YYXBC_BTN_STATE_DEFAULT);

        
        Serial.println("成功绑定");
        
        pBlinkerButton->set_debug("1");
        pBlinkerButton->set_text("已经绑定");
        pBlinkerButton->set_cmd(YYXBC_CMD_BUTTON_SAVE_DEBUG_RES);
        pBlinkerButton->set_state(getStateByIndex(index));
        if (client.connected())  SendMsgBtnUI(FeiYangJeki::instance().getBtnNameByIndex(index).c_str(),pBlinkerButton->print());
        
      }//else if(cmd == YYXBC_CMD_BUTTON_SAVE_DEBUG_REQ){
      else if(cmd == YYXBC_CMD_BUTTON_WIFI_CONFIG_REQ){
        Serial.println("正在重启esp8266，启动后进入配网模式...");
        #if (defined(YYXBC_WEBCONFIG))
          WIFI_RestartToCfg();
        #else
          ESP.restart();
        #endif
        pBlinkerButton->set_cmd(YYXBC_CMD_BUTTON_WIFI_CONFIG_RES);
        pBlinkerButton->set_state(getStateByIndex(index));
        if (client.connected())  SendMsgBtnUI(FeiYangJeki::instance().getBtnNameByIndex(index).c_str(),pBlinkerButton->print());
        
      }//else if(cmd == YYXBC_CMD_BUTTON_WIFI_CONFIG_REQ){
        
   }//else if(cmd == YYXBC_CMD_BUTTON_BEGIN_DEBUG_REQ){
}

/////////////////////////////////MQTT////////////////////////////////
String getTcpServerTopic(){
  return TCP_SERVER_TOPIC;
}
void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    payload[length] = '\0';

    String msg = String((char *)payload);
    
    Serial.println(msg);
    
    if(TCP_SERVER_TOPIC == String(topic)){
      
      #ifdef HANDLER_DEBUG  
        Serial.printf("from App server :%s\r\n", (char*)msg.c_str());
      #endif 
   
      handlerAppMsgData(topic,String((char *)payload));
    }else{
      #ifdef HANDLER_DEBUG  
        Serial.printf("from Bemfa server :%s\r\n", (char*)msg.c_str());
      #endif 

      ParserAligenieMsgData(topic,String((char *)payload));
    }
}


void mqttCheckConnect(char*CLIENT_ID,char* MQTT_USRNAME ,char* MQTT_PASSWD)
{
    if (!client.connected())
    {
        client.setServer(TCP_SERVER_ADDR.c_str(), TCP_SERVER_PORT);   /* 连接WiFi之后，连接MQTT服务器 */
        Serial.print("\nConnected to server:");
        Serial.printf("%s:%d\r\n",(char*)TCP_SERVER_ADDR.c_str(),TCP_SERVER_PORT);
        Serial.println("Connecting to MQTT Server ...");
        if (client.connect(CLIENT_ID))
        {
            startMQttClient();
            Serial.println("MQTT Connected!");

        }
        else
        {
            Serial.print("MQTT Connect err:");
            Serial.println(client.state());
        }
    }
}

void mqttMsgPost(String topic,String msg)
{
    #ifdef HANDLER_DEBUG  
      Serial.printf("mqttMsgPost,topic:%s,msg:%s \n",topic.c_str(),msg.c_str());
    #endif 
   
    
    if (WiFi.status() == WL_CONNECTED)
   {
    if(client.connected()){
      boolean d = client.publish(topic.c_str(), msg.c_str());
      Serial.print("publish:0 失败;1成功");
      Serial.println(d);
    }else{
      Serial.println("还没有连接MQtt server，不能发送消息");
    }
  }
}



/*
  * 订阅topic的消息
*/
void startMQttClient(){
  Serial.printf("topic count :%d\r\n",topiclstMap.size());
  if(client.connected()){
    if(getTcpServerTopic().length() > 0 ){
        boolean d = client.subscribe(getTcpServerTopic().c_str());
        Serial.printf("subscribe tcpServerTopic: %s\n",getTcpServerTopic().c_str());
        Serial.print("subscribe:0 失败;1成功");
        Serial.println(d);
    }else{
      Serial.println("err: TcpServerTopic is empty");
    }

    topiclstMapit it = topiclstMap.begin();
    for(;it != topiclstMap.end();it++){
      TopicNodeData node = it->second;
      boolean d = client.subscribe(node.topic_id.c_str());
      Serial.print("subscribe:0 失败;1成功");
      Serial.println(d);
    }
    
  }
  
}

/**
*配网模式下，loop在这里执行
*/
int loop_callback (){
  
#if (defined(SUPPORT_XG_MAGIC_BOX))
  int state =  digitalRead(YYXBC_LED_LIGHT);
  digitalWrite(YYXBC_LED_LIGHT, !state);
  delay(500);
  digitalWrite(YYXBC_LED_LIGHT, state);
#endif

}

/**************************************************************************
                                 自己的业务逻辑
***************************************************************************/
void handlerAppMsgData(String topic,String msg){
  ParserMsgData(topic,msg);
}

/*
*{
"cmd":1,
"type":"yyxbcButton",
"id": "btn-abc",
"state":"on",
"icon":"icon_1",
"color":"#FFFFFF",
"text":"Your button name or describe",
"timestramp":1622182935
}
*/
void ParserMsgData(const String& topic,const String& msg){
   //json数据解析
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, msg);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  
   #ifdef HANDLER_DEBUG  
    Serial.print("ParserMsgData:"); 
    Serial.println(msg); 
  #endif 

  JsonVariant cmd = doc["cmd"];
  JsonVariant type = doc["type"];
  String getcmd = cmd.as<String>();
  String gettype = type.as<String>();
  lasttimestamp = doc["timestamp"];
  if(getcmd == YYXBC_CMD_BUTTON_QUERY_STATE_REQ){
    reportState();
  }
  else if(getcmd == YYXBC_CMD_BUTTON_DEVICE_CONFIG_REQ){
//    hanelerDeviceConfig(topic,doc.as<JsonObject>());
  }
  else if(getcmd == YYXBC_CMD_BUTTON_QUERY_STATE_REQ ||
         getcmd == YYXBC_CMD_BUTTON_BEGIN_DEBUG_REQ ||
         getcmd == YYXBC_CMD_BUTTON_SAVE_DEBUG_REQ ||
         getcmd == YYXBC_CMD_BUTTON_CHANGE_STATE_REQ ){
    if (gettype == YYXBC_CMD_BUTTON){
        ParserBlinkerButtonMsgData(topic,doc.as<JsonObject>());
    }
  }else{
    Serial.println("Msg from myself");
  }
}


/****
 * 解析BlinkerButton类型的控件消息,msg的值
 */
void ParserBlinkerButtonMsgData(const String& topic,const JsonObject& data){
    String state = data["state"];
    String icon = data["icon"];
    String color = data["color"];
    String text = data["text"];
    String debug = data["debug"];
    String cmd = data["cmd"];
    String id = data["id"];
    lasttimestamp = data["timestamp"];
    
    char szLog[256];
    sprintf(szLog,"topic:%s,id:%s,state:%s,icon:%s,color:%s,text:%s,timestamp:%l",
    topic.c_str(),
    id.c_str(),
    state != NULL ? state.c_str():"",
    icon != NULL ? icon.c_str():"",
    color != NULL ? color.c_str():"",
    text != NULL ? text.c_str():"",
    lasttimestamp);

 #ifdef HANDLER_DEBUG  
    Serial.println(szLog); 
  #endif 

    BlinkerButton* pBlinkerButton = FeiYangJeki::instance().getBlinkerButton(id);
    if(pBlinkerButton){
      if(state != NULL && state.length() >0 )pBlinkerButton->set_state(state);
      if(icon != NULL && icon.length() >0 )pBlinkerButton->set_icon(icon);
      if(color != NULL && color.length() >0 )pBlinkerButton->set_color(color);
      if(text != NULL && text.length() >0 )pBlinkerButton->set_text(text);
      if(debug != NULL && debug.length() >0 )pBlinkerButton->set_debug(debug);
      if(cmd != NULL && cmd.length() >0 )pBlinkerButton->set_cmd(cmd);
      pBlinkerButton->set_timestamp(lasttimestamp);
      pBlinkerButton->run();  
    }
    
}

//###以下是msg消息的定义，msg是base64加密了的json消息结构体，消息消息定义如下
bool SendMsgBtnUI(String btnId, String json ){

    String topic = getTcpServerTopic();
    if(topic.length() >0 ){
      //send message
      mqttMsgPost(topic.c_str(),json);
    }else{
       Serial.println("con't find CMD topic :" + topic);
    }
  
  return true;
}

//###以下是msg消息的定义，msg是base64加密了的json消息结构体，消息消息定义如下
bool SendMsgUI( String cmd ,int errCode){

    DynamicJsonDocument doc(1024);
    doc["type"] = "yyxbcCMD";
    doc["cmd"] = cmd;
    doc["errCode"] = errCode;
    doc["timestamp"] = lasttimestamp;
    
    String json;
    serializeJson(doc, json);
    doc.clear();

 #ifdef HANDLER_DEBUG  
     Serial.println(json);
  #endif 
  
    String topic = getTcpServerTopic();
    if(topic.length() >0 ){
      //send message
      mqttMsgPost(topic.c_str(),json);
    }else{
       Serial.println("con't find CMD topic :" + topic);
    }

  return true;
}

bool SendMsgAudioBoxUI(String btnId,  String state){

    TopicNodeData topic;
    bool bret = getTopicByBtnId(btnId,topic);
    if(bret){
      //send message
      mqttMsgPost(topic.topic_id.c_str(),state.c_str());
    }else{
       Serial.println("con't find topic by btnid :" + btnId);
    }

}

/*
*查找topic by 按钮的id
*/
bool getTopicByBtnId(String btnId , TopicNodeData& topic){
   topiclstMapit it = topiclstMap.find(btnId.c_str());
   if(it != topiclstMap.end()){
      topic = it->second;
      return true;
    }
    return false;
}


//*******如果天猫精灵有对设备进行操作就执行下面

/****
 * 解析天猫精灵类型的控件消息,msg的值
 */
void ParserAligenieMsgData(const String& topic, const String& msg){
  
  #ifdef HANDLER_DEBUG  
    Serial.println("ParserAligenieMsgData:");
    Serial.println(msg);
  #endif 


  String deviceType = FeiYangJeki::instance().getDeviceTypeByeTopic(topic);
  Serial.println("deviceType:");
  Serial.println(deviceType);
  
  //开关类型设备处理
  if(deviceType = YYXBC_BEMFA_DEVICE_TYPE_SWITCH){
    bool bfind = false;
    std::string btnId = "";
    topiclstMapit it  = topiclstMap.begin();
    for(;it != topiclstMap.end();it++){
      TopicNodeData& node = it->second;
      if(node.topic_id == topic.c_str()){
        btnId = it->first;
        bfind = true;
        break;
      }
    }
    if(msg == BLINKER_CMD_ON){
       String cmd = YYXBC_CMD_BUTTON_CHANGE_STATE_REQ;
       String state  = BLINKER_CMD_ON;
       BlinkerButton* pBlinkerButton = FeiYangJeki::instance().getBlinkerButton(btnId.c_str());
      if(pBlinkerButton){
        if(state != NULL && state.length() >0 )pBlinkerButton->set_state(state);
        if(cmd != NULL && cmd.length() >0 )pBlinkerButton->set_cmd(cmd);
        pBlinkerButton->run();  
      }
    }
    else if(msg == BLINKER_CMD_OFF){
       String cmd = YYXBC_CMD_BUTTON_CHANGE_STATE_REQ;
       String state  = BLINKER_CMD_OFF;
       BlinkerButton* pBlinkerButton = FeiYangJeki::instance().getBlinkerButton(btnId.c_str());
      if(pBlinkerButton){
        if(state != NULL && state.length() >0 )pBlinkerButton->set_state(state);
        if(cmd != NULL && cmd.length() >0 )pBlinkerButton->set_cmd(cmd);
        pBlinkerButton->run();  
      }
    }
  }
}

void button_callback(int index, const String & state) { 
  buttonx_callback(index,state);
}




/**************************************************************************
                                 getTopicList
***************************************************************************/
/**
 * https://iot.xn2.net/api/app/device/login?auth=IlONWVTkyHmJbgSo
{
    "status": 0,
    "tips": "OK",
    "data": {
        "tcp_server": "bemfa.com",
        "tcp_port": 8433,
        "uid": "320dac242b1311358d774e818f6de17a",
        "topic_list": [
            {
                "topic_id": "DPHGFnuNSQ002",
                "msg": ""
            },
            {
                "topic_id": "DPENZRjxlO002",
                "msg": ""
            },
            {
                "topic_id": "DPhtmpPbNT002",
                "msg": ""
            },
            {
                "topic_id": "DPAzHKkrqI002",
                "msg": ""
            }
        ]
    }
}
*/


  // 发送HTTP请求并且将服务器响应通过串口输出
bool getTopicList(){
  
  bool bret = false;
  //创建 HTTPClient 对象
  HTTPClient httpClient;

   Serial.print("[HTTPS] begin...\n");  
   
  char URL[256];
  sprintf(URL,"%s/api/app/device/login?auth=%s",(char*)weburl.c_str(),(char*)auth.c_str());
  httpClient.begin(URL); 
  
   #ifdef HANDLER_DEBUG  
      Serial.print("URL: "); Serial.println(URL);
   #endif 
 
 
  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.print("Send GET request to URL: ");
  
  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  //如果服务器不响应OK则将服务器响应状态码通过串口输出
  if (httpCode == HTTP_CODE_OK) {
    String responsePayload = httpClient.getString();

      
   #ifdef HANDLER_DEBUG  
      Serial.println("Server Response Payload: ");
      Serial.println(responsePayload);
   #endif 


    Serial.println(String(ESP.getFreeHeap()));
    
    ArudinoStreamParser parser;
    TopicNodestHandler custom_handler;
    
    parser.setHandler(&custom_handler); // Link to customer listener (parser to be honest)
    for (int i = 0; i < responsePayload.length(); i++) {
      parser.parse(responsePayload.charAt(i)); 
    }
    Serial.println(String(ESP.getFreeHeap()));

    bret =true;

      
  } else {
    Serial.println("Server Respose Code：");
    Serial.println(httpCode);
  }

  //关闭ESP8266与服务器连接
  httpClient.end();

  return bret;

}

bool isGetTopicList(){
  if(topiclstMap.size() >0 ){
    return true;
  }
  return false;
}

void topicInit(){
   if(getTopicList() && getLastErr() == 0){

   }
}

void reportState(){
    for(int i=1;i<= YYXBC_BUTTON_NUM;i++){
     BlinkerButton* pBlinkerButton = FeiYangJeki::instance().getButton(i);
     if(pBlinkerButton){
          pBlinkerButton->set_cmd(YYXBC_CMD_BUTTON_CHANGE_STATE_RES);
          if(YYXBC_HIGH == oState[i]){
            #ifdef HANDLER_DEBUG  
            Serial.printf("reportState,%s state: BLINKER_CMD_ON \n",FeiYangJeki::instance().getBtnNameByIndex(i).c_str());
            #endif  
            if (client.connected())  SendMsgBtnUI(FeiYangJeki::instance().getBtnNameByIndex(i).c_str(),pBlinkerButton->print("on"));
          }else{
            if (client.connected())  SendMsgBtnUI(FeiYangJeki::instance().getBtnNameByIndex(i).c_str(),pBlinkerButton->print("off"));
            #ifdef HANDLER_DEBUG  
            Serial.printf("reportState,%s state: BLINKER_CMD_OFF \n",FeiYangJeki::instance().getBtnNameByIndex(i).c_str());
            #endif 
          }
     }
  }
}

//心跳回调
void heartbeat()
{
    Serial.println("heartbeat ----- " );
    //较正app的按钮状态
    reportState();
}

void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);

    // 初始化有LED的IO
    pinMode(  YYXBC_LED_LIGHT, OUTPUT);
    digitalWrite(  YYXBC_LED_LIGHT, LOW);

//
  #if (defined(SUPPORT_XG_MAGIC_BOX))
//    pinMode(  YYXBC_BUTTON_RESET, OUTPUT);
//    digitalWrite(  YYXBC_BUTTON_RESET, HIGH);
  #else
   pinMode(  YYXBC_BUTTON_RESET, OUTPUT);
   digitalWrite(  YYXBC_BUTTON_RESET, LOW);
  #endif


   // 初始化有LED的IO
    FeiYangJeki::instance().initBlinkerButtonList(button_callback);

  #if (defined(YYXBC_WITH_IR))
   IRHwaiHandler::instance().Init();  
  #endif

  #if(defined(YYXBC_WITH_433))
    IR433Handler::instance().Init();
  #endif  

  #if(defined(YYXBC_WITH_315))
    IR315Handler::instance().Init();
  #endif  


 #if (defined(YYXBC_WEBCONFIG))
      //wifi 配网
      attach(loop_callback);
      WIFI_Init();
      Settings& cfg = getSettings();
      Serial.printf("WiFi.begin(%s,%s)\n",WiFi.SSID().c_str(),WiFi.psk().c_str());
      WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str());
      auth = cfg.auth;
 #else
      WiFi.begin(ssid, pswd);
 #endif  
 
    client.setCallback(mqtt_callback);

    change_rf_ticker.attach_ms_scheduled(100, changeReceiver);

    button_event_init();//按钮事件初始化
}

void updateLEDLight(){
  static int is_btn = WiFi.status();
  int is = WiFi.status(); 
  if ( is != is_btn)
  {
    if(WiFi.status() != WL_CONNECTED){
      pinMode(YYXBC_LED_LIGHT,OUTPUT);
      digitalWrite(YYXBC_LED_LIGHT, HIGH);
    }else{
      pinMode(YYXBC_LED_LIGHT,OUTPUT);
      digitalWrite(YYXBC_LED_LIGHT, LOW);
    }

    is_btn = is;
  
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if( WiFi.status() != WL_CONNECTED)
  {
     static int physicalkeylastms = millis();
    if (millis()-physicalkeylastms > 1000) {
        physicalkeylastms = millis();
        Serial.println("WiFi not Connect");
    }
  }

  //连接上wifi，修改led灯状态
  updateLEDLight();

  //不断检测按钮按下状态
  button_attach_loop();

  if(WiFi.status() == WL_CONNECTED && !isGetTopicList()){
    
    Serial.println("Connected to AP");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("espClient [");
    
    Serial.printf(PSTR("Running Free mem=%d\n"), ESP.getFreeHeap());    
    if(LAST_ERR != 102 ){
      topicInit();
    }
    else {
      static int tm1stms = millis();
      if (millis()-tm1stms > 30000) {
          tm1stms = millis();
          Serial.println(" 设备不存在！ ");
      }

    }

    if(isGetTopicList()){
  //    TCP_SERVER_PORT = 9501;
      client.setServer(TCP_SERVER_ADDR.c_str(), TCP_SERVER_PORT);   /* 连接WiFi之后，连接MQTT服务器 */
      //检查MQtt server 连接
      mqttCheckConnect((char*)UID.c_str(),"",""); 
    }
      
  }

  static int lastheartbeatms = millis();
  if (millis()-lastheartbeatms > 30000) {
      lastheartbeatms = millis();
      Serial.printf(PSTR("Running (%s),version %s for %d Free mem=%d\n"),
                    WiFi.localIP().toString().c_str(),
                    version.c_str(), lastheartbeatms/1000, ESP.getFreeHeap());

      if(isGetTopicList()){
    //    TCP_SERVER_PORT = 9501;
        client.setServer(TCP_SERVER_ADDR.c_str(), TCP_SERVER_PORT);   /* 连接WiFi之后，连接MQTT服务器 */
        //检查MQtt server 连接
        mqttCheckConnect((char*)UID.c_str(),"",""); 
      }
      /* 上报 */
//      heartbeat();

      Serial.printf(PSTR("Running Free mem=%d\n"), ESP.getFreeHeap());

  }

  client.loop();

 #if (defined(YYXBC_WITH_IR))
   if(IRHwaiHandler::instance().recvIR()){
     if(IRHwaiHandler::instance().getIsDebug()){
        int index = IRHwaiHandler::instance().getIndex();
        mySwitchHandler(index,"Hwai");
     }
   }
             
 #endif

//同一时间，433和315只能有一个能接收
  if (rf_receiver == RF433)
  {
      IR315Handler::instance().disableReceive();
      IR433Handler::instance().enableReceive();
      delay(100);
      #if (defined(YYXBC_WITH_433))
      if(IR433Handler::instance().recvIR()){
        if(IR433Handler::instance().getIsDebug()){
          int index = IR433Handler::instance().getIndex();
          mySwitchHandler(index,"433");
        }
      }
 #endif
  }
  else
  {
      IR433Handler::instance().disableReceive();
      IR315Handler::instance().enableReceive();
      delay(100);
      #if (defined(YYXBC_WITH_315))
      if(IR315Handler::instance().recvIR()){
        if(IR315Handler::instance().getIsDebug()){
          int index = IR315Handler::instance().getIndex();
          mySwitchHandler(index,"315");
        }
      }
      #endif

  }
        
// //物理开关重置按键
// if(btnResetHandler()){
//  Serial.println("正在重启esp8266，启动后进入配网模式...");
//          
//  #if(defined(YYXBC_WITH_315))
//    IR315Handler::instance().clearBinFiles();
//  #endif  
//  IR433Handler::instance().clearBinFiles();
//  IRHwaiHandler::instance().clearBinFiles();
//
//  Serial.println("配置信息已经清除...");
//
//  #if (defined(YYXBC_WEBCONFIG))
//    WIFI_RestartToCfg();
//  #else
//  ESP.restart();
//  #endif
// }
 
}


//###以下是msg消息的定义，msg是base64加密了的json消息结构体，消息消息定义如下
bool SendMsgBtnUI( String cmd, String  uid,
                   String btnId, String btnState, String btnIcon, String btnColor, String btnText ){

    DynamicJsonDocument doc(1024);
    doc["type"] = "yyxbcButton";
    doc["id"] = btnId.c_str();
    doc["cmd"] = cmd;
    if(btnState.length() >0 ) doc["state"] = btnState.c_str();
    if(btnIcon.length() >0 )  doc["icon"] = btnIcon.c_str();
    if(btnColor.length() >0 ) doc["color"] = btnColor.c_str();
    if(btnText.length() >0 ) doc["text"] = btnText.c_str();
    doc["timestamp"] = lasttimestamp;
    
    String json;
    serializeJson(doc, json);
    doc.clear();

 #ifdef HANDLER_DEBUG  
     Serial.println(json);
  #endif 
  
    String topic = getTcpServerTopic();
    if(topic.length() >0 ){
      //send message
      mqttMsgPost(topic.c_str(),json);
    }else{
       Serial.println("con't find topic,btnid :" + btnId);
    }
 
  return true;
}


//自锁模式按钮，监听433按钮状态，执行相应处理
bool mySwitchHandler(const int& index ,String str)
{
  static long lastphysicsms = millis();
  int dif = millis()-lastphysicsms;
  bool ret = false;
  Serial.println(dif);
  if (dif > 500) {
      bool is_led = getStateByIndex(index);
      oState[index] = !oState[index];
      if (is_led == YYXBC_HIGH)
      {
        SendMsgBtnUI(YYXBC_CMD_BUTTON_BEGIN_DEBUG_RES,UID,FeiYangJeki::instance().getBtnNameByIndex(index),getStateByIndex(index),"","",str);
        Serial.println("按钮对灯已执行关闭");
      }
      else
      {
        SendMsgBtnUI(YYXBC_CMD_BUTTON_BEGIN_DEBUG_RES,UID,FeiYangJeki::instance().getBtnNameByIndex(index),getStateByIndex(index),"","",str);
        Serial.println("按钮对灯已执行打开");
      }
      is_led = oState[index];  //更新按钮状态
      ret =  true;
 }

 lastphysicsms = millis();

 return ret;
}


//
//点动模式按钮，监听按钮状态，执行相应处理
bool btnResetHandler()
{
  static int physicalkeylastms = millis();
  static bool oButtonState = false;
  int state1 =  digitalRead(YYXBC_BUTTON_RESET); 
  
  #if (defined(SUPPORT_XG_MAGIC_BOX))
    if(state1 == LOW )
  #else
    if(state1 == HIGH )
  #endif
  
  {
      if (millis()-physicalkeylastms > 5000) {
          physicalkeylastms = millis();
          Serial.printf("HIGH reset key：,physicalKey:%d\n",YYXBC_BUTTON_RESET);
          return true;   
      }
 
  }else{
      oButtonState = true;
      physicalkeylastms = millis();
  }
  
  return false;
}

////////////////////////////////////////////////////////////////////////////////////
//按键事件回调函数
//单击
void attachClick()
{
    Serial.println("click-单击");
    Serial.println("正在重启esp8266...");  
    ESP.restart();
}

//双击
void attachDoubleClick()
{
    Serial.println("doubleclick-双击");
}

unsigned long lastpresstms = 0;
//长铵开始
void attachLongPressStart()
{
    
    Serial.println("longPressStart-长按开始");
    lastpresstms =  millis();
}

//长按过程
void attachDuringLongPress()
{
  if (button.isLongPressed())
  {
      Serial.println("duringLongPress-长按期间");
  }
}

//长按结束
void attachLongPressStop()
{
    Serial.println("longPressStop-长按结束");
    if (millis()-lastpresstms > 5000) {
        Serial.println("正在重启esp8266，启动后进入配网模式...");  
        #if(defined(YYXBC_WITH_315))
          IR315Handler::instance().clearBinFiles();
        #endif  
        IR433Handler::instance().clearBinFiles();
        IRHwaiHandler::instance().clearBinFiles();
      
        Serial.println("配置信息已经清除...");
      
        #if (defined(YYXBC_WEBCONFIG))
          WIFI_RestartToCfg();
        #else
        ESP.restart();
        #endif
    } 
}

//按下多次
void attachMultiClick()
{
  Serial.printf("getNumberClicks-总共按了：%d次。\r\n",button.getNumberClicks());
  switch(button.getNumberClicks()){
      case 3:{Serial.printf("switch语句判断出打印3次。\r\n");break;}
      case 4:{Serial.printf("switch语句判断出打印4次。\r\n");break;}
      case 5:{Serial.printf("switch语句判断出打印5次。\r\n");break;}
      case 6:{Serial.printf("switch语句判断出打印6次。\r\n");break;}
      default:{Serial.printf("switch语句判断出打印其它次数:[%d]。\r\n",button.getNumberClicks());break;}
  
  
  }
}

//回调函数绑定子程序
void button_event_init(){

  button.reset();//清除一下按钮状态机的状态
   /**
   * set # millisec after safe click is assumed.
   */
  //void setDebounceTicks(const int ticks);
  button.setDebounceTicks(80);//设置消抖时长为80毫秒,默认值为：50毫秒

  /**
   * set # millisec after single click is assumed.
   */
  //void setClickTicks(const int ticks);
  button.setClickTicks(500);//设置单击时长为500毫秒,默认值为：400毫秒

  /**
   * set # millisec after press is assumed.
   */
  //void setPressTicks(const int ticks);
  button.setPressTicks(1000);//设置长按时长为1000毫秒,默认值为：800毫秒
  
  button.attachClick(attachClick);//初始化单击回调函数
  button.attachDoubleClick(attachDoubleClick);//初始化双击回调函数
  button.attachLongPressStart(attachLongPressStart);//初始化长按开始回调函数
  button.attachDuringLongPress(attachDuringLongPress);//初始化长按期间回调函数
  button.attachLongPressStop(attachLongPressStop);//初始化长按结束回调函数
  button.attachMultiClick(attachMultiClick);//初始化按了多次(3次或以上)回调函数
}

//按钮检测状态子程序
void button_attach_loop(){
    //不断检测按钮按下状态
    button.tick();
}
