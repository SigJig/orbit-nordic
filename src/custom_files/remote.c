
#include "remote.h"

#define LOG_MODULE_NAME remote
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static K_SEM_DEFINE(blt_init_ok, 0, 1);

static void
enable_cb(int err)
{
    if (err) {
        LOG_ERR("bt enable error: %i\n", err);
    }

    k_sem_give(&blt_init_ok);
}

static uint8_t button_value = 0;
static struct blt_remote_service_cb remote_service_callbacks;

static ssize_t
blt_read(
    struct bt_conn* conn,
    const struct bt_gatt_attr* attr,
    void* buf,
    uint16_t len,
    uint16_t offset)
{
    return bt_gatt_attr_read(
        conn, attr, buf, len, offset, &button_value, sizeof(button_value));
}

static ssize_t
blt_write(
    struct bt_conn* conn,
    const struct bt_gatt_attr* attr,
    const void* buf,
    uint16_t len,
    uint16_t offset,
    uint8_t flags)
{
    LOG_INF("recieved data, handle %d conn %p\n", attr->handle, (void*)conn);

    if (remote_service_callbacks.data_recieved) {
        remote_service_callbacks.data_recieved(conn, buf, len);
    }

    return len;
}

static void
blt_chrc_changed(const struct bt_gatt_attr* attr, uint16_t value)
{
    bool notif_enabled = value == BT_GATT_CCC_NOTIFY;
    LOG_INF("notifications %s", notif_enabled ? "enabled" : "disabled");

    if (remote_service_callbacks.notif_changed) {
        remote_service_callbacks.notif_changed(
            notif_enabled ? BLT_BUTTON_NOTIF_ENABLED
                          : BLT_BUTTON_NOTIF_DISABLED);
    }
}

BT_GATT_SERVICE_DEFINE(
    remote_srv,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_REMOTE_SERVICE),
    BT_GATT_CHARACTERISTIC(
        BT_UUID_REMOTE_BUTTON_CHRC,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_READ,
        blt_read,
        NULL,
        NULL),
    BT_GATT_CCC(blt_chrc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(
        BT_UUID_REMOTE_MESSAGE_CHRC,
        BT_GATT_CHRC_WRITE_WITHOUT_RESP,
        BT_GATT_PERM_WRITE,
        NULL,
        blt_write,
        NULL));

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN)};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_REMOTE_SERV_VAL),
};

void
blt_set_button_value(uint8_t value)
{
    button_value = value;
}

uint8_t
blt_get_button_value(void)
{
    return button_value;
}

static void
blt_on_sent(struct bt_conn* conn, void* user_data)
{
    ARG_UNUSED(user_data);
    LOG_INF("notification sent on connection %p", (void*)conn);
}

int
blt_send_button_notif(struct bt_conn* conn, uint8_t value)
{
    int err = 0;

    struct bt_gatt_notify_params params = {0};
    const struct bt_gatt_attr* attr = &remote_srv.attrs[2];

    params.attr = attr;
    params.data = &value;
    params.len = 1;
    params.func = blt_on_sent;

    err = bt_gatt_notify_cb(conn, &params);

    return err;
}

int
blt_init(struct bt_conn_cb* callbacks, struct blt_remote_service_cb* notifcb)
{
    LOG_INF("initializing bluetooth module\n");

    if (!callbacks || !notifcb) {
        return NRFX_ERROR_NULL;
    }

    bt_conn_cb_register(callbacks);
    remote_service_callbacks.notif_changed = notifcb->notif_changed;
    remote_service_callbacks.data_recieved = notifcb->data_recieved;

    int err = bt_enable(enable_cb);

    if (err) {

        return err;
    }

    k_sem_take(&blt_init_ok, K_FOREVER);

    err = bt_le_adv_start(
        BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

    if (err) {
        LOG_ERR("couldn't start advertising (err = %d", err);

        return err;
    }

    return 0;
}