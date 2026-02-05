/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>

typedef enum {
    SSR_EVENT_CONNECTION_STATE,
    SSR_EVENT_CENTRAL_BATTERY_LEVEL,
    SSR_EVENT_PERIPHERAL_BATTERY_LEVEL,
    SSR_EVENT_HIGHEST_ACTIVE_LAYER,
    SSR_EVENT_WPM,
    SSR_EVENT_TRANSPORT,
    SSR_EVENT_ACTIVE_BLE_PROFILE,
    SSR_EVENT_CENTRAL_USB_CONNECTION_STATE,
    SSR_EVENT_PERIPHERAL_USB_CONNECTION_STATE,
} ssr_event_type_t;

struct ssr_asdc_event {
    ssr_event_type_t type;
    uint16_t data_length;
    uint8_t data[];
};
