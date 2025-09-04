
#ifndef __COMMAND_PROCESSOR_H__
#define __COMMAND_PROCESSOR_H__

#include <Arduino.h>

/**
 * @brief 处理从LoRa接收到的纯命令字符串
 * @param command 经过剥离DeviceID和trim后的命令
 */
void processCommand(const String &command, const String &args);

#endif // __COMMAND_PROCESSOR_H__