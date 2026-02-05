/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_ssr_central_layer_state_changed {
    uint8_t layer;
    const char *layer_name;
};

ZMK_EVENT_DECLARE(zmk_ssr_central_layer_state_changed);
