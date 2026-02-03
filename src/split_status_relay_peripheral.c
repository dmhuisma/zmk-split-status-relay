
#define DT_DRV_COMPAT zmk_split_status_relay

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <arbitrary_split_data_channel.h>
#include <zephyr/logging/log.h>

#include "split_status_relay.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static void ssrc_rx_callback(const struct device *dev, uint8_t *data, size_t len) {
    if (len < sizeof(ssrc_event_t)) {
        LOG_ERR("SSRC: Received message too small, got %d from %s, expected %d", len, dev->name, sizeof(ssrc_event_t));
        return;
    }

    ssrc_event_t *event = (ssrc_event_t *)data;

    switch (event->type) {
        case SSRC_EVENT_CONNECTION_STATE_CHANGED:
            LOG_DBG("SSRC: Connection state event, slot %d %s", event->connection_state_changed.slot, 
                    event->connection_state_changed.connected ? "connected" : "disconnected");
            break;
        case SSRC_EVENT_CENTRAL_BATTERY_LEVEL_CHANGED:
            LOG_DBG("SSRC: Central Battery level event, level %d%%", event->battery_level_changed.battery_level);
            break;
        case SSRC_EVENT_PERIPHERAL_BATTERY_LEVEL_CHANGED:
            LOG_DBG("SSRC: Peripheral Battery level event, slot %d level %d%%", event->battery_level_changed.slot, event->battery_level_changed.battery_level);
            break;
        default:
            // Unknown event type
            break;
    }
}

static int srcc_init(const struct device *dev) {
    const struct ssrc_config *config = (const struct ssrc_config *)dev->config;
    const struct device *asdc_dev = config->asdc_channel;
    asdc_register_recv_cb(asdc_dev, (asdc_rx_cb)ssrc_rx_callback);
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