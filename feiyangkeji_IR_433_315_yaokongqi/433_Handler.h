#pragma once
#include <map>
#include<vector>
#include<iostream>



#include "defines.h"
#include <LittleFS.h>
#include <FS.h>
#include <RCSwitch.h>



//433指令数据格式
struct Raw433Data{
  uint16_t index;
  uint16_t bits;
  uint16_t protocol;
  uint16_t nState;
  long unsigned  data;
  Raw433Data():protocol(0),data(0){}
  
};
//433指令列表


typedef std::map<std::string, Raw433Data>::iterator mySwitch433ListMapit;

class IR433Handler{
  
private:
  RCSwitch my433Switch = RCSwitch();
  bool is_433_315_debug_ = false;
  int  is_433_315_index_ = -1;
  int  debug_state_ = 0;
  Raw433Data rawData_ ;
  long unsigned lastSendms_ = millis();
  std::map<std::string, Raw433Data>mySwitch433ListMap;
public:
  static IR433Handler* instance_;
  static IR433Handler& instance(){ 
    if(instance_ == NULL){
      instance_ = new  IR433Handler();
    }
    return *instance_;
  }

long unsigned getLastSendms(){return lastSendms_;}  
Raw433Data& getCurrentData(){return rawData_; }
void resetCurrentData(){memset(&rawData_,0,sizeof(Raw433Data));};
void setIsDebug(bool is_433_315_debug){ is_433_315_debug_ = is_433_315_debug; }
bool getIsDebug(){return is_433_315_debug_;}
void setIndex(int is_433_315_index){is_433_315_index_ = is_433_315_index;}
int  getIndex(){return is_433_315_index_;}
void setDebugState(int debug_state){debug_state_ = debug_state;}
int  getDebugState(){return debug_state_;}
  
void Init(){
    memset(&rawData_,0,sizeof(Raw433Data));
    //load 433 configure
    load433Config();
//    pinMode(PHYSICAL_443_RECV, FUNCTION_3);
//    Serial.println("pinMode FUNCTION_3");
     //设置射频发射模式的数据io引脚
    my433Switch.enableTransmit(PHYSICAL_443_SEND);
    my433Switch.enableReceive(PHYSICAL_443_RECV);

    // Optional set number of transmission repetitions.
    my433Switch.setRepeatTransmit(15);
  
    Serial.println("started  433 RCSwitch");
}

void enableReceive(){
       my433Switch.enableReceive(PHYSICAL_443_RECV);
}

void disableReceive(){
      my433Switch.disableReceive();  
}

bool recvIR(){
  bool ret = false;
 if (my433Switch.available()) {
    uint16_t bit = my433Switch.getReceivedBitlength();
    uint16_t protocol = my433Switch.getReceivedProtocol();
    long unsigned switchKey = my433Switch.getReceivedValue();
    Serial.printf("Received 433:%u,/bit %u,Protocol %u \r\n",switchKey,bit,protocol);
    my433Switch.resetAvailable();
    
    if(bit > 0 ){
      if( getIsDebug() && (millis()- getLastSendms() > 1000)){
          int index =  getIndex();
          Serial.println("射频debug模式");
          Serial.println(index);
          if(getDebugState() >0 ) {
              memset(&rawData_,0,sizeof(Raw433Data));
              rawData_.index = 0;
              rawData_.bits = bit;
              rawData_.protocol = protocol;
              rawData_.nState = getDebugState();
              rawData_.data = switchKey;
          } 
          ret = true;    
      }

    } //if(bit > 0 ){

 }

 return ret;
 
}

//绑定当前指令到指令按钮上
void bindRICodeToBlinkerButton(const int& index,const int & state){
  Raw433Data&  raw = getCurrentData();
  if(raw.data > 0){
     addRaw433Data(index, raw.bits, raw.protocol,raw.data,getDebugState()); 
  }else{
     Serial.printf("invalid 433 code, bindCurrentRICode:%u,state:%d \n", index,state);
  }
}

 bool findRaw433Data(const std::string& key , Raw433Data& raw)
{
  mySwitch433ListMapit it = mySwitch433ListMap.find(key);
  if(it != mySwitch433ListMap.end()){
    raw = it->second;
    return true;
  }
  return false;
}

 void delRaw433Data(const std::string& key){
   mySwitch433ListMapit it = mySwitch433ListMap.find(key);
  if(it != mySwitch433ListMap.end()){
    mySwitch433ListMap.erase(it);
  }
}

  std::string getKey(uint16_t index,int state){
    char szKey[50];
    sprintf(szKey,"433_%u_%d",index,state);
    return std::string(szKey);
  }

 void addRaw433Data(uint16_t index,uint16_t bit,uint16_t protocol,long unsigned data ,int state ){

  Raw433Data raw;
  std::string key = getKey(index,state);
  if(findRaw433Data(key,raw) ){
    delRaw433Data(key);
    Serial.println("433指令已经存在，已经删除旧的数据");
  }
  Raw433Data tempRaw433Data_ ;
  tempRaw433Data_.index = index;
  tempRaw433Data_.bits = bit;
  tempRaw433Data_.protocol = protocol;
  tempRaw433Data_.nState = state;
  tempRaw433Data_.data = data;
  mySwitch433ListMap[key] = tempRaw433Data_;
  Serial.println("指令对码完成");
  
}

 bool save433Config(){
  if (!LittleFS.begin()) {
    Serial.println("Unable to begin(), aborting");
    return false;
  }
  Serial.println("Save file, may take a while...");
  long start = millis();
  File f = LittleFS.open(BIN_SAVE_433Cfg_PATH, "w");
  if (!f) {
    Serial.println("Unable to open file for writing, aborting");
    return false;
  }
  int nSize = mySwitch433ListMap.size();
  int nwrite = f.write((char*)&nSize, sizeof(uint16_t));
  // assert(nwrite != sizeof(uint16_t));
  mySwitch433ListMapit it = mySwitch433ListMap.begin();
  for(;it != mySwitch433ListMap.end();it++){
    Raw433Data data = it->second;
    f.write((char*)&data.index, sizeof(uint16_t));
    f.write((char*)&data.bits, sizeof(uint16_t));
    f.write((char*)&data.protocol, sizeof(uint16_t));
    f.write((uint8_t*)&data.nState, sizeof(uint16_t));
    f.write((char*)&data.data, sizeof(long unsigned));
  }

  f.close();
  long stop = millis();
  Serial.printf("==> Time to write  chunks = %ld milliseconds\n", stop - start);
  return true;
}

bool load433Config(){
if (!LittleFS.begin()) {
    Serial.println("Unable to begin(), aborting");
    return false;
  }
  Serial.println("Reading file, may take a while...");
  long start = millis();
  File f = LittleFS.open(BIN_SAVE_433Cfg_PATH, "r");
  if (!f) {
    Serial.println("Unable to open file for reading, aborting");
    return false;
  }
  int nSize = 0;
  int nread = f.read((uint8_t*)&nSize, sizeof(uint16_t));
  // assert(nread != sizeof(uint16_t));
   Serial.println(nread);
  for(int i = 0;i< nSize ;i++){
    Raw433Data pRaw433Data ;
    f.read((uint8_t*)&pRaw433Data.index, sizeof(uint16_t));
    f.read((uint8_t*)&pRaw433Data.bits, sizeof(uint16_t));
    f.read((uint8_t*)&pRaw433Data.protocol, sizeof(uint16_t));
    f.read((uint8_t*)&pRaw433Data.nState, sizeof(uint16_t));
    f.read((uint8_t*)&pRaw433Data.data, sizeof(long unsigned));
    Serial.printf("index:%u,bit:%u,protocol:%u,data:%u \r\n",pRaw433Data.index,pRaw433Data.bits,pRaw433Data.protocol,pRaw433Data.data);
    mySwitch433ListMap[getKey(pRaw433Data.index,pRaw433Data.nState)] = pRaw433Data;
  }
  f.close();
  long stop = millis();
  Serial.printf("==> Time to write  chunks = %ld milliseconds\n", stop - start);
  return true;
}

bool SendIRMsg(const int& index, const int & state){
    if(getIsDebug()){
       Serial.println("433 射频学习模式");
       Raw433Data&  raw = getCurrentData();
       if(raw.data > 0){
          // Optional set protocol (default is 1, will work for most outlets)
          my433Switch.setProtocol(raw.protocol);
          
          // Optional set pulse length.
          // my315Switch.setPulseLength(320);
          
          my433Switch.send(raw.data, raw.bits);
          lastSendms_ = millis();

          Serial.printf("btn-%d,keydata:%u, bits %u,protocol %u 已执行\n",index,raw.data,raw.bits,raw.protocol);  
          
       }else{
         Serial.println("--不执行");
       }
      
    }else{
      Serial.println("433 射频工作模式");
      Raw433Data raw;
      if( findRaw433Data(getKey(index,state),raw)){

        int index =  raw.index;
        Serial.println(index);
            // Optional set protocol (default is 1, will work for most outlets)
        my433Switch.setProtocol(raw.protocol);
        
        // Optional set pulse length.
        // my433Switch.setPulseLength(320);
        
        my433Switch.send(raw.data, raw.bits);
  
        lastSendms_ = millis();
        
        Serial.printf("btn-%d,keydata:%u, bits %u,protocol %u 已执行\n",index,raw.data,raw.bits,raw.protocol); 
     }else{
      Serial.println("--不执行");
     }
    
   }

  return false;
}
void clearBinFiles() {
//    Dir root = LittleFS.openDir(BIN_SAVE_PATH);
//    while (root.next()) {
//        LittleFS.remove(BIN_SAVE_PATH + root.fileName());
//    }
   LittleFS.remove(BIN_SAVE_433Cfg_PATH);
}
};

IR433Handler* IR433Handler::instance_  = NULL;
