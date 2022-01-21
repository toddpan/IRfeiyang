#pragma once
#include "defines.h"
#include <map>
#include<vector>
#include<iostream>
#include <LittleFS.h>
#include <FS.h>
#include <RCSwitch.h>


//433指令数据格式
struct Raw315Data{
  uint16_t index;
  uint16_t bits;
  uint16_t protocol;
  uint16_t nState;
  long unsigned data;
  Raw315Data():protocol(0),data(0){}
  
};
//433指令列表
typedef std::map<std::string, Raw315Data>::iterator mySwitch315ListMapit;

class IR315Handler{
  
private:
  RCSwitch my315Switch = RCSwitch(); 
  bool is_433_315_debug_ = false;
  int  is_433_315_index_ = -1;
  int  debug_state_ = 0;
  Raw315Data rawData_ ;
  long unsigned lastSendms_ = millis();
  std::map<std::string, Raw315Data>mySwitch315ListMap;
public:
  static IR315Handler* instance_;
  static IR315Handler& instance(){ 
    if(instance_ == NULL){
      instance_ = new  IR315Handler();
    }
    return *instance_;
  }

long unsigned getLastSendms(){return lastSendms_;}
Raw315Data& getCurrentData(){return rawData_; }
void resetCurrentData(){memset(&rawData_,0,sizeof(Raw315Data));};
void setIsDebug(bool is_433_315_debug){ is_433_315_debug_ = is_433_315_debug; }
bool getIsDebug(){return is_433_315_debug_;}
void setIndex(int is_433_315_index){is_433_315_index_ = is_433_315_index;}
int  getIndex(){return is_433_315_index_;}
void setDebugState(int debug_state){debug_state_ = debug_state;}
int  getDebugState(){return debug_state_;}
  
void Init(){
    memset(&rawData_,0,sizeof(Raw315Data));
    //load 433 configure
    load315Config();
//    pinMode(PHYSICAL_315_RECV, FUNCTION_3);
//    Serial.println("pinMode FUNCTION_3");
     //设置射频发射模式的数据io引脚
    my315Switch.enableTransmit(PHYSICAL_315_SEND);
    my315Switch.enableReceive(PHYSICAL_315_RECV);
    Serial.println("started  315 RCSwitch");
}

void enableReceive(){
      my315Switch.enableReceive(PHYSICAL_315_RECV);
}

void disableReceive(){
      my315Switch.disableReceive();  
}

bool recvIR(){
 int ret = false;
 if (my315Switch.available()) {
    uint16_t bit = my315Switch.getReceivedBitlength();
    uint16_t protocol = my315Switch.getReceivedProtocol();
    long unsigned switchKey = my315Switch.getReceivedValue();
    Serial.printf("Received 315:%u,/bit %u,Protocol %u \r\n",switchKey,bit,protocol); 
    my315Switch.resetAvailable();
    
    if(bit > 0 ){
      if( getIsDebug() &&  (millis()-getLastSendms() > 1000)){
          int index =  getIndex();
          Serial.println("射频debug模式");
          Serial.println(index);
          if(getDebugState() >0 ) {
              memset(&rawData_,0,sizeof(Raw315Data));
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
  Raw315Data&  raw = getCurrentData();
  if(raw.data > 0){
     addRaw315Data(index, raw.bits, raw.protocol,raw.data,getDebugState()); 
  }else{
    Serial.printf("invalid 315 code, bindCurrentRICode:%u,state:%d \n", index,state);
  }
}


 bool findRaw315Data(const std::string& key , Raw315Data& raw)
{
  mySwitch315ListMapit it = mySwitch315ListMap.find(key);
  if(it != mySwitch315ListMap.end()){
    raw = it->second;
    return true;
  }
  return false;
}

 void delRaw315Data(const std::string& key){
   mySwitch315ListMapit it = mySwitch315ListMap.find(key);
  if(it != mySwitch315ListMap.end()){
    mySwitch315ListMap.erase(it);
  }
}

  std::string getKey(uint16_t index,int state){
    char szKey[50];
    sprintf(szKey,"433_%u_%d",index,state);
    return std::string(szKey);
  }

 void addRaw315Data(uint16_t index,uint16_t bit,uint16_t protocol,long unsigned data ,int state ){

  Raw315Data raw;
  std::string key = getKey(index,state);
  if(findRaw315Data(key,raw) ){
    delRaw315Data(key);
    Serial.println("433指令已经存在，已经删除旧的数据");
  }
  Raw315Data tempRaw315Data_ ;
  tempRaw315Data_.index = index;
  tempRaw315Data_.bits = bit;
  tempRaw315Data_.protocol = protocol;
  tempRaw315Data_.data = data;
  tempRaw315Data_.nState = state;
  mySwitch315ListMap[key] = tempRaw315Data_;
  Serial.println("指令对码完成");
  
}

 bool save315Config(){
  if (!LittleFS.begin()) {
    Serial.println("Unable to begin(), aborting");
    return false;
  }
  Serial.println("Save file, may take a while...");
  long start = millis();
  File f = LittleFS.open(BIN_SAVE_315Cfg_PATH, "w");
  if (!f) {
    Serial.println("Unable to open file for writing, aborting");
    return false;
  }
  int nSize = mySwitch315ListMap.size();
  int nwrite = f.write((char*)&nSize, sizeof(uint16_t));
  // assert(nwrite != sizeof(uint16_t));
  mySwitch315ListMapit it = mySwitch315ListMap.begin();
  for(;it != mySwitch315ListMap.end();it++){
    Raw315Data data = it->second;
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

bool load315Config(){
if (!LittleFS.begin()) {
    Serial.println("Unable to begin(), aborting");
    return false;
  }
  Serial.println("Reading file, may take a while...");
  long start = millis();
  File f = LittleFS.open(BIN_SAVE_315Cfg_PATH, "r");
  if (!f) {
    Serial.println("Unable to open file for reading, aborting");
    return false;
  }
  int nSize = 0;
  int nread = f.read((uint8_t*)&nSize, sizeof(uint16_t));
  // assert(nread != sizeof(uint16_t));
   Serial.println(nread);
  for(int i = 0;i< nSize ;i++){
    Raw315Data pRaw315Data ;
    f.read((uint8_t*)&pRaw315Data.index, sizeof(uint16_t));
    f.read((uint8_t*)&pRaw315Data.bits, sizeof(uint16_t));
    f.read((uint8_t*)&pRaw315Data.protocol, sizeof(uint16_t));
    f.read((uint8_t*)&pRaw315Data.nState, sizeof(uint16_t));
    f.read((uint8_t*)&pRaw315Data.data, sizeof(long unsigned));
    Serial.printf("index:%u,bit:%u,protocol:%u,data:%u \r\n",pRaw315Data.index,pRaw315Data.bits,pRaw315Data.protocol,pRaw315Data.data);
    mySwitch315ListMap[getKey(pRaw315Data.index,pRaw315Data.nState)] = pRaw315Data;
  }
  f.close();
  long stop = millis();
  Serial.printf("==> Time to write  chunks = %ld milliseconds\n", stop - start);
  return true;
}

bool SendIRMsg(const int& index, const int & state){
    if(getIsDebug()){
       Serial.println("315 射频学习模式");
       Raw315Data&  raw = getCurrentData();
       if(raw.data > 0){
          // Optional set protocol (default is 1, will work for most outlets)
          my315Switch.setProtocol(raw.protocol);
          
          // Optional set pulse length.
          // my315Switch.setPulseLength(320);
          
          my315Switch.send(raw.data, raw.bits);
          lastSendms_ = millis();
          Serial.println("已执行");
       }else{
         Serial.println("315 不执行");
       }
           
    }else{
      Serial.println("315 射频工作模式");
      Raw315Data raw;
      if( findRaw315Data(getKey(index,state),raw)){
        int index =  raw.index;
        Serial.println(index);
            // Optional set protocol (default is 1, will work for most outlets)
        my315Switch.setProtocol(raw.protocol);
        
        // Optional set pulse length.
        // my315Switch.setPulseLength(320);
        
        my315Switch.send(raw.data, raw.bits);
        lastSendms_ = millis();
        Serial.println("已执行");
     }else{
        Serial.println("315 不执行");
     }
    
   }

  return false;
}
void clearBinFiles() {
//    Dir root = LittleFS.openDir(BIN_SAVE_PATH);
//    while (root.next()) {
//        LittleFS.remove(BIN_SAVE_PATH + root.fileName());
//    }
   LittleFS.remove(BIN_SAVE_315Cfg_PATH);
}
};
IR315Handler* IR315Handler::instance_  = NULL;
