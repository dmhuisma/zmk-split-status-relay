/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_ssr_central_ble_profile_changed {
    uint8_t active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
};

ZMK_EVENT_DECLARE(zmk_ssr_central_ble_profile_changed);
