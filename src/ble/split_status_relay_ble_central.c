
#include <stdbool.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>

void on_split_peripheral_connected(uint8_t slot);
void on_split_peripheral_disconnected(uint8_t slot);

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct peripheral_ble_state {
    struct bt_conn *conn;
    bool connected;
};

static struct peripheral_ble_state peripheral_ble_connections[CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS];

int8_t get_peripheral_index_by_conn(void *conn) {
    struct bt_conn* conn_bt = (struct bt_conn*)(conn);
    for (uint8_t i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        if (peripheral_ble_connections[i].conn == conn_bt) {
            return i;
        }
    }
    return -1;
}

static void on_peripheral_connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        return;
    }

    int8_t slot = -1;
    for (uint8_t i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        if (peripheral_ble_connections[i].conn == conn) {
            slot = i;
            break;
        }
        if (!peripheral_ble_connections[i].connected) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        LOG_WRN("SSRC BLE: No available slot found for new peripheral connection");
        return;
    }

    peripheral_ble_connections[slot].conn = conn;
    peripheral_ble_connections[slot].connected = true;
    
    on_split_peripheral_connected(slot);
}

static void on_peripheral_disconnected(struct bt_conn *conn, uint8_t reason) {
    int8_t slot = get_peripheral_index_by_conn(conn);
    if (slot >= 0) {
        peripheral_ble_connections[slot].connected = false;
        peripheral_ble_connections[slot].conn = NULL;
        on_split_peripheral_disconnected(slot);
    }
}

BT_CONN_CB_DEFINE(split_status_relay_conn_callbacks) = {
    .connected = on_peripheral_connected,
    .disconnected = on_peripheral_disconnected,
};
