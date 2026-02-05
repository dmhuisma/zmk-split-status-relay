
#define DT_DRV_COMPAT zmk_split_status_relay

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <arbitrary_split_data_channel.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <ssr_event.h>
#include <zmk/events/ssr_peripheral_connection_state_changed.h>
#include <zmk/events/ssr_central_battery_state_changed.h>
#include <zmk/events/ssr_peripheral_battery_state_changed.h>
#include <zmk/events/ssr_central_layer_state_changed.h>
#include <zmk/events/ssr_central_wpm_state_changed.h>
#include <zmk/events/ssr_central_transport_changed.h>
#include <zmk/events/ssr_central_ble_profile_changed.h>
#include <zmk/events/ssr_central_usb_conn_state_changed.h>
#include <zmk/events/ssr_peripheral_usb_conn_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Config structure
struct ssrp_config {
    const struct device *asdc_channel;
};

struct ssrp_state_t {
    #if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    bool usb_connection_state;
    #endif
};

static struct ssrp_state_t ssrp_state;

void send_ssrp_event(const struct device *dev, struct ssr_asdc_event *event, uint32_t delay_ms) {
    const struct ssrp_config *config = (const struct ssrp_config *)dev->config;
    const struct device *asdc_dev = config->asdc_channel;
    int ret = asdc_send(asdc_dev, (const uint8_t*)event, sizeof(struct ssr_asdc_event) + event->data_length, delay_ms);
    if (ret < 0) {
        LOG_ERR("Failed to send ASDC message on device %s: %d", dev->name, ret);
    }
}

#define SSRP_SEND_FOR_EVERY_DEV(n)                                      \
    do {                                                                \
        const struct device *d = DEVICE_DT_INST_GET(n);                 \
        if (d && device_is_ready(d)) {                                  \
            send_ssrp_event(d, event, delay_ms);                        \
        }                                                               \
    } while (0);

void send_ssrp_event_for_every_dev(struct ssr_asdc_event *event, uint32_t delay_ms) {
    DT_INST_FOREACH_STATUS_OKAY(SSRP_SEND_FOR_EVERY_DEV)
}

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
void send_usb_connection_state_event(bool connected, uint32_t delay_ms) {
    uint8_t event_buf[sizeof(struct ssr_asdc_event) + sizeof(struct zmk_ssr_peripheral_usb_conn_state_changed)];
    struct ssr_asdc_event *event = (struct ssr_asdc_event *)event_buf;
        event->type = SSR_EVENT_PERIPHERAL_USB_CONNECTION_STATE;
        event->data_length = sizeof(struct zmk_ssr_peripheral_usb_conn_state_changed);
    struct zmk_ssr_peripheral_usb_conn_state_changed *data = (struct zmk_ssr_peripheral_usb_conn_state_changed *)event->data;
        // the peripheral does not know its slot, the central will fill this in when it relays the message
        data->slot = 0xff;
        data->connected = connected;
    send_ssrp_event_for_every_dev(event, delay_ms);
}
#endif

static void send_all_events(uint32_t initial_delay_ms) {
    send_usb_connection_state_event(ssrp_state.usb_connection_state, initial_delay_ms);
}

//
// USB connection state listener
//

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)

static int central_usb_connection_state_listener(const zmk_event_t *eh) {
    ssrp_state.usb_connection_state = zmk_usb_is_powered();
    send_usb_connection_state_event(ssrp_state.usb_connection_state, 0);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(central_usb_connection_state_listener, central_usb_connection_state_listener);
ZMK_SUBSCRIPTION(central_usb_connection_state_listener, zmk_usb_conn_state_changed);

#endif

//
// ASDC receive callback
//

static void asdc_rx_callback(const struct device *asdc_dev, void* sender_conn, uint8_t *data, size_t len) {
    struct ssr_asdc_event *event = (struct ssr_asdc_event *)data;

    if (len < sizeof(struct ssr_asdc_event) + event->data_length) {
        LOG_ERR("SSRP: Received message too small, got %d, expected %d", len, sizeof(struct ssr_asdc_event) + event->data_length);
        return;
    }

    // Raise individual ZMK events based on the event type
    switch (event->type) {
        case SSR_EVENT_CONNECTION_STATE: {
            struct zmk_ssr_peripheral_connection_state_changed *conn_event = (struct zmk_ssr_peripheral_connection_state_changed *)event->data;
            raise_zmk_ssr_peripheral_connection_state_changed(
                (struct zmk_ssr_peripheral_connection_state_changed){
                    .slot = conn_event->slot,
                    .connected = conn_event->connected
                });
        }
        break;
        case SSR_EVENT_CENTRAL_BATTERY_LEVEL: {
            struct zmk_ssr_central_battery_state_changed *battery_event = (struct zmk_ssr_central_battery_state_changed *)event->data;
            raise_zmk_ssr_central_battery_state_changed(
                (struct zmk_ssr_central_battery_state_changed){
                    .battery_level = battery_event->battery_level
                });
        }
        break;
        case SSR_EVENT_PERIPHERAL_BATTERY_LEVEL: {
            struct zmk_ssr_peripheral_battery_state_changed *battery_event = (struct zmk_ssr_peripheral_battery_state_changed *)event->data;
            raise_zmk_ssr_peripheral_battery_state_changed(
                (struct zmk_ssr_peripheral_battery_state_changed){
                    .slot = battery_event->slot,
                    .battery_level = battery_event->battery_level
                });
        }
        break;
        case SSR_EVENT_HIGHEST_ACTIVE_LAYER: {
            // Validate data contains at least the layer field plus null terminator
            if (event->data_length < sizeof(uint8_t) + 1) {
                LOG_ERR("SSRP: Layer event data too small: %d", event->data_length);
                break;
            }
            
            uint8_t layer = event->data[0];
            const char *layer_name = (const char *)&event->data[1];
            
            // Ensure null termination within the data bounds
            size_t max_name_len = event->data_length - sizeof(uint8_t);
            bool null_found = false;
            for (size_t i = 0; i < max_name_len; i++) {
                if (layer_name[i] == '\0') {
                    null_found = true;
                    break;
                }
            }
            
            if (!null_found) {
                LOG_ERR("SSRP: Layer name not null-terminated, data_length=%d", event->data_length);
                break;
            }
            
            raise_zmk_ssr_central_layer_state_changed(
                (struct zmk_ssr_central_layer_state_changed){
                    .layer = layer,
                    .layer_name = layer_name
                });
        }
        break;
        case SSR_EVENT_WPM: {
            struct zmk_ssr_central_wpm_state_changed *wpm_event = (struct zmk_ssr_central_wpm_state_changed *)event->data;
            raise_zmk_ssr_central_wpm_state_changed(
                (struct zmk_ssr_central_wpm_state_changed){
                    .wpm = wpm_event->wpm
                });
        }
        break;
        case SSR_EVENT_TRANSPORT: {
            struct zmk_ssr_central_transport_changed *transport_event = (struct zmk_ssr_central_transport_changed *)event->data;
            raise_zmk_ssr_central_transport_changed(
                (struct zmk_ssr_central_transport_changed){
                    .transport = transport_event->transport
                });
        }
        break;
        case SSR_EVENT_ACTIVE_BLE_PROFILE: {
            struct zmk_ssr_central_ble_profile_changed *ble_event = (struct zmk_ssr_central_ble_profile_changed *)event->data;
            raise_zmk_ssr_central_ble_profile_changed(
                (struct zmk_ssr_central_ble_profile_changed){
                    .active_profile_index = ble_event->active_profile_index,
                    .active_profile_connected = ble_event->active_profile_connected,
                    .active_profile_bonded = ble_event->active_profile_bonded
                });
        }
        break;
        case SSR_EVENT_CENTRAL_USB_CONNECTION_STATE: {
            struct zmk_ssr_central_usb_conn_state_changed *usb_event = (struct zmk_ssr_central_usb_conn_state_changed *)event->data;
            raise_zmk_ssr_central_usb_conn_state_changed(
                (struct zmk_ssr_central_usb_conn_state_changed){
                    .connected = usb_event->connected
                });
        }
        break;
        case SSR_EVENT_PERIPHERAL_USB_CONNECTION_STATE: {
            struct zmk_ssr_peripheral_usb_conn_state_changed *usb_event = (struct zmk_ssr_peripheral_usb_conn_state_changed *)event->data;
            raise_zmk_ssr_peripheral_usb_conn_state_changed(
                (struct zmk_ssr_peripheral_usb_conn_state_changed){
                    .slot = usb_event->slot,
                    .connected = usb_event->connected
                });
        }
        break;
        default:
            LOG_WRN("SSRP peripheral: Unknown event type %d", event->type);
            break;
    }
}

void on_split_connected(void) {
    send_all_events(2000);
}

void on_split_disconnected(void) {

}

static int ssrp_init(const struct device *dev) {
    const struct ssrp_config *config = (const struct ssrp_config *)dev->config;
    const struct device *asdc_dev = config->asdc_channel;
    asdc_register_recv_cb(asdc_dev, asdc_rx_callback);

    // initial values
    #if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    ssrp_state.usb_connection_state = zmk_usb_is_powered();
    #endif

    return 0;
}

//
// Define config structs for each instance
//

#define SSRP_CFG_DEFINE(n)                                                      \
    static const struct ssrp_config config_##n = {                              \
        .asdc_channel = DEVICE_DT_GET(DT_INST_PHANDLE(n, asdc_channel))         \
    };

DT_INST_FOREACH_STATUS_OKAY(SSRP_CFG_DEFINE)

#define SSRP_DEVICE_DEFINE(n)                                                   \
    DEVICE_DT_INST_DEFINE(n, ssrp_init, NULL, NULL,                             \
                          &config_##n, POST_KERNEL,                             \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(SSRP_DEVICE_DEFINE)