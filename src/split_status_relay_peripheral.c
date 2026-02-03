
#define DT_DRV_COMPAT zmk_split_status_relay

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <arbitrary_split_data_channel.h>
#include <zephyr/logging/log.h>

#include "split_status_relay.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static void ssrc_rx_callback(const struct device *asdc_dev, uint8_t *data, size_t len) {
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

static int ssrc_init(const struct device *dev) {
    const struct ssrc_config *config = (const struct ssrc_config *)dev->config;
    const struct device *asdc_dev = config->asdc_channel;
    asdc_register_recv_cb(asdc_dev, (asdc_rx_cb)ssrc_rx_callback);
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