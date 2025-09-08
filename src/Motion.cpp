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
    this->global_voltage = 0;

    pinMode(CTRL_PWM, OUTPUT);
    pinMode(Amp_en, OUTPUT);
    digitalWrite(Amp_en, LOW);
    pinMode(4, OUTPUT); // !!!!!!!!!!!!!!!!!注意新板子需要把这个删除
    digitalWrite(4, LOW);

    setupMCPWM();
    _step_timer_init();
    _applyVoltage();
}

Motion::Motion()
    : global_voltage(DEFAULT_VOLTAGE), global_duty_cycle(DEFAULT_DUTY_CYCLE),
      forward_freq(DEFAULT_FWD_FREQ), forward_phase_deg(DEFAULT_FWD_PHASE),
      backward_freq(DEFAULT_BWD_FREQ), backward_phase_deg(DEFAULT_BWD_PHASE),
      _isDirectionReversed(false), _step_time_ms(100), _still_time_ms(100),
      _is_step_mode_enabled(false), step_timer_handle(NULL),
      isCurrentlyStepping(false) {}
Motion::~Motion() {
    if (step_timer_handle != NULL) {
        esp_timer_stop(step_timer_handle);
        esp_timer_delete(step_timer_handle);
    }
}

void Motion::stop() {
    // 如果高精度定时器正在运行，则停止它
    if (step_timer_handle != NULL && esp_timer_is_active(step_timer_handle)) {
        esp_timer_stop(step_timer_handle);
    }

    _internal_stop_mcpwm();    // 停止高频PWM
    digitalWrite(Amp_en, LOW); // 关闭运放
    digitalWrite(4, LOW);
    isCurrentlyStepping = false; // 重置状态
    safePrintln("Motion stopped.");
}

void Motion::moveForward() {
    // 1. 应用高频PWM参数
    if (_isDirectionReversed) {
        _applyMovementParams(backward_freq, backward_phase_deg);
    } else {
        _applyMovementParams(forward_freq, forward_phase_deg);
    }

    // 2. 根据模式启动运动
    if (_is_step_mode_enabled) {
        if (step_timer_handle == NULL)
            return;
        safePrintln("Starting Step Motion...");
        digitalWrite(Amp_en, HIGH); // 持续使能运放
        digitalWrite(4, HIGH);
        _internal_start_mcpwm(); // 开始第一个“步进”
        isCurrentlyStepping = true;
        // 使用 esp_timer_start_once 启动高精度定时器
        // 定时器周期直接使用微秒 _step_time_us
        esp_timer_start_once(step_timer_handle, _step_time_us);
    } else {
        safePrintln("Starting Continuous Motion...");
        digitalWrite(Amp_en, HIGH); // 使能运放
        digitalWrite(4, HIGH);
        _internal_start_mcpwm(); // 启动高频PWM
    }
}

void Motion::moveBackward() {
    // 1. 应用高频PWM参数
    if (_isDirectionReversed) {
        _applyMovementParams(forward_freq, forward_phase_deg);
    } else {
        _applyMovementParams(backward_freq, backward_phase_deg);
    }

    // 2. 根据模式启动运动
    if (_is_step_mode_enabled) {
        if (step_timer_handle == NULL)
            return;
        safePrintln("Starting Step Motion...");
        digitalWrite(Amp_en, HIGH); // 持续使能运放
        digitalWrite(4, HIGH);
        _internal_start_mcpwm(); // 开始第一个“步进”
        isCurrentlyStepping = true;
        // 使用 esp_timer_start_once 启动高精度定时器
        esp_timer_start_once(step_timer_handle, _step_time_us);
    } else {
        safePrintln("Starting Continuous Motion...");
        digitalWrite(Amp_en, HIGH); // 使能运放
        digitalWrite(4, HIGH);
        _internal_start_mcpwm(); // 启动高频PWM
    }
}

void Motion::_step_timer_init() {
    const esp_timer_create_args_t timer_args = {
        .callback = &stepTimerCallback,
        .arg = this, // 将 Motion 实例指针作为参数
        .name = "step-timer"};
    esp_err_t err = esp_timer_create(&timer_args, &step_timer_handle);
    if (err != ESP_OK) {
        safePrintln("FATAL: Failed to create esp_timer!");
        step_timer_handle = NULL;
    }
}
// 定时器回调函数
void Motion::stepTimerCallback(void *arg) {
    // 从参数中获取Motion实例指针
    Motion *motion_ptr = (Motion *)arg;
    if (motion_ptr == NULL)
        return;

    if (motion_ptr->isCurrentlyStepping) {
        // 当前是“步进”状态，现在切换到“静止”状态
        motion_ptr->_internal_stop_mcpwm();
        motion_ptr->isCurrentlyStepping = false;
        // 使用 esp_timer_start_once 启动下一次定时
        esp_timer_start_once(motion_ptr->step_timer_handle,
                             motion_ptr->_still_time_us);
    } else {
        // 当前是“静止”状态，现在切换到“步进”状态

        // 重新启动前，必须重新触发软件同步，以重建相位关系
        mcpwm_timer_trigger_soft_sync(MCPWM_UNIT_0, MCPWM_TIMER_0);
        mcpwm_timer_trigger_soft_sync(MCPWM_UNIT_1, MCPWM_TIMER_1);

        motion_ptr->_internal_start_mcpwm();
        motion_ptr->isCurrentlyStepping = true;
        // 使用 esp_timer_start_once 启动下一次定时
        esp_timer_start_once(motion_ptr->step_timer_handle,
                             motion_ptr->_step_time_us);
    }
}

void Motion::_internal_start_mcpwm() {
    mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    mcpwm_start(MCPWM_UNIT_1, MCPWM_TIMER_1);
}

void Motion::_internal_stop_mcpwm() {
    mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    mcpwm_stop(MCPWM_UNIT_1, MCPWM_TIMER_1);
}

// 方向切换
void Motion::swapDirection() { _isDirectionReversed = !_isDirectionReversed; }

// 获取方向状态
bool Motion::isDirectionReversed() const { return _isDirectionReversed; }

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
    mcpwm_set_duty_type(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A,
                        MCPWM_DUTY_MODE_0);
    mcpwm_set_duty_type(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_B,
                        MCPWM_DUTY_MODE_1);
}

void Motion::enableStepMode(bool enable) {
    if (_is_step_mode_enabled == enable)
        return;
    _is_step_mode_enabled = enable;
    safePrintln("Step mode " + String(enable ? "ENABLED" : "DISABLED"));
    // 切换模式时强制停止，确保状态正确
    stop();
}

bool Motion::isStepModeEnabled() const { return _is_step_mode_enabled; }

// ************************步进模式************************
bool Motion::setStepTime(float step_time_ms) {
    if (step_time_ms < 0) { // 等于0是允许的，表示没有步进时间
        safePrintln("Error: Step time must be non-negative.");
        return false;
    }
    _step_time_us = (uint32_t)(step_time_ms * 1000.0f);
    if (step_time_ms > 0 && _step_time_us == 0) {
        // 对于大于0但小于1微秒的输入，强制设为1微秒
        _step_time_us = 1;
        safePrintln("Warning: Step time is less than 1us, setting to 1us.");
    }
    return true;
}
bool Motion::setStillTime(float still_time_ms) {
    if (still_time_ms < 0) {
        safePrintln("Error: Still time must be non-negative.");
        return false;
    }
    _still_time_us = (uint32_t)(still_time_ms * 1000.0f);
    if (still_time_ms > 0 && _still_time_us == 0) {
        _still_time_us = 1;
        safePrintln("Warning: Still time is less than 1us, setting to 1us.");
    }
    return true;
}

String Motion::getAllParamsAsString() {
    String params = "";
    params += "VOLTAGE:" + String(this->global_voltage) + ";";
    params += "DUTY:" + String(this->global_duty_cycle, 2) + ";";
    params += "FWD_FREQ:" + String(this->forward_freq) + ";";
    params += "FWD_PHASE:" + String(this->forward_phase_deg, 2) + ";";
    params += "BWD_FREQ:" + String(this->backward_freq) + ";";
    params += "BWD_PHASE:" + String(this->backward_phase_deg, 2) + ";";
    params += "STEP_TIME_MS:" + String(this->_step_time_ms, 2) + ";";
    params += "STILL_TIME_MS:" + String(this->_still_time_ms, 2) + ";";
    params +=
        "STEP_MODE_ENABLED:" + String(this->_is_step_mode_enabled ? "1" : "0") +
        ";";
    params +=
        "direction_reversed:" + String(this->_isDirectionReversed ? "1" : "0");
    return params;
}