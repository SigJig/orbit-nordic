/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"
#include "custom_files/motor_control.h"
#include "custom_files/mpu_sensor.h"
#include "custom_files/remote.h"
#include <dk_buttons_and_leds.h>
#include <logging/log.h>
#include <zephyr/zephyr.h>

#define LOG_MODULE_NAME app
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define RUN_STATUS_LED DK_LED1
#define CONN_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

static accel_values_t accel_values;

static struct bt_conn* current_conn;

static void
blt_notif_changed(enum blt_button_notif_enabled status)
{
    LOG_INF(
        "notifications %s",
        status == BLT_BUTTON_NOTIF_ENABLED ? "enabled" : "disabled");
}

static void
blt_data_recieved(struct bt_conn* conn, const uint8_t* const data, uint16_t len)
{
    uint8_t temp_str[len + 1];
    memcpy(temp_str, data, len);
    temp_str[len] = 0x00;

    LOG_INF("received data on conn %p. len: %d\n", (void*)conn, len);
    LOG_INF("data: %s\n", temp_str);
}

static struct blt_remote_service_cb remote_callbacks = {
    .notif_changed = blt_notif_changed, .data_recieved = blt_data_recieved};

static void
blt_on_conn(struct bt_conn* conn, uint8_t err)
{
    if (err) {
        LOG_ERR("connection failed with error %i\n", err);
    } else {
        LOG_INF("connection established\n");
    }

    current_conn = bt_conn_ref(conn);
    dk_set_led_on(CONN_STATUS_LED);
}

static void
blt_on_disconn(struct bt_conn* conn, uint8_t reason)
{
    LOG_INF("disconnected (reason %i)\n", reason);

    dk_set_led_off(CONN_STATUS_LED);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
}

static struct bt_conn_cb blt_callbacks = {
    .connected = blt_on_conn,
    .disconnected = blt_on_disconn,
};

static void
button_handler(uint32_t state, uint32_t changed)
{
    // and with 0x4 to only use 4 bits
    uint8_t pressed = state & changed & 0x1f;

    if (!pressed) {
        return;
    }

    if (pressed & 0x8) {
        uint8_t val = !blt_get_button_value();
        LOG_INF("setting bt button value %i\n", val);
        blt_set_button_value(val);

        int err = blt_send_button_notif(current_conn, val);

        if (err) {
            LOG_ERR("unable to send button notif (%i)\n", err);
        }
    }

    for (uint8_t i = 0; i < 4; i++) {
        if ((pressed) & (0x1 << i)) {
            LOG_INF("button %i pressed\n", i + 1);

            int err = motor_set_angle(60 * i);

            if (err) {
                LOG_ERR("unable to set motor angle: %i\n", err);
            }

            return;
        }
    }
}

static void
init_pinout(void)
{
    int err = dk_leds_init();

    if (err) {
        LOG_ERR(INIT_UNABLE, "leds");
    }

    if ((err = dk_buttons_init(button_handler))) {
        LOG_ERR(INIT_UNABLE, "buttons");
    }
}

static void
init_mpu(void)
{
    int err = mpu_sensor_init();

    if (err) {
        LOG_ERR(INIT_UNABLE, "mpu");
    }
}

void
main(void)
{
    init_pinout();
    init_mpu();
    motor_init();
    blt_init(&blt_callbacks, &remote_callbacks);

    // LOG_INF("Hello World! %s\n", CONFIG_BOARD);

    for (uint8_t state = 0;; state = !state) {
        dk_set_led(RUN_STATUS_LED, state);
        if (read_accel_values(&accel_values) == 0) {
            LOG_INF(
                "# %d, Accel: X: %06d, Y: %06d, Z: %06d",
                state,
                accel_values.x,
                accel_values.y,
                accel_values.z);
        }
        k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
    }
}
