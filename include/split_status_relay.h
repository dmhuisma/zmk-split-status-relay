
#include <zephyr/kernel.h>

// device configuration data
struct ssrc_config {
    const struct device *asdc_channel;
};

typedef enum {
    SSRC_EVENT_CONNECTION_STATE,
    SSRC_EVENT_CENTRAL_BATTERY_LEVEL,
    SSRC_EVENT_PERIPHERAL_BATTERY_LEVEL,
    SSRC_EVENT_HIGHEST_ACTIVE_LAYER,
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
    ssrc_event_type_t type;
    uint16_t data_length;
    uint8_t data[];
} ssrc_event_t;

void on_split_peripheral_connected(uint8_t slot);
void on_split_peripheral_disconnected(uint8_t slot);
