
#include "command_processor.h"
#include "tasks.h" // 需要用到事件组句柄

// 包含所有命令具体实现所需要的模块
#include "Command.h"
#include "LED_Status.h"
#include "LORA.h"
#include "Motion.h"
#include <WiFi.h>

// --- 1. 定义所有命令的具体处理函数 ---

static void handle_Forward(const String &args) {
    safePrintln("Executing FORWARD command with args: " + args);
    ledStatus.setStatus(LED_MOTION_ACTIVE);
    // TODO: 在这里解析args以获取频率、电压等参数
    motion.start();
}

static void handle_Backward(const String &args) {
    safePrintln("Executing BACKWARD command with args: " + args);
    ledStatus.setStatus(LED_MOTION_ACTIVE);
    motion.start();
}

static void handle_Step_Forward(const String &args) {
    safePrintln("Executing STEP_FORWARD command with args: " + args);
    ledStatus.setStatus(LED_MOTION_ACTIVE);
    motion.step_start();
}

static void handle_Stop(const String &args) {
    safePrintln("Executing STOP command.");
    ledStatus.setStatus(LED_STANDBY);
    motion.stop();
}

static void handle_OtaEnable(const String &args) {
    safePrintln("Executing OTA_ENABLE command.");
    xEventGroupSetBits(xOtaEventGroup, OTA_START_BIT);
}

static void handle_OtaDisable(const String &args) {
    safePrintln("Executing OTA_DISABLE command.");
    xEventGroupSetBits(xOtaEventGroup, OTA_STOP_BIT);
}

// --- 2. 定义命令处理函数的类型别名，方便书写 ---
//  这个函数指针指向一个函数，该函数接收一个String类型的参数且无返回值
typedef void (*CommandHandler)(const String &args);

// --- 3. 创建命令分派表 ---
//  这是一个结构体，用于将命令字符串和处理函数绑定在一起
struct CommandEntry {
    const char *commandName;
    CommandHandler handler;
};

//  这就是我们的“表格”，所有命令都在这里注册
static const CommandEntry commandTable[] = {
    {Forward, handle_Forward},
    {Backward, handle_Backward},
    {STOP, handle_Stop},
    {Step_Forward, handle_Step_Forward},
    {OTA_ENABLE, handle_OtaEnable},
    {OTA_DISABLE, handle_OtaDisable},

};

// --- 4. 实现主分派函数 ---
void processCommand(const String &command, const String &args) {
    // 遍历命令表，查找匹配的命令
    for (const auto &entry : commandTable) {
        if (command == entry.commandName) {
            // 调用对应的处理函数，并传入参数
            entry.handler(args);
            return; // 处理完成，退出函数
        }
    }
    safePrintln("Unknown command for me: " + command);
}