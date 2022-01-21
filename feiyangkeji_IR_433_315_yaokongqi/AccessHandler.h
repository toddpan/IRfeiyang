#pragma once

#include "JsonHandler.h"
#include "ConfigHandler.h"
#include <map>
#include <string>


//#define HANDLER_DEBUG

//巴法云服务器地址默认即可
String TCP_SERVER_ADDR = "bemfa.com";
///****************需要修改的地方*****************///
//服务器端口//TCP创客云端口8344//TCP设备云端口8340
int TCP_SERVER_PORT = 9501;
//用户私钥，可在控制台获取,修改为自己的UID
String UID = "";
//控制消息的通道走这个topic
String TCP_SERVER_TOPIC ="";
//lasterr
int LAST_ERR = -1;
//tips
String LAST_TIPS = "";

//https://feiyang.sooncore.com/api/app/device/login?auth=zFHursbkCcypQDwd

//{
//    "status": 0,
//    "tips": "OK",
//    "data": {
//        "tcp_server": "bemfa.com",
//        "tcp_port": 9501,
//        "uid": "434b626e7a63a10fa616920f8c01ee9f",
//        "tcp_server_topic": "MSGXKCzxNSYMSG",
//        "config": {
//            "EventType": 1,
//            "wifiConfig": 2,
//            "triggerType": 1,
//            "initState": 2,
//            "bgImg": "image/app/index_bg3.jpg",
//            "theme": "normal"
//        },
//        "topic_list": [
//            {
//                "topic_id": "CZwpZrDxok001",
//                "msg": "",
//                "btn_id": "btn-1",
//                "btn_type": "yyxbcButton"
//            },
//            {
//                "topic_id": "CZhAmszcYE001",
//                "msg": "",
//                "btn_id": "btn-2",
//                "btn_type": "yyxbcButton"
//            },
//            {
//                "topic_id": "CZWCuhSOKE001",
//                "msg": "",
//                "btn_id": "btn-3",
//                "btn_type": "yyxbcButton"
//            },
//            {
//                "topic_id": "CZyjOoEtTD001",
//                "msg": "",
//                "btn_id": "btn-4",
//                "btn_type": "yyxbcButton"
//            }
//        ]
//    }
//}



 //{"topic_id":"light002","msg":"on"}
typedef struct{
  std::string topic_id;
  std::string btn_id;
  std::string btn_theme;
  std::string btn_type;
  std::string msg;
}TopicNodeData;

//vector<TopicNodeData> topiclst;

int getLastErr(){return LAST_ERR;}

static std::map<std::string, TopicNodeData>topiclstMap;
typedef std::map<std::string, TopicNodeData>::iterator topiclstMapit;

class TopicNodestHandler: public JsonHandler {
  
private:
  bool in_json_object_of_interest = false;
  std::string topic_id_;
  std::string btn_id_;
  std::string btn_type_;
  std::string btn_theme_;
  std::string msg_;
public:
  // Function Implementation
  void startDocument() {
  #ifdef HANDLER_DEBUG  
    Serial.println("start document");
  #endif  
  }
  
  void startArray(ElementPath path) {
  #ifdef HANDLER_DEBUG  
    Serial.println("start array. ");
  #endif  
  }
  
  void startObject(ElementPath path) {
  #ifdef HANDLER_DEBUG  
    Serial.println("start object. ");
  #endif  
  }
  
  void value(ElementPath path, ElementValue value) {
    
    char fullPath[200] = "";  
    path.toString(fullPath);
    
//    char valueBuffer[50] = "";  
//    value.toString(valueBuffer);
//    
//  #ifdef HANDLER_DEBUG  
//    Serial.print(fullPath);
//    Serial.print(": ");
//    Serial.println(valueBuffer);
//  #endif    
    
    const char* currentKey = path.getKey();
  
    if(strcmp(fullPath, "data.tcp_server") == 0) {  
       Serial.println("data.tcp_server:");    
       TCP_SERVER_ADDR =   value.getString();
       Serial.println(TCP_SERVER_ADDR);  
    }
    else if(strcmp(fullPath, "data.tcp_port") == 0) {  
       Serial.println("data.tcp_port:");    
       TCP_SERVER_PORT =   value.getInt();
       Serial.println(TCP_SERVER_PORT);  
    }
    else if(strcmp(fullPath, "data.tcp_server_topic") == 0) {  
       Serial.println("data.tcp_server_topic:");    
       TCP_SERVER_TOPIC =  value.getString();
       Serial.println(TCP_SERVER_TOPIC);  
    }
    else if(strcmp(fullPath, "data.uid") == 0) {  
       Serial.println("data.uid:");    
       UID =   value.getString();
       Serial.println(UID);  
    }
    else if(strcmp(fullPath, "status") == 0) {  
       Serial.println("status:");    
       LAST_ERR =   value.getInt();
       Serial.println(LAST_ERR);  
    }
    else if(strcmp(fullPath, "tips") == 0) {  
       Serial.println("tips:");    
       LAST_TIPS =   value.getString();
       Serial.println(LAST_TIPS);  
    }
    else if(strcmp(fullPath, "data.config.EventType") == 0) {  
       Serial.println("EventType:");    
       loginConfig.EventType =   value.getInt();
       Serial.println(loginConfig.EventType);  
    }
    else if(strcmp(fullPath, "data.config.wifiConfig") == 0) {  
       Serial.println("wifiConfig:");    
       loginConfig.wifiConfig =   value.getInt();
       Serial.println(loginConfig.wifiConfig);  
    }
    else if(strcmp(fullPath, "data.config.triggerType") == 0) {  
       Serial.println("triggerType:");    
       loginConfig.triggerType =   value.getInt();
       Serial.println(loginConfig.triggerType);  
    }
    else if(strcmp(fullPath, "data.config.initState") == 0) {  
       Serial.println("initState:");    
       loginConfig.initState =   value.getInt();
       Serial.println(loginConfig.initState);  
    }

    // Object entry?
    if(currentKey[0] != '\0') {
      
      if(strcmp(currentKey, "topic_id") == 0) {   
//        Serial.println(value.getString());
        topic_id_ = value.getString();      
      }
      else if(strcmp(currentKey, "btn_id") == 0) {
//        Serial.println(value.getString());    
        btn_id_ = value.getString();            
        //btn_type
      } 
      else if(strcmp(currentKey, "btn_type") == 0) {
//        Serial.println(value.getString());    
        btn_type_ = value.getString();            
        //
      } 
      else if(strcmp(currentKey, "btn_theme") == 0) {
//        Serial.println(value.getString());    
        btn_theme_ = value.getString();            
        //
      } 
      else if(strcmp(currentKey, "msg") == 0) {
//        Serial.println(value.getString());    
//        msg_ = value.getString();  
        in_json_object_of_interest = true;
      }
    
    
    } 
    // Array item.
    else {
      int currentIndex = path.getIndex();
      if(currentIndex == 0) {
        //TODO: use the value.
      } else if(currentIndex < 5) {
        //TODO: use the value.
      }
      // else ... 
    }
  }
  
  void endArray(ElementPath path) {
  #ifdef HANDLER_DEBUG  
    Serial.println("end array. ");
  #endif  
  }
  
  void endObject(ElementPath path) {
    if(in_json_object_of_interest){
      in_json_object_of_interest = false;
      TopicNodeData topicNode;
      topicNode.topic_id = topic_id_;
      topicNode.btn_id = btn_id_;
      topicNode.btn_type = btn_type_;
      topicNode.btn_theme = btn_theme_;
      topicNode.msg = msg_;
      topiclstMap[btn_id_] = topicNode;
    }
  
  #ifdef HANDLER_DEBUG  
    Serial.printf(PSTR("Running Free mem=%d\n"), ESP.getFreeHeap());
    Serial.println("end object. ");
  #endif  
  }
  
  void endDocument() {
  #ifdef HANDLER_DEBUG  
    Serial.println("end document. ");
  #endif  
  }
  
  void whitespace(char c) {
  #ifdef HANDLER_DEBUG  
    Serial.println("whitespace");
  #endif  
  }
};
