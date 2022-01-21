#pragma once
#include "defines.h"
#include <map>
#include <vector>
#include <string>

static unsigned long lasttimestamp = 0;

#define BLINKER_CMD_OFF "off"
#define BLINKER_CMD_ON  "on"
#define BLINKER_CMD_TAP "tap"

#define YYXBC_CMD_BUTTON "yyxbcButton"

//cmd值的定义：
//按钮打开，关闭 发送指令 ：
//发送端  cmd = 1，state = on/off
//设备端回  cmd =2，state = on/off
//
//开始和关闭对码指令:
//发送端cmd=3,state = on/off,debug = 1
//设备端回 cmd 4,state = on/off,debug = 1，"text":"对码中。。"
//debug: 1 对码模式  ，0 工作模式
//
//保存对码指令
//发送端cmd=5,state = on/off,debug = 0
//设备端回 cmd 6,state = on/off,debug = 0

#define YYXBC_CMD_BUTTON_CHANGE_STATE_REQ "1"
#define YYXBC_CMD_BUTTON_CHANGE_STATE_RES "2"
#define YYXBC_CMD_BUTTON_BEGIN_DEBUG_REQ "3"
#define YYXBC_CMD_BUTTON_BEGIN_DEBUG_RES "4"
#define YYXBC_CMD_BUTTON_SAVE_DEBUG_REQ "5"
#define YYXBC_CMD_BUTTON_SAVE_DEBUG_RES "6"
#define YYXBC_CMD_BUTTON_QUERY_STATE_REQ "7"
#define YYXBC_CMD_BUTTON_QUERY_STATE_RES "8"
#define YYXBC_CMD_BUTTON_WIFI_CONFIG_REQ "9"
#define YYXBC_CMD_BUTTON_WIFI_CONFIG_RES "10"
#define YYXBC_CMD_BUTTON_DEVICE_CONFIG_REQ "11"
#define YYXBC_CMD_BUTTON_DEVICE_CONFIG_RES "12"

//当主题名字后三位是001时为插座设备。
//当主题名字后三位是002时为灯泡设备。
//当主题名字后三位是003时为风扇设备
//当主题名字后三位是004时为传感器设备。

#define YYXBC_BEMFA_DEVICE_TYPE_SWITCH "001"
#define YYXBC_BEMFA_DEVICE_TYPE_LIGHT "002"
#define YYXBC_BEMFA_DEVICE_TYPE_FAN "003"
#define YYXBC_BEMFA_DEVICE_TYPE_SENSOR "004"

//消息来源
#define YYXBC_BEMFA_MSG_FROM_433 "433"
#define YYXBC_BEMFA_MSG_FROM_APP "001"
#define YYXBC_BEMFA_MSG_FROM_AUDIOBOX "AUDIOBOX"


//按健名字
#define YYXBC_BEMFA_BTN1  "btn-1"
#define YYXBC_BEMFA_BTN2  "btn-2"
#define YYXBC_BEMFA_BTN3  "btn-3"
#define YYXBC_BEMFA_BTN4  "btn-4"
#define YYXBC_BEMFA_BTN5  "btn-5"
#define YYXBC_BEMFA_BTN6  "btn-6"
#define YYXBC_BEMFA_BTN7  "btn-7"
#define YYXBC_BEMFA_BTN8  "btn-8"
#define YYXBC_BEMFA_BTN9  "btn-9"
#define YYXBC_BEMFA_BTN10 "btn-10"


enum YYXBC_BTN_STATE{
  YYXBC_BTN_STATE_DEFAULT = 0,
  YYXBC_BTN_STATE_ON ,
  YYXBC_BTN_STATE_OFF,
  YYXBC_BTN_STATE_TAP
};

extern "C" {
    // typedef void (*callbackFunction)(void);

    typedef void (*blinker_callback_t)(void);
    typedef void (*blinker_callback_with_arg_t)(void*);
    typedef void (*blinker_callback_with_string_arg_t)(int index ,const String & data);

}

class BlinkerButton
{
  public :
    BlinkerButton(const int& index,const String& _name):index_(index),id_(_name){}
    void attach(blinker_callback_with_string_arg_t func){func_ = func;}
    void set_state(const String & state){ if(state != NULL && state.length() > 0){state_ = state;} }
    void set_icon(const String & icon){ if(icon != NULL && icon.length() > 0){icon_ = icon;}}
    void set_color(const String & color){ if(color != NULL && color.length() > 0){color_ = color;}}
    void set_text(const String & text){ if(text != NULL && text.length() > 0){text_ = text;}}
//    void set_topic(const String & topic){ if(topic != NULL && topic.length() > 0){topicid_ = topic;}}
    void set_debug(const String & debug){ if(debug != NULL && debug.length() > 0){debug_ = debug;}}
    void set_cmd(const String & cmd){ if(cmd != NULL && cmd.length() > 0){cmd_ = cmd;}}
    void set_timestamp(const long& timestamp){ timestamp_ =timestamp;}
    String print(String state){ 
      if(state.length() >0) {state_ = state;}
      if(UID.length() == 0){Serial.println("UID error");}
        return SendMsgBtnUI(cmd(),UID,id_,state_,"","","");
    }
    String print(){
      if(UID.length() == 0){Serial.println("UID error");}
      return SendMsgBtnUI(cmd(),UID,id_,state_,icon_,color_,text_);
    }
    void run(){
      func_(index(),state());
    }
    String type(){return "yyxbcButton";}
    String id(){ return id_;}
    int    index(){return index_;}
    String state(){ return state_;}
//    String topic(){ return topicid_;}
    String debug(){return debug_;}
    String cmd(){return cmd_;}
    long timestamp(){return timestamp_;}
    
    //###以下是msg消息的定义，msg是base64加密了的json消息结构体，消息消息定义如下
    String SendMsgBtnUI( String cmd, String  uid,
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

   return json;
}
  private :
    int    index_ = 0;
    String id_ = "";
    String state_ = "";
    String icon_  = "";
    String color_ = "";
    String text_ = "";
//    String topicid_ = "";
    String debug_ = "";
    String cmd_ = "";
    long   timestamp_  = 0;
    blinker_callback_with_string_arg_t func_;
};

typedef std::map<std::string, BlinkerButton*>::iterator BlinkerButtonMapit;


class FeiYangJeki
{
  public :
   FeiYangJeki(){}
  
  static FeiYangJeki* instance_;
  static FeiYangJeki& instance(){ 
    if(instance_ == NULL){
      instance_ = new  FeiYangJeki();
    }
    return *instance_;
  }
  
  //按键列表
  std::map<std::string, BlinkerButton*>myBlinkerButtonList = {
      { "btn-1",  new BlinkerButton(1,getBtnNameByIndex(1))},
      { "btn-2",  new BlinkerButton(2,getBtnNameByIndex(2))},
      { "btn-3",  new BlinkerButton(3,getBtnNameByIndex(3))},
      { "btn-4",  new BlinkerButton(4,getBtnNameByIndex(4))},
      { "btn-5",  new BlinkerButton(5,getBtnNameByIndex(5))},
      { "btn-6",  new BlinkerButton(6,getBtnNameByIndex(6))},
      { "btn-7",  new BlinkerButton(7,getBtnNameByIndex(7))},
      { "btn-8",  new BlinkerButton(8,getBtnNameByIndex(8))},
      { "btn-9",  new BlinkerButton(9,getBtnNameByIndex(9))},
      { "btn-10",  new BlinkerButton(10,getBtnNameByIndex(10))}
  };
  
  //根据toipic的后三位得到设备类型
  String getDeviceTypeByeTopic(String topic){
    int len = topic.length();
    String type = topic.substring(len - 3);
    if(type == YYXBC_BEMFA_DEVICE_TYPE_SWITCH ){
      return YYXBC_BEMFA_DEVICE_TYPE_SWITCH;
    }else if(type == YYXBC_BEMFA_DEVICE_TYPE_LIGHT ){
      return YYXBC_BEMFA_DEVICE_TYPE_LIGHT;
    }else if(type == YYXBC_BEMFA_DEVICE_TYPE_FAN ){
      return YYXBC_BEMFA_DEVICE_TYPE_FAN;
    }else if (type == YYXBC_BEMFA_DEVICE_TYPE_SENSOR ){
      return YYXBC_BEMFA_DEVICE_TYPE_SENSOR;
    }
     return "";
  }
  
  String getBtnNameByIndex(int index){
    if(index == 1){
      return YYXBC_BEMFA_BTN1;
    }else if(index == 2){
      return YYXBC_BEMFA_BTN2;
    }else if(index == 3){
      return YYXBC_BEMFA_BTN3;
    }else if(index == 4){
      return YYXBC_BEMFA_BTN4;
    }else if(index == 5){
      return YYXBC_BEMFA_BTN5;
    }else if(index == 6){
      return YYXBC_BEMFA_BTN6;
    }else if(index == 7){
      return YYXBC_BEMFA_BTN7;
    }else if(index == 8){
      return YYXBC_BEMFA_BTN8;
    }else if(index == 9){
      return YYXBC_BEMFA_BTN9;
    }else if(index == 10){
      return YYXBC_BEMFA_BTN10;
    }
    return "";
  }
  
  BlinkerButton* getBlinkerButton(const String& id)
  {
    BlinkerButtonMapit it = myBlinkerButtonList.find(id.c_str());
    if(it != myBlinkerButtonList.end()){
       BlinkerButton* pBlinkerButton = it->second;
      if(pBlinkerButton && pBlinkerButton->id() == id){
        Serial.println("find BlinkerButton id:" + String(id));
        return pBlinkerButton;
      }
    }
    Serial.println("don't find BlinkerButton:" + String(id));
    return NULL;
  }
  
  BlinkerButton* getButton(const uint16_t index){
    BlinkerButtonMapit it = myBlinkerButtonList.begin();
    for(;it != myBlinkerButtonList.end();it++){
      BlinkerButton* pBlinkerButton = it->second;
      if(pBlinkerButton && pBlinkerButton->index() == index){
        Serial.println("find BlinkerButton index:" + String(index));
        return pBlinkerButton;
      }
    }
    Serial.println("don't find BlinkerButton index:" + index );
    return NULL;
  }
  
  void initBlinkerButtonList(blinker_callback_with_string_arg_t button_callback ){
    BlinkerButtonMapit it = myBlinkerButtonList.begin();
    for(;it != myBlinkerButtonList.end();it++){
      BlinkerButton* pBlinkerButton = it->second;
      if(pBlinkerButton){
        pBlinkerButton->attach(button_callback); 
      }
    }
  }

};

FeiYangJeki* FeiYangJeki::instance_  = NULL;
