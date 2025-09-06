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

    void moveForward();
    void moveBackward();

    // --- 参数设置函数 ---

    bool setGlobalVoltage(int voltage);
    bool setGlobalDutyCycle(float dutyCycle);
    bool setForwardFreq(uint32_t freq);
    bool setForwardPhase(float phase);
    bool setBackwardFreq(uint32_t freq);
    bool setBackwardPhase(float phase);

    void swapDirection();             // 切换运动方向
    bool isDirectionReversed() const; // 获取运动状态

    //*****************Step motion*****************

    void enableStepMode(bool enable); // 切换步进模式
    bool isStepModeEnabled() const;   // 获取当前是否为步进模式
    bool setStepTime(float step_time_ms);
    bool setStillTime(float still_time_ms);

  private:
    void setupMCPWM();
    void start();

    void step_init();

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
    bool _isDirectionReversed; // 运动方向切换标志位

    //*****************Step motion*****************
    void _apply_step_pwm();
    uint32_t _step_time_us;  // 微秒
    uint32_t _still_time_us; // 微秒
    bool _is_step_mode_enabled;

    const int resolution = 10; // 精度2^10=1024 (取值0 ~ 20)
};

extern Motion motion;

#endif
