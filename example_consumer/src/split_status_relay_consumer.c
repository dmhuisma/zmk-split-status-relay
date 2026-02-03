
#define DT_DRV_COMPAT zmk_split_status_relay_consumer

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <split_status_relay_consumer.h>
#include <split_status_relay.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static void ssrc_consumer_rx_callback(const struct device *dev, ssrc_event_t *event, size_t event_length) {
    if (event_length < sizeof(ssrc_event_t) + event->data_length) {
        LOG_ERR("SSRCC: Received message too small, got %d from %s, expected %d", event_length, dev->name, sizeof(ssrc_event_t) + event->data_length);
        return;
    }

    switch (event->type) {
        case SSRC_EVENT_CONNECTION_STATE: {
            ssrc_connection_state_event_t *conn_event = (ssrc_connection_state_event_t *)event->data;
            LOG_DBG("SSRCC: Connection state event, slot %d %s", conn_event->slot, conn_event->connected ? "connected" : "disconnected");
        }
        break;
        case SSRC_EVENT_CENTRAL_BATTERY_LEVEL: {
            ssrc_central_battery_level_event_t *battery_event = (ssrc_central_battery_level_event_t *)event->data;
            LOG_DBG("SSRCC: Central Battery level event, level %d%%", battery_event->battery_level);
        }
        break;
        case SSRC_EVENT_PERIPHERAL_BATTERY_LEVEL: {
            ssrc_peripheral_battery_level_event_t *battery_event = (ssrc_peripheral_battery_level_event_t *)event->data;
            LOG_DBG("SSRCC: Peripheral Battery level event, slot %d level %d%%", battery_event->slot, battery_event->battery_level);
        }
        break;
        case SSRC_EVENT_HIGHEST_ACTIVE_LAYER: {
            ssrc_highest_active_layer_event_t *layer_event = (ssrc_highest_active_layer_event_t *)event->data;
            LOG_DBG("SSRCC: Highest active layer event, layer %d name %s", layer_event->layer, layer_event->layer_name);
        }
        break;
        case SSRC_EVENT_WPM: {
            ssrc_wpm_event_t *wpm_event = (ssrc_wpm_event_t *)event->data;
            LOG_DBG("SSRCC: WPM event, wpm %d", wpm_event->wpm);
        }
        break;
        case SSRC_EVENT_TRANSPORT: {
            ssrc_transport_event_t *transport_event = (ssrc_transport_event_t *)event->data;
            LOG_DBG("SSRCC: Transport event, transport %u", transport_event->transport);
        }
        break;
        default: {
            // Unknown event type
            LOG_DBG("SSRCC: unknown event type %d", event->type);
        }
        break;
    }
}

static int ssrc_consumer_init(const struct device *dev)
{
    const struct ssrc_consumer_config *config = (const struct ssrc_consumer_config *)dev->config;
    const struct device *ssrc_dev = config->ssrc;
    if (!device_is_ready(ssrc_dev)) {
        LOG_ERR("SSRC device %s not ready", ssrc_dev->name);
        return -ENODEV;
    }
    // register callback
    ssrc_register_recv_cb(ssrc_dev, ssrc_consumer_rx_callback);
    return 0;
}

//
// Define config structs for each instance
//

#define SSRCC_CFG_DEFINE(n)                                                     \
    static const struct ssrc_consumer_config config_##n = {                     \
        .ssrc = DEVICE_DT_GET(DT_INST_PHANDLE(n, ssrc)),                        \
    };

DT_INST_FOREACH_STATUS_OKAY(SSRCC_CFG_DEFINE)

#define SSRCC_DEVICE_DEFINE(n)                                                  \
    DEVICE_DT_INST_DEFINE(n, ssrc_consumer_init, NULL, NULL,                    \
                          &config_##n, POST_KERNEL,                             \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(SSRCC_DEVICE_DEFINE)