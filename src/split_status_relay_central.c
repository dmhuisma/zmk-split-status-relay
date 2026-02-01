
#define DT_DRV_COMPAT zmk_split_status_relay

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zmk/split/transport/central.h>
#include <zmk/split/central.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

#include "split_status_relay.h"

#include <zephyr/logging/log.h>

// TODO - this has BLE specific code, put into BLE transport subfolder
// TODO - verify with multiple peripherals that conn and battery_level slots match to the same devices

// TODO - listeners for HID indicators, active modifiers, highest layer name, output status, WPM

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct peripheral_state {
    struct bt_conn *conn;
    bool connected;
    uint8_t battery_level;
};

static struct peripheral_state peripheral_connections[CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS];

static int8_t find_peripheral_index(struct bt_conn *conn) {
    for (uint8_t i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        if (peripheral_connections[i].conn == conn) {
            return i;
        }
    }
    return -1;
}

static void on_peripheral_connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        return;
    }

    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    int8_t slot = -1;
    for (uint8_t i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        if (peripheral_connections[i].conn == conn) {
            slot = i;
            break;
        }
        if (!peripheral_connections[i].connected) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        LOG_WRN("SSRC: No available slot found for new peripheral connection: %s", addr);
        return;
    }

    peripheral_connections[slot].conn = conn;
    peripheral_connections[slot].connected = true;

    // Keep existing battery level if reconnecting, otherwise mark as unknown
    if (peripheral_connections[slot].battery_level == 0) {
        peripheral_connections[slot].battery_level = 0xFF;
    }
    LOG_INF("SSRC: Peripheral %d connected: %s", slot, addr);
    
    // TODO: Send connection status via ASDC channel
}

static void on_peripheral_disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    int8_t slot = find_peripheral_index(conn);
    if (slot >= 0) {
        LOG_INF("SSRC: Peripheral %d disconnected: %s (reason: %u), last battery: %u%%", 
                slot, addr, reason, peripheral_connections[slot].battery_level);
        
        peripheral_connections[slot].connected = false;
        peripheral_connections[slot].conn = NULL;
        // Keep battery level on disconnect for reconnection tracking
        
        // TODO: Send disconnection status via ASDC channel
    }
}

BT_CONN_CB_DEFINE(split_status_relay_conn_callbacks) = {
    .connected = on_peripheral_connected,
    .disconnected = on_peripheral_disconnected,
};

//
// Battery level change event listener
//

static int peripheral_battery_listener(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    
    if (!ev) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->source >= CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS) {
        LOG_WRN("SSRC: Got battery level for out of range peripheral index %u", 
                ev->source);
        return ZMK_EV_EVENT_BUBBLE;
    }

    // Verify this peripheral slot is actually connected before updating
    // Note: ev->source comes from ZMK's internal peripheral slot assignment
    // which should match our tracking if peripherals connect in the same order
    if (!peripheral_connections[ev->source].connected) {
        LOG_WRN("SSRC: Got battery level for disconnected peripheral %u, marking as connected", 
                ev->source);
        peripheral_connections[ev->source].connected = true;
    }

    peripheral_connections[ev->source].battery_level = ev->state_of_charge;
    
    LOG_INF("SSRC: Peripheral %u battery level changed to %u%% (connected: %s)", 
            ev->source, ev->state_of_charge, 
            peripheral_connections[ev->source].connected ? "yes" : "no");

    // TODO: Send battery level via ASDC channel

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(split_status_relay_battery, peripheral_battery_listener);
ZMK_SUBSCRIPTION(split_status_relay_battery, zmk_peripheral_battery_state_changed);

//
// Device initialization
//

static int srcc_init(const struct device *dev) {
    // Initialize peripheral connection tracking
    for (uint8_t i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        peripheral_connections[i].conn = NULL;
        peripheral_connections[i].connected = false;
        peripheral_connections[i].battery_level = 0xFF;  // 0xFF = unknown
    }
    
    LOG_INF("Split status relay central initialized");
    return 0;
}

//
// Define config structs for each instance
//

#define SSRC_CFG_DEFINE(n)                                                      \
    static const struct srcc_config config_##n = {                              \
        .asdc_channel = DEVICE_DT_GET(DT_INST_PHANDLE(n, asdc_channel))         \
    };

DT_INST_FOREACH_STATUS_OKAY(SSRC_CFG_DEFINE)

#define SSRC_DEVICE_DEFINE(n)                                                   \
    static struct srcc_data srcc_data_##n;                                      \
    DEVICE_DT_INST_DEFINE(n, srcc_init, NULL, &srcc_data_##n,                   \
                          &config_##n, POST_KERNEL,                             \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(SSRC_DEVICE_DEFINE)
