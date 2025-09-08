// LORA命令与通讯

#include "LORA.h"
#include "Command.h"
#include "Pins.h"
#include "tasks.h"
#include <HardwareSerial.h>
#include <Preferences.h>

LORA lora; // 全局变量定义
String deviceID = "";
String hostID = "HOST";

String LORA::getDeviceID() {
    Preferences prefs;
    prefs.begin("robot", false);

    String currentNvsID = prefs.getString("DEVICE_ID", "");
    String masterID = MASTER_DEVICE_ID;

    if (currentNvsID != masterID) {
        Serial.println("NVS Device ID mismatch or not set. Writing new ID: " +
                       masterID);
        prefs.putString("DEVICE_ID", masterID);

        prefs.end();
        return masterID;
    } else {
        prefs.end();
        return currentNvsID;
    }
}

void LORA::sendHexCommand(const char *hexString, HardwareSerial &serialPort) {
    int len = strlen(hexString) / 2;
    uint8_t sendBuffer[len];
    for (int i = 0; i < len; i++) {
        char byteChars[3] = {hexString[i * 2], hexString[i * 2 + 1], 0};
        sendBuffer[i] = strtol(byteChars, NULL, 16);
    }
    // 一次性写出全部字节
    serialPort.write(sendBuffer, len);

    // 打印调试信息
    Serial.print("发送指令: ");
    for (int i = 0; i < len; i++) {
        Serial.print("0x");
        if (sendBuffer[i] < 0x10)
            Serial.print("0");
        Serial.print(sendBuffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

// 等待AUX引脚高电平，表示模块准备就绪
static void waitAUXReady() {
    while (digitalRead(AUX) == LOW)
        ;      // 等待模块处理完
    delay(20); // 稳定延迟
}

void LORA::initLORA() {
    // 1. 基础硬件和串口初始化
    Serial1.begin(9600, SERIAL_8N1, U1RXD, U1TXD);
    pinMode(MD0, OUTPUT);
    pinMode(MD1, OUTPUT);
    pinMode(AUX, INPUT);

    // 2. 检查手动配置开关
#if FORCE_LORA_CONFIG == true
    // --- 如果开关为 true, 执行完整的配置流程 ---
    Serial.println(
        "FORCE_LORA_CONFIG is true. Performing full LoRa configuration...");

    waitAUXReady();

    digitalWrite(MD0, LOW);
    digitalWrite(MD1, LOW);
    delay(100);

    sendHexCommand(LoraCMD, Serial1);
    delay(500);

    while (Serial1.available()) {
        uint8_t byteIn = Serial1.read();
        Serial.print("模块回复: 0x");
        if (byteIn < 0x10)
            Serial.print("0");
        Serial.println(byteIn, HEX);
    }

    Serial.println("LORA configuration complete.");

#else
    // --- 如果开关为 false, 直接跳过 ---
    Serial.println("FORCE_LORA_CONFIG is false. Skipping LoRa configuration.");
#endif

    // 3. 无论如何，最后都切换到正常工作模式
    waitAUXReady();
    digitalWrite(MD0, HIGH);
    digitalWrite(MD1, LOW);
    delay(100);
    Serial.println("LORA switched to normal working mode.");
}

void LORA::sendData(const String &data) {
    // 确保是正常工作模式 0
    digitalWrite(MD0, HIGH);
    digitalWrite(MD1, LOW);

    waitAUXReady();

    Serial1.print(data); // 直接发送字符串

    waitAUXReady();
    safePrintln("Send: " + data);
}

LORA::LORA() {}

LORA::~LORA() {}
