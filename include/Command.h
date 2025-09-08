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

#define EMG_F "EMG_F" // 电磁铁前进侧_两触点
#define EMG_B "EMG_B" // 电磁铁后退侧_两触点

#define STOP "STOP" // 停止命令

// 【命令字】+【频率】+【电压】
// 相位-封装在内部
#define Forward "M_F"
#define Backward "M_B"

#define VOLTAGE "VOLT"    // 电压命令
#define MOV_PAR_F "M_P_F" // 前进参数编辑：【M_P】+【频率】+【相位】
#define MOV_PAR_B "M_P_B" // 后退参数编辑：【M_P】+【频率】+【相位】

#define OTA_ENABLE "OTA_ENABLE"   // 开启OTA服务
#define OTA_DISABLE "OTA_DISABLE" // 关闭OTA服务

#define GET_IP "GET_IP" // 获得自身连接的IP地址

#define SET_BATCH_PARAMS "SET_BATCH_PARAMS" // 批处理命令
// SET_PARAMS 的 payload 格式: "PARAM_NAME,VALUE"
// 可用的 PARAM_NAME 包括: 所有数值都是int
// - "VOLTAGE"      (0-80)
// - "DUTY"         (1-99)
// - "FWD_FREQ"     (100-50000)
// - "FWD_PHASE"    (0-360)
// - "BWD_FREQ"     (100-50000)
// - "BWD_PHASE"    (0-360)

#define REPORT_ALL_PARAMS "REPORT_ALL_PARAMS" // 设备启动报告自身参数

#define SWAP_DIRECTION "SWAP_DIR" // 切换运动方向 (前进/后退)

// 启用/禁用步进模式; payload: "1" 或 "0"
#define ENABLE_STEP_MODE "STEP_MODE"

// 统一的确认回复命令  原参数返回，加一个ACK
#define ACK "ACK"

#endif /* Command LORA */