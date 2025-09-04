#ifndef MY_OTA_H
#define MY_OTA_H

#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

class MyOTA {
  private:
    WebServer server;
    bool wifiEnabled = false;

  public:
    MyOTA(int port = 80); // 默认监听 8080 端口
    void begin(const char *ssid, const char *password);
    void handle(); // 在 loop() 中循环调用
    void end();    // 关闭WIFI，停止OTA服务
};

extern MyOTA ota; // 默认8080

#endif
