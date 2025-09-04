#include "Motion.h"
#include <pwmWrite.h>

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "tasks.h"
#include <soc/mcpwm_periph.h>

// 需要封装一个【前进】、【后退】，phase、-phase，然后一个反转标志。

Motion motion;
Pwm pwm;

Motion::Motion()
    : _step_time_us(500),   // 默认步进500us
      _still_time_us(1000), // 默认静止1000us
      _is_stepping(false)   // 默认不处于步进模式
{}

Motion::~Motion() {}

void Motion::init() {
    pinMode(CTRL_PWM, OUTPUT);
    setupMCPWM(); // 默认无相位差，但是有偏移，约5°
}

void Motion::start() {
    mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    mcpwm_start(MCPWM_UNIT_1, MCPWM_TIMER_1);
}
void Motion::stop() {
    mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    mcpwm_stop(MCPWM_UNIT_1, MCPWM_TIMER_1);
}

void Motion::setupMCPWM() {
    // PWM A
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A,
                    MOTOR_IN1); // MCPWMXA X:0~2 需要与定时器对应
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, MOTOR_IN2);

    // PWM B
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1A, MOTOR_IN3);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1B, MOTOR_IN4);

    // 初始化 PWM 单元
    mcpwm_config_t pwm_config;
    pwm_config.frequency = pwmFreq;
    pwm_config.cmpr_a = pwmDuty; // A 通道初始占空比
    pwm_config.cmpr_b = pwmDuty; // B 通道初始占空比
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0; // 高电平有效

    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0,
               &pwm_config); // 两个MCPWM单元都用的这一个config
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_1, &pwm_config);

    mcpwm_set_duty_type(
        MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A,
        MCPWM_DUTY_MODE_0); // 正常  这个函数里边直接会启动输出pwm
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B,
                        MCPWM_DUTY_MODE_1); // 反向互补
    mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);

    mcpwm_set_duty_type(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A,
                        MCPWM_DUTY_MODE_0);
    mcpwm_set_duty_type(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_B,
                        MCPWM_DUTY_MODE_1);
    mcpwm_stop(MCPWM_UNIT_1, MCPWM_TIMER_1);
}

void Motion::setPhase(float phaseDeg) {
    pwmPhaseDeg = phaseDeg;
    // 设置 PWM B 的相位偏移
    // 相位差转换为 Ticks 这个逻辑是对的。
    float shift_phase = pwmFreq / 500; // 每增加500Hz，相位需要补偿1-2°
    uint32_t offset_ticks =
        (uint32_t)((((pwmPhaseDeg / 360.0f)) * 1000) +
                   shift_phase); // 1°对应2.7~3的offset_ticks
    offset_ticks %= 1000;        // 不能超过1000

    // 设置同步发送端为MCPWM_0，定时器为Timer_0
    mcpwm_set_timer_sync_output(MCPWM_UNIT_0, MCPWM_TIMER_0,
                                MCPWM_SWSYNC_SOURCE_TEZ);
    // 设置同步接收端为MCPWM_1，定时器Timer_1
    mcpwm_sync_config_t sync_conf = {
        MCPWM_SELECT_TIMER0_SYNC, // sync_sig
        offset_ticks,             // timer_val 0~999对应0~99.9%   0~0  999~360°
                                  // 1/1000的分辨率
        MCPWM_TIMER_DIRECTION_UP  // counter_dir
    };

    mcpwm_sync_configure(MCPWM_UNIT_1, MCPWM_TIMER_1, &sync_conf);
    mcpwm_timer_trigger_soft_sync(MCPWM_UNIT_0, MCPWM_TIMER_0); // 清零主计时器
    mcpwm_timer_trigger_soft_sync(MCPWM_UNIT_1, MCPWM_TIMER_1); // 清零副计时器
}

void Motion::setFreq(uint32_t frequency) {
    pwmFreq = frequency;
    mcpwm_set_frequency(MCPWM_UNIT_0, MCPWM_TIMER_0, pwmFreq);
    mcpwm_set_frequency(MCPWM_UNIT_1, MCPWM_TIMER_1, pwmFreq);

    // 重新应用同步偏移
    setPhase(pwmPhaseDeg);
}

void Motion::setDuty(float dutyCycle) {
    pwmDuty = dutyCycle;
    // 设置 PWM A（MCPWM0）
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, pwmDuty);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, pwmDuty);

    // 设置 PWM B（MCPWM1）
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, pwmDuty);
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_B, pwmDuty);
}

void Motion::voltSet(int newVolt) {
    int voltDuty = int(1024 * newVolt / 80);
    if (voltDuty == 0) {
        stop();
        digitalWrite(CTRL_PWM, LOW);
    }
    pwm.write(CTRL_PWM, voltDuty, 10000, resolution, 0);
}

// ************************步进模式************************
/**
 * @brief 初始化步进控制引脚
 */
void Motion::step_init() {
    pinMode(Amp_en, OUTPUT);

    // 初始状态禁用运放
    digitalWrite(Amp_en, LOW);

    pwm.attach(Amp_en);

    // 初始化后先暂停PWM输出，等待 step_start() 命令
    pwm.pause(pwm.attached(Amp_en));
}
/**
 * @brief 设置步进参数
 */
void Motion::step_set_params(
    float step_time_ms,
    float still_time_ms) // *** 更改点：接收 float 类型的毫秒 ***
{
    // 输入参数校验
    if (step_time_ms <= 0 || still_time_ms <= 0) {
        safePrintln("Error: Step times must be greater than zero.");
        return;
    }

    _step_time_us = (uint32_t)(step_time_ms * 1000.0f);
    _still_time_us = (uint32_t)(still_time_ms * 1000.0f);

    // 再次确认转换后不为0
    if (_step_time_us == 0 || _still_time_us == 0) {
        safePrintln("Error: Step times are too small to be represented.");
        return;
    }

    // 如果当前正在步进中，则动态更新PWM参数以立即生效
    if (_is_stepping) {
        _apply_step_pwm();
    }
}

/**
 * @brief 启动步进模式
 */
void Motion::step_start() {
    if (_is_stepping)
        return;
    _is_stepping = true;

    this->start();

    _apply_step_pwm();
    pwm.resume(pwm.attached(Amp_en));

    Serial.println("Stepping mode started.");
}

/**
 * @brief 停止步进模式
 */
void Motion::step_stop() {
    if (!_is_stepping)
        return;
    _is_stepping = false;

    pwm.pause(pwm.attached(Amp_en));

    pwm.detach(Amp_en);
    digitalWrite(Amp_en, LOW);

    this->stop();

    pwm.attach(Amp_en);

    Serial.println("Stepping mode stopped.");
}

/**
 * @brief [私有] 根据步进参数计算并应用PWM设置
 */
void Motion::_apply_step_pwm() {
    uint32_t total_period_us = _step_time_us + _still_time_us;
    if (total_period_us == 0)
        return;

    // 1. 根据微秒计算频率
    //    频率 = 1 / 周期(秒) = 1 / (总微秒 / 1,000,000) = 1,000,000 / 总微秒
    float step_freq_hz = 1000000.0f / total_period_us;

    // 2. 根据微秒计算占空比对应的PWM值
    uint32_t max_duty_value = (1 << STEP_PWM_RESOLUTION) - 1;
    uint32_t step_duty_value =
        (uint32_t)(((float)_step_time_us / total_period_us) * max_duty_value);

    // 3. 将计算出的频率和占空比写入PWM通道
    pwm.write(Amp_en, step_duty_value, (uint32_t)step_freq_hz,
              STEP_PWM_RESOLUTION);

    safePrintln("Applying step params: Freq=" + String(step_freq_hz, 2) +
                " Hz, DutyValue=" + String(step_duty_value) + "/" +
                String(max_duty_value) + " (Step: " + String(_step_time_us) +
                "us, Still: " + String(_still_time_us) + "us)");
}