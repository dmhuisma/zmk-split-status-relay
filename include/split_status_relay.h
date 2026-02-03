
#ifndef ZMK_SPLIT_STATUS_RELAY_H_
#define ZMK_SPLIT_STATUS_RELAY_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zmk/endpoints.h>

typedef enum {
    SSRC_EVENT_CONNECTION_STATE,
    SSRC_EVENT_CENTRAL_BATTERY_LEVEL,
    SSRC_EVENT_PERIPHERAL_BATTERY_LEVEL,
    SSRC_EVENT_HIGHEST_ACTIVE_LAYER,
    SSRC_EVENT_WPM,
    SSRC_EVENT_TRANSPORT,
} ssrc_event_type_t;

typedef struct {
    uint8_t slot;
    bool connected;
} ssrc_connection_state_event_t;

typedef struct {
    uint8_t battery_level;
} ssrc_central_battery_level_event_t;

typedef struct {
    uint8_t slot;
    uint8_t battery_level;
} ssrc_peripheral_battery_level_event_t;

typedef struct {
    uint8_t layer;
    char layer_name[];
} ssrc_highest_active_layer_event_t;

typedef struct {
    uint8_t wpm;
} ssrc_wpm_event_t;

typedef struct {
    // matches enum zmk_transport, but that is only available in centrals
    uint8_t transport;
} ssrc_transport_event_t;

typedef struct {
    ssrc_event_type_t type;
    uint16_t data_length;
    uint8_t data[];
} ssrc_event_t;

typedef void (*ssrc_rx_cb)(const struct device *dev, ssrc_event_t *event, size_t event_length);
typedef void (*ssrc_register_rx_cb)(const struct device *dev, ssrc_rx_cb cb);

// device configuration data
struct ssrc_config {
    const struct device *asdc_channel;
};

// device runtime data structure
struct ssrc_data {
    ssrc_rx_cb recv_cb;
};

__subsystem struct ssrc_driver_api {
    ssrc_register_rx_cb register_recv_cb;
};

__syscall void ssrc_register_recv_cb(const struct device *dev, ssrc_rx_cb cb);

static inline void z_impl_ssrc_register_recv_cb(const struct device *dev, ssrc_rx_cb cb)
{
    const struct ssrc_driver_api *api = (const struct ssrc_driver_api *)dev->api;
	if (api->register_recv_cb == NULL) {
		return;
	}
	api->register_recv_cb(dev, cb);
}

#include <syscalls/split_status_relay.h>

#endif // ZMK_SPLIT_STATUS_RELAY_H_
