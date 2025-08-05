#ifndef __WIFI_CFG_H__
#define __WIFI_CFG_H__

typedef struct {
  int16_t  checksum;
  int16_t  runmode;
  String sn;
  String auth;
  String ssid;
  String pswd;
}Settings;

void attach(int (*pfun)());
bool WIFI_Init();
void WIFI_RestartToCfg();
Settings& getSettings();

#endif  /* __WIFI_CFG_H__ */
