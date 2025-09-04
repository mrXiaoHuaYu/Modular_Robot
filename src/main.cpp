#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <Arduino.h>
#include <HardwareSerial.h>

// 包含所有自定义模块的头文件
#include "Command.h"
#include "LED_Status.h"
#include "LORA.h"
#include "Motion.h"
#include "MyOTA.h"
#include "Pins.h"
#include "tasks.h"

// 协议：RECEIVER_ID:SENDER_ID:COMMAND:PAYLOAD\n
// 例如：SR_01:HOST:REPORT_IP:172.20.10.2  上位机--->下位机SR_01
// 例如：SR_01:SR_02:MOVE                  下位机SR_02--->下位机SR_01

void setup() {
    Serial.begin(9600);
    // task 配置之前都不允许用safePrintln

    deviceID = lora.getDeviceID(); // 获取内部设备ID

    // 初始化硬件和模块
    ledStatus.begin();
    ledStatus.setStatus(LED_STANDBY); // 设置为待机状态
    lora.initLORA();                  // 初始化LORA模块
    motion.init();                    // 初始化运动控制模块
    tasks_init();
    safePrintln("Modules Initialized.");
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }
// printf("Device ID: %s\n", deviceID);
// printf("成功\n");

// for (int freq = 1000; freq<=20000; freq+=1000){
//   motion.update(freq, 50.0, 50.0);
//   delay(5000);
// }
// motion.update(20000, 50.0, 50);
// delay(5000);
// if (flag == false)
// {
//   motion.update(20000, 50, 50);
//   for (int volt = 1; volt <= 20; volt += 1)
//   {
//     motion.voltSet(volt);
//     Serial.print("电压："+ String(volt));
//     delay(1000);
//   }
//   flag = true;
// }else{
//   motion.voltSet(0);
// }