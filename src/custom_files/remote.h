
#ifndef REMOTE_H
#define REMOTE_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <logging/log.h>
#include <zephyr.h>

#define BT_UUID_REMOTE_SERV_VAL                                                \
    BT_UUID_128_ENCODE(0x94e00001, 0x3256, 0x4177, 0xa640, 0xb5e6ea354ec8)

#define BT_UUID_REMOTE_SERVICE BT_UUID_DECLARE_128(BT_UUID_REMOTE_SERV_VAL)

#define BT_UUID_REMOTE_BUTTON_CHRC_VAL                                         \
    BT_UUID_128_ENCODE(0x94e00002, 0x3256, 0x4177, 0xa640, 0xb5e6ea354ec8)

#define BT_UUID_REMOTE_BUTTON_CHRC                                             \
    BT_UUID_DECLARE_128(BT_UUID_REMOTE_BUTTON_CHRC_VAL)

#define BT_UUID_REMOTE_MESSAGE_CHRC_VAL                                        \
    BT_UUID_128_ENCODE(0x94e00003, 0x3256, 0x4177, 0xa640, 0xb5e6ea354ec8)

#define BT_UUID_REMOTE_MESSAGE_CHRC                                            \
    BT_UUID_DECLARE_128(BT_UUID_REMOTE_MESSAGE_CHRC_VAL)

enum blt_button_notif_enabled {
    BLT_BUTTON_NOTIF_ENABLED,
    BLT_BUTTON_NOTIF_DISABLED,
};

struct blt_remote_service_cb {
    void (*notif_changed)(enum blt_button_notif_enabled status);
    void (*data_recieved)(
        struct bt_conn* conn, const uint8_t* const data, uint16_t len);
};

int
blt_init(struct bt_conn_cb* callbacks, struct blt_remote_service_cb* notifcb);
void blt_set_button_value(uint8_t value);
uint8_t blt_get_button_value(void);
int blt_send_button_notif(struct bt_conn* conn, uint8_t value);

#endif // REMOTE_H