
#ifndef ZMK_SPLIT_STATUS_RELAY_CONSUMER_H_
#define ZMK_SPLIT_STATUS_RELAY_CONSUMER_H_

#include <zephyr/kernel.h>

// device config structure
struct ssrc_consumer_config {
    const struct device *ssrc;
};

#endif // ZMK_SPLIT_STATUS_RELAY_CONSUMER_H_