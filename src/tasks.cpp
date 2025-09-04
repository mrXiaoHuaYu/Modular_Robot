// --- 文件名: src/tasks.cpp ---

#include "tasks.h"
#include <Arduino.h>
#include <HardwareSerial.h>

// 包含所有任务逻辑需要用到的自定义模块头文件
#include "Command.h"
#include "LED_Status.h"
#include "LORA.h"
#include "Motion.h"
#include "MyOTA.h"
#include "Pins.h"
#include "command_processor.h"

// --- 全局RTOS句柄定义 (实体) ---
TaskHandle_t xTaskLoRaHandle = NULL;
TaskHandle_t xTaskOTAHandle = NULL;
TaskHandle_t xTaskLEDHandle = NULL;

EventGroupHandle_t xOtaEventGroup;
const int OTA_START_BIT = BIT0;
const int OTA_STOP_BIT = BIT1;

SemaphoreHandle_t xSerialMutex;

// --- 任务函数声明 (因为在本文件内使用，也可声明为static) ---
static void Task_LoRa(void *pvParameters);
static void Task_OTA(void *pvParameters);
static void Task_LED(void *pvParameters);

// --- 公共函数定义 ---

void safePrintln(String msg) {
    if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE) {
        Serial.println(msg);
        xSemaphoreGive(xSerialMutex);
    }
}

void tasks_init() {
    // 创建互斥锁
    xSerialMutex = xSemaphoreCreateMutex();

    // 创建事件组
    xOtaEventGroup = xEventGroupCreate();

    // 创建任务
    xTaskCreatePinnedToCore(Task_LED, "LED_Task", 2048, NULL, 1,
                            &xTaskLEDHandle, 1);

    xTaskCreatePinnedToCore(Task_LoRa, "LoRa_Task", 4096, NULL, 3,
                            &xTaskLoRaHandle, 1);

    xTaskCreatePinnedToCore(Task_OTA, "OTA_Task", 8192, NULL, 2,
                            &xTaskOTAHandle, 1);

    safePrintln("Tasks Created by tasks_init().");
}

// --- 任务函数定义 ---

static void Task_LED(void *pvParameters) {
    for (;;) {
        ledStatus.update();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void Task_LoRa(void *pvParameters) {
    String receivedData = "";
    char endMarker = '\n';

    // 检查本机DeviceID是否有效，无效则持续警告且不处理任何指令
    if (deviceID == "" || deviceID == "unknown") {
        while (1) {
            safePrintln(
                "FATAL ERROR: Device ID is not set! Halting LoRa Task.");
            ledStatus.setStatus(LED_ERROR);
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    safePrintln("LoRa Task started. My Device ID is: " + deviceID);

    for (;;) {
        if (Serial1.available() > 0) {
            char inChar = Serial1.read();
            if (inChar != endMarker) {
                receivedData += inChar;
            } else {
                // 收到一个完整的消息 (以 '\n' 结尾)
                safePrintln("LoRa Received Raw: " + receivedData);
                // 1. 寻找三个分隔符的位置
                int firstColon = receivedData.indexOf(':');
                int secondColon = receivedData.indexOf(':', firstColon + 1);
                int thirdColon = receivedData.indexOf(':', secondColon + 1);
                // 2. 检查协议格式是否基本正确（必须至少有3个冒号）
                if (firstColon > 0 && secondColon > firstColon &&
                    thirdColon > secondColon) {

                    // 3. 提取接收者ID
                    String receiver = receivedData.substring(0, firstColon);

                    // 4. 检查是否为本机消息
                    if (receiver == deviceID || receiver == "ALL") {

                        // 是发给我的，继续解析其他部分
                        String sender =
                            receivedData.substring(firstColon + 1, secondColon);
                        String command =
                            receivedData.substring(secondColon + 1, thirdColon);
                        String payload = receivedData.substring(thirdColon + 1);

                        // 清理可能存在的空格
                        command.trim();
                        payload.trim();

                        // 5. 将分离好的命令和参数交给处理器
                        safePrintln("For me! From: " + sender + ", Cmd: '" +
                                    command + "', Payload: '" + payload + "'");
                        processCommand(command, payload);

                    } else {
                        // 不是发给我的消息，直接忽略 (未来可用于设备间转发)
                        safePrintln("Ignoring command for other device: " +
                                    receiver);
                    }
                } else {
                    safePrintln("Invalid protocol format received: " +
                                receivedData);
                }

                receivedData = "";
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void Task_OTA(void *pvParameters) {
    bool isOtaRunning = false;
    for (;;) {
        EventBits_t uxBits =
            xEventGroupWaitBits(xOtaEventGroup, OTA_START_BIT | OTA_STOP_BIT,
                                pdTRUE, pdFALSE, portMAX_DELAY);

        if ((uxBits & OTA_START_BIT) != 0) {
            if (!isOtaRunning) {
                safePrintln("OTA Task: Starting WiFi and OTA service...");
                ledStatus.setStatus(LED_WIFI_CONNECTING);

                ota.begin(WIFI_SSID, WIFI_PASS);

                if (WiFi.status() == WL_CONNECTED) {
                    ledStatus.setStatus(LED_WIFI_CONNECTED);
                    isOtaRunning = true;
                    safePrintln("OTA service is now running.");
                    // 上报IP地址
                    String ipAddress = WiFi.localIP().toString();
                    String response = hostID + ":" + deviceID +
                                      ":REPORT_IP:" + ipAddress + "\n";
                    lora.sendData(response);

                    while (isOtaRunning) {
                        ota.handle();

                        EventBits_t stopBits =
                            xEventGroupGetBits(xOtaEventGroup);
                        if ((stopBits & OTA_STOP_BIT) != 0) {
                            xEventGroupClearBits(xOtaEventGroup, OTA_STOP_BIT);
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(5));
                    }
                } else {
                    ledStatus.setStatus(LED_ERROR);
                    safePrintln("OTA Task: Failed to connect to WiFi.");
                    ota.end();
                }
            } else {
                safePrintln("OTA Task: Service is already running.");
            }
        }
        // 更新完毕时isOtaRunning还没来得及清零，且程序继续向下运行，执行wifi关闭操作
        if (((uxBits & OTA_STOP_BIT) != 0) || isOtaRunning == true) {
            if (isOtaRunning) {
                safePrintln("OTA Task: Stopping service...");
                ota.end();
                isOtaRunning = false;
                ledStatus.setStatus(LED_STANDBY);
                safePrintln("OTA service stopped.");
            }
        }
    }
}