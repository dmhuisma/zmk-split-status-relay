
#include <stdbool.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>

void on_split_connected(void);
void on_split_disconnected(void);

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static void on_connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        return;
    }
    on_split_connected();
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason) {
    on_split_disconnected();
}

BT_CONN_CB_DEFINE(split_status_relay_conn_callbacks) = {
    .connected = on_connected,
    .disconnected = on_disconnected,
};
