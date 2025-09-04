#ifndef __Command_H
#define __Command_H
// 上位机通过LORA命令字与下位机通讯

// 指令格式：【DeviceID】+【命令字】

#define GroupNum 0x00 // 组号
#define Address 0x05  // 地址
#define Channel 0x20  // 信道

// 空速62.5k，功率20dBm,透明传输模式，波特率9600，组号地址信道如上，目标组号目标地址同上
#define LoraCMD                                                                \
    "80041E0000258000041F00010503E8007777772E617368696E696E672E636F6D7C7C7C7C" \
    "7C054000230000003C3C000A19000000050005000000002000"

// 【命令字】+【频率】+【电压】
// 相位-封装在内部
#define Forward "M_F"
#define Backward "M_B"

// 【命令字】+【占空比】+【步进频率】
// 频率封装在内部
#define Step_Forward "M_SF"  // 步进前进
#define Step_Backward "M_SB" // 步进后退

#define EMG_F "EMG_F" // 电磁铁前进侧_两触点
#define EMG_B "EMG_B" // 电磁铁后退侧_两触点

#define STOP "STOP" // 停止命令

#define VOLTAGE "VOLT"    // 电压命令
#define MOV_PAR_F "M_P_F" // 前进参数编辑：【M_P】+【频率】+【相位】
#define MOV_PAR_B "M_P_B" // 后退参数编辑：【M_P】+【频率】+【相位】

#define OTA_ENABLE "OTA_ENABLE"   // 开启OTA服务
#define OTA_DISABLE "OTA_DISABLE" // 关闭OTA服务

#define GET_IP "GET_IP" // 获得自身连接的IP地址

// 编写命令 对应编号

#endif /* Command LORA */