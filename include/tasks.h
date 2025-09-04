// --- 文件名: include/tasks.h ---

#ifndef __TASKS_H__
#define __TASKS_H__

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h" // For semaphores
#include "freertos/task.h"
#include <Arduino.h>

// --- 全局RTOS句柄声明 (extern) ---
// 使用 extern 关键字表明这些变量的实体在其他地方定义（在tasks.cpp中）
// 这样其他文件如果 #include "tasks.h"，就可以访问这些句柄

// 任务句柄
extern TaskHandle_t xTaskLoRaHandle;
extern TaskHandle_t xTaskOTAHandle;
extern TaskHandle_t xTaskLEDHandle;

// 事件组句柄
extern EventGroupHandle_t xOtaEventGroup;
extern const int OTA_START_BIT;
extern const int OTA_STOP_BIT;

// 互斥锁
extern SemaphoreHandle_t xSerialMutex;

// --- 公共函数声明 ---

/**
 * @brief 初始化并创建所有应用程序任务
 */
void tasks_init();

/**
 * @brief 一个线程安全的打印函数
 * @param msg 要打印的字符串
 */
void safePrintln(String msg);

#endif // __TASKS_H__