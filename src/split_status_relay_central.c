
#define DT_DRV_COMPAT zmk_split_status_relay

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zmk/split/transport/central.h>
#include <zmk/split/central.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zephyr/logging/log.h>
#include <arbitrary_split_data_channel.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/events/wpm_state_changed.h>

#include "split_status_relay.h"

// TODO - listeners for HID indicators, active modifiers, output status, WPM

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct peripheral_state {
    bool connected;
    uint8_t battery_level;
};

struct ssrc_state_t {
    struct peripheral_state peripheral_connections[CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS];
    uint8_t central_battery_level;
    uint8_t highest_active_layer;
    int wpm;
};

static struct ssrc_state_t ssrc_state = {
    // init to values that indicate "not set"
    .central_battery_level = 0xff,
    .highest_active_layer = 0xff,
    .wpm = 0xff,
};

void send_ssrc_event(const struct device *dev, ssrc_event_t *event, uint32_t delay_ms) {
    const struct ssrc_config *config = (const struct ssrc_config *)dev->config;
    const struct device *asdc_dev = config->asdc_channel;
    int ret = asdc_send(asdc_dev, (const uint8_t*)event, sizeof(ssrc_event_t) + event->data_length, delay_ms);
    if (ret < 0) {
        LOG_ERR("Failed to send ASDC message on device %s: %d", dev->name, ret);
    }
}

#define SSRC_SEND_FOR_EVERY_DEV(n)                                      \
    do {                                                                \
        const struct device *d = DEVICE_DT_INST_GET(n);                 \
        if (d && device_is_ready(d)) {                                  \
            send_ssrc_event(d, event, delay_ms);                        \
        }                                                               \
    } while (0);

void send_ssrc_event_for_every_dev(ssrc_event_t *event, uint32_t delay_ms) {
    DT_INST_FOREACH_STATUS_OKAY(SSRC_SEND_FOR_EVERY_DEV)
}

static void send_active_layer_event(uint8_t layer, uint32_t delay_ms) {
    const char *layer_name = zmk_keymap_layer_name(layer);
    if (layer_name == NULL) {
        layer_name = "unknown";
    }

    uint8_t event_buf[sizeof(ssrc_event_t) + sizeof(ssrc_highest_active_layer_event_t) + strlen(layer_name) + 1];
    ssrc_event_t *event = (ssrc_event_t *)event_buf;
        event->type = SSRC_EVENT_HIGHEST_ACTIVE_LAYER;
        event->data_length = sizeof(ssrc_highest_active_layer_event_t) + strlen(layer_name) + 1;
    ssrc_highest_active_layer_event_t *data = (ssrc_highest_active_layer_event_t *)event->data;
        data->layer = layer;
        strcpy(data->layer_name, layer_name);
    send_ssrc_event_for_every_dev(event, 0);
}

static void send_connection_state_event(uint8_t slot, bool connected, uint32_t delay_ms) {
    uint8_t event_buf[sizeof(ssrc_event_t) + sizeof(ssrc_connection_state_event_t)];
    ssrc_event_t *event = (ssrc_event_t *)event_buf;
        event->type = SSRC_EVENT_CONNECTION_STATE;
        event->data_length = sizeof(ssrc_connection_state_event_t);
    ssrc_connection_state_event_t *data = (ssrc_connection_state_event_t *)event->data;
        data->slot = slot;
        data->connected = connected;
    send_ssrc_event_for_every_dev(event, delay_ms);
}

static void send_peripheral_battery_level_event(uint8_t slot, uint8_t battery_level, uint32_t delay_ms) {
    uint8_t event_buf[sizeof(ssrc_event_t) + sizeof(ssrc_peripheral_battery_level_event_t)];
    ssrc_event_t *event = (ssrc_event_t *)event_buf;
        event->type = SSRC_EVENT_PERIPHERAL_BATTERY_LEVEL;
        event->data_length = sizeof(ssrc_peripheral_battery_level_event_t);
    ssrc_peripheral_battery_level_event_t *data = (ssrc_peripheral_battery_level_event_t *)event->data;
        data->slot = slot;
        data->battery_level = battery_level;
    send_ssrc_event_for_every_dev(event, delay_ms);
}

static void send_central_battery_level_event(uint8_t battery_level, uint32_t delay_ms) {
    uint8_t event_buf[sizeof(ssrc_event_t) + sizeof(ssrc_central_battery_level_event_t)];
    ssrc_event_t *event = (ssrc_event_t *)event_buf;
        event->type = SSRC_EVENT_CENTRAL_BATTERY_LEVEL;
        event->data_length = sizeof(ssrc_central_battery_level_event_t);
    ssrc_central_battery_level_event_t *data = (ssrc_central_battery_level_event_t *)event->data;
        data->battery_level = battery_level;
    send_ssrc_event_for_every_dev(event, delay_ms);
}

static void send_wpm_event(int wpm, uint32_t delay_ms) {
    uint8_t event_buf[sizeof(ssrc_event_t) + sizeof(ssrc_wpm_event_t)];
    ssrc_event_t *event = (ssrc_event_t *)event_buf;
        event->type = SSRC_EVENT_WPM;
        event->data_length = sizeof(ssrc_wpm_event_t);
    ssrc_wpm_event_t *data = (ssrc_wpm_event_t *)event->data;
        data->wpm = wpm;
    send_ssrc_event_for_every_dev(event, delay_ms);
}

static void send_all_events(uint32_t initial_delay_ms) {
    // 0xff means that a value has not been set yet, so do not send

    for (uint8_t i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        send_connection_state_event(i, ssrc_state.peripheral_connections[i].connected, i == 0 ? initial_delay_ms : 0);
        if (ssrc_state.peripheral_connections[i].connected && ssrc_state.peripheral_connections[i].battery_level != 0xFF) {
            send_peripheral_battery_level_event(i, ssrc_state.peripheral_connections[i].battery_level, initial_delay_ms);
        }
    }

    if (ssrc_state.central_battery_level != 0xff) {
        send_central_battery_level_event(ssrc_state.central_battery_level, 0);
    }

    if (ssrc_state.highest_active_layer != 0xff) {
        send_active_layer_event(ssrc_state.highest_active_layer, 0);
    }

    if (ssrc_state.wpm != 0xff) {
        send_wpm_event(ssrc_state.wpm, 0);
    }
}

//
// Peripheral connection event handlers
//

void on_split_peripheral_connected(uint8_t slot) {
    ssrc_state.peripheral_connections[slot].connected = true;
    LOG_INF("SSRC: Peripheral %d connected", slot);

    // update the new peripheral with all current data
    send_all_events(2000);
    
}

void on_split_peripheral_disconnected(uint8_t slot) {
    // Keep battery level on disconnect for reconnection tracking
    ssrc_state.peripheral_connections[slot].connected = false;
    LOG_INF("SSRC: Peripheral %d disconnected", slot);
    send_connection_state_event(slot, false, 0);
}

//
// Peripheral battery level change event listener
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

    if (!ssrc_state.peripheral_connections[ev->source].connected) {
        LOG_WRN("SSRC: Got battery level for disconnected peripheral %u, ignoring", ev->source);
        return ZMK_EV_EVENT_BUBBLE;
    }

    ssrc_state.peripheral_connections[ev->source].battery_level = ev->state_of_charge;
    LOG_INF("SSRC: Peripheral %u battery level changed to %u%%", ev->source, ev->state_of_charge);
    send_peripheral_battery_level_event(ev->source, ev->state_of_charge, 0);

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(split_status_relay_battery, peripheral_battery_listener);
ZMK_SUBSCRIPTION(split_status_relay_battery, zmk_peripheral_battery_state_changed);

//
// Central battery level change event listener
//

static int central_battery_listener(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    
    if (!ev) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    
    LOG_INF("SSRC: Central battery level: %u%%", ev->state_of_charge);
    ssrc_state.central_battery_level = ev->state_of_charge;

    send_central_battery_level_event(ev->state_of_charge, 0);
    
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(central_battery_listener, central_battery_listener);
ZMK_SUBSCRIPTION(central_battery_listener, zmk_battery_state_changed);

//
// Layer change event listener
//

static int central_layer_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    
    if (!ev) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // Get the highest active layer
    uint8_t layer = zmk_keymap_highest_layer_active();
    if (layer == ssrc_state.highest_active_layer) {
        // no change
        return ZMK_EV_EVENT_BUBBLE;
    }
    ssrc_state.highest_active_layer = layer;
    LOG_DBG("SSRC: Central highest active layer changed to %u", layer);

    send_active_layer_event(layer, 0);

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(central_layer_listener, central_layer_listener);
ZMK_SUBSCRIPTION(central_layer_listener, zmk_layer_state_changed);

#ifdef CONFIG_ZMK_WPM

//
// Words per minute event listener
//

static int central_wpm_listener(const zmk_event_t *eh) {
    int wpm = zmk_wpm_get_state();
    if (wpm == 0xff || wpm == ssrc_state.wpm) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    ssrc_state.wpm = wpm;
    send_wpm_event(ssrc_state.wpm, 0);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(central_wpm_listener, central_wpm_listener);
ZMK_SUBSCRIPTION(central_wpm_listener, zmk_wpm_state_changed);

#endif

//
// ASDC receive callback
//

static void ssrc_rx_callback(const struct device *dev, uint8_t *data, size_t len) {
    //
    // the central relays all messages to the peripheral(s)
    if (len < sizeof(ssrc_event_t)) {
        LOG_ERR("SSRC: Received message too small, got %d from %s, expected %d", len, dev->name, sizeof(ssrc_event_t));
        return;
    }
    ssrc_event_t *event = (ssrc_event_t *)data;
    LOG_DBG("SSRC: Received event type %d from %s, relaying to all peripherals", event->type, dev->name);
    send_ssrc_event_for_every_dev(event, 0);
}

//
// Device initialization
//

static int srcc_init(const struct device *dev) {
    // Initialize peripheral connection tracking
    for (uint8_t i = 0; i < CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS; i++) {
        ssrc_state.peripheral_connections[i].connected = false;
        ssrc_state.peripheral_connections[i].battery_level = 0xFF;  // 0xFF = unknown
    }

    // Get the initial highest active layer
    ssrc_state.highest_active_layer = zmk_keymap_highest_layer_active();

    #ifdef CONFIG_ZMK_WPM
    // get the initial WPM
    ssrc_state.wpm = zmk_wpm_get_state();
    #endif

    // Register ASDC receive callback
    const struct ssrc_config *config = (const struct ssrc_config *)dev->config;
    const struct device *asdc_dev = config->asdc_channel;
    asdc_register_recv_cb(asdc_dev, (asdc_rx_cb)ssrc_rx_callback);
    
    LOG_INF("Split status relay central initialized");
    return 0;
}

//
// Define config structs for each instance
//

#define SSRC_CFG_DEFINE(n)                                                      \
    static const struct ssrc_config config_##n = {                              \
        .asdc_channel = DEVICE_DT_GET(DT_INST_PHANDLE(n, asdc_channel))         \
    };

DT_INST_FOREACH_STATUS_OKAY(SSRC_CFG_DEFINE)

#define SSRC_DEVICE_DEFINE(n)                                                   \
    DEVICE_DT_INST_DEFINE(n, srcc_init, NULL, NULL,                             \
                          &config_##n, POST_KERNEL,                             \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(SSRC_DEVICE_DEFINE)
