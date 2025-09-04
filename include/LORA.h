#ifndef __LORA_H
#define __LORA_H

// 全局用透明模式，发送内容：【目标设备ID】+【命令字】+【发送设备ID】，符合设备ID的应答；

#include <Arduino.h>
#include <Preferences.h>

extern String deviceID;
extern String hostID;

class LORA {
  private:
    Preferences prefs;
    String deviceID;

  public:
    LORA(); // 构造函数
    ~LORA();
    String getDeviceID(); // 返回设备ID，如果失败返回"unknown"
    void sendHexCommand(const char *hexString,
                        HardwareSerial &serialPort); // 发送LORA指令
    void initLORA();
    void sendData(const String &data); // 给上位机HOST发送数据
};

extern LORA lora; // 还需要在LORA.cpp中定义这个变量,这里只是全局引用声明而已

#endif /* __WIFICONFIG_H */

// 命令解析：
/*信道0-127 2^7     4种功率等级2^2      8种空速2^3
信道0x50 功率等级2，空速等级2 等级越高约大；
1010000 01 001
0a09

80（CMD关键字） 04（参数起始地址，波特率开始）
1E（要配置的寄存器长度，默认30个） （00 01 C2 00波特率 10进制直接转换）
00（校验位，ODD16，EVEN14） 0A 09（信道+功率+空速） 00
04（0001透传/0004主从/0002定点/0021中继）05 03
E8（默认保留，不动）00（主机00/从机01）77 77 77 2E 61 73 68 69 6E 69 6E 67 2E 63
6F 6D（16个密钥） 7C 7C 7C 7C 7C（保留5个）05 40 00 23 00 00 00 3C 3C（保留）
00（唤醒时间 100ms为单位，对于接收方即为休眠时间，两者配置相同或唤醒＞休眠）0A
19（保留） 00 00（保留）00
00（0000不发送唤醒码，0080发送唤醒码）00（本地组号/FF为广播）
10（本地地址/FF为广播）00（目标地址） 10（目标组号）00 00 00 00 50
00（保留，中继模式相关设置）
*/

/*
主从模式下，主机无需配置目标地址，从机需要/透明模式下每个人均需配置目标地址。
配置模式下，串口必须为9600 8N1

主从模式,主从机信道需要相同，透明模式组号地址信道均需相同。
主从模式格式：组号+地址+内容，需用16进制发送。
透明模式任意，所见即所得。
透明模式需要配置目标组号和目标地址，均需一致；


*/
