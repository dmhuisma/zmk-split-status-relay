
#include <zephyr/kernel.h>

// device configuration data
struct ssrc_config {
    const struct device *asdc_channel;
};

typedef enum {
    SSRC_EVENT_CONNECTION_STATE_CHANGED,
    SSRC_EVENT_CENTRAL_BATTERY_LEVEL_CHANGED,
    SSRC_EVENT_PERIPHERAL_BATTERY_LEVEL_CHANGED,
} ssrc_event_type_t;

typedef struct {
    uint8_t slot;
    bool connected;
} ssrc_connection_state_changed_event_t;

typedef struct {
    uint8_t slot;
    uint8_t battery_level;
} ssrc_battery_level_changed_event_t;

typedef struct {
    ssrc_event_type_t type;
    union {
        ssrc_connection_state_changed_event_t connection_state_changed;
        ssrc_battery_level_changed_event_t battery_level_changed;
    };
} ssrc_event_t;

void on_split_peripheral_connected(uint8_t slot);
void on_split_peripheral_disconnected(uint8_t slot);
