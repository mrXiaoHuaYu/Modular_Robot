// 这里定义输出PWM、相关运动逻辑
#ifndef __Motion_H
#define __Motion_H

#include "Pins.h"
#include <Arduino.h>
#include <stdint.h> //这里定义uint32_t和uint16_t数据变量

// 类的声明
class Motion {
  public:
    Motion();
    ~Motion();
    //*****************MCPWM*****************
    void init();
    void stop();

    /**
     * @brief [核心] 控制机器人前进
     * @details 内部会自动调用前进频率和前进相位参数来配置MCPWM并启动电机
     */
    void moveForward();

    /**
     * @brief [核心] 控制机器人后退
     * @details 内部会自动调用后退频率和后退相位参数来配置MCPWM并启动电机
     */
    void moveBackward();

    // --- 参数设置函数 (带范围检查) ---

    /**
     * @brief 设置全局电压
     * @param voltage 电压值 (0-80V)
     * @return bool 设置是否成功
     */
    bool setGlobalVoltage(int voltage);

    /**
     * @brief 设置全局PWM占空比
     * @param dutyCycle 占空比值 (0.1 - 99.9 %)
     * @return bool 设置是否成功
     */
    bool setGlobalDutyCycle(float dutyCycle);

    /**
     * @brief 设置前进时的频率
     * @param freq 频率值 (100 - 50000 Hz)
     * @return bool 设置是否成功
     */
    bool setForwardFreq(uint32_t freq);

    /**
     * @brief 设置前进时的相位
     * @param phase 相位值 (0 - 360 度)
     * @return bool 设置是否成功
     */
    bool setForwardPhase(float phase);

    /**
     * @brief 设置后退时的频率
     * @param freq 频率值 (100 - 50000 Hz)
     * @return bool 设置是否成功
     */
    bool setBackwardFreq(uint32_t freq);

    /**
     * @brief 设置后退时的相位
     * @param phase 相位值 (0 - 360 度)
     * @return bool 设置是否成功
     */
    bool setBackwardPhase(float phase);

    //*****************Step motion*****************
    void step_init();
    void step_set_params(float step_time_ms, float still_time_ms);
    void step_start();
    void step_stop();

  private:
    void setupMCPWM();
    void start();

    /**
     * @brief [核心] 将指定的运动参数应用到MCPWM硬件
     * @param freq 要设置的频率
     * @param phase_deg 要设置的相位
     */
    void _applyMovementParams(uint32_t freq, float phase_deg);

    /**
     * @brief 将全局电压值应用到调压PWM引脚
     */
    void _applyVoltage();

    // --- 存储所有运动参数的成员变量 ---
    int global_voltage;
    float global_duty_cycle;
    uint32_t forward_freq;
    float forward_phase_deg;
    uint32_t backward_freq;
    float backward_phase_deg;

    //*****************Step motion*****************
    void _apply_step_pwm();
    uint32_t _step_time_us;  // 微秒
    uint32_t _still_time_us; // 微秒
    bool _is_stepping;
    const int STEP_PWM_RESOLUTION = 10;

    //*****************Step motion*****************

    const int resolution = 10; // 精度2^10=1024 (取值0 ~ 20)
};

extern Motion motion;

#endif
