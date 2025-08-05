#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include "wificfg.h"
#include <DNSServer.h>
#include "mylog.h"  

#if (defined(YYXBC_HOMEKIT))
 #include <arduino_homekit_server.h>
#endif

 
#define LED_BUILTIN_KED 2

bool WIFI_Status = true;
bool WIFI_Config = false;
bool beginConfig = false;

int count=0;

static Settings cfg;

//webserver for wifi configure
static ESP8266WebServer esp8266_server(80);

// 在这里填入WiFi名称
const String wifissid = "Esp-阳阳学编程";
//https://www.arduino.cn/thread-93593-1-1.html
IPAddress myIP(192,168,4,1);
DNSServer dnsServer;//创建dnsServer实例
const byte DNS_PORT = 53;//DNS端口号

typedef int (*cb_func)();//函数指针的定义
cb_func cb_ = NULL;

static void handleNotFound();
static void handleRoot();
static void getWifi();
static void connect();
static String MyconnectWIFI(String& wifissid ,String&  passwd) ;
static bool loadConfig(Settings& cfg);
static bool saveConfig(const Settings&);
static String IpAddress2String(const IPAddress& ipAddress);

Settings& getSettings(){ return cfg;}


void attach(int (*pfun)()){
   cb_ = pfun;
}

void initDNS(void){//初始化DNS服务器
  if(dnsServer.start(DNS_PORT, "*", myIP)){//判断将所有地址映射到esp8266的ip上是否成功
    MyLog::logger().println(LOG_DEBUG,"start dnsserver success.");
  }
  else MyLog::logger().println(LOG_DEBUG,"start dnsserver failed.");
}

void initSoftAP(void){//初始化AP模式
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(myIP, myIP, IPAddress(255, 255, 255, 0));
  if(WiFi.softAP(wifissid)){
    MyLog::logger().println(LOG_DEBUG,"ESP8266 SoftAP is right");
  }
}

void initWebServer(void){//初始化WebServer
  //启动webserver ,提供接口给wifi configure用
  esp8266_server.on("/",HTTP_GET, handleRoot); 
  esp8266_server.on("/getwifi", getWifi); 
  esp8266_server.on("/connect", connect);           
  esp8266_server.onNotFound(handleRoot);  
  esp8266_server.begin();                  

  MyLog::logger().println(LOG_DEBUG,"HTTP esp8266_server started");
}

//配网函数
 void Wifi_Config(){
  
  MyLog::logger().println(LOG_INFO,"进入WIFI配网模式,LED灯闪烁，360秒内配网不成功，自动重启");
  WIFI_Config = true;
  
  pinMode(LED_BUILTIN_KED, OUTPUT);

  int n =0;
  
  while (WIFI_Config == true)
  {
    static int lastms = 0;
    if (millis()-lastms > 1000) {
      lastms = millis();
      //等待过程中一秒打印一个.
      Serial.print(".");
      
      int state =  digitalRead(LED_BUILTIN_KED);
      digitalWrite(LED_BUILTIN_KED, !state);
      delay(500);
      digitalWrite(LED_BUILTIN_KED, state);
      
      n++;
      Serial.print("*");
      if(n%20 == 0){
         Serial.print("#");
         Serial.printf("%d",n);
         Serial.print("#");
         
      }
          
    }
    
    static int lastexitms = 0;
    if (beginConfig == false && millis()-lastexitms > 360000) {
       Serial.print("#");
       Serial.printf("%d\n",n);
       Serial.println("正在重启。。。");
       ESP.restart();
       break;
    }
      //Serial.println("processNextRequest");
    esp8266_server.handleClient();// 处理http服务器访问
    dnsServer.processNextRequest();
    
    //调用回调函数
    if(cb_ != NULL){cb_();}
  }

  MyLog::logger().println(LOG_INFO,"10秒后退出WIFI配网模式");

 //配网已经成功，准备安全退出配网服务
  int i =0;
  //等待连接
  while ( i++ < 10) {
    delay(500);
    Serial.print("#");
    dnsServer.processNextRequest();
    esp8266_server.handleClient();// 处理http服务器访问
    //调用回调函数
    if(cb_ != NULL){cb_();}
  }
  
#if (defined(YYXBC_HOMEKIT))
 MyLog::logger().println(LOG_DEBUG,"started homekit_storage_reset()");
 homekit_storage_reset(); // to remove the previous HomeKit pairing storage when you first run this new HomeKit example

#endif

 esp8266_server.stop();  
 MyLog::logger().println(LOG_INFO,"WIFI配网模式成功，请重启esp8266");
 
 ESP.restart();
}

//连接WIFI
String MyconnectWIFI(String& wifissid ,String&  passwd) {
  WiFi.begin(wifissid.c_str(), passwd.c_str());//路由器的WiFi名称和密码
  MyLog::logger().printf(LOG_DEBUG,"\nConnecting to ");
  MyLog::logger().println(LOG_DEBUG,wifissid);
  int i =0;
  //等待连接
  while (WiFi.status() != WL_CONNECTED && i++ < 200) {
    delay(500);
    MyLog::logger().println(LOG_DEBUG,"#");
  }
  
  if (i == 201) {
        MyLog::logger().println(LOG_DEBUG,"Could not connect to ");
        MyLog::logger().println(LOG_DEBUG,wifissid);
        return "";
    
  } else {
        MyLog::logger().println(LOG_DEBUG,"Ready Use ip: ");
        MyLog::logger().println(LOG_DEBUG,WiFi.localIP());
        return IpAddress2String(WiFi.localIP());
  }
}


// author apicquot from https://forum.arduino.cc/index.php?topic=228884.0
 String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}




bool WIFI_Init(){
  bool ret = false;
  WiFi.hostname("Smart-ESP8266");//设置ESP8266设备名
  WiFi.mode(WIFI_STA);//切换为STA模式
  WiFi.setAutoConnect(true);//设置自动连接
  WiFi.begin();//连接上一次连接成功的wifi
  
  MyLog::logger().println(LOG_DEBUG,"");
  MyLog::logger().println(LOG_DEBUG,"Connect to wifi");
  
  if (LittleFS.begin()) {
    MyLog::logger().println(LOG_DEBUG,"success to mount file system"); 
  }else{
    MyLog::logger().println(LOG_DEBUG,"fail to mount file system"); 
  }

  loadConfig(cfg);
  /**
  *快速按三下物理开关进入配网模式逻辑
  */
  if(cfg.runmode == 2 || cfg.auth.length() == 0 ){
     ret = true;
     
    //只执行一次，下次重启后进入正常模式
    Settings tempcfg;
		tempcfg.auth = cfg.auth;
		tempcfg.sn = cfg.sn;
    tempcfg.ssid = cfg.ssid;
    tempcfg.pswd = cfg.pswd;
		tempcfg.runmode = 1;
		saveConfig(tempcfg);
		

		//进入配网模式
		MyLog::logger().println(LOG_DEBUG,"WiFi连接失败,请在浏览器中打开http://192.168.4.1 重新配置wifi上网信息"); 
		initSoftAP();
		initWebServer();
		initDNS();
		Wifi_Config();//串口配置网 
  }
  
  /*
  else{
     MyLog::logger().println(LOG_DEBUG,"\r\n正在连接,20秒后连接不成功会进入配网模式");
    //当设备没有联网的情况下，执行下面的操作
    while(WiFi.status()!=WL_CONNECTED)
    {
        if(cfg.auth.length() > 0 && WIFI_Status)//WIFI_Status为真,尝试使用flash里面的信息去 连接路由器
        {
            delay(1000);                                        
            count++;
            if(count >= 300) {
                WIFI_Status = false;
                MyLog::logger().printf(LOG_DEBUG,".");
                MyLog::logger().printf(LOG_DEBUG,"WiFi连接失败,请在浏览器中打开http://192.168.4.1 重新配置wifi上网信息"); 
            }
        }
        else//使用flash中的信息去连接wifi失败，执行
        {
          Wifi_Config();//串口配置网
          count = 0;
          WIFI_Status = true;
    
        }
        
     }  
  }
  */
   
   //串口打印连接成功的IP地址
   MyLog::logger().println(LOG_DEBUG,"连接成功");  
   MyLog::logger().println(LOG_DEBUG,"IP:");
   MyLog::logger().println(LOG_DEBUG,WiFi.localIP());
   
   return ret;
   
}

bool saveConfig(const Settings& cfg) {
//  StaticJsonDocument<1024*7> doc;
  DynamicJsonDocument doc(1024*1);
  doc["auth"] = cfg.auth;
  doc["sn"] = cfg.sn;
  doc["checksum"]= 0x1234;
  doc["runmode"]= cfg.runmode;
  doc["ssid"] = cfg.ssid;
  doc["pswd"] = cfg.pswd;
  serializeJson(doc, Serial);
  MyLog::logger().printf(LOG_DEBUG,"\r\n");
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    MyLog::logger().println(LOG_DEBUG,"Failed to open config file for writing");
    return false;
  }
  serializeJson(doc, configFile);
  doc.clear();
  return true;
}

void WIFI_RestartToCfg()
{
  MyLog::logger().println(LOG_DEBUG,"正在重启esp8266，启动后进入配网模式...");
  Settings& cfg =  getSettings();
  cfg.runmode = 2;
  saveConfig(cfg);
  ESP.restart();
}
// 修改函数定义
bool loadConfig(Settings& cfg) {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    MyLog::logger().println(LOG_DEBUG,"Failed to open config file");
    return false;
  }
  size_t size = configFile.size();
  if (size > 1024) {
    MyLog::logger().println(LOG_DEBUG,"Config file size is too large");
    return false;
  }
  
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    MyLog::logger().println(LOG_DEBUG,"Failed to parse config file");
    return false;
  }
  
  cfg.sn = doc["sn"].as<String>();
  cfg.auth = doc["auth"].as<String>();
  cfg.runmode = doc["runmode"].as<int>();
  cfg.checksum = doc["checksum"].as<int>(); 
  cfg.ssid = doc["ssid"].as<String>();
  cfg.pswd = doc["pswd"].as<String>();
  
  MyLog::logger().printf(LOG_DEBUG,"sn:%s\r\n",cfg.sn.c_str());
  MyLog::logger().printf(LOG_DEBUG,"authkey:%s\r\n", cfg.auth.c_str());
  MyLog::logger().printf(LOG_DEBUG,"runmode:%d\r\n", cfg.runmode);
  MyLog::logger().printf(LOG_DEBUG,"checksum:%d\r\n", cfg.checksum);
  MyLog::logger().printf(LOG_DEBUG,"ssid:%s\r\n", cfg.ssid.c_str());
  MyLog::logger().printf(LOG_DEBUG,"pswd:%s\r\n", cfg.pswd.c_str());

  doc.clear();
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++配网部分+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//*******如果iOS的siri有对设备进行操作就执行下面
 // C++11 multiline string constants are neato...
static const char HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
 <head>
  <meta charset="utf-8">
  <title>阳阳学编程WIFI配网</title>
  <meta charset='UTF-8'>
  <meta name="renderer" content="webkit">
  <meta http-equiv="X-UA-Compatible" content="IE=edge,chrome=1">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, minimum-scale=1.0, maximum-scale=1.0, user-scalable=0">
 </head>
 <style>
	html,body,body *{margin: 0;padding: 0;outline:none;}
	html,body{ background: #1aceff; color: #fff; font-size: 14px;}
	.box{ max-width: 400px; min-width: 300px; margin: 0 auto;}
	.tit{ font-size: 26px; text-align: center; font-weight: bold;margin-top:20px; line-height: 1;}
	.desc{ font-size: 12px; text-align: center; line-height: 2; color: #f0f0f0; margin-bottom: 10px;}
	.sm{ font-size: 14px; color: #fefefe; margin: 0; font-weight: 400;}
	.wifi-b{ background: #fff; border-radius: 10px; box-sizing: border-box; margin: 20px; color: #333; padding: 0;overflow: hidden;margin-bottom: 80px;}
	.wifi-li{font-size: 16px;display: flex; justify-content: space-between; border-top: 1px solid #efefef; line-height: 3;padding: 0 15px;}
	.wifi-li:first-child{ border: 0;}
	.wifi-li .sname{max-width: 80%;overflow: hidden;white-space: nowrap;text-overflow: ellipsis;align-items: center;}
	.wifi-li .right{display: flex; align-items: center;}
	.wifi-li:active,.wifi-li:hover,.wifi-li:focus{ background: #e9faff;color: #2196F3;}
	.ico{ font-style: normal; font-family: Arial, Helvetica, sans-serif;}
	.msg-box{ position: fixed;left:0;top:0;right:0;bottom:0;z-index: 9999; background: rgba(0,0,0,.7); display: flex; align-items: center; justify-content: center;}
	.msg{width: 300px; margin: 0 auto; background: #fff;border-radius:10px; padding: 20px; box-sizing: border-box;color:#333;}
	.ipt-box{display: flex;flex-direction: row;border: 1px solid #2196F3;margin-bottom: 10px;}
	.ipt-box>input{flex:1; border: none; background: none; line-height: 40px;}
	.label{margin:0 10px; line-height: 40px;}
	.ipt-box.s{margin: 0;}
	.msg-tit{ font-size: 16px; color: #000; font-weight: bold; text-align: center; line-height: 20px; height: 40px; position: relative;}
	#submit{background: #2196F3; color: #fff;padding: 0; font-size: 16px;}
	.alert-content{text-align: center;line-height: 1.5;margin-bottom: 20px;}
	#alertBox .s1{border:none; margin: 0;}
	#alertBox .okbtn{color:#419fff;font-weight:100;font-size:16px;}
	#alertBox .msg{padding-bottom:10px;}
  #submit.disabled{background: #eee;color: #c0b9b9; cursor:no-drop; border-color:#eee;}
	#close{position: absolute;right: -14px;top: -14px;font-size: 30px;display: block;line-height: 25px;width: 30px;height: 30px; cursor: pointer;}
	#footer{ position: fixed; left: 0;bottom:0;right:0;height: 60px; background:rgba(0,0,0,.3); text-align: center;}
	.scan{font-size: 14px;color: #1aceff;background: #fff;width: 6em;border-radius: 4px;margin: 13px auto 0;line-height: 30px;height: 30px;cursor: pointer;}
	.scan.s-disabled{background: #eee;color: #c0b9b9; cursor:no-drop;}
	.lock{ background: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAB5ElEQVRYR+3XO2gVQRQG4C+ixEYsUlj7BsEiCsHCzkJ8gqSxsDSN6bTwgWBEYlDQSrHQUrGzUaOFltoIUUIg+EKwsLBMCpFYhCOjJGv23tnNXi6KB5Y77D3zn3/O/jPnTI8uW0+X46tDYAUOYQi78A1v8Bw38KPKoqoSGMB9bCgJMoEj+JxLogqBHXiJXtzFCD5iGw7gNPrwBf34mkMil0D4TaVgEejqEuDr8CQFj9/9TRLYg2d4jchEmW3ENFYlsjFuabkZuIaTOIXrbTBDI0dxAreaIvAgieswHrYBPY9LuJh00kgGnmIv9iHGrewMxnAFMa5FYE0Ktjl9z2OI73sPH9pg7kZo5kXSzRzeJ4HOFucupYFNeISt7dhX/P8tDhYXUCSwEnGYbK8Inus+iZ0LT8sigdi7j3PRavrFoTX+a26RwFlcrgmcO+1cEulP/yKBOF4v5CLV9Fu0Pf96AluwFq8qZKOxDHzH6vRET5Br/wn8OxmI3i/qftSNmVwBFKvkcrdhFKcQ4mC3CFSI+9u1MQ3UCR5zWhI4jtt1kTPnxX3iTlkxWo93iLLcCQvhxun5qYxAvI9qNdqJ6Ih+cRF2WVc8jHiiJVtuNqIliwtMXNtuFheW25Z3KCF/9gMdC1QGPA9czWEhdOxqMgAAAABJRU5ErkJggg==') no-repeat center center /cover; width: 16px;height: 16px;display: inline-block; margin-right: 5px;}
	#loading{position:fixed;z-index:99999;width:30px;height:30px;background:#9beccd;border-radius:50px;animation: loading 1.5s infinite linear;left: 50%;top: 50%;margin-left: -15px;margin-top: -15px;}
	#loading:after{
	position:absolute;width:50px;height:50px;border-top:10px solid #ffee07;border-bottom:10px solid #ffee07;border-left:10px solid transparent;border-right:10px solid transparent;border-radius:50px;content:'';top:-20px;left:-20px;animation: loading_after 1.5s infinite linear;}
	@keyframes loading { 0% {transform: rotate(0deg);} 50% {transform: rotate(180deg);background:#ffee07;} 100% {transform: rotate(360deg);}}
	@keyframes loading_after { 0% {border-top:10px solid #ffee07;border-bottom:10px solid #ffee07;} 50% {border-top:10px solid #9beccd;border-bottom:10px solid #9beccd;} 100% {border-top:10px solid #ffee07;border-bottom:10px solid #ffee07;}}
 </style>
 <body> 
  <div class="box">
    <div class="tit">阳阳学编程WIFI配网</div>
    <div class="desc">--->抖音号:yyxbc2010<----</div>
    <div class="tit sm">请选择一个WIFI</div>
    <div class="wifi-b" id="wifiList"><!--WIFI列表--></div>
  </div>
  <div id="footer">
    <div class="scan" onclick="scanWifi()">扫描WIFI</div>
  </div>
  <div id="loading" style="display:none"></div>
  <!--弹窗-->
  <div class="msg-box" id="msgBox" style="display:none">
    <div class="msg">
      <div class="msg-tit">请填写以下信息<span id="close" onclick="msgBox('hide')">×</span></div>
      <div class="ipt-box" id="wifiLock">
        <span class="label">密码</span>
        <input type="text" id="passwd" value="" placeholder="请输入WIFI密码" />
      </div>
      <div class="ipt-box">
        <span class="label">密钥</span>
        <input type="text" id="authkey" value="" placeholder="请输入设备密钥" />
      </div>
      <div class="ipt-box s">
        <input type="button" id="submit" value="确&emsp;定" onclick="submit()" />
      </div>
    </div>
  </div>
  <div class="msg-box" id="alertBox" style="display:none;z-index:999999;">
    <div class="msg">
      <div class="msg-tit">系统提示</div>
      <div class="alert-content" id="abContent">我是提示消息</div>
      <div class="ipt-box s1">
        <input type="button" class="okbtn" id="okBtn" value="确定" onclick="" />
      </div>
    </div>
  </div>
 </body>
 <script type="text/javascript">
  var wifissid = "";
  var wifiLock = true;  //WIFI是否有密码
  function loading(type,timeout,step){
    if(type=='hide'){
      document.getElementById("loading").style = "display:none";
      if(!step){
        document.getElementsByClassName("scan")[0].innerHTML = "扫描WIFI";
        document.getElementsByClassName("scan")[0].className = "scan";
        document.getElementsByClassName("sm")[0].innerHTML = "请选择一个WIFI";
      }
      document.getElementById("submit").className = "";  
      document.getElementsByClassName("s")[0].style = "";
    }else if(type=='show'){
      document.getElementById("loading").style = "";
      if(!step){
        document.getElementsByClassName("scan")[0].innerHTML = "正在扫描...";
        document.getElementsByClassName("scan")[0].className = "scan s-disabled";
        document.getElementsByClassName("sm")[0].innerHTML = "正在扫描WIFI...";
      }else{
        document.getElementById("submit").className = "disabled";
        document.getElementsByClassName("s")[0].style = "border-color:#eee";
      }
      if(timeout>0){
        setTimeout(function(){
          loading('hide');
        },timeout)
      }
    }
  }

  function selectSSID(ssid,state){
    wifissid = ssid;
    wifiLock = state;
    msgBox('show');
  }
  function alertBox(msg,funcName,hide){
	document.getElementById("abContent").innerHTML = msg;
	document.getElementById("okBtn").setAttribute("onclick",funcName?funcName:'hideAlertBox()');
	if(hide){
      document.getElementById("alertBox").style = "display:none;z-index:999999;";
    }else{
      document.getElementById("alertBox").style = "z-index:999999;";
    }
  }
  function hideAlertBox(){
	document.getElementById("alertBox").style = "display:none;z-index:999999;";
  }
  function msgBox(type){
    if(wifiLock){
      document.getElementById("wifiLock").style = "display:block";
    }else{
      document.getElementById("wifiLock").style = "display:none";
    }
    if(type=='hide'){
      document.getElementById("msgBox").style = "display:none";
      document.getElementById("passwd").value = "";
      document.getElementById("authkey").value = "";
    }else{
      document.getElementById("msgBox").style = "";
      document.getElementById("submit").className = "";  
      document.getElementsByClassName("s")[0].style = "";
    }
  }
  function scanWifi(){
    //let data = '{"code":200,"message":"ok","wifi":[{"ssid":"1111111111111111","channel":"channel1","entype":"1"},{"ssid":"111","channel":"channel1","entype":"0"}]}';
    if(document.getElementsByClassName("s-disabled")[0]){
      //alertBox('正在扫描中请不要一直点');
      return
    }
    loading('show',30000) //加载动画30秒超时
    get('/getwifi','',function(data){
      loading('hide');
      let obj = JSON.parse(data);
      if(obj.code!==200){
        document.getElementById("wifiList").innerHTML = "";
        document.getElementsByClassName("sm")[0].innerHTML = (obj.message || "获取WIFI列表错误");
        return
      }

      document.getElementsByClassName("sm")[0].innerHTML = "请选择一个WIFI";

      let arr = obj.wifi;
      let html = '';
      for(let i in arr){
        let wifi = arr[i];
        let lock = wifi.entype=='1' ? '<i class="lock"></i>' : '';  //是否带锁
        let type = wifi.entype=='1' ? true : false; 
        html += '<div class="wifi-li" onclick="selectSSID(\''+wifi.ssid+'\','+type+')">'+
              '<span class="sname">'+wifi.ssid+'</span><span class="right">'+lock+'<i class="ico">&raquo;</i></span>'+
            '</div>';
      }
      document.getElementById("wifiList").innerHTML = html;
    });
  }
  
  function get(url, data, callback){
     var xhr = new XMLHttpRequest();
     if(data){
       url=url+'?'+data;
     }
     xhr.timeout = 30000; // 超时时间，单位是毫秒
     xhr.ontimeout=function(){
      loading('hide');
      alertBox('请求超时');
	  return;
     }
     xhr.open('get',url);
     //注册回调函数
     xhr.onreadystatechange = function(){
      if(xhr.readyState==4&&xhr.status==200){
        //调用传递的回调函数
        callback(xhr.responseText);
      }
     }
     xhr.send(null);
  }
  
  function post(url, data, callback){
     var xhr = new XMLHttpRequest();
     xhr.timeout = 30000; // 超时时间，单位是毫秒
     xhr.ontimeout=function(){
      msgBox('hide');
      loading('hide');
      alertBox('请求超时');
	  return;
     }
     xhr.open('post',url);
     //设置请求头(post有数据发送才需要设置请求头)
     //判断是否有数据发送
     if(data){
       xhr.setRequestHeader('content-type','application/x-www-form-urlencoded');
     }
     xhr.onreadystatechange = function(){
       if(xhr.readyState==4&&xhr.status==200){
         callback(xhr.responseText);
       }else if(xhr.status!=200){
        loading('hide');
        callback('error');
       }
     }
     xhr.send(data);
  }
  
  function submit(){
    if(document.getElementById("submit").className=='disabled'){
      //alertBox('请休息一会不要一直点');
      return
    }
    var passwd = document.getElementById("passwd").value;
    var authkey = document.getElementById("authkey").value;
    if(!wifissid){
      msgBox('hide');
      alertBox('请从WIFI列表选择一个WIFI');
      return;
    }
    if(!authkey){
      alertBox('请输入设备密钥');
      document.getElementById("authkey").focus();
      return;
    }
    //点确定提交
    //console.log("SSID->",wifissid,"WIWI密码->",passwd,"点灯密钥",authkey);
    let params = 'ssid='+wifissid+'&authkey='+authkey+'&passwd='+passwd;
    loading('show',0,2);
    post('/connect',params,function(data){
      loading('hide');
      if(data=='error'){
        msgBox('hide'); //隐藏弹窗
        alertBox("配网失败请重试");
        return
      }
      let obj = JSON.parse(data);
      if(obj.code==200){
        msgBox('hide'); //配网成功
	    document.getElementById("wifiList").innerHTML = "";
	    document.getElementById("footer").style = "display:none;";
	    document.getElementsByClassName("sm")[0].innerHTML = (obj.msg);
        alertBox(obj.msg);
      }else{
        msgBox('hide'); //隐藏弹窗
        alertBox("配网失败请重试");
      }
    });
    
  }
  scanWifi();
 </script>
</html>)KEWL";
                                                          
void handleRoot() {   //处理网站根目录“/”的访问请求 
   beginConfig = true;
   esp8266_server.send(200, "text/html; charset=utf-8", HTML);
} 

////接口1，获取wifi ssid list
//路径：/getwifi
//返回：{"code":200,"message":"ok","wifi":[{"ssid":"111","channel","channel1","entype":"entype1"},{"ssid":"111","channel","channel1","entype":"entype1"}]}
//
////接口2，参数 ssid,authkey,passwd
//路径：/connect
//返回：{"code":200,"message":"配网成功","ip":"192.168.10.2"}
//
void getWifi(){
  String data ="{\"code\":200,\"message\":\"ok\",\"wifi\":[";
  int n = WiFi.scanNetworks();//同步扫描,同步扫描，等待返回结果---不需要填任何参数
  MyLog::logger().printf(LOG_DEBUG,"%d个网络找到\n", n);
  
  for (int i = 0; i < n; i++){
    MyLog::logger().printf(LOG_DEBUG,"%d: %s, Ch:%d (%ddBm) %s\n", i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
    
    char szssid[256];
    sprintf(szssid,"{\"ssid\":\"%s\",\"channel\":\"%d\",\"rssi\":\"%ddBm\",\"entype\":\"%s\"}",
      WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "0" : "1");
    data += szssid;
    if(i < n-1){
      data += ",";
    }
  }
  data += "]}";
  MyLog::logger().println(LOG_DEBUG,data);
  esp8266_server.send(200, "text/html; charset=utf-8", data); 
}

void connect(){
  if (esp8266_server.method() != HTTP_POST) {
    esp8266_server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String wifissid,authkey,passwd;
    for (uint8_t i = 0; i < esp8266_server.args(); i++) {
       String name = esp8266_server.argName(i);
       if(name == "ssid") {
           wifissid = esp8266_server.arg(i);
       }
       else if (name == "authkey"){
           authkey = esp8266_server.arg(i);
       } 
       else if (name == "passwd"){
           passwd = esp8266_server.arg(i);
       }     
    }
    
    int errcode = 0;
    String msg = "配网失败";
    String ip ;
    if(wifissid.length() >0 && authkey.length() >0 && passwd.length() >0) {
       char szSn[50];
       sprintf(szSn,"%lu",millis());
       cfg.auth = authkey;
       cfg.runmode = 1;
       cfg.sn = szSn;
       cfg.ssid = wifissid;
       cfg.pswd = passwd;
       saveConfig(cfg);
       ip =  MyconnectWIFI(wifissid ,passwd) ;

    }
    
		if(ip.length() >0 ){
			errcode = 200;
			msg = "配网成功";
		}else{
		  msg = "配网失败";
		}

    char message[256];
    sprintf(message,"{\"code\":%d,\"msg\":\"%s\",\"ip\":\"%s\"}",errcode,msg.c_str(),ip.c_str());
    MyLog::logger().printf(LOG_DEBUG,message );   
    
    esp8266_server.send(200, "text/html; charset=utf-8", message);
    
    if(errcode == 200){
        WIFI_Config = false;
    }
  }
}

// 设置处理404情况的函数'handleNotFound'
 void handleNotFound(){   
//  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += esp8266_server.uri();
  message += "\nMethod: ";
  message += (esp8266_server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += esp8266_server.args();
  message += "\n";
  for (uint8_t i = 0; i < esp8266_server.args(); i++) {
    message += " " + esp8266_server.argName(i) + ": " + esp8266_server.arg(i) + "\n";
  }
  esp8266_server.send(404, "text/plain", message);

}
