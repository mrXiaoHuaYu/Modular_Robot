#ifndef LED_STATUS_H
#define LED_STATUS_H

#include <Arduino.h>

#define LED 20 // 板载LED 高电平点亮

enum LEDStatus {
    LED_OFF,              // 关闭
    LED_WIFI_CONNECTING,  // WiFi连接中 - 快闪
    LED_WIFI_CONNECTED,   // WiFi已连接 - 慢闪
    LED_OTA_UPDATE,       // OTA更新中 - 双闪
    LED_LORA_SENDING,     // LoRa发送中 - 三连闪
    LED_MOTION_ACTIVE,    // 运动中 - 常亮
    LED_ERROR,            // 错误状态 - 极快闪
    LED_STANDBY           // 待机 - 心跳闪烁
};

class LEDStatusManager {
private:
    LEDStatus currentStatus;
    unsigned long lastUpdate;
    bool ledState;
    int flashCount;
    int maxFlashCount;
    unsigned long flashInterval;
    unsigned long pauseInterval;
    bool inPause;
    unsigned long pauseStart;
    
    void setFlashPattern(int count, unsigned long interval, unsigned long pause = 1000);
    
public:
    LEDStatusManager();
    void begin();  //初始化LED引脚
    void setStatus(LEDStatus status); // 设置LED状态
    void update(); // loop中需要持续运行以维持闪烁状态
    LEDStatus getCurrentStatus(); //获取当前状态
    void turnOff(); //关闭
    void turnOn(); //常亮
};

extern LEDStatusManager ledStatus;

#endif