
#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <logging/log.h>
#include <zephyr.h>
#include <zephyr/drivers/pwm.h>

int motor_init(void);
int motor_set_angle(uint8_t);

#endif // MOTOR_CONTROL_H