/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_ssr_central_usb_conn_state_changed {
    bool connected;
};

ZMK_EVENT_DECLARE(zmk_ssr_central_usb_conn_state_changed);
