#include "LED_Status.h"

LEDStatusManager ledStatus;

LEDStatusManager::LEDStatusManager() {
    currentStatus = LED_OFF;
    lastUpdate = 0;
    ledState = false;
    flashCount = 0;
    maxFlashCount = 1;
    flashInterval = 500;
    pauseInterval = 1000;
    inPause = false;
    pauseStart = 0;
}

void LEDStatusManager::begin() {
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW); // 初始化为关闭状态
}

void LEDStatusManager::setStatus(LEDStatus status) {
    if (currentStatus != status) {
        currentStatus = status;
        flashCount = 0;
        ledState = false;
        inPause = false;
        lastUpdate = millis();
        
        switch (status) {
            case LED_OFF:
                digitalWrite(LED, LOW);
                break;
                
            case LED_WIFI_CONNECTING:
                // 快闪 - 200ms间隔
                setFlashPattern(1, 200, 200);
                break;
                
            case LED_WIFI_CONNECTED:
                // 慢闪 - 1秒间隔
                setFlashPattern(1, 1000, 1000);
                break;
                
            case LED_OTA_UPDATE:
                // 双闪 - 每次闪烁150ms，间隔100ms，然后暂停1秒
                setFlashPattern(2, 150, 1000);
                break;
                
            case LED_LORA_SENDING:
                // 三连闪 - 每次闪烁100ms，间隔100ms，然后暂停1.5秒
                setFlashPattern(3, 100, 1500);
                break;
                
            case LED_MOTION_ACTIVE:
                // 常亮
                digitalWrite(LED, HIGH);
                break;
                
            case LED_ERROR:
                // 极快闪 - 100ms间隔
                setFlashPattern(1, 100, 100);
                break;
                
            case LED_STANDBY:
                // 心跳闪烁 - 短亮长暗
                setFlashPattern(1, 100, 2000);
                break;
        }
    }
}

void LEDStatusManager::setFlashPattern(int count, unsigned long interval, unsigned long pause) {
    maxFlashCount = count;
    flashInterval = interval;
    pauseInterval = pause;
}

void LEDStatusManager::update() {
    if (currentStatus == LED_OFF || currentStatus == LED_MOTION_ACTIVE) {
        return; // 关闭或常亮状态不需要更新
    }
    
    unsigned long currentTime = millis();
    
    if (inPause) {
        // 在暂停期间
        if (currentTime - pauseStart >= pauseInterval) {
            inPause = false;
            flashCount = 0;
            lastUpdate = currentTime;
        }
        return;
    }
    
    if (currentTime - lastUpdate >= flashInterval) {
        if (flashCount < maxFlashCount * 2) { // *2 因为包含开和关
            ledState = !ledState;
            digitalWrite(LED, ledState ? HIGH : LOW);
            flashCount++;
            lastUpdate = currentTime;
        } else {
            // 完成一组闪烁，进入暂停
            digitalWrite(LED, LOW);
            ledState = false;
            inPause = true;
            pauseStart = currentTime;
        }
    }
}

LEDStatus LEDStatusManager::getCurrentStatus() {
    return currentStatus;
}

void LEDStatusManager::turnOff() {
    setStatus(LED_OFF);
}

void LEDStatusManager::turnOn() {
    digitalWrite(LED, HIGH);
    currentStatus = LED_MOTION_ACTIVE;
}