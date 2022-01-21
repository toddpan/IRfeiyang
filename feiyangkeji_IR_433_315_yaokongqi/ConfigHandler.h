#pragma once

#include <map>
#include <string>
#include <LittleFS.h>
#include <time.h>

//1，触发方式，1，高电平（默认）2，低电平    triggerType: 1：高电平 （默认）2：低电平
//2，是否物理开关连续5次进入配网模式。wifiConfig[1,2] 1:连续开、关5次物理开关，进入配网模式。 2、长按物理开关10秒进入配置模式（默认）（点动模式有效）
//3，点动模式，自锁模式  EventType 1:点动模式（默认） ，2：自锁模式
//4，通电时开关状态开还是关，initState 1:开 ，2：关 （默认） 

enum EventType{
  ELECTRONIC =1,//1:点动模式（默认)
  SELF_LOCK = 2//自锁模式
};

enum WifiConfig{
  FIVE_TIMES = 1,//连续开、关5次物理开关，进入配网模式
  LONG_TOUCH = 2,//长按物理开关10秒进入配置模式（默认）（点动模式有效）
  WifiConfig_NONE = 3 //不执行
};

enum TriggerType{
  TriggerType_HIGH = 1,//高电平
  TriggerType_LOW  = 2//低电平
};

enum InitState{
  INIT_ON = 1,//开
  INIT_OFF = 2//关 （默认） int
};

typedef struct{
  int  EventType = 1;
  int  wifiConfig = 2;
  int  triggerType = 1;
  int  initState = 2;
}LoginConfig;

static LoginConfig loginConfig;

static int getEventType(){
  return loginConfig.EventType;
}

static void setEventType(int EventType){
   loginConfig.EventType = EventType;
}

static int getWifiConfig(){
  return loginConfig.wifiConfig;
}

static void setWifiConfig(int wifiConfig){
   loginConfig.wifiConfig = wifiConfig;
}


static int getTriggerType(){
  return loginConfig.triggerType;
}

static void setTriggerType(int triggerType){
   loginConfig.triggerType = triggerType;
}

static int getInitState(){
  return loginConfig.initState;
}

static void setInitState(int initState){
   loginConfig.initState = initState;
}


bool getLocalTime(struct tm * info, uint32_t ms) {
  uint32_t count = ms / 10;
  time_t now;

  time(&now);
  localtime_r(&now, info);

  if (info->tm_year > (2016 - 1900)) {
    return true;
  }

  while (count--) {
    delay(10);
    time(&now);
    localtime_r(&now, info);
    if (info->tm_year > (2016 - 1900)) {
      return true;
    }
  }
  return false;
}



bool saveDeviceConfig(){
  LittleFS.end();
  if (!LittleFS.begin()) {
    Serial.println("Unable to begin(), aborting");
    return false;
  }
  Serial.println("Save file, may take a while...");
  long start = millis();
  File f = LittleFS.open("/deviceconfig.bin", "w");
  if (!f) {
    Serial.println("Unable to open deviceconfig file for writing");
    return false;
  }

  f.write((char*)&loginConfig.EventType, sizeof(uint16_t));
  f.write((char*)&loginConfig.wifiConfig, sizeof(uint16_t));
  f.write((char*)&loginConfig.triggerType, sizeof(uint16_t));
  f.write((char*)&loginConfig.initState, sizeof(uint16_t));

  
  f.close();
  long stop = millis();
  Serial.printf("==> Time to write  chunks = %ld milliseconds\n", stop - start);
  return true;
}

bool loadDeviceConfig(){
  LittleFS.end();
if (!LittleFS.begin()) {
    Serial.println("Unable to begin(), aborting");
    return false;
  }
  Serial.println("Reading file, may take a while...");
  long start = millis();
  File f = LittleFS.open("/deviceconfig.bin", "r");
  if (!f) {
    Serial.println("Unable to open file for reading, aborting");
    return false;
  }

  f.read((uint8_t*)&loginConfig.EventType, sizeof(uint16_t));
  f.read((uint8_t*)&loginConfig.wifiConfig, sizeof(uint16_t));
  f.read((uint8_t*)&loginConfig.triggerType, sizeof(uint16_t));
  f.read((uint8_t*)&loginConfig.initState, sizeof(uint16_t));

  f.close();
  long stop = millis();
  Serial.printf("==> Time to write  chunks = %ld milliseconds\n", stop - start);
  return true;
}
