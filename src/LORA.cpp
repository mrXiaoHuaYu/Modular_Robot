// LORA命令与通讯

#include "LORA.h"
#include "Command.h"
#include "Pins.h"
#include "tasks.h"
#include <HardwareSerial.h>

LORA lora; // 全局变量定义
String deviceID = "";
String hostID = "HOST";

String LORA::getDeviceID() {
    Preferences prefs;

    // 先尝试以只读模式打开
    if (prefs.begin("robot", true)) {
        String deviceID = prefs.getString("DEVICE_ID", "");
        prefs.end();

        // 如果成功读取到非空值，直接返回
        if (deviceID != "") {
            return deviceID;
        }
    } else {
        prefs.end();
    }

    return deviceID;
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
    Serial.print("模块自检完成");
}

void LORA::initLORA() {

    Serial1.begin(9600, SERIAL_8N1, U1RXD,
                  U1TXD); // 使用Serial1连接LoRa,配置成9600 8N1
    pinMode(MD0, OUTPUT);
    pinMode(MD1, OUTPUT);
    pinMode(AUX, INPUT);
    waitAUXReady();
    // 配置模式
    digitalWrite(MD0, LOW);
    digitalWrite(MD1, LOW);

    // 建议直接用他们的软件转码命令，手册是错误的
    // 成功码是80 04 1E
    sendHexCommand(LoraCMD, Serial1);
    while (Serial1.available()) {
        uint8_t byteIn = Serial1.read();
        Serial.print("模块回复: 0x");
        if (byteIn < 0x10)
            Serial.print("0");
        Serial.println(byteIn, HEX);
    }
    delay(500);
    Serial.println("LORA设置完毕...");

    // 一般工作模式
    digitalWrite(MD0, HIGH);
    digitalWrite(MD1, LOW);
    Serial.println("LORA工作模式启动完成... ...");
}

void LORA::sendData(const String &data) {
    // 确保是正常工作模式 0
    digitalWrite(MD0, HIGH);
    digitalWrite(MD1, LOW);

    waitAUXReady();

    Serial1.print(data); // 直接发送字符串

    waitAUXReady();
    safePrintln("LoRa Sent: " + data);
}

LORA::LORA() {}

LORA::~LORA() {}
