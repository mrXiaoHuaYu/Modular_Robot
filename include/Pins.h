#ifndef __PIN_H
#define __PIN_H
// ESP32-PICO-V3

#define MASTER_DEVICE_ID "SR_01" // 全局设备ID，烧写NVS用的

#define FORCE_LORA_CONFIG false // LORA配置开关，true的时候更新LORA命令

#define WIFI_SSID "gaoyu"
#define WIFI_PASS "123456789" // OTA用

// ==========================================================
// [新增] 默认运动参数 (出厂设置)
// 每次设备启动时都会加载这些值
// ==========================================================
#define DEFAULT_DUTY_CYCLE 50.0f // 默认占空比 50%
#define DEFAULT_VOLTAGE 0        // 安全起见，电压总是以0开始
#define DEFAULT_FWD_FREQ 20000   // 默认前进频率 (Hz)
#define DEFAULT_FWD_PHASE 90.0f  // 默认前进相位 (度)
#define DEFAULT_BWD_FREQ 20000   // 默认后退频率 (Hz)
#define DEFAULT_BWD_PHASE 270.0f // 默认后退相位 (度)

// LORA
#define MD0 22  // 00：配置模式
#define MD1 19  // 10：工作模式  11：深度睡眠模式
#define AUX 21  // 状态读取  高电平自检完成
#define U1RXD 9 // serial1 --- LORA
#define U1TXD 10
// 调压引脚
#define CTRL_PWM 13 // 61170 PWM升压引脚 30K 0~80V
// 互补PWM
#define MOTOR_IN1 25 // 互补输出A
#define MOTOR_IN2 26
#define MOTOR_IN3 32 // 互补输出B
#define MOTOR_IN4 33
// 运放开关
#define Amp_en 27 // 低电平关闭所有通道
// 开关引脚
#define PWR_ON_PIN 7  // 电池开关   低电平关机
#define EMG_TRIGGER 8 // 电磁铁开关 高电平释放电磁铁

#define LED 20 // 板载LED  高电平点亮

#endif
