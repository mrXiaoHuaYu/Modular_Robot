#include "Motion.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "tasks.h"
#include <Preferences.h>
#include <pwmWrite.h>
#include <soc/mcpwm_periph.h>

Motion motion;
Pwm pwm;

void Motion::init() {

    // [第2步] 安全第一：无论加载的电压是多少，强制设为0
    this->global_voltage = 0;

    // [第3步] 继续执行原有的初始化流程
    pinMode(CTRL_PWM, OUTPUT);
    setupMCPWM();
    _applyVoltage(); // 此时会应用安全值 0V
}

Motion::Motion()
    : global_voltage(DEFAULT_VOLTAGE), global_duty_cycle(DEFAULT_DUTY_CYCLE),
      forward_freq(DEFAULT_FWD_FREQ), forward_phase_deg(DEFAULT_FWD_PHASE),
      backward_freq(DEFAULT_BWD_FREQ), backward_phase_deg(DEFAULT_BWD_PHASE),
      _step_time_us(500), _still_time_us(1000), _is_stepping(false) {}
Motion::~Motion() {}

void Motion::stop() {
    mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    mcpwm_stop(MCPWM_UNIT_1, MCPWM_TIMER_1);
}

void Motion::start() {
    mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    mcpwm_start(MCPWM_UNIT_1, MCPWM_TIMER_1);
}

// [核心]
void Motion::moveForward() {
    _applyMovementParams(forward_freq, forward_phase_deg);
    start();
}

// [核心]
void Motion::moveBackward() {
    _applyMovementParams(backward_freq, backward_phase_deg);
    start();
}

// --- 参数设置函数的实现 ---
bool Motion::setGlobalVoltage(int voltage) {
    if (voltage < 0 || voltage > 80) {
        return false; // 超出范围
    }
    this->global_voltage = voltage;
    _applyVoltage();
    return true;
}

bool Motion::setGlobalDutyCycle(float dutyCycle) {
    if (dutyCycle < 0.1f || dutyCycle > 99.9f)
        return false;
    this->global_duty_cycle = dutyCycle;

    return true;
}

bool Motion::setForwardFreq(uint32_t freq) {
    if (freq < 100 || freq > 50000)
        return false;
    this->forward_freq = freq;

    return true;
}

bool Motion::setForwardPhase(float phase) {
    if (phase < 0.0f || phase > 360.0f)
        return false;
    this->forward_phase_deg = phase;

    return true;
}

bool Motion::setBackwardFreq(uint32_t freq) {
    if (freq < 100 || freq > 50000)
        return false;
    this->backward_freq = freq;

    return true;
}

bool Motion::setBackwardPhase(float phase) {
    if (phase < 0.0f || phase > 360.0f)
        return false;
    this->backward_phase_deg = phase;

    return true;
}

void Motion::_applyVoltage() {
    int voltDuty = map(this->global_voltage, 0, 80, 0, 1023);
    if (voltDuty <= 0) {
        voltDuty = 0;
    }
    pwm.write(CTRL_PWM, voltDuty, 30000, this->resolution, 0);
}

void Motion::_applyMovementParams(uint32_t freq, float phase_deg) {
    mcpwm_set_frequency(MCPWM_UNIT_0, MCPWM_TIMER_0, freq);
    mcpwm_set_frequency(MCPWM_UNIT_1, MCPWM_TIMER_1, freq);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A,
                   this->global_duty_cycle);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B,
                   this->global_duty_cycle);
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A,
                   this->global_duty_cycle);
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_B,
                   this->global_duty_cycle);

    const float PHASE_COMPENSATION_FREQ_DIVISOR = 500.0f;
    float shift_phase = (float)freq / PHASE_COMPENSATION_FREQ_DIVISOR;

    uint32_t offset_ticks =
        (uint32_t)(((phase_deg / 360.0f) * 1000.0f) + shift_phase);
    offset_ticks %= 1000;

    mcpwm_set_timer_sync_output(MCPWM_UNIT_0, MCPWM_TIMER_0,
                                MCPWM_SWSYNC_SOURCE_TEZ);
    mcpwm_sync_config_t sync_conf = {.sync_sig = MCPWM_SELECT_TIMER0_SYNC,
                                     .timer_val = offset_ticks,
                                     .count_direction =
                                         MCPWM_TIMER_DIRECTION_UP};
    mcpwm_sync_configure(MCPWM_UNIT_1, MCPWM_TIMER_1, &sync_conf);

    mcpwm_timer_trigger_soft_sync(MCPWM_UNIT_0, MCPWM_TIMER_0);
    mcpwm_timer_trigger_soft_sync(MCPWM_UNIT_1, MCPWM_TIMER_1);
}

void Motion::setupMCPWM() {
    // MCPWMXA X:0~2 需要与定时器对应
    // PWM A
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, MOTOR_IN1);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, MOTOR_IN2);
    // PWM B
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1A, MOTOR_IN3);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1B, MOTOR_IN4);

    // 初始化 PWM 单元
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 20000;
    pwm_config.cmpr_a = 50.0; // A 通道初始占空比
    pwm_config.cmpr_b = 50.0; // B 通道初始占空比

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