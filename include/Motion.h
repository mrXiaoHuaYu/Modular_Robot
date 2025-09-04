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
    void start();
    void stop();
    void setFreq(uint32_t frequency);
    void setDuty(float dutyCycle);
    void setPhase(float phaseDeg);
    //*****************MCPWM*****************
    void voltSet(int newVolt);
    //*****************Step motion*****************
    void step_init();
    void step_set_params(float step_time_ms, float still_time_ms);
    void step_start();
    void step_stop();

  private:
    void setupMCPWM();

    uint32_t pwmFreq = 10000;
    float pwmDuty = 50.0;     // 占空比（0.0 - 99.9）
    float pwmPhaseDeg = 90.0; // 相位差（单位：度）0~360

    //*****************Step motion*****************
    void _apply_step_pwm();
    uint32_t _step_time_us;  // 微秒
    uint32_t _still_time_us; // 微秒
    bool _is_stepping;
    const int STEP_PWM_RESOLUTION = 10;

    //*****************Step motion*****************

    const int shiftFreq = 4;
    const int shiftDuty = 4;
    const int shiftPhase = 4;
    const int resolution = 10; // 精度2^10=1024 (取值0 ~ 20)
};

extern Motion motion;

#endif
