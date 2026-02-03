
#define DT_DRV_COMPAT zmk_split_status_relay

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <arbitrary_split_data_channel.h>
#include <zephyr/logging/log.h>
#include <zmk/events/usb_conn_state_changed.h>
#include "split_status_relay.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct ssrp_state_t {
    #if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    bool usb_connection_state;
    #endif
};

static struct ssrp_state_t ssrp_state;

void send_ssrp_event(const struct device *dev, ssrc_event_t *event, uint32_t delay_ms) {
    const struct ssrc_config *config = (const struct ssrc_config *)dev->config;
    const struct device *asdc_dev = config->asdc_channel;
    int ret = asdc_send(asdc_dev, (const uint8_t*)event, sizeof(ssrc_event_t) + event->data_length, delay_ms);
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

void send_ssrp_event_for_every_dev(ssrc_event_t *event, uint32_t delay_ms) {
    DT_INST_FOREACH_STATUS_OKAY(SSRP_SEND_FOR_EVERY_DEV)
}

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
void send_usb_connection_state_event(bool connected, uint32_t delay_ms) {
    uint8_t event_buf[sizeof(ssrc_event_t) + sizeof(ssrc_peripheral_usb_connection_state_t)];
    ssrc_event_t *event = (ssrc_event_t *)event_buf;
        event->type = SSRC_EVENT_PERIPHERAL_USB_CONNECTION_STATE;
        event->data_length = sizeof(ssrc_peripheral_usb_connection_state_t);
    ssrc_peripheral_usb_connection_state_t *data = (ssrc_peripheral_usb_connection_state_t *)event->data;
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
    ssrc_event_t *event = (ssrc_event_t *)data;

    if (len < sizeof(ssrc_event_t) + event->data_length) {
        LOG_ERR("SSRC: Received message too small, got %d from %s, expected %d", len, asdc_dev->name, sizeof(ssrc_event_t) + event->data_length);
        return;
    }

    // Iterate through all SSRC instances and invoke callback on those with matching asdc_channel

    #define SSRC_INVOKE_CB_IF_MATCH(n)                                                          \
        do {                                                                                    \
            const struct device *ssrc_dev = DEVICE_DT_INST_GET(n);                              \
            if (ssrc_dev && device_is_ready(ssrc_dev)) {                                        \
                const struct ssrc_config *cfg = (const struct ssrc_config *)ssrc_dev->config;   \
                if (cfg->asdc_channel == asdc_dev) {                                            \
                    struct ssrc_data *data = (struct ssrc_data *)ssrc_dev->data;                \
                    if (data->recv_cb != NULL) {                                                \
                        data->recv_cb(ssrc_dev, event, event_len);                              \
                    }                                                                           \
                }                                                                               \
            }                                                                                   \
        } while (0);

    size_t event_len = sizeof(ssrc_event_t) + event->data_length;
    DT_INST_FOREACH_STATUS_OKAY(SSRC_INVOKE_CB_IF_MATCH)
}

void on_split_connected(void) {
    send_all_events(2000);
}

void on_split_disconnected(void) {

}

static int ssrc_init(const struct device *dev) {
    const struct ssrc_config *config = (const struct ssrc_config *)dev->config;
    const struct device *asdc_dev = config->asdc_channel;
    asdc_register_recv_cb(asdc_dev, asdc_rx_callback);

    // initial values
    #if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    ssrp_state.usb_connection_state = zmk_usb_is_powered();
    #endif

    return 0;
}

static void ssrc_reg_recv_cb(const struct device *dev, ssrc_rx_cb cb)
{
    struct ssrc_data *ssrc_data = (struct ssrc_data *)dev->data;
    ssrc_data->recv_cb = cb;
}

static const struct ssrc_driver_api ssrc_api = {
    .register_recv_cb = &ssrc_reg_recv_cb,
};

//
// Define config structs for each instance
//

#define SSRC_CFG_DEFINE(n)                                                      \
    static const struct ssrc_config config_##n = {                              \
        .asdc_channel = DEVICE_DT_GET(DT_INST_PHANDLE(n, asdc_channel))         \
    };

DT_INST_FOREACH_STATUS_OKAY(SSRC_CFG_DEFINE)

#define SSRC_DEVICE_DEFINE(n)                                                   \
    static struct ssrc_data ssrc_data_##n;                                      \
    DEVICE_DT_INST_DEFINE(n, ssrc_init, NULL, &ssrc_data_##n,                   \
                          &config_##n, POST_KERNEL,                             \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ssrc_api);

DT_INST_FOREACH_STATUS_OKAY(SSRC_DEVICE_DEFINE)