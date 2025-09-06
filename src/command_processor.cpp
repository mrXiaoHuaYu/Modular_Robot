
#include "command_processor.h"
#include "tasks.h" // 需要用到事件组句柄

// 包含所有命令具体实现所需要的模块
#include "Command.h"
#include "LED_Status.h"
#include "LORA.h"
#include "Motion.h"
#include <WiFi.h>
#include <cstdlib>

/**
 * @brief 安全地将String转换为long
 * @param s 输入字符串
 * @param result 输出转换后的long值
 * @return bool 转换是否成功
 */
static bool parseStringToInt(const String &s, long &result) {
    char *endptr;
    // 使用 strtol，它比 .toInt() 更健壮
    result = strtol(s.c_str(), &endptr, 10);
    // 如果 endptr 指向字符串末尾的'\0'，说明整个字符串都是有效的数字
    return (*endptr == '\0');
}

/**
 * @brief 安全地将String转换为float
 * @param s 输入字符串
 * @param result 输出转换后的float值
 * @return bool 转换是否成功
 */
static bool parseStringToFloat(const String &s, float &result) {
    char *endptr;
    // 使用 strtof，它比 .toFloat() 更健壮
    result = strtof(s.c_str(), &endptr);
    return (*endptr == '\0');
}

// --- 1. 定义所有命令的具体处理函数 ---

static void handle_Forward(const String &args) {
    safePrintln("Executing FORWARD command with args: " + args);
    ledStatus.setStatus(LED_MOTION_ACTIVE);
    // TODO: 在这里解析args以获取频率、电压等参数
    motion.moveForward();
}

static void handle_Backward(const String &args) {
    safePrintln("Executing BACKWARD command with args: " + args);
    ledStatus.setStatus(LED_MOTION_ACTIVE);
    motion.moveBackward();
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

static void handle_SetParams(const String &args) {
    // args 格式: PARAM_NAME,VALUE
    int separatorIndex = args.indexOf(',');
    if (separatorIndex == -1) {
        safePrintln("Invalid SET_PARAMS format: " + args);
        return;
    }

    String paramName = args.substring(0, separatorIndex);
    String paramValueStr = args.substring(separatorIndex + 1);

    safePrintln("Setting param '" + paramName + "' to '" + paramValueStr + "'");

    bool success = false;
    long intValue;
    float floatValue;

    if (paramName == "VOLTAGE") {
        if (parseStringToInt(paramValueStr, intValue)) {
            success = motion.setGlobalVoltage((int)intValue);
        }
    } else if (paramName == "DUTY") {
        if (parseStringToFloat(paramValueStr, floatValue)) {
            success = motion.setGlobalDutyCycle(floatValue);
        }
    } else if (paramName == "FWD_FREQ") {
        if (parseStringToInt(paramValueStr, intValue)) {
            success = motion.setForwardFreq((uint32_t)intValue);
        }
    } else if (paramName == "FWD_PHASE") {
        if (parseStringToFloat(paramValueStr, floatValue)) {
            success = motion.setForwardPhase(floatValue);
        }
    } else if (paramName == "BWD_FREQ") {
        if (parseStringToInt(paramValueStr, intValue)) {
            success = motion.setBackwardFreq((uint32_t)intValue);
        }
    } else if (paramName == "BWD_PHASE") {
        if (parseStringToFloat(paramValueStr, floatValue)) {
            success = motion.setBackwardPhase(floatValue);
        }
    } else {
        safePrintln("Unknown parameter name: " + paramName);
        success = false;
    }

    if (!success) {
        safePrintln("Failed to set param '" + paramName + "'. Value '" +
                    paramValueStr + "' may be invalid or out of range.");
        return; // 设置失败，不回复ACK
    }

    // 设置成功，回复一个 ACK 消息
    String ackPayload = "SET_PARAMS," + args;
    String response =
        hostID + ":" + deviceID + ":" + ACK + ":" + ackPayload + "\n";
    lora.sendData(response);
}

static void handle_SwapDirection(const String &args) {
    safePrintln("Executing SWAP_DIRECTION command.");
    motion.swapDirection(); // 调用 motion 对象的函数来切换方向

    // 回复一个 ACK 消息，并告知当前的状态
    String currentState = motion.isDirectionReversed() ? "REVERSED" : "NORMAL";
    String ackPayload = "SWAP_DIR," + currentState;
    String response =
        hostID + ":" + deviceID + ":" + ACK + ":" + ackPayload + "\n";
    lora.sendData(response);
}

static void handle_EnableStepMode(const String &args) {
    if (args == "1") {
        motion.enableStepMode(true);
    } else if (args == "0") {
        motion.enableStepMode(false);
    } else {
        safePrintln("Invalid payload for STEP_MODE: " + args);
        return;
    }

    // 回复ACK
    String currentState = motion.isStepModeEnabled() ? "ENABLED" : "DISABLED";
    String ackPayload = "STEP_MODE," + currentState;
    String response =
        hostID + ":" + deviceID + ":" + ACK + ":" + ackPayload + "\n";
    lora.sendData(response);
}

static void handle_SetStepTime(const String &args) {
    float step_ms;
    if (parseStringToFloat(args, step_ms)) {
        if (motion.setStepTime(step_ms)) {
            // 设置成功，回复ACK
            String ackPayload = "SET_STEP_TIME," + args;
            String response =
                hostID + ":" + deviceID + ":" + ACK + ":" + ackPayload + "\n";
            lora.sendData(response);
        } else {
            safePrintln("Failed to set step time, value out of range: " + args);
        }
    } else {
        safePrintln("Failed to parse SET_STEP_TIME payload: " + args);
    }
}

static void handle_SetStillTime(const String &args) {
    float still_ms;
    if (parseStringToFloat(args, still_ms)) {
        if (motion.setStillTime(still_ms)) {
            // 设置成功，回复ACK
            String ackPayload = "SET_STILL_TIME," + args;
            String response =
                hostID + ":" + deviceID + ":" + ACK + ":" + ackPayload + "\n";
            lora.sendData(response);
        } else {
            safePrintln("Failed to set still time, value out of range: " +
                        args);
        }
    } else {
        safePrintln("Failed to parse SET_STILL_TIME payload: " + args);
    }
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
    {OTA_ENABLE, handle_OtaEnable},
    {OTA_DISABLE, handle_OtaDisable},
    {SET_PARAMS, handle_SetParams},
    {SWAP_DIRECTION, handle_SwapDirection},
    {SET_STEP_TIME, handle_SetStepTime},
    {SET_STILL_TIME, handle_SetStillTime}};

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