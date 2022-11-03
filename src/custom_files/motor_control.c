
#include "motor_control.h"

#define LOG_MODULE_NAME motor_control
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define PERIOD 20e6

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

int
motor_init(void)
{
    if (!device_is_ready(pwm_led0.dev)) {
        LOG_ERR("pwm device %s is not ready\n", pwm_led0.dev->name);

        return -EBUSY;
    }

    return 0;
}

static uint32_t
deg_map(
    uint8_t x,
    uint8_t in_min,
    uint8_t in_max,
    uint32_t out_min,
    uint32_t out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int
motor_set_angle(uint8_t angle)
{
    return pwm_set_dt(&pwm_led0, PERIOD, deg_map(angle, 0, 180, 0.4e6, 2.4e6));
}