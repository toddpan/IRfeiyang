
#ifndef _DEFINES_H
#define _DEFINES_H

typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned char uint8_t;

#define FIRMWARE_VERSION "v1.0" // version name
#define VERSION_CODE 10          // version code

const String version = "v3.0.1009";

/* log settings */
#define BAUD_RATE 115200
#ifndef LOG_DEBUG
#define LOG_DEBUG false
#endif
#ifndef LOG_INFO
#define LOG_INFO true
#endif
#ifndef LOG_ERROR
#define LOG_ERROR true
#endif

/* ----------------- user settings ----------------- */
/* mqtt settings */
#define MQTT_CHECK_INTERVALS 15      // seconds
#define MQTT_CONNECT_WAIT_TIME 20000 // MQTT 连接等待时间

/* receive disable */
#define DISABLE_SIGNAL_INTERVALS 600 // seconds

#define SAVE_DATA_INTERVALS 300 // seconds

#define HTTP_UPDATE

//适配艾韵智能遥控器
//#define SUPPORT_XG_MAGIC_BOX

#define YYXBC_WEBCONFIG

#define YYXBC_WITH_433
#define YYXBC_WITH_315
#define YYXBC_WITH_IR

/* ----------------- default pin setting --------------- */
//支持433自锁开关功能，需要用蜂鸟无线公司出的远-R1 433模块，
//将R1的VCC和GND分别接在esp-01s的VCC和GND上，两个DAT引脚的随意一个接在esp-01s的RX上YYXBC_BUTTON_RESET

// 艾韵红外盒子适配固件io配置

//315：发送 IO15(D8)  接收：IO4 (D2) 
//433: 发送 IO2（D4） 接收：IO12 (D6)
//红外: 发送 IO13 (D7) 接收：IO14(D5)
//LED: wifi连接上 io16(D0)高电平，断开低电平
//RESET: 长按5秒进入配网模式 IO5(D1)
//D4,D7上并接一个LED灯，发射时会闪烁

// 艾韵红外盒子适配固件io配置 2021/-8/29

//315：发送 IO2(D4)  接收：IO4 (D2) 
//433: 发送 IO2（D4） 接收：IO12 (D6)
//红外: 发送 IO13 (D7) 接收：IO14(D5)
//LED: wifi连接上 io16(D0)高电平，断开低电平
//RESET: 长按5秒进入配网模式 IO5(D1)
//D4,D7上并接一个LED灯，发射时会闪烁


//面包板NodeMcu 12f io配置
//315：发送 IO15(D8)  接收：IO4 (D2) 
//433: 发送 IO2（D4） 接收：IO12 (D6)
//红外: 发送 IO13 (D7) 接收：IO14(D5)
//LED: wifi连接上 io2(D2）高电平，断开低电平
//RESET: 长按5秒进入配网模式 IO5(D1)
//D4,D7，D8上并接一个LED灯，发射时会闪烁


const int YYXBC_BUTTON_RESET = 5;

#if (defined(SUPPORT_XG_MAGIC_BOX))
#define PHYSICAL_IR_SEND 13
#define PHYSICAL_IR_RECV 14
#define BIN_SAVE_IRCfg_PATH "/irconfig.bin"
#define PHYSICAL_443_SEND 2
#define PHYSICAL_443_RECV 12
#define BIN_SAVE_433Cfg_PATH "/433config.bin"
#define PHYSICAL_315_SEND 15
#define PHYSICAL_315_RECV 4
#define BIN_SAVE_315Cfg_PATH "/315config.bin"
const int YYXBC_LED_LIGHT  = 16;
#else
const int YYXBC_LED_LIGHT  = 2;
#define PHYSICAL_IR_SEND 13
#define PHYSICAL_IR_RECV 14
#define BIN_SAVE_IRCfg_PATH "/irconfig.bin"
#define PHYSICAL_443_SEND 2
#define PHYSICAL_443_RECV 12
#define BIN_SAVE_433Cfg_PATH "/433config.bin"
#define PHYSICAL_315_SEND 15
#define PHYSICAL_315_RECV 4
#define BIN_SAVE_315Cfg_PATH "/315config.bin"
#endif


#endif  // _DEFINES_H
