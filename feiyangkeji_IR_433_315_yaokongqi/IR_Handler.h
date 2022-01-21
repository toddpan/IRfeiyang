#pragma once
#include "defines.h"
#include <map>
#include <vector>
#include <string>
#include <TimeLib.h>
#include <IRsend.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>
#include <LittleFS.h>

//红外指令数据格式
struct IrRawData{
  uint16_t bits;
  uint16_t nIndex;
  uint16_t nState;
  uint16_t nSize;
  uint16_t* data;
  IrRawData():nIndex(0),nSize(0),
  data(NULL){}
};


typedef std::map<std::string, IrRawData>::iterator RawIRDataMapit;

void resultToIrRawData(const decode_results * const results,std::vector<uint16_t>& sourceCode);

  // The Serial connection baud rate.
  const uint32_t kBaudRate = 115200;
  // As this program is a special purpose capture/decoder, let us use a larger
  // than normal buffer so we can handle Air Conditioner remote codes.
  const uint16_t kCaptureBufferSize = 1024;
  
  #if DECODE_AC
  // Some A/C units have gaps in their protocols of ~40ms. e.g. Kelvinator
  // A value this large may swallow repeats of some protocols
  const uint8_t kTimeout = 50;
  #else   // DECODE_AC
  // Suits most messages, while not swallowing many repeats.
  const uint8_t kTimeout = 15;
  #endif  // DECODE_AC
  // Alternatives:
  const uint16_t kMinUnknownSize = 12;
  
      // Use turn on the save buffer feature for more complete capture coverage.
  IRrecv irrecv(PHYSICAL_IR_RECV, kCaptureBufferSize, kTimeout, true);
  
class IRHwaiHandler{
  
private:
  // ==================== start of TUNEABLE PARAMETERS ====================
  

  // ==================== end of TUNEABLE PARAMETERS ====================

  IrRawData CurrentIrRawData ;
  std::map<std::string, IrRawData>RawIRDataMap;

  bool is_433_315_debug_ = false;
  int  is_433_315_index_ = -1;
  int  debug_state_ = 0;
  long unsigned lastSendms_ = millis();
public:
  static IRHwaiHandler* instance_;
  static IRHwaiHandler& instance(){ 
    if(instance_ == NULL){
      instance_ = new  IRHwaiHandler();
    }
    return *instance_;
  }

long unsigned getLastSendms(){return lastSendms_;}    
void setIsDebug(bool is_433_315_debug){ is_433_315_debug_ = is_433_315_debug; }
bool getIsDebug(){return is_433_315_debug_;}
void setIndex(int is_433_315_index){is_433_315_index_ = is_433_315_index;}
int  getIndex(){return is_433_315_index_;}
void setDebugState(int debug_state){debug_state_ = debug_state;}
int getDebugState(){return debug_state_;}

void resetCurrentData(){
   if(CurrentIrRawData.data != NULL) {
      //先释放原来指令的内存
      delete CurrentIrRawData.data;
      CurrentIrRawData.data = NULL;
   }
};
 
void Init(){

  #if (defined(SUPPORT_XG_MAGIC_BOX))
    pinMode(PHYSICAL_IR_SEND,OUTPUT);
    digitalWrite(PHYSICAL_IR_SEND,LOW);
    pinMode(PHYSICAL_IR_RECV,OUTPUT);
    digitalWrite(PHYSICAL_IR_RECV,LOW);
  #endif
      //load IR configure
    loadHwaiConfig();
    RawIRDataMapit it = RawIRDataMap.begin();
    for(;it != RawIRDataMap.end();it++ ){ 
      IrRawData& irRawData = it->second;
      decode_results results;
      results.bits = irRawData.bits;
      results.rawlen = irRawData.nSize;
      results.rawbuf = irRawData.data;
      print_decode_results(results);
    }

  #if DECODE_HASH
    // Ignore messages with less than minimum on or off pulses.
    irrecv.setUnknownThreshold(kMinUnknownSize);
  #endif  // DECODE_HASH
    irrecv.enableIRIn();  // Start the receiver
    Serial.println("started  IR RCSwitch");
}

void resetHigh()
{
    pinMode(PHYSICAL_IR_SEND,OUTPUT);
    digitalWrite(PHYSICAL_IR_SEND,LOW);
    pinMode(PHYSICAL_IR_RECV,INPUT_PULLUP);
}


void enableIRIn() {
  pinMode(PHYSICAL_IR_RECV,INPUT_PULLUP);
  pinMode(PHYSICAL_IR_SEND,OUTPUT);
  digitalWrite(PHYSICAL_IR_SEND,LOW);
}

void disableIRIn() {
  pinMode(PHYSICAL_IR_RECV,OUTPUT);
  digitalWrite(PHYSICAL_IR_RECV,LOW);
}

////
bool recvIR(){
  
  #if (defined(SUPPORT_XG_MAGIC_BOX))
    resetHigh();
  #endif

  bool ret = false;
  // Check if the IR code has been received.
  decode_results results;  // Somewhere to store the results
  if ( irrecv.decode(&results)) {
    Serial.println("接收指令:"); 
    print_decode_results(results);
    long nowtime =  millis();
    if(results.decode_type != UNKNOWN && getIsDebug() &&
      (millis()-getLastSendms() > 2000)) {
      resetCurrentData();
      std::vector<uint16_t> sourceCode;
      resultToIrRawData(&results,sourceCode);
      //保存新的指令
      CurrentIrRawData.bits = results.value;
      CurrentIrRawData.nIndex = 0;
      CurrentIrRawData.nSize = sourceCode.size();
      CurrentIrRawData.data = new uint16_t[sourceCode.size()];
      for(int i = 0;i< sourceCode.size();i++){
        CurrentIrRawData.data[i] = sourceCode[i];
        Serial.print( CurrentIrRawData.data[i]);
        Serial.print(",");
      }

      Serial.println("已经保存为测试指令");
      ret = true;
    }
      
     
  }

  return ret;
}



void print_decode_results(const decode_results& results){
    // Display a crude timestamp.
    uint32_t now = millis();
    Serial.printf(D_STR_TIMESTAMP " : %06u.%03u\n", now / 1000, now % 1000);
    // Check if we got an IR message that was to big for our capture buffer.
    if (results.overflow)
      Serial.printf(D_WARN_BUFFERFULL "\n", kCaptureBufferSize);
    // Display the library version the message was captured with.
    Serial.println(D_STR_LIBRARY "   : v" _IRREMOTEESP8266_VERSION_ "\n");
    // Display the basic output of what we found.
    String basic =  resultToHumanReadableBasic(&results);
    basic.replace(" ","");
    Serial.print(basic.c_str());
    // Display any extra A/C info if we have it.
    String description = IRAcUtils::resultAcToString(&results);
    description.replace(" ","");
    if (description.length()) Serial.println(D_STR_MESGDESC ": " + description);
    yield();  // Feed the WDT as the text output can take a while to print.
#if LEGACY_TIMING_INFO
    // Output legacy RAW timing info of the result.
    String results = resultToTimingInfo(&results);
    Serial.println(results.c_str());
    yield();  // Feed the WDT (again)
#endif  // LEGACY_TIMING_INFO
    // Output the results as source code
    String sourcecode = resultToSourceCode(&results);
    Serial.println(sourcecode.c_str());
    Serial.println();    // Blank line between entries
    yield();             // Feed the WDT (again)
}

void printIrRawData(IrRawData* pIrRawData){
        for(int i = 0;i< pIrRawData->nSize;i++){
        Serial.print( pIrRawData->data[i]);
        Serial.print(",");
      }
   
  if(pIrRawData){
    decode_results results;
    results.rawlen = pIrRawData->nSize;
    results.value = pIrRawData->bits;
    results.rawbuf = (uint16_t *)pIrRawData->data;
    Serial.println("发送指令:");
    print_decode_results(results);

  }

}
//
void SendIrData( const uint16_t* rawbuf, uint16_t nSize){
  if(rawbuf && nSize >0 ){
    IRsend irsend(PHYSICAL_IR_SEND);  // Set the GPI13 to be used to sending the message.
    irsend.begin();
    Serial.printf("RawData length: %u",nSize);
    Serial.println("#");

   #if (defined(SUPPORT_XG_MAGIC_BOX))
    disableIRIn();
    irsend.sendRaw(rawbuf, nSize, 38);
    enableIRIn();
   #else
    irsend.sendRaw((short unsigned int*)rawbuf, nSize, 38);
   #endif
   
    lastSendms_ =  millis();
  
  }else{
    Serial.println("illegal IrRawData");
  }
}

std::string getKey(uint16_t index,int state){
    char szKey[50];
    sprintf(szKey,"433_%u_%d",index,state);
      return std::string(szKey);
}

//
//绑定当前指令到指令按钮上
void bindRICodeToBlinkerButton(const int& index,const int & state){
  Serial.printf("红外指令和按钮绑定 index:%u,state:%d \n",index ,state);
  if(CurrentIrRawData.nSize > 0 && CurrentIrRawData.data != NULL){
    RawIRDataMapit it = RawIRDataMap.find(getKey(index,state));
    if(it != RawIRDataMap.end()){
        Serial.println("红外指令已经存在，准备替换");
        IrRawData& tempRaw = it->second;
        delete tempRaw.data;
        tempRaw.data = NULL;
        RawIRDataMap.erase(it);
    }
    
    IrRawData tempRaw;
    tempRaw.bits = CurrentIrRawData.bits;
    tempRaw.nState = state;
    tempRaw.nIndex = index;
    tempRaw.nSize = CurrentIrRawData.nSize;
    tempRaw.data = new uint16_t[CurrentIrRawData.nSize];
    memcpy((void*)tempRaw.data,(void*)CurrentIrRawData.data,CurrentIrRawData.nSize * sizeof(uint16_t));
    RawIRDataMap[getKey(index,state)] = tempRaw;
    
    Serial.println("红外指令绑定完成");

  }else{
      Serial.printf("invalid Hwai code, bindCurrentRICode:%u,state:%d \n", index,state);
  }
}

bool saveHwaiConfig(){
  if (!LittleFS.begin()) {
    Serial.println("Unable to begin(), aborting");
    return false;
  }
  Serial.println("Creating file, may take a while...");
  long start = millis();
  File f = LittleFS.open(BIN_SAVE_IRCfg_PATH, "w");
  if (!f) {
    Serial.println("Unable to open file for writing, aborting");
    return false;
  }
  
  int nSize = RawIRDataMap.size();
  int nwrite = f.write((char*)&nSize, sizeof(uint16_t));
  // assert(nwrite != sizeof(uint16_t));
  RawIRDataMapit it = RawIRDataMap.begin();
  for(;it != RawIRDataMap.end();it++){
    IrRawData& rawdata = it->second;
      f.write((char*)&rawdata.bits, sizeof(uint16_t));
      f.write((char*)&rawdata.nIndex, sizeof(uint16_t));
      f.write((uint8_t*)&rawdata.nState, sizeof(uint16_t)); 
      f.write((char*)&rawdata.nSize, sizeof(uint16_t));
      f.write((char*)rawdata.data, rawdata.nSize * sizeof(uint16_t));
  }
  
  f.close();
  long stop = millis();
  Serial.printf("==> Time to write  IrRawData = %ld milliseconds\n", stop - start);
  return true;
}

bool loadHwaiConfig(){
if (!LittleFS.begin()) {
    Serial.println("Unable to begin(), aborting");
    return false;
  }
  Serial.println("Reading file, may take a while...");
  long start = millis();
  File f = LittleFS.open(BIN_SAVE_IRCfg_PATH, "r");
  if (!f) {
    Serial.println("Unable to open file for reading, aborting");
    return false;
  }
  int nSize = 0;
  int nread = f.read((uint8_t*)&nSize, sizeof(uint16_t));
  // assert(nread != sizeof(uint16_t));
   Serial.println(nread);
  for(int i = 0;i< nSize ;i++){
    IrRawData rawdata ;
    f.read((uint8_t*)&rawdata.bits, sizeof(uint16_t));
    f.read((uint8_t*)&rawdata.nIndex, sizeof(uint16_t));
    f.read((uint8_t*)&rawdata.nState, sizeof(uint16_t)); 
    f.read((uint8_t*)&rawdata.nSize, sizeof(uint16_t));
    Serial.printf("bits:%u,nIndex:%u,nSize:%u,nState:%u\n",
        rawdata.bits,rawdata.nIndex,rawdata.nSize,rawdata.nState);

    if(rawdata.nSize > 0){
      rawdata.data = new uint16_t[rawdata.nSize];
      if(rawdata.data){
         f.read((uint8_t*)rawdata.data, rawdata.nSize * sizeof(uint16_t));
      }  
    }

    RawIRDataMap[getKey(rawdata.nIndex,rawdata.nState)] = rawdata;
  }
  f.close();
  long stop = millis();
  Serial.printf("==> Time to write  chunks = %ld milliseconds\n", stop - start);
  return true;
}

// Return a String containing the key values of a decode_results structure
 void resultToIrRawData(const decode_results * const results,std::vector<uint16_t>& sourceCode) {
  // int nLength = getCorrectedRawLength(results);
  // ppIrRawData->nSize = nLength;
  // Dump data
  for (uint16_t i = 1; i < results->rawlen; i++) {
    uint32_t usecs;
    for (usecs = results->rawbuf[i] * kRawTick; usecs > UINT16_MAX; usecs -= UINT16_MAX) {
      sourceCode.push_back(UINT16_MAX);
    }
     sourceCode.push_back(usecs);
  }
 }

 bool SendIRMsg(const int& index, const int & state){

  if(getIsDebug()){
    Serial.println("红外学习模式");
    if(CurrentIrRawData.data != NULL && CurrentIrRawData.nSize >0){
      SendIrData( CurrentIrRawData.data, CurrentIrRawData.nSize);
      Serial.println("--已执行");
    }else{
      Serial.println("--不执行");
    }
    
  }else{
    Serial.println("红外工作模式");
    RawIRDataMapit it = RawIRDataMap.find(getKey(index,state));
    if(it != RawIRDataMap.end()){
        IrRawData& tempRaw = it->second;
        SendIrData( tempRaw.data, tempRaw.nSize);
        Serial.println("红外指令存在，执行成功");
    }else{
      Serial.println("--不执行");
    }
    
  }
  return true;
}

void clearBinFiles() {
//    Dir root = LittleFS.openDir(BIN_SAVE_PATH);
//    while (root.next()) {
//        LittleFS.remove(BIN_SAVE_PATH + root.fileName());
//    }
   LittleFS.remove(BIN_SAVE_IRCfg_PATH);
}

};

IRHwaiHandler* IRHwaiHandler::instance_  = NULL;
